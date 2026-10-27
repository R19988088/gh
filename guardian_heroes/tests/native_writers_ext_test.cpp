#include "native_helpers_test.h"

#include <array>
#include <bit>
#include <random>
#include <utility>

REX_EXTERN(sub_82410160);
REX_EXTERN(__imp__sub_82410160);
REX_EXTERN(sub_82422F90);
REX_EXTERN(__imp__sub_82422F90);
REX_EXTERN(sub_8242B748);
REX_EXTERN(__imp__sub_8242B748);
REX_EXTERN(sub_8242B820);
REX_EXTERN(__imp__sub_8242B820);
REX_EXTERN(sub_8242B830);
REX_EXTERN(__imp__sub_8242B830);
REX_EXTERN(sub_8242B840);
REX_EXTERN(__imp__sub_8242B840);
REX_EXTERN(sub_8242BC58);
REX_EXTERN(__imp__sub_8242BC58);
REX_EXTERN(sub_82434B90);
REX_EXTERN(__imp__sub_82434B90);
REX_EXTERN(sub_82435B98);
REX_EXTERN(__imp__sub_82435B98);
REX_EXTERN(sub_8243EC38);
REX_EXTERN(__imp__sub_8243EC38);
REX_EXTERN(sub_82496728);
REX_EXTERN(__imp__sub_82496728);
REX_EXTERN(sub_824A4508);
REX_EXTERN(__imp__sub_824A4508);
REX_EXTERN(sub_82667CA8);
REX_EXTERN(__imp__sub_82667CA8);
REX_EXTERN(sub_82764FE8);
REX_EXTERN(__imp__sub_82764FE8);
REX_EXTERN(sub_82886DF0);
REX_EXTERN(__imp__sub_82886DF0);
REX_EXTERN(sub_8288C0E8);
REX_EXTERN(__imp__sub_8288C0E8);
REX_EXTERN(sub_8288EC58);
REX_EXTERN(__imp__sub_8288EC58);
REX_EXTERN(sub_824118F8);
REX_EXTERN(__imp__sub_824118F8);
REX_EXTERN(sub_82411918);
REX_EXTERN(__imp__sub_82411918);
REX_EXTERN(sub_82411938);
REX_EXTERN(__imp__sub_82411938);
REX_EXTERN(sub_82411958);
REX_EXTERN(__imp__sub_82411958);
REX_EXTERN(sub_82419DB0);
REX_EXTERN(__imp__sub_82419DB0);
REX_EXTERN(sub_82421B80);
REX_EXTERN(__imp__sub_82421B80);
REX_EXTERN(sub_82421C70);
REX_EXTERN(__imp__sub_82421C70);
REX_EXTERN(sub_82422F68);
REX_EXTERN(__imp__sub_82422F68);
REX_EXTERN(sub_82429A50);
REX_EXTERN(__imp__sub_82429A50);
REX_EXTERN(sub_8242A3D8);
REX_EXTERN(__imp__sub_8242A3D8);
REX_EXTERN(sub_8242A3E8);
REX_EXTERN(__imp__sub_8242A3E8);
REX_EXTERN(sub_8242C3F0);
REX_EXTERN(__imp__sub_8242C3F0);
REX_EXTERN(sub_8242C400);
REX_EXTERN(__imp__sub_8242C400);
REX_EXTERN(sub_8242F400);
REX_EXTERN(__imp__sub_8242F400);
REX_EXTERN(sub_8242F420);
REX_EXTERN(__imp__sub_8242F420);
REX_EXTERN(sub_82434968);
REX_EXTERN(__imp__sub_82434968);
REX_EXTERN(sub_82434AC0);
REX_EXTERN(__imp__sub_82434AC0);
REX_EXTERN(sub_82435B00);
REX_EXTERN(__imp__sub_82435B00);
REX_EXTERN(sub_82496538);
REX_EXTERN(__imp__sub_82496538);
REX_EXTERN(sub_825D8E30);
REX_EXTERN(__imp__sub_825D8E30);
REX_EXTERN(sub_82691810);
REX_EXTERN(__imp__sub_82691810);
REX_EXTERN(sub_8269AD68);
REX_EXTERN(__imp__sub_8269AD68);
REX_EXTERN(sub_8269BCA8);
REX_EXTERN(__imp__sub_8269BCA8);
REX_EXTERN(sub_826CAD80);
REX_EXTERN(__imp__sub_826CAD80);
REX_EXTERN(sub_82774B00);
REX_EXTERN(__imp__sub_82774B00);
REX_EXTERN(sub_8288E530);
REX_EXTERN(__imp__sub_8288E530);
REX_EXTERN(sub_82410548);
REX_EXTERN(__imp__sub_82410548);
REX_EXTERN(sub_82410590);
REX_EXTERN(__imp__sub_82410590);
REX_EXTERN(sub_824105A8);
REX_EXTERN(__imp__sub_824105A8);
REX_EXTERN(sub_82419D58);
REX_EXTERN(__imp__sub_82419D58);
REX_EXTERN(sub_825CFF78);
REX_EXTERN(__imp__sub_825CFF78);
REX_EXTERN(sub_825EF4D8);
REX_EXTERN(__imp__sub_825EF4D8);
REX_EXTERN(sub_825EF4F0);
REX_EXTERN(__imp__sub_825EF4F0);
REX_EXTERN(sub_825EF5E0);
REX_EXTERN(__imp__sub_825EF5E0);
REX_EXTERN(sub_825EF5F8);
REX_EXTERN(__imp__sub_825EF5F8);
REX_EXTERN(sub_8262DF00);
REX_EXTERN(__imp__sub_8262DF00);
REX_EXTERN(sub_82651510);
REX_EXTERN(__imp__sub_82651510);
REX_EXTERN(sub_8267FB40);
REX_EXTERN(__imp__sub_8267FB40);
REX_EXTERN(sub_8276AEC8);
REX_EXTERN(__imp__sub_8276AEC8);
REX_EXTERN(sub_828037C0);
REX_EXTERN(__imp__sub_828037C0);
REX_EXTERN(sub_82803EB0);
REX_EXTERN(__imp__sub_82803EB0);
REX_EXTERN(sub_82821D70);
REX_EXTERN(__imp__sub_82821D70);
REX_EXTERN(sub_8288EC40);
REX_EXTERN(__imp__sub_8288EC40);
REX_EXTERN(sub_82904398);
REX_EXTERN(__imp__sub_82904398);
REX_EXTERN(sub_82904E98);
REX_EXTERN(__imp__sub_82904E98);
REX_EXTERN(sub_82410560);
REX_EXTERN(__imp__sub_82410560);
REX_EXTERN(sub_82410578);
REX_EXTERN(__imp__sub_82410578);
REX_EXTERN(sub_8242AE70);
REX_EXTERN(__imp__sub_8242AE70);
REX_EXTERN(sub_8242AE88);
REX_EXTERN(__imp__sub_8242AE88);
REX_EXTERN(sub_825D8258);
REX_EXTERN(__imp__sub_825D8258);
REX_EXTERN(sub_8269CAA8);
REX_EXTERN(__imp__sub_8269CAA8);
REX_EXTERN(sub_826F0DC8);
REX_EXTERN(__imp__sub_826F0DC8);
REX_EXTERN(sub_826F1570);
REX_EXTERN(__imp__sub_826F1570);
REX_EXTERN(sub_82707800);
REX_EXTERN(__imp__sub_82707800);
REX_EXTERN(sub_827078C8);
REX_EXTERN(__imp__sub_827078C8);
REX_EXTERN(sub_82707BE0);
REX_EXTERN(__imp__sub_82707BE0);
REX_EXTERN(sub_82803898);
REX_EXTERN(__imp__sub_82803898);
REX_EXTERN(sub_82891930);
REX_EXTERN(__imp__sub_82891930);
REX_EXTERN(sub_82899950);
REX_EXTERN(__imp__sub_82899950);
REX_EXTERN(sub_824189D0);
REX_EXTERN(__imp__sub_824189D0);
REX_EXTERN(sub_82421B90);
REX_EXTERN(__imp__sub_82421B90);
REX_EXTERN(sub_82421BB0);
REX_EXTERN(__imp__sub_82421BB0);
REX_EXTERN(sub_82434948);
REX_EXTERN(__imp__sub_82434948);
REX_EXTERN(sub_824A4948);
REX_EXTERN(__imp__sub_824A4948);
REX_EXTERN(sub_825D75B0);
REX_EXTERN(__imp__sub_825D75B0);
REX_EXTERN(sub_825D75E0);
REX_EXTERN(__imp__sub_825D75E0);
REX_EXTERN(sub_825D7AD0);
REX_EXTERN(__imp__sub_825D7AD0);
REX_EXTERN(sub_825D7D18);
REX_EXTERN(__imp__sub_825D7D18);
REX_EXTERN(sub_825D7DF8);
REX_EXTERN(__imp__sub_825D7DF8);
REX_EXTERN(sub_825D7E98);
REX_EXTERN(__imp__sub_825D7E98);
REX_EXTERN(sub_825D7EC8);
REX_EXTERN(__imp__sub_825D7EC8);
REX_EXTERN(sub_825D7F68);
REX_EXTERN(__imp__sub_825D7F68);
REX_EXTERN(sub_825D8228);
REX_EXTERN(__imp__sub_825D8228);
REX_EXTERN(sub_825D8590);
REX_EXTERN(__imp__sub_825D8590);
REX_EXTERN(sub_825D86C0);
REX_EXTERN(__imp__sub_825D86C0);
REX_EXTERN(sub_825D8B70);
REX_EXTERN(__imp__sub_825D8B70);
REX_EXTERN(sub_825D8BA0);
REX_EXTERN(__imp__sub_825D8BA0);
REX_EXTERN(sub_825D8BC8);
REX_EXTERN(__imp__sub_825D8BC8);
REX_EXTERN(sub_825D8BF8);
REX_EXTERN(__imp__sub_825D8BF8);
REX_EXTERN(sub_825D8CD8);
REX_EXTERN(__imp__sub_825D8CD8);
REX_EXTERN(sub_825D8D70);
REX_EXTERN(__imp__sub_825D8D70);
REX_EXTERN(sub_825D8DA0);
REX_EXTERN(__imp__sub_825D8DA0);
REX_EXTERN(sub_825D8DD0);
REX_EXTERN(__imp__sub_825D8DD0);
REX_EXTERN(sub_826C9770);
REX_EXTERN(__imp__sub_826C9770);
REX_EXTERN(sub_826CF0E8);
REX_EXTERN(__imp__sub_826CF0E8);
REX_EXTERN(sub_826DA900);
REX_EXTERN(__imp__sub_826DA900);
REX_EXTERN(sub_82707DF8);
REX_EXTERN(__imp__sub_82707DF8);
REX_EXTERN(sub_8283C078);
REX_EXTERN(__imp__sub_8283C078);
REX_EXTERN(sub_82873180);
REX_EXTERN(__imp__sub_82873180);
REX_EXTERN(sub_82890350);
REX_EXTERN(__imp__sub_82890350);
REX_EXTERN(sub_82899430);
REX_EXTERN(__imp__sub_82899430);
REX_EXTERN(sub_82411D48);
REX_EXTERN(__imp__sub_82411D48);
REX_EXTERN(sub_825D8480);
REX_EXTERN(__imp__sub_825D8480);
REX_EXTERN(sub_825D85B0);
REX_EXTERN(__imp__sub_825D85B0);
REX_EXTERN(sub_82707BC0);
REX_EXTERN(__imp__sub_82707BC0);
REX_EXTERN(sub_825D7C00);
REX_EXTERN(__imp__sub_825D7C00);
REX_EXTERN(sub_825D7D48);
REX_EXTERN(__imp__sub_825D7D48);
REX_EXTERN(sub_825D7DC0);
REX_EXTERN(__imp__sub_825D7DC0);
REX_EXTERN(sub_825D7E28);
REX_EXTERN(__imp__sub_825D7E28);
REX_EXTERN(sub_825D7E60);
REX_EXTERN(__imp__sub_825D7E60);
REX_EXTERN(sub_825D7EF8);
REX_EXTERN(__imp__sub_825D7EF8);
REX_EXTERN(sub_825D7F30);
REX_EXTERN(__imp__sub_825D7F30);
REX_EXTERN(sub_825D84A0);
REX_EXTERN(__imp__sub_825D84A0);
REX_EXTERN(sub_825D84C8);
REX_EXTERN(__imp__sub_825D84C8);
REX_EXTERN(sub_825D84F0);
REX_EXTERN(__imp__sub_825D84F0);
REX_EXTERN(sub_825D8518);
REX_EXTERN(__imp__sub_825D8518);
REX_EXTERN(sub_825D8540);
REX_EXTERN(__imp__sub_825D8540);
REX_EXTERN(sub_825D8568);
REX_EXTERN(__imp__sub_825D8568);
REX_EXTERN(sub_825D85D0);
REX_EXTERN(__imp__sub_825D85D0);
REX_EXTERN(sub_825D85F8);
REX_EXTERN(__imp__sub_825D85F8);
REX_EXTERN(sub_825D8620);
REX_EXTERN(__imp__sub_825D8620);
REX_EXTERN(sub_825D8648);
REX_EXTERN(__imp__sub_825D8648);
REX_EXTERN(sub_825D8670);
REX_EXTERN(__imp__sub_825D8670);
REX_EXTERN(sub_825D8698);
REX_EXTERN(__imp__sub_825D8698);
REX_EXTERN(sub_825D8B00);
REX_EXTERN(__imp__sub_825D8B00);
REX_EXTERN(sub_8269AD40);
REX_EXTERN(__imp__sub_8269AD40);
REX_EXTERN(sub_826CAF20);
REX_EXTERN(__imp__sub_826CAF20);
REX_EXTERN(sub_826DA850);
REX_EXTERN(__imp__sub_826DA850);
REX_EXTERN(sub_82421B18);
REX_EXTERN(__imp__sub_82421B18);
REX_EXTERN(sub_826506A0);
REX_EXTERN(__imp__sub_826506A0);
REX_EXTERN(sub_826DA8A0);
REX_EXTERN(__imp__sub_826DA8A0);
REX_EXTERN(sub_827502D0);
REX_EXTERN(__imp__sub_827502D0);
REX_EXTERN(sub_82863090);
REX_EXTERN(__imp__sub_82863090);
REX_EXTERN(sub_8242B7F0);
REX_EXTERN(__imp__sub_8242B7F0);
REX_EXTERN(sub_82434860);
REX_EXTERN(__imp__sub_82434860);
REX_EXTERN(sub_82623768);
REX_EXTERN(__imp__sub_82623768);
REX_EXTERN(sub_8243EC58);
REX_EXTERN(__imp__sub_8243EC58);
REX_EXTERN(sub_827163F0);
REX_EXTERN(__imp__sub_827163F0);
REX_EXTERN(sub_8282CC90);
REX_EXTERN(__imp__sub_8282CC90);
REX_EXTERN(sub_82419218);
REX_EXTERN(__imp__sub_82419218);
REX_EXTERN(sub_8248B408);
REX_EXTERN(__imp__sub_8248B408);
REX_EXTERN(sub_8248B440);
REX_EXTERN(__imp__sub_8248B440);
REX_EXTERN(sub_825D95C8);
REX_EXTERN(__imp__sub_825D95C8);
REX_EXTERN(sub_825D9618);
REX_EXTERN(__imp__sub_825D9618);
REX_EXTERN(sub_825D9668);
REX_EXTERN(__imp__sub_825D9668);
REX_EXTERN(sub_825DD468);
REX_EXTERN(__imp__sub_825DD468);
REX_EXTERN(sub_829013A0);
REX_EXTERN(__imp__sub_829013A0);
REX_EXTERN(sub_8248B478);
REX_EXTERN(__imp__sub_8248B478);
REX_EXTERN(sub_8248B4B8);
REX_EXTERN(__imp__sub_8248B4B8);
REX_EXTERN(sub_8248B4F8);
REX_EXTERN(__imp__sub_8248B4F8);
REX_EXTERN(sub_8248B538);
REX_EXTERN(__imp__sub_8248B538);
REX_EXTERN(sub_825D9198);
REX_EXTERN(__imp__sub_825D9198);
REX_EXTERN(sub_825D96B8);
REX_EXTERN(__imp__sub_825D96B8);
REX_EXTERN(sub_825D9710);
REX_EXTERN(__imp__sub_825D9710);
REX_EXTERN(sub_825D9768);
REX_EXTERN(__imp__sub_825D9768);
REX_EXTERN(sub_825D97C0);
REX_EXTERN(__imp__sub_825D97C0);
REX_EXTERN(sub_82632DF8);
REX_EXTERN(__imp__sub_82632DF8);
REX_EXTERN(sub_8281D570);
REX_EXTERN(__imp__sub_8281D570);
REX_EXTERN(sub_82496088);
REX_EXTERN(__imp__sub_82496088);
REX_EXTERN(sub_825D9818);
REX_EXTERN(__imp__sub_825D9818);
REX_EXTERN(sub_82684FC0);
REX_EXTERN(__imp__sub_82684FC0);
REX_EXTERN(sub_828242B0);
REX_EXTERN(__imp__sub_828242B0);
REX_EXTERN(sub_828ED510);
REX_EXTERN(__imp__sub_828ED510);
REX_EXTERN(sub_82708080);
REX_EXTERN(__imp__sub_82708080);
REX_EXTERN(sub_82847EB8);
REX_EXTERN(__imp__sub_82847EB8);
REX_EXTERN(sub_82847F78);
REX_EXTERN(__imp__sub_82847F78);
REX_EXTERN(sub_824960D0);
REX_EXTERN(__imp__sub_824960D0);
REX_EXTERN(sub_82712B18);
REX_EXTERN(__imp__sub_82712B18);
REX_EXTERN(sub_8281D8C0);
REX_EXTERN(__imp__sub_8281D8C0);
REX_EXTERN(sub_8289A098);
REX_EXTERN(__imp__sub_8289A098);
REX_EXTERN(sub_82901088);
REX_EXTERN(__imp__sub_82901088);
REX_EXTERN(sub_829010E0);
REX_EXTERN(__imp__sub_829010E0);
REX_EXTERN(sub_82901138);
REX_EXTERN(__imp__sub_82901138);
REX_EXTERN(sub_82901190);
REX_EXTERN(__imp__sub_82901190);
REX_EXTERN(sub_829011E8);
REX_EXTERN(__imp__sub_829011E8);
REX_EXTERN(sub_82901240);
REX_EXTERN(__imp__sub_82901240);
REX_EXTERN(sub_82901298);
REX_EXTERN(__imp__sub_82901298);
REX_EXTERN(sub_829012F0);
REX_EXTERN(__imp__sub_829012F0);
REX_EXTERN(sub_82901348);
REX_EXTERN(__imp__sub_82901348);
REX_EXTERN(sub_82495F90);
REX_EXTERN(__imp__sub_82495F90);
REX_EXTERN(sub_82496B50);
REX_EXTERN(__imp__sub_82496B50);
REX_EXTERN(sub_82496DF8);
REX_EXTERN(__imp__sub_82496DF8);
REX_EXTERN(sub_8268F440);
REX_EXTERN(__imp__sub_8268F440);
REX_EXTERN(sub_82713030);
REX_EXTERN(__imp__sub_82713030);
REX_EXTERN(sub_82841488);
REX_EXTERN(__imp__sub_82841488);
REX_EXTERN(sub_82651948);
REX_EXTERN(__imp__sub_82651948);
REX_EXTERN(sub_8260E368);
REX_EXTERN(__imp__sub_8260E368);
REX_EXTERN(sub_8243EC90);
REX_EXTERN(__imp__sub_8243EC90);
REX_EXTERN(sub_825F95A8);
REX_EXTERN(__imp__sub_825F95A8);
REX_EXTERN(sub_826F21C8);
REX_EXTERN(__imp__sub_826F21C8);
REX_EXTERN(sub_825F7608);
REX_EXTERN(__imp__sub_825F7608);
REX_EXTERN(sub_825D90F8);
REX_EXTERN(__imp__sub_825D90F8);
REX_EXTERN(sub_82850218);
REX_EXTERN(__imp__sub_82850218);
REX_EXTERN(sub_825D8F50);
REX_EXTERN(__imp__sub_825D8F50);
REX_EXTERN(sub_825D91F0);
REX_EXTERN(__imp__sub_825D91F0);
REX_EXTERN(sub_82886AD8);
REX_EXTERN(__imp__sub_82886AD8);
REX_EXTERN(sub_82775648);
REX_EXTERN(__imp__sub_82775648);
REX_EXTERN(sub_82774DE0);
REX_EXTERN(__imp__sub_82774DE0);
REX_EXTERN(sub_825FA528);
REX_EXTERN(__imp__sub_825FA528);
REX_EXTERN(sub_827FB8B0);
REX_EXTERN(__imp__sub_827FB8B0);
REX_EXTERN(sub_827D36B8);
REX_EXTERN(__imp__sub_827D36B8);
REX_EXTERN(sub_825F7690);
REX_EXTERN(__imp__sub_825F7690);

