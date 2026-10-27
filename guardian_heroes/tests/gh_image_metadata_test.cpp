#include "gh_image.h"
#include "gh_image_metadata.h"
#include "pe/pe_image.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <map>

namespace {
uint32_t Be32(const uint8_t* p) {
  return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3];
}
uint16_t Be16(const uint8_t* p) { return uint16_t(p[0]) << 8 | p[1]; }
void Put(std::vector<uint8_t>& data, size_t offset, uint32_t value) {
  for (unsigned i = 0; i < 4; ++i) data[offset+i] = uint8_t(value >> (24-i*8));
}
void TestXex1AndPatchRefresh() {
  std::vector<uint8_t> file(0x200);
  Put(file,0,0x58455831); Put(file,8,file.size()); Put(file,16,24);
  Put(file,24+0x130,0x82000000); Put(file,24+0x158,7);
  Put(file,24+0x15C,0x82000100); Put(file,24+0x164,1);
  Put(file,24+0x168,0x31); file[24+0x134]=0xA5;
  gh::image::XexMetadata metadata;
  assert(metadata.Load(file) && metadata.IsXex1());
  assert(metadata.Security().load_address==0x82000000 && metadata.Security().key[0]==0xA5);
  assert(metadata.Security().flags==7 && metadata.Security().export_table==0x82000100);
  assert(metadata.ImageSize(65536)==3*65536);
  Put(metadata.HeaderBytes(),24+0x168,0x51);
  assert(metadata.Refresh() && metadata.ImageSize(65536)==5*65536);
  Put(metadata.HeaderBytes(),24+0x164,100);
  assert(!metadata.Refresh());
}

void TestExports() {
  std::vector<uint8_t> table(52);
  Put(table,32,0x8200); Put(table,36,2); Put(table,40,5);
  Put(table,44,0x100); Put(table,48,0x200);
  assert(gh::image::FindXexExport(table,5)==0x82000100);
  assert(gh::image::FindXexExport(table,6)==0x82000200);
  assert(!gh::image::FindXexExport(table,4));
  std::vector<uint8_t> image(128);
  auto store=[&](size_t offset,uint32_t value){std::memcpy(image.data()+offset,&value,4);};
  const uint32_t directory=16;
  store(directory+20,2);store(directory+24,1);store(directory+28,40);
  store(directory+32,48);store(directory+36,52);
  store(directory+40,0x1000);store(directory+44,0x2000);store(directory+48,56);
  image[directory+52]=1;
  std::memcpy(image.data()+directory+56,"Named",6);
  assert(gh::image::FindPeExport(image,directory,0x82000000,uint16_t(0))==0x82001000);
  assert(gh::image::FindPeExport(image,directory,0x82000000,"Named")==0x82002000);
  assert(!gh::image::FindPeExport(image,directory,0x82000000,"Absent"));
}
}