namespace {
struct ReadField { uint8_t bits; uint32_t address; };
struct Function {
  const char* name;
  GuestFunction* original;
  GuestFunction* native;
  uint32_t pointer_registers;
  uint32_t index_registers;
  uint32_t used_registers;
  std::array<int32_t, 4> alias_offsets;
  void (*check)(NativeHelpersTest&, const Function&, const GuestContext&, uint64_t, uint32_t);
};

template <size_t N, size_t M, size_t... I>
void CheckFields(NativeHelpersTest& test, const Function& function,
                 const GuestContext& ctx, uint64_t value, uint32_t seed,
                 const std::array<NativeTestRegion, N>& regions,
                 const std::array<ReadField, M>& reads, std::index_sequence<I...>) {
  for (const auto& region : regions) test.Fill(region.address, region.size, seed++);
  for (size_t i = 0; i < reads.size(); ++i) {
    const auto field = reads[i];
    const uint64_t data = value ^ (uint64_t(i) * 0x13579BDF02468ACEull);
    if (field.bits == 8) test.Store8(field.address, uint8_t(data));
    else if (field.bits == 16) test.Store16(field.address, uint16_t(data));
    else if (field.bits == 32) test.Store32(field.address, uint32_t(data));
    else test.Store64(field.address, data);
  }
  test.Check(function.name, function.original, function.native, ctx, {regions[I]...});
}

template <size_t N, size_t M>
void CheckFields(NativeHelpersTest& test, const Function& function,
                 const GuestContext& ctx, uint64_t value, uint32_t seed,
                 const std::array<NativeTestRegion, N>& regions,
                 const std::array<ReadField, M>& reads) {
  CheckFields(test, function, ctx, value, seed, regions, reads, std::make_index_sequence<N>{});
}

const Function functions[] = {
  {"sub_82410160", __imp__sub_82410160, sub_82410160, 0x0u, 0x0u, 0x8u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x8297B179ull), 33},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82422F90", __imp__sub_82422F90, sub_82422F90, 0x0u, 0x0u, 0x8u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x829109B8ull), 36},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_8242B748", __imp__sub_8242B748, sub_8242B748, 0x0u, 0x0u, 0x8u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x82911E40ull), 33},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_8242B820", __imp__sub_8242B820, sub_8242B820, 0x0u, 0x0u, 0x8u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x829A1914ull), 36},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_8242B830", __imp__sub_8242B830, sub_8242B830, 0x0u, 0x0u, 0x8u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x829A1918ull), 36},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_8242B840", __imp__sub_8242B840, sub_8242B840, 0x0u, 0x0u, 0x8u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x829A191Cull), 36},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_8242BC58", __imp__sub_8242BC58, sub_8242BC58, 0x0u, 0x0u, 0x8u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x829A1834ull), 36},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82434B90", __imp__sub_82434B90, sub_82434B90, 0x0u, 0x0u, 0x8u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x829A2204ull), 36},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82435B98", __imp__sub_82435B98, sub_82435B98, 0x0u, 0x0u, 0x8u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x82911E8Cull), 33},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_8243EC38", __imp__sub_8243EC38, sub_8243EC38, 0x0u, 0x0u, 0x8u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x829AB39Cull), 36},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82496728", __imp__sub_82496728, sub_82496728, 0x8u, 0x10u, 0x38u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(((ctx.r4.u64 + ctx.r3.u64) + 4ull)), 33},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_824A4508", __imp__sub_824A4508, sub_824A4508, 0x0u, 0x0u, 0x8u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x829CC990ull), 36},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82667CA8", __imp__sub_82667CA8, sub_82667CA8, 0x10u, 0x40u, 0x58u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((((uint64_t(std::rotl(uint32_t(ctx.r6.u64), 1)) & 0xFFFFFFFEull) + ctx.r4.u64) - 16ull)), 34},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82764FE8", __imp__sub_82764FE8, sub_82764FE8, 0x8u, 0x10u, 0x38u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(((ctx.r3.u64 + ctx.r4.u64) - 3ull)), 33},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82886DF0", __imp__sub_82886DF0, sub_82886DF0, 0x0u, 0x0u, 0x8u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x82A05A0Cull), 36},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_8288C0E8", __imp__sub_8288C0E8, sub_8288C0E8, 0x0u, 0x0u, 0x8u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x82A05A68ull), 36},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_8288EC58", __imp__sub_8288EC58, sub_8288EC58, 0x0u, 0x0u, 0x8u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x82A05BFCull), 36},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_824118F8", __imp__sub_824118F8, sub_824118F8, 0x0u, 0x0u, 0x8u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x8297BF5Bull), 33},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82411918", __imp__sub_82411918, sub_82411918, 0x0u, 0x0u, 0x8u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x8297BF58ull), 33},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82411938", __imp__sub_82411938, sub_82411938, 0x0u, 0x0u, 0x8u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x8297BF59ull), 33},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82411958", __imp__sub_82411958, sub_82411958, 0x0u, 0x0u, 0x8u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x8297BF5Aull), 33},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82419DB0", __imp__sub_82419DB0, sub_82419DB0, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x829910F3ull), 33},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82421B80", __imp__sub_82421B80, sub_82421B80, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x82991508ull), 34},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82421C70", __imp__sub_82421C70, sub_82421C70, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 36},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82422F68", __imp__sub_82422F68, sub_82422F68, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x829916C0ull), 33},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82429A50", __imp__sub_82429A50, sub_82429A50, 0x0u, 0x0u, 0x8u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x82911D70ull), 36},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_8242A3D8", __imp__sub_8242A3D8, sub_8242A3D8, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x82991C48ull), 33},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_8242A3E8", __imp__sub_8242A3E8, sub_8242A3E8, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x82991C48ull), 33},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_8242C3F0", __imp__sub_8242C3F0, sub_8242C3F0, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x829A18E8ull), 36},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_8242C400", __imp__sub_8242C400, sub_8242C400, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x829A18E8ull), 36},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_8242F400", __imp__sub_8242F400, sub_8242F400, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x829A1941ull), 33},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_8242F420", __imp__sub_8242F420, sub_8242F420, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x829A1942ull), 33},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82434968", __imp__sub_82434968, sub_82434968, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x829A21FDull), 33},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82434AC0", __imp__sub_82434AC0, sub_82434AC0, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x829A21DAull), 33},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82435B00", __imp__sub_82435B00, sub_82435B00, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x829AACD8ull), 36},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82496538", __imp__sub_82496538, sub_82496538, 0x8u, 0x10u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(((ctx.r4.u64 + ctx.r3.u64) + 4ull)), 33},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_825D8E30", __imp__sub_825D8E30, sub_825D8E30, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 + 12140ull)), 36},
         }},
         std::array<ReadField, 1>{{
             {32, uint32_t((ctx.r3.u64 + 12156ull))},
         }});
   }},
  {"sub_82691810", __imp__sub_82691810, sub_82691810, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 36},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_8269AD68", __imp__sub_8269AD68, sub_8269AD68, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 36},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_8269BCA8", __imp__sub_8269BCA8, sub_8269BCA8, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 36},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_826CAD80", __imp__sub_826CAD80, sub_826CAD80, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 36},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82774B00", __imp__sub_82774B00, sub_82774B00, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 + 76ull)), 36},
         }},
         std::array<ReadField, 1>{{
             {32, uint32_t((ctx.r3.u64 + 92ull))},
         }});
   }},
  {"sub_8288E530", __imp__sub_8288E530, sub_8288E530, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x82A07310ull), 36},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82410548", __imp__sub_82410548, sub_82410548, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x8297B17Cull), 36},
         }},
         std::array<ReadField, 1>{{
             {32, uint32_t(0xFFFFFFFF8297B18Cull)},
         }});
   }},
  {"sub_82410590", __imp__sub_82410590, sub_82410590, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x8297B17Cull), 36},
         }},
         std::array<ReadField, 1>{{
             {32, uint32_t(0xFFFFFFFF8297B18Cull)},
         }});
   }},
  {"sub_824105A8", __imp__sub_824105A8, sub_824105A8, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x8297B17Cull), 36},
         }},
         std::array<ReadField, 1>{{
             {32, uint32_t(0xFFFFFFFF8297B18Cull)},
         }});
   }},
  {"sub_82419D58", __imp__sub_82419D58, sub_82419D58, 0x0u, 0x10u, 0x18u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(((uint64_t(std::rotl(uint32_t(ctx.r4.u64), 2)) & 0xFFFFFFFCull) - 2103972540ull)), 36},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_825CFF78", __imp__sub_825CFF78, sub_825CFF78, 0x0u, 0x0u, 0x8u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x82A0674Cull), 36},
         }},
         std::array<ReadField, 1>{{
             {32, uint32_t(0xFFFFFFFF82A0675Cull)},
         }});
   }},
  {"sub_825EF4D8", __imp__sub_825EF4D8, sub_825EF4D8, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x829CE8D7ull), 33},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_825EF4F0", __imp__sub_825EF4F0, sub_825EF4F0, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x829CE8D7ull), 33},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_825EF5E0", __imp__sub_825EF5E0, sub_825EF5E0, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x829CE8D5ull), 33},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_825EF5F8", __imp__sub_825EF5F8, sub_825EF5F8, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x829CE8D4ull), 33},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_8262DF00", __imp__sub_8262DF00, sub_8262DF00, 0x8u, 0x30u, 0x78u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(((((uint64_t(std::rotl(uint32_t((ctx.r4.u64 + 32ull)), 2)) & 0xFFFFFFFCull) + ctx.r5.u64) + ctx.r3.u64) - 16ull)), 33},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82651510", __imp__sub_82651510, sub_82651510, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 + 4ull)), 76},
         }},
         std::array<ReadField, 1>{{
             {32, uint32_t((ctx.r3.u64 + 20ull))},
         }});
   }},
  {"sub_8267FB40", __imp__sub_8267FB40, sub_8267FB40, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 12ull)), 36},
         }},
         std::array<ReadField, 1>{{
             {32, uint32_t((ctx.r3.u64 + 4ull))},
         }});
   }},
  {"sub_8276AEC8", __imp__sub_8276AEC8, sub_8276AEC8, 0x8u, 0x0u, 0x38u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 + 484ull)), 40},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_828037C0", __imp__sub_828037C0, sub_828037C0, 0x8u, 0x0u, 0x38u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 + 1508ull)), 40},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82803EB0", __imp__sub_82803EB0, sub_82803EB0, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 + 496ull)), 33},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82821D70", __imp__sub_82821D70, sub_82821D70, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 + 7184ull)), 36},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_8288EC40", __imp__sub_8288EC40, sub_8288EC40, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x82A05BF8ull), 36},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82904398", __imp__sub_82904398, sub_82904398, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 2>{{
             {uint32_t(0x823A2958ull), 36},
             {uint32_t(0x829DCA88ull), 36},
         }},
         std::array<ReadField, 1>{{
             {32, uint32_t(0xFFFFFFFF823A2968ull)},
         }});
   }},
  {"sub_82904E98", __imp__sub_82904E98, sub_82904E98, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x82979590ull), 36},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82410560", __imp__sub_82410560, sub_82410560, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x8297B17Cull), 36},
         }},
         std::array<ReadField, 1>{{
             {32, uint32_t(0xFFFFFFFF8297B18Cull)},
         }});
   }},
  {"sub_82410578", __imp__sub_82410578, sub_82410578, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x8297B17Cull), 36},
         }},
         std::array<ReadField, 1>{{
             {32, uint32_t(0xFFFFFFFF8297B18Cull)},
         }});
   }},
  {"sub_8242AE70", __imp__sub_8242AE70, sub_8242AE70, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 40},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_8242AE88", __imp__sub_8242AE88, sub_8242AE88, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 40},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_825D8258", __imp__sub_825D8258, sub_825D8258, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 + 16ull)), 10756},
         }},
         std::array<ReadField, 1>{{
             {64, uint32_t((ctx.r3.u64 + 32ull))},
         }});
   }},
  {"sub_8269CAA8", __imp__sub_8269CAA8, sub_8269CAA8, 0x8u, 0x0u, 0x38u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 42},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_826F0DC8", __imp__sub_826F0DC8, sub_826F0DC8, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 8ull)), 36},
         }},
         std::array<ReadField, 1>{{
             {32, uint32_t((ctx.r3.u64 + 8ull))},
         }});
   }},
  {"sub_826F1570", __imp__sub_826F1570, sub_826F1570, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 4ull)), 36},
         }},
         std::array<ReadField, 1>{{
             {32, uint32_t((ctx.r3.u64 + 12ull))},
         }});
   }},
  {"sub_82707800", __imp__sub_82707800, sub_82707800, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 + 308ull)), 36},
         }},
         std::array<ReadField, 1>{{
             {32, uint32_t((ctx.r3.u64 + 324ull))},
         }});
   }},
  {"sub_827078C8", __imp__sub_827078C8, sub_827078C8, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 + 304ull)), 36},
         }},
         std::array<ReadField, 1>{{
             {32, uint32_t((ctx.r3.u64 + 320ull))},
         }});
   }},
  {"sub_82707BE0", __imp__sub_82707BE0, sub_82707BE0, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 + 28ull)), 44},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82803898", __imp__sub_82803898, sub_82803898, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 + 1508ull)), 40},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82891930", __imp__sub_82891930, sub_82891930, 0x0u, 0x0u, 0x38u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x8297A0D4ull), 44},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82899950", __imp__sub_82899950, sub_82899950, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 40},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_824189D0", __imp__sub_824189D0, sub_824189D0, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x8297ECB4ull), 44},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82421B90", __imp__sub_82421B90, sub_82421B90, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x82991508ull), 34},
         }},
         std::array<ReadField, 1>{{
             {16, uint32_t(0xFFFFFFFF82991518ull)},
         }});
   }},
  {"sub_82421BB0", __imp__sub_82421BB0, sub_82421BB0, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x82991508ull), 34},
         }},
         std::array<ReadField, 1>{{
             {16, uint32_t(0xFFFFFFFF82991518ull)},
         }});
   }},
  {"sub_82434948", __imp__sub_82434948, sub_82434948, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x829A21FCull), 40},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_824A4948", __imp__sub_824A4948, sub_824A4948, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 + 1176ull)), 567},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_825D75B0", __imp__sub_825D75B0, sub_825D75B0, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10588},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10568ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D75E0", __imp__sub_825D75E0, sub_825D75E0, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10588},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10568ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D7AD0", __imp__sub_825D7AD0, sub_825D7AD0, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10576},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10556ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D7D18", __imp__sub_825D7D18, sub_825D7D18, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10568},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10548ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D7DF8", __imp__sub_825D7DF8, sub_825D7DF8, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10568},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10548ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D7E98", __imp__sub_825D7E98, sub_825D7E98, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10568},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10548ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D7EC8", __imp__sub_825D7EC8, sub_825D7EC8, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10568},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10548ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D7F68", __imp__sub_825D7F68, sub_825D7F68, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10568},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10548ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D8228", __imp__sub_825D8228, sub_825D8228, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10588},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10568ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D8590", __imp__sub_825D8590, sub_825D8590, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10560},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10540ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D86C0", __imp__sub_825D86C0, sub_825D86C0, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10564},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10544ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D8B70", __imp__sub_825D8B70, sub_825D8B70, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10588},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10568ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D8BA0", __imp__sub_825D8BA0, sub_825D8BA0, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10476},
         }},
         std::array<ReadField, 1>{{
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D8BC8", __imp__sub_825D8BC8, sub_825D8BC8, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10576},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10556ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D8BF8", __imp__sub_825D8BF8, sub_825D8BF8, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10576},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10556ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D8CD8", __imp__sub_825D8CD8, sub_825D8CD8, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 12176},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 12156ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D8D70", __imp__sub_825D8D70, sub_825D8D70, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10580},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10560ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D8DA0", __imp__sub_825D8DA0, sub_825D8DA0, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10580},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10560ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D8DD0", __imp__sub_825D8DD0, sub_825D8DD0, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10580},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10560ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_826C9770", __imp__sub_826C9770, sub_826C9770, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 44},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_826CF0E8", __imp__sub_826CF0E8, sub_826CF0E8, 0x18u, 0x0u, 0x18u,
   {0, 28},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 2>{{
             {uint32_t((ctx.r3.u64 + 12ull)), 38},
             {uint32_t((ctx.r4.u64 - 16ull)), 38},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 28ull))},
             {16, uint32_t((ctx.r3.u64 + 32ull))},
         }});
   }},
  {"sub_826DA900", __imp__sub_826DA900, sub_826DA900, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 39},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82707DF8", __imp__sub_82707DF8, sub_82707DF8, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 + 36ull)), 36},
         }},
         std::array<ReadField, 1>{{
             {32, uint32_t((ctx.r3.u64 + 52ull))},
         }});
   }},
  {"sub_8283C078", __imp__sub_8283C078, sub_8283C078, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 + 2140ull)), 40},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82873180", __imp__sub_82873180, sub_82873180, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 + 7068ull)), 40},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82890350", __imp__sub_82890350, sub_82890350, 0x0u, 0x0u, 0x18u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x82979E80ull), 36},
         }},
         std::array<ReadField, 1>{{
             {32, uint32_t(0xFFFFFFFF82979E90ull)},
         }});
   }},
  {"sub_82899430", __imp__sub_82899430, sub_82899430, 0x8u, 0x0u, 0xF8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 48},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82411D48", __imp__sub_82411D48, sub_82411D48, 0x0u, 0x0u, 0x38u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x8297BBD4ull), 36},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_825D8480", __imp__sub_825D8480, sub_825D8480, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10560},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10540ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D85B0", __imp__sub_825D85B0, sub_825D85B0, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10564},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10544ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_82707BC0", __imp__sub_82707BC0, sub_82707BC0, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 + 28ull)), 44},
         }},
         std::array<ReadField, 1>{{
             {64, uint32_t((ctx.r3.u64 + 48ull))},
         }});
   }},
  {"sub_825D7C00", __imp__sub_825D7C00, sub_825D7C00, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 + 16ull)), 10684},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10680ull))},
             {64, uint32_t((ctx.r3.u64 + 32ull))},
         }});
   }},
  {"sub_825D7D48", __imp__sub_825D7D48, sub_825D7D48, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10568},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10548ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D7DC0", __imp__sub_825D7DC0, sub_825D7DC0, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10568},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10548ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D7E28", __imp__sub_825D7E28, sub_825D7E28, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10568},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10548ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D7E60", __imp__sub_825D7E60, sub_825D7E60, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10568},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10548ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D7EF8", __imp__sub_825D7EF8, sub_825D7EF8, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10568},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10548ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D7F30", __imp__sub_825D7F30, sub_825D7F30, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10568},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10548ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D84A0", __imp__sub_825D84A0, sub_825D84A0, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10560},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10540ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D84C8", __imp__sub_825D84C8, sub_825D84C8, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10560},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10540ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D84F0", __imp__sub_825D84F0, sub_825D84F0, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10560},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10540ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D8518", __imp__sub_825D8518, sub_825D8518, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10560},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10540ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D8540", __imp__sub_825D8540, sub_825D8540, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10560},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10540ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D8568", __imp__sub_825D8568, sub_825D8568, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10560},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10540ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D85D0", __imp__sub_825D85D0, sub_825D85D0, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10564},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10544ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D85F8", __imp__sub_825D85F8, sub_825D85F8, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10564},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10544ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D8620", __imp__sub_825D8620, sub_825D8620, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10564},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10544ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D8648", __imp__sub_825D8648, sub_825D8648, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10564},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10544ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D8670", __imp__sub_825D8670, sub_825D8670, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10564},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10544ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D8698", __imp__sub_825D8698, sub_825D8698, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(ctx.r3.u64), 10564},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10544ull))},
             {64, uint32_t((ctx.r3.u64 + 16ull))},
         }});
   }},
  {"sub_825D8B00", __imp__sub_825D8B00, sub_825D8B00, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 + 8ull)), 10628},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 10616ull))},
             {64, uint32_t((ctx.r3.u64 + 24ull))},
         }});
   }},
  {"sub_8269AD40", __imp__sub_8269AD40, sub_8269AD40, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 + 4ull)), 53},
         }},
         std::array<ReadField, 1>{{
             {8, uint32_t((ctx.r3.u64 + 40ull))},
         }});
   }},
  {"sub_826CAF20", __imp__sub_826CAF20, sub_826CAF20, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 48},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_826DA850", __imp__sub_826DA850, sub_826DA850, 0x8u, 0x0u, 0x38u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 43},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82421B18", __imp__sub_82421B18, sub_82421B18, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x82991500ull), 42},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_826506A0", __imp__sub_826506A0, sub_826506A0, 0x8u, 0x0u, 0xF8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 56},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_826DA8A0", __imp__sub_826DA8A0, sub_826DA8A0, 0x8u, 0x0u, 0x38u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 41},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_827502D0", __imp__sub_827502D0, sub_827502D0, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 + 444ull)), 15204},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82863090", __imp__sub_82863090, sub_82863090, 0x12u, 0x40u, 0x5Au,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 2>{{
             {uint32_t((ctx.r1.u64 - 32ull)), 36},
             {uint32_t((((ctx.r6.u64 + (uint64_t(std::rotl(uint32_t(ctx.r6.u64), 1)) & 0xFFFFFFFEull)) + ctx.r4.u64) - 16ull)), 35},
         }},
         std::array<ReadField, 2>{{
             {8, uint32_t((ctx.r1.u64 - 14ull))},
             {16, uint32_t((ctx.r1.u64 - 16ull))},
         }});
   }},
  {"sub_8242B7F0", __imp__sub_8242B7F0, sub_8242B7F0, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x829A18B8ull), 64},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82434860", __imp__sub_82434860, sub_82434860, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 12ull)), 40},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((ctx.r3.u64 + 8ull))},
             {32, uint32_t((ctx.r3.u64 + 4ull))},
         }});
   }},
  {"sub_82623768", __imp__sub_82623768, sub_82623768, 0x78u, 0x0u, 0x78u,
   {0, 0, 0, 0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 4>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 36},
             {uint32_t((ctx.r4.u64 - 16ull)), 36},
             {uint32_t((ctx.r5.u64 - 16ull)), 36},
             {uint32_t((ctx.r6.u64 - 16ull)), 36},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_8243EC58", __imp__sub_8243EC58, sub_8243EC58, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 45},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_827163F0", __imp__sub_827163F0, sub_827163F0, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 38},
         }},
         std::array<ReadField, 6>{{
             {8, uint32_t(ctx.r3.u64)},
             {8, uint32_t((ctx.r3.u64 + 5ull))},
             {8, uint32_t((ctx.r3.u64 + 4ull))},
             {8, uint32_t((ctx.r3.u64 + 3ull))},
             {8, uint32_t((ctx.r3.u64 + 1ull))},
             {8, uint32_t((ctx.r3.u64 + 2ull))},
         }});
   }},
  {"sub_8282CC90", __imp__sub_8282CC90, sub_8282CC90, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 + 27956ull)), 48},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82419218", __imp__sub_82419218, sub_82419218, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 157},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_8248B408", __imp__sub_8248B408, sub_8248B408, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 57},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_8248B440", __imp__sub_8248B440, sub_8248B440, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 57},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_825D95C8", __imp__sub_825D95C8, sub_825D95C8, 0x8u, 0x10u, 0x38u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 2>{{
             {uint32_t(((((ctx.r4.u64 + 48ull) * 24ull) + ctx.r3.u64) - 16ull)), 36},
             {uint32_t((ctx.r3.u64 + 8ull)), 40},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((((ctx.r4.u64 + 48ull) * 24ull) + ctx.r3.u64))},
             {64, uint32_t((ctx.r3.u64 + 24ull))},
         }});
   }},
  {"sub_825D9618", __imp__sub_825D9618, sub_825D9618, 0x8u, 0x10u, 0x38u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 2>{{
             {uint32_t(((((ctx.r4.u64 + 48ull) * 24ull) + ctx.r3.u64) - 16ull)), 36},
             {uint32_t((ctx.r3.u64 + 8ull)), 40},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((((ctx.r4.u64 + 48ull) * 24ull) + ctx.r3.u64))},
             {64, uint32_t((ctx.r3.u64 + 24ull))},
         }});
   }},
  {"sub_825D9668", __imp__sub_825D9668, sub_825D9668, 0x8u, 0x10u, 0x38u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 2>{{
             {uint32_t(((((ctx.r4.u64 + 48ull) * 24ull) + ctx.r3.u64) - 16ull)), 36},
             {uint32_t((ctx.r3.u64 + 8ull)), 40},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t((((ctx.r4.u64 + 48ull) * 24ull) + ctx.r3.u64))},
             {64, uint32_t((ctx.r3.u64 + 24ull))},
         }});
   }},
  {"sub_825DD468", __imp__sub_825DD468, sub_825DD468, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 72},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_829013A0", __imp__sub_829013A0, sub_829013A0, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 2>{{
             {uint32_t(0x820825C8ull), 3308},
             {uint32_t(0x829CCDC4ull), 60},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t(0xFFFFFFFF820832A0ull)},
             {32, uint32_t(0xFFFFFFFF820825D8ull)},
         }});
   }},
  {"sub_8248B478", __imp__sub_8248B478, sub_8248B478, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 57},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_8248B4B8", __imp__sub_8248B4B8, sub_8248B4B8, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 57},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_8248B4F8", __imp__sub_8248B4F8, sub_8248B4F8, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 57},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_8248B538", __imp__sub_8248B538, sub_8248B538, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 57},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_825D9198", __imp__sub_825D9198, sub_825D9198, 0x8u, 0x10u, 0x38u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 2>{{
             {uint32_t(((((ctx.r4.u64 + 48ull) * 24ull) + ctx.r3.u64) - 4ull)), 36},
             {uint32_t((ctx.r3.u64 + 8ull)), 40},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t(((((ctx.r4.u64 + 48ull) * 24ull) + ctx.r3.u64) + 12ull))},
             {64, uint32_t((ctx.r3.u64 + 24ull))},
         }});
   }},
  {"sub_825D96B8", __imp__sub_825D96B8, sub_825D96B8, 0x8u, 0x10u, 0x38u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 2>{{
             {uint32_t(((((ctx.r4.u64 + 48ull) * 24ull) + ctx.r3.u64) + 4ull)), 36},
             {uint32_t((ctx.r3.u64 + 8ull)), 40},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t(((((ctx.r4.u64 + 48ull) * 24ull) + ctx.r3.u64) + 20ull))},
             {64, uint32_t((ctx.r3.u64 + 24ull))},
         }});
   }},
  {"sub_825D9710", __imp__sub_825D9710, sub_825D9710, 0x8u, 0x10u, 0x38u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 2>{{
             {uint32_t((((ctx.r4.u64 + 48ull) * 24ull) + ctx.r3.u64)), 36},
             {uint32_t((ctx.r3.u64 + 8ull)), 40},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t(((((ctx.r4.u64 + 48ull) * 24ull) + ctx.r3.u64) + 16ull))},
             {64, uint32_t((ctx.r3.u64 + 24ull))},
         }});
   }},
  {"sub_825D9768", __imp__sub_825D9768, sub_825D9768, 0x8u, 0x10u, 0x38u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 2>{{
             {uint32_t((((ctx.r4.u64 + 48ull) * 24ull) + ctx.r3.u64)), 36},
             {uint32_t((ctx.r3.u64 + 8ull)), 40},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t(((((ctx.r4.u64 + 48ull) * 24ull) + ctx.r3.u64) + 16ull))},
             {64, uint32_t((ctx.r3.u64 + 24ull))},
         }});
   }},
  {"sub_825D97C0", __imp__sub_825D97C0, sub_825D97C0, 0x8u, 0x10u, 0x38u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 2>{{
             {uint32_t(((((ctx.r4.u64 + 48ull) * 24ull) + ctx.r3.u64) + 4ull)), 36},
             {uint32_t((ctx.r3.u64 + 8ull)), 40},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t(((((ctx.r4.u64 + 48ull) * 24ull) + ctx.r3.u64) + 20ull))},
             {64, uint32_t((ctx.r3.u64 + 24ull))},
         }});
   }},
  {"sub_82632DF8", __imp__sub_82632DF8, sub_82632DF8, 0x18u, 0x0u, 0x18u,
   {0, -8},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 2>{{
             {uint32_t(ctx.r3.u64), 416},
             {uint32_t((ctx.r4.u64 + 8ull)), 60},
         }},
         std::array<ReadField, 7>{{
             {32, uint32_t((ctx.r3.u64 + 16ull))},
             {32, uint32_t((ctx.r3.u64 + 396ull))},
             {32, uint32_t((ctx.r3.u64 + 20ull))},
             {32, uint32_t((ctx.r3.u64 + 24ull))},
             {32, uint32_t((ctx.r3.u64 + 28ull))},
             {32, uint32_t((ctx.r3.u64 + 32ull))},
             {32, uint32_t((ctx.r3.u64 + 36ull))},
         }});
   }},
  {"sub_8281D570", __imp__sub_8281D570, sub_8281D570, 0x8u, 0x0u, 0x78u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 92},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82496088", __imp__sub_82496088, sub_82496088, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 14ull)), 40},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_825D9818", __imp__sub_825D9818, sub_825D9818, 0x8u, 0x10u, 0x38u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 2>{{
             {uint32_t(((((ctx.r4.u64 + 48ull) * 24ull) + ctx.r3.u64) - 12ull)), 36},
             {uint32_t((ctx.r3.u64 + 8ull)), 40},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t(((((ctx.r4.u64 + 48ull) * 24ull) + ctx.r3.u64) + 4ull))},
             {64, uint32_t((ctx.r3.u64 + 24ull))},
         }});
   }},
  {"sub_82684FC0", __imp__sub_82684FC0, sub_82684FC0, 0x8u, 0x0u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 176},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_828242B0", __imp__sub_828242B0, sub_828242B0, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 + 8192ull)), 60},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_828ED510", __imp__sub_828ED510, sub_828ED510, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x82991714ull), 64},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82708080", __imp__sub_82708080, sub_82708080, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 + 28ull)), 124},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82847EB8", __imp__sub_82847EB8, sub_82847EB8, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 164},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82847F78", __imp__sub_82847F78, sub_82847F78, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 172},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_824960D0", __imp__sub_824960D0, sub_824960D0, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 6ull)), 42},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82712B18", __imp__sub_82712B18, sub_82712B18, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 + 316ull)), 21572},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_8281D8C0", __imp__sub_8281D8C0, sub_8281D8C0, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 + 2040ull)), 64},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_8289A098", __imp__sub_8289A098, sub_8289A098, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 100},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82901088", __imp__sub_82901088, sub_82901088, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 2>{{
             {uint32_t(0x8205452Cull), 25492},
             {uint32_t(0x829CCCC8ull), 60},
         }},
         std::array<ReadField, 6>{{
             {32, uint32_t(0xFFFFFFFF820592DCull)},
             {32, uint32_t(0xFFFFFFFF8205453Cull)},
             {32, uint32_t(0xFFFFFFFF82055AC4ull)},
             {32, uint32_t(0xFFFFFFFF82058728ull)},
             {32, uint32_t(0xFFFFFFFF82057068ull)},
             {32, uint32_t(0xFFFFFFFF8205A8ACull)},
         }});
   }},
  {"sub_829010E0", __imp__sub_829010E0, sub_829010E0, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 4>{{
             {uint32_t(0x82044AE0ull), 35520},
             {uint32_t(0x82039D28ull), 23172},
             {uint32_t(0x820529CCull), 36},
             {uint32_t(0x829CCCE4ull), 60},
         }},
         std::array<ReadField, 6>{{
             {32, uint32_t(0xFFFFFFFF8204D58Cull)},
             {32, uint32_t(0xFFFFFFFF82039D38ull)},
             {32, uint32_t(0xFFFFFFFF8203F798ull)},
             {32, uint32_t(0xFFFFFFFF8204A648ull)},
             {32, uint32_t(0xFFFFFFFF82044AF0ull)},
             {32, uint32_t(0xFFFFFFFF820529DCull)},
         }});
   }},
  {"sub_82901138", __imp__sub_82901138, sub_82901138, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 2>{{
             {uint32_t(0x82030A50ull), 16508},
             {uint32_t(0x829CCD00ull), 60},
         }},
         std::array<ReadField, 6>{{
             {32, uint32_t(0xFFFFFFFF82033DC8ull)},
             {32, uint32_t(0xFFFFFFFF82030A60ull)},
             {32, uint32_t(0xFFFFFFFF8203187Cull)},
             {32, uint32_t(0xFFFFFFFF820332B8ull)},
             {32, uint32_t(0xFFFFFFFF82032594ull)},
             {32, uint32_t(0xFFFFFFFF82034AB8ull)},
         }});
   }},
  {"sub_82901190", __imp__sub_82901190, sub_82901190, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 3>{{
             {uint32_t(0x82072528ull), 50364},
             {uint32_t(0x8206ECCCull), 36},
             {uint32_t(0x829CCD1Cull), 60},
         }},
         std::array<ReadField, 6>{{
             {32, uint32_t(0xFFFFFFFF8207B1CCull)},
             {32, uint32_t(0xFFFFFFFF8206ECDCull)},
             {32, uint32_t(0xFFFFFFFF82072538ull)},
             {32, uint32_t(0xFFFFFFFF82079904ull)},
             {32, uint32_t(0xFFFFFFFF82075D30ull)},
             {32, uint32_t(0xFFFFFFFF8207E9D0ull)},
         }});
   }},
  {"sub_829011E8", __imp__sub_829011E8, sub_829011E8, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 3>{{
             {uint32_t(0x82060B60ull), 43092},
             {uint32_t(0x8205DABCull), 36},
             {uint32_t(0x829CCD38ull), 60},
         }},
         std::array<ReadField, 6>{{
             {32, uint32_t(0xFFFFFFFF820683E0ull)},
             {32, uint32_t(0xFFFFFFFF8205DACCull)},
             {32, uint32_t(0xFFFFFFFF82060B70ull)},
             {32, uint32_t(0xFFFFFFFF82066EB4ull)},
             {32, uint32_t(0xFFFFFFFF82063D50ull)},
             {32, uint32_t(0xFFFFFFFF8206B3A0ull)},
         }});
   }},
  {"sub_82901240", __imp__sub_82901240, sub_82901240, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 2>{{
             {uint32_t(0x820212E0ull), 13264},
             {uint32_t(0x829CCD54ull), 60},
         }},
         std::array<ReadField, 6>{{
             {32, uint32_t(0xFFFFFFFF82023B6Cull)},
             {32, uint32_t(0xFFFFFFFF820212F0ull)},
             {32, uint32_t(0xFFFFFFFF82021E3Cull)},
             {32, uint32_t(0xFFFFFFFF820235C8ull)},
             {32, uint32_t(0xFFFFFFFF820229C0ull)},
             {32, uint32_t(0xFFFFFFFF8202469Cull)},
         }});
   }},
  {"sub_82901298", __imp__sub_82901298, sub_82901298, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 2>{{
             {uint32_t(0x82026CE4ull), 34832},
             {uint32_t(0x829CCD70ull), 60},
         }},
         std::array<ReadField, 6>{{
             {32, uint32_t(0xFFFFFFFF8202D718ull)},
             {32, uint32_t(0xFFFFFFFF82026CF4ull)},
             {32, uint32_t(0xFFFFFFFF820288D8ull)},
             {32, uint32_t(0xFFFFFFFF8202C6B8ull)},
             {32, uint32_t(0xFFFFFFFF8202A4B4ull)},
             {32, uint32_t(0xFFFFFFFF8202F4E0ull)},
         }});
   }},
  {"sub_829012F0", __imp__sub_829012F0, sub_829012F0, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 2>{{
             {uint32_t(0x82026CE8ull), 34832},
             {uint32_t(0x829CCD8Cull), 60},
         }},
         std::array<ReadField, 6>{{
             {32, uint32_t(0xFFFFFFFF8202D71Cull)},
             {32, uint32_t(0xFFFFFFFF82026CF8ull)},
             {32, uint32_t(0xFFFFFFFF820288DCull)},
             {32, uint32_t(0xFFFFFFFF8202C6BCull)},
             {32, uint32_t(0xFFFFFFFF8202A4B8ull)},
             {32, uint32_t(0xFFFFFFFF8202F4E4ull)},
         }});
   }},
  {"sub_82901348", __imp__sub_82901348, sub_82901348, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 2>{{
             {uint32_t(0x820837ACull), 7956},
             {uint32_t(0x829CCDA8ull), 60},
         }},
         std::array<ReadField, 6>{{
             {32, uint32_t(0xFFFFFFFF820850C8ull)},
             {32, uint32_t(0xFFFFFFFF820837BCull)},
             {32, uint32_t(0xFFFFFFFF82083EC4ull)},
             {32, uint32_t(0xFFFFFFFF82084E14ull)},
             {32, uint32_t(0xFFFFFFFF82084510ull)},
             {32, uint32_t(0xFFFFFFFF820856ACull)},
         }});
   }},
  {"sub_82495F90", __imp__sub_82495F90, sub_82495F90, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 112},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82496B50", __imp__sub_82496B50, sub_82496B50, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 850},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82496DF8", __imp__sub_82496DF8, sub_82496DF8, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 2ull)), 51},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_8268F440", __imp__sub_8268F440, sub_8268F440, 0x8u, 0x0u, 0x38u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 96},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82713030", __imp__sub_82713030, sub_82713030, 0x8u, 0x10u, 0x18u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 2>{{
             {uint32_t((ctx.r3.u64 + 392ull)), 60},
             {uint32_t(((uint64_t(std::rotl(uint32_t(ctx.r4.u64), 2)) & 0xFFFFFFFCull) - 2110111452ull)), 52},
         }},
         std::array<ReadField, 2>{{
             {32, uint32_t(((uint64_t(std::rotl(uint32_t(ctx.r4.u64), 2)) & 0xFFFFFFFCull) - 2110111436ull))},
             {32, uint32_t(((uint64_t(std::rotl(uint32_t(ctx.r4.u64), 2)) & 0xFFFFFFFCull) - 2110111420ull))},
         }});
   }},
  {"sub_82841488", __imp__sub_82841488, sub_82841488, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t(0x829FFD5Cull), 56},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_82651948", __imp__sub_82651948, sub_82651948, 0x8u, 0x0u, 0x38u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 112},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_8260E368", __imp__sub_8260E368, sub_8260E368, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 164},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_8243EC90", __imp__sub_8243EC90, sub_8243EC90, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 112},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_825F95A8", __imp__sub_825F95A8, sub_825F95A8, 0x18u, 0x0u, 0x18u,
   {0, 0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 2>{{
             {uint32_t((ctx.r4.u64 - 13ull)), 45},
             {uint32_t((ctx.r3.u64 - 13ull)), 45},
         }},
         std::array<ReadField, 6>{{
             {16, uint32_t((ctx.r4.u64 + 3ull))},
             {16, uint32_t((ctx.r4.u64 + 5ull))},
             {16, uint32_t((ctx.r4.u64 + 8ull))},
             {16, uint32_t((ctx.r4.u64 + 10ull))},
             {16, uint32_t((ctx.r4.u64 + 12ull))},
             {16, uint32_t((ctx.r4.u64 + 14ull))},
         }});
   }},
  {"sub_826F21C8", __imp__sub_826F21C8, sub_826F21C8, 0x0u, 0x0u, 0x0u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 2>{{
             {uint32_t(0x829E4498ull), 72},
             {uint32_t(0x829463D0ull), 40},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_825F7608", __imp__sub_825F7608, sub_825F7608, 0x18u, 0x0u, 0x18u,
   {0, 0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 2>{{
             {uint32_t((ctx.r4.u64 - 16ull)), 46},
             {uint32_t((ctx.r3.u64 - 16ull)), 46},
         }},
         std::array<ReadField, 5>{{
             {16, uint32_t(ctx.r4.u64)},
             {32, uint32_t((ctx.r4.u64 + 2ull))},
             {16, uint32_t((ctx.r4.u64 + 6ull))},
             {16, uint32_t((ctx.r4.u64 + 8ull))},
             {32, uint32_t((ctx.r4.u64 + 10ull))},
         }});
   }},
  {"sub_825D90F8", __imp__sub_825D90F8, sub_825D90F8, 0xAu, 0x10u, 0xC000003Au,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 4>{{
             {uint32_t((ctx.r1.u64 - 32ull)), 48},
             {uint32_t(((ctx.r3.u64 + ctx.r4.u64) + 10874ull)), 33},
             {uint32_t(((((ctx.r4.u64 + 48ull) * 24ull) + ctx.r3.u64) - 4ull)), 40},
             {uint32_t((ctx.r3.u64 + 8ull)), 40},
         }},
         std::array<ReadField, 6>{{
             {8, uint32_t(((ctx.r3.u64 + ctx.r4.u64) + 10890ull))},
             {32, uint32_t(((((ctx.r4.u64 + 48ull) * 24ull) + ctx.r3.u64) + 12ull))},
             {32, uint32_t(((((ctx.r4.u64 + 48ull) * 24ull) + ctx.r3.u64) + 16ull))},
             {64, uint32_t((ctx.r3.u64 + 24ull))},
             {64, uint32_t((ctx.r1.u64 - 16ull))},
             {64, uint32_t((ctx.r1.u64 - 8ull))},
         }});
   }},
  {"sub_82850218", __imp__sub_82850218, sub_82850218, 0xAu, 0x0u, 0xC000000Au,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 2>{{
             {uint32_t((ctx.r1.u64 - 32ull)), 48},
             {uint32_t((ctx.r3.u64 + 3152ull)), 17588},
         }},
         std::array<ReadField, 2>{{
             {64, uint32_t((ctx.r1.u64 - 16ull))},
             {64, uint32_t((ctx.r1.u64 - 8ull))},
         }});
   }},
  {"sub_825D8F50", __imp__sub_825D8F50, sub_825D8F50, 0xAu, 0x10u, 0xC000003Au,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 4>{{
             {uint32_t((ctx.r1.u64 - 32ull)), 48},
             {uint32_t(((ctx.r3.u64 + ctx.r4.u64) + 10874ull)), 33},
             {uint32_t(((((ctx.r4.u64 + 48ull) * 24ull) + ctx.r3.u64) - 4ull)), 40},
             {uint32_t((ctx.r3.u64 + 8ull)), 40},
         }},
         std::array<ReadField, 6>{{
             {8, uint32_t(((ctx.r3.u64 + ctx.r4.u64) + 10890ull))},
             {32, uint32_t(((((ctx.r4.u64 + 48ull) * 24ull) + ctx.r3.u64) + 12ull))},
             {32, uint32_t(((((ctx.r4.u64 + 48ull) * 24ull) + ctx.r3.u64) + 16ull))},
             {64, uint32_t((ctx.r3.u64 + 24ull))},
             {64, uint32_t((ctx.r1.u64 - 16ull))},
             {64, uint32_t((ctx.r1.u64 - 8ull))},
         }});
   }},
  {"sub_825D91F0", __imp__sub_825D91F0, sub_825D91F0, 0xAu, 0x10u, 0xC000003Au,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 4>{{
             {uint32_t((ctx.r1.u64 - 32ull)), 48},
             {uint32_t(((ctx.r3.u64 + ctx.r4.u64) + 10874ull)), 33},
             {uint32_t(((((ctx.r4.u64 + 48ull) * 24ull) + ctx.r3.u64) - 4ull)), 40},
             {uint32_t((ctx.r3.u64 + 8ull)), 40},
         }},
         std::array<ReadField, 6>{{
             {8, uint32_t(((ctx.r3.u64 + ctx.r4.u64) + 10890ull))},
             {32, uint32_t(((((ctx.r4.u64 + 48ull) * 24ull) + ctx.r3.u64) + 12ull))},
             {32, uint32_t(((((ctx.r4.u64 + 48ull) * 24ull) + ctx.r3.u64) + 16ull))},
             {64, uint32_t((ctx.r3.u64 + 24ull))},
             {64, uint32_t((ctx.r1.u64 - 16ull))},
             {64, uint32_t((ctx.r1.u64 - 8ull))},
         }});
   }},
  {"sub_82886AD8", __imp__sub_82886AD8, sub_82886AD8, 0x2u, 0x0u, 0xC0000002u,
   {},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 2>{{
             {uint32_t((ctx.r1.u64 - 32ull)), 48},
             {uint32_t(0x82979AA8ull), 72},
         }},
         std::array<ReadField, 2>{{
             {64, uint32_t((ctx.r1.u64 - 16ull))},
             {64, uint32_t((ctx.r1.u64 - 8ull))},
         }});
   }},
  {"sub_82775648", __imp__sub_82775648, sub_82775648, 0x18u, 0x0u, 0x18u,
   {0, 30976},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 2>{{
             {uint32_t((ctx.r4.u64 - 16ull)), 136},
             {uint32_t((ctx.r3.u64 + 30960ull)), 140},
         }},
         std::array<ReadField, 26>{{
             {32, uint32_t(ctx.r4.u64)},
             {32, uint32_t((ctx.r4.u64 + 4ull))},
             {32, uint32_t((ctx.r4.u64 + 8ull))},
             {32, uint32_t((ctx.r4.u64 + 12ull))},
             {32, uint32_t((ctx.r4.u64 + 16ull))},
             {32, uint32_t((ctx.r4.u64 + 20ull))},
             {32, uint32_t((ctx.r4.u64 + 28ull))},
             {32, uint32_t((ctx.r4.u64 + 24ull))},
             {32, uint32_t((ctx.r4.u64 + 32ull))},
             {32, uint32_t((ctx.r4.u64 + 36ull))},
             {32, uint32_t((ctx.r4.u64 + 40ull))},
             {32, uint32_t((ctx.r4.u64 + 44ull))},
             {32, uint32_t((ctx.r4.u64 + 48ull))},
             {32, uint32_t((ctx.r4.u64 + 52ull))},
             {32, uint32_t((ctx.r4.u64 + 64ull))},
             {32, uint32_t((ctx.r4.u64 + 56ull))},
             {32, uint32_t((ctx.r4.u64 + 68ull))},
             {32, uint32_t((ctx.r4.u64 + 72ull))},
             {32, uint32_t((ctx.r4.u64 + 76ull))},
             {32, uint32_t((ctx.r4.u64 + 84ull))},
             {32, uint32_t((ctx.r4.u64 + 80ull))},
             {32, uint32_t((ctx.r4.u64 + 92ull))},
             {32, uint32_t((ctx.r4.u64 + 96ull))},
             {32, uint32_t((ctx.r4.u64 + 88ull))},
             {32, uint32_t((ctx.r4.u64 + 60ull))},
             {32, uint32_t((ctx.r4.u64 + 100ull))},
         }});
   }},
  {"sub_82774DE0", __imp__sub_82774DE0, sub_82774DE0, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 + 756ull)), 18468},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_825FA528", __imp__sub_825FA528, sub_825FA528, 0x18u, 0x0u, 0x18u,
   {0, 0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 2>{{
             {uint32_t((ctx.r4.u64 - 16ull)), 64},
             {uint32_t((ctx.r3.u64 - 16ull)), 64},
         }},
         std::array<ReadField, 8>{{
             {32, uint32_t(ctx.r4.u64)},
             {32, uint32_t((ctx.r4.u64 + 4ull))},
             {32, uint32_t((ctx.r4.u64 + 8ull))},
             {32, uint32_t((ctx.r4.u64 + 12ull))},
             {32, uint32_t((ctx.r4.u64 + 16ull))},
             {32, uint32_t((ctx.r4.u64 + 20ull))},
             {32, uint32_t((ctx.r4.u64 + 24ull))},
             {32, uint32_t((ctx.r4.u64 + 28ull))},
         }});
   }},
  {"sub_827FB8B0", __imp__sub_827FB8B0, sub_827FB8B0, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 14700},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_827D36B8", __imp__sub_827D36B8, sub_827D36B8, 0x8u, 0x0u, 0x8u,
   {0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 1>{{
             {uint32_t((ctx.r3.u64 - 16ull)), 14752},
         }},
         std::array<ReadField, 0>{{
         }});
   }},
  {"sub_825F7690", __imp__sub_825F7690, sub_825F7690, 0x18u, 0x0u, 0x18u,
   {0, 0},
   [](NativeHelpersTest& test, const Function& function, const GuestContext& ctx, uint64_t value, uint32_t seed) {
     CheckFields(test, function, ctx, value, seed,
         std::array<NativeTestRegion, 2>{{
             {uint32_t((ctx.r4.u64 - 16ull)), 72},
             {uint32_t((ctx.r3.u64 - 16ull)), 72},
         }},
         std::array<ReadField, 11>{{
             {32, uint32_t(ctx.r4.u64)},
             {32, uint32_t((ctx.r4.u64 + 4ull))},
             {32, uint32_t((ctx.r4.u64 + 8ull))},
             {16, uint32_t((ctx.r4.u64 + 12ull))},
             {16, uint32_t((ctx.r4.u64 + 14ull))},
             {32, uint32_t((ctx.r4.u64 + 16ull))},
             {32, uint32_t((ctx.r4.u64 + 20ull))},
             {32, uint32_t((ctx.r4.u64 + 24ull))},
             {32, uint32_t((ctx.r4.u64 + 28ull))},
             {32, uint32_t((ctx.r4.u64 + 32ull))},
             {32, uint32_t((ctx.r4.u64 + 36ull))},
         }});
   }},
};

GuestRegister& Register(GuestContext& ctx, unsigned index) {
  switch (index) {
    case 0: return ctx.r0;
    case 1: return ctx.r1;
    case 2: return ctx.r2;
    case 3: return ctx.r3;
    case 4: return ctx.r4;
    case 5: return ctx.r5;
    case 6: return ctx.r6;
    case 7: return ctx.r7;
    case 8: return ctx.r8;
    case 9: return ctx.r9;
    case 10: return ctx.r10;
    case 11: return ctx.r11;
    case 12: return ctx.r12;
    case 13: return ctx.r13;
    case 14: return ctx.r14;
    case 15: return ctx.r15;
    case 16: return ctx.r16;
    case 17: return ctx.r17;
    case 18: return ctx.r18;
    case 19: return ctx.r19;
    case 20: return ctx.r20;
    case 21: return ctx.r21;
    case 22: return ctx.r22;
    case 23: return ctx.r23;
    case 24: return ctx.r24;
    case 25: return ctx.r25;
    case 26: return ctx.r26;
    case 27: return ctx.r27;
    case 28: return ctx.r28;
    case 29: return ctx.r29;
    case 30: return ctx.r30;
    case 31: return ctx.r31;
    default: std::abort();
  }
}

void CheckCase(NativeHelpersTest& test, const Function& function, uint64_t value,
               unsigned layout, uint32_t seed) {
  auto ctx = test.Context(seed);
  const uint32_t object = layout == 4 ? 0xE0010000u : NativeHelpersTest::kObject;
  constexpr std::array<uint32_t, 12> indices = {0, 1, 2, 3, 7, 15, 31, 32, 63, 64, 0xFFFFFFE0u, 0xFFFFFFDFu};
  unsigned pointer_index = 0;
  for (unsigned r = 0; r < 32; ++r) {
    if (!(function.used_registers & (1u << r)) || r == 1) continue;
    auto& reg = Register(ctx, r);
    const uint64_t high = uint64_t(((seed & 1) ? 0xFEDCBA98u : 0x13579BDFu) ^ (r * 0x1020304u)) << 32;
    if (function.pointer_registers & (1u << r)) {
      uint32_t address = object + pointer_index * 0x20000;
      if (layout == 1) address = object;
      if (layout == 2) address = object + function.alias_offsets[pointer_index] + pointer_index * 4;
      if (layout == 3) address = object + function.alias_offsets[pointer_index] + pointer_index * 2 + 1;
      ++pointer_index;
      reg.u64 = high | address;
    } else if (function.index_registers & (1u << r)) {
      reg.u64 = high | indices[seed % indices.size()];
    } else {
      const uint32_t scalar = uint32_t(value) ^
          uint32_t((value >> 32) * (r + 1) * 0x9E3779B1ull);
      reg.u64 = high | scalar;
    }
  }
  function.check(test, function, ctx, value, seed);
}
}  // namespace

int main() {
  NativeHelpersTest test;
  uint32_t seed = 0;
  constexpr std::array<uint64_t, 14> values = {
      0, 1, 0x7F, 0x80, 0xFF, 0x100, 0xFFFF, 0x10000,
      0x7FFFFFFF, 0x80000000, 0xFFFFFFFF, 0xFFFFFFFFFFFFFFFFull,
      0x8000000000000000ull, 0x0123456789ABCDEFull};
  std::mt19937_64 random(0x825D8E30);
  for (const auto& function : functions) {
    for (unsigned layout = 0; layout < 5; ++layout)
      for (uint64_t value : values) CheckCase(test, function, value, layout, seed++);
    for (unsigned i = 0; i < 16; ++i)
      CheckCase(test, function, random(), i % 5, seed++);
  }
  test.Report("native extended field writers");
}