int main(int argc,char** argv) {
  assert(argc==2);
  TestXex1AndPatchRefresh(); TestExports();
  std::ifstream input(argv[1],std::ios::binary);
  std::vector<uint8_t> file((std::istreambuf_iterator<char>(input)),{});
  gh::image::XexMetadata metadata;
  assert(metadata.Load(file) && !metadata.IsXex1());
  const auto& security=metadata.Security();
  const uint32_t security_offset=Be32(file.data()+16);
  assert(security.load_address==Be32(file.data()+security_offset+0x110));
  assert(security.image_size==Be32(file.data()+security_offset+4));
  uint32_t page_size=65536,total=0;
  for(size_t i=0;i<security.pages.size();++i) {
    const uint32_t descriptor=Be32(file.data()+security_offset+0x184+i*24);
    assert(security.pages[i].count==(descriptor>>4) && security.pages[i].type==(descriptor&15));
    total+=(descriptor>>4)*page_size;
  }
  assert(metadata.ImageSize(page_size)==total);
  const uint32_t count=Be32(file.data()+20);
  for(uint32_t i=0;i<count;++i) {
    const auto* entry=file.data()+24+i*8;
    const uint32_t key=Be32(entry),value=Be32(entry+4);
    gh::image::OptionalHeader field;
    assert(gh::image::FindOptionalHeader(metadata.HeaderBytes(),key,field));
    assert(field.value==value);
    assert(field.offset==((key&255)<=1 ? 28+i*8 : value));
  }
  gh::image::OptionalHeader format,base;
  assert(gh::image::FindOptionalHeader(metadata.HeaderBytes(),0x3FF,format));
  assert(gh::image::FindOptionalHeader(metadata.HeaderBytes(),0x10201,base));
  const uint32_t image_base=Be32(metadata.HeaderBytes().data()+base.offset);
  auto key=gh::image::SessionKey(security.key,false);
  std::vector<uint8_t> image;
  assert(!gh::image::DecodePayload(std::span(file).subspan(Be32(file.data()+8)),
      std::span(metadata.HeaderBytes()).subspan(format.offset,format.size),key,total,image));
  gh::image::PeMetadata pe;
  assert(gh::image::ParsePe(image,image_base,pe));
  // Compare against the original loader's PE struct interpretation.
  const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(image.data());
  const auto* nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(image.data()+dos->e_lfanew);
  assert(pe.timestamp==nt->FileHeader.TimeDateStamp);
  assert(pe.exception_rva==nt->OptionalHeader.DataDirectory[3].VirtualAddress);
  assert(pe.exception_size==nt->OptionalHeader.DataDirectory[3].Size);
  assert(pe.sections.size()==nt->FileHeader.NumberOfSections);
  const auto* sections=reinterpret_cast<const IMAGE_SECTION_HEADER*>(
      reinterpret_cast<const uint8_t*>(&nt->OptionalHeader)+nt->FileHeader.SizeOfOptionalHeader);
  for(size_t i=0;i<pe.sections.size();++i) {
    const auto& a=pe.sections[i];const auto& b=sections[i];
    assert(!std::memcmp(a.name,b.Name,8));
    assert(a.address==image_base+b.VirtualAddress && a.size==b.Misc.VirtualSize);
    assert(a.raw_address==b.PointerToRawData && a.raw_size==b.SizeOfRawData && a.flags==b.Characteristics);
  }
  std::vector<gh::image::ImportLibrary> libraries;
  assert(metadata.ParseImports(image,image_base,libraries));
  gh::image::OptionalHeader imports;
  assert(gh::image::FindOptionalHeader(metadata.HeaderBytes(),0x103FF,imports));
  const auto* data=metadata.HeaderBytes().data()+imports.offset;
  std::vector<std::string> names;
  for(size_t o=0,i=0;o<Be32(data+8);++o) {
    names.emplace_back(reinterpret_cast<const char*>(data+12+i));
    i=(i+names.back().size()+1+3)&~size_t(3);
  }
  size_t index=0,records=0;
  for(size_t offset=Be32(data+4)+12;offset<Be32(data);) {
    const auto* source=data+offset;
    if(!Be32(source))break;
    const auto& actual=libraries.at(index++);
    assert(actual.name==names.at(Be16(source+36)&255));
    assert(actual.id==Be32(source+24) && actual.version.value==Be32(source+28));
    assert(actual.min_version.value==Be32(source+32));
    std::map<uint16_t,std::pair<uint32_t,uint32_t>> expected;
    for(uint32_t i=0;i<Be16(source+38);++i) {
      const uint32_t address=Be32(source+40+i*4);
      if(!address)continue;
      const uint32_t record=Be32(image.data()+address-image_base);
      auto& paired=expected[uint16_t(record)];
      if(record>>24==0)paired.first=address;
      else if(record>>24==1)paired.second=address;
      ++records;
    }
    assert(actual.imports.size()==expected.size());
    size_t ordinal_index=0;
    for(const auto& [ordinal,entry]:expected) {
      const auto& actual_entry=actual.imports[ordinal_index++];
      assert(actual_entry.ordinal==ordinal && actual_entry.value_address==entry.first &&
             actual_entry.thunk_address==entry.second);
    }
    offset+=Be32(source);
  }
  assert(index==libraries.size());
  auto truncated=std::span(image).first(64);
  assert(!gh::image::ParsePe(truncated,image_base,pe));
  assert(!metadata.ParseImports(truncated,image_base,libraries));
  std::printf("XEX metadata, %zu PE sections and %zu import records match legacy layout\n",
              pe.sections.size(),records);
}
