#include "native_helpers_test.h"

#include <array>
#include <random>

REX_EXTERN(sub_8262E8F8);
REX_EXTERN(__imp__sub_8262E8F8);
REX_EXTERN(sub_827789A0);
REX_EXTERN(__imp__sub_827789A0);
REX_EXTERN(sub_8281E1F8);
REX_EXTERN(__imp__sub_8281E1F8);
REX_EXTERN(sub_825FB190);
REX_EXTERN(__imp__sub_825FB190);
REX_EXTERN(sub_827871A8);
REX_EXTERN(__imp__sub_827871A8);
REX_EXTERN(sub_824A4AB0);
REX_EXTERN(__imp__sub_824A4AB0);
REX_EXTERN(sub_826CF0D8);
REX_EXTERN(__imp__sub_826CF0D8);
REX_EXTERN(sub_82703778);
REX_EXTERN(__imp__sub_82703778);
REX_EXTERN(sub_8242F430);
REX_EXTERN(__imp__sub_8242F430);
REX_EXTERN(sub_8243F2A8);
REX_EXTERN(__imp__sub_8243F2A8);
REX_EXTERN(sub_82441E08);
REX_EXTERN(__imp__sub_82441E08);
REX_EXTERN(sub_82411A50);
REX_EXTERN(__imp__sub_82411A50);
REX_EXTERN(sub_8247BAD8);
REX_EXTERN(__imp__sub_8247BAD8);
REX_EXTERN(sub_82481800);
REX_EXTERN(__imp__sub_82481800);
REX_EXTERN(sub_826C9A90);
REX_EXTERN(__imp__sub_826C9A90);
REX_EXTERN(sub_8276E040);
REX_EXTERN(__imp__sub_8276E040);
REX_EXTERN(sub_82774B20);
REX_EXTERN(__imp__sub_82774B20);
REX_EXTERN(sub_8260B3E0);
REX_EXTERN(__imp__sub_8260B3E0);
REX_EXTERN(sub_826F4140);
REX_EXTERN(__imp__sub_826F4140);
REX_EXTERN(sub_827073C8);
REX_EXTERN(__imp__sub_827073C8);
REX_EXTERN(sub_8243AA18);
REX_EXTERN(__imp__sub_8243AA18);
REX_EXTERN(sub_824A6C30);
REX_EXTERN(__imp__sub_824A6C30);
REX_EXTERN(sub_824D5C98);
REX_EXTERN(__imp__sub_824D5C98);
REX_EXTERN(sub_827EF388);
REX_EXTERN(__imp__sub_827EF388);
REX_EXTERN(sub_824A5960);
REX_EXTERN(__imp__sub_824A5960);
REX_EXTERN(sub_825F74D0);
REX_EXTERN(__imp__sub_825F74D0);
REX_EXTERN(sub_82415238);
REX_EXTERN(__imp__sub_82415238);
REX_EXTERN(sub_8247BAB8);
REX_EXTERN(__imp__sub_8247BAB8);
REX_EXTERN(sub_82496C40);
REX_EXTERN(__imp__sub_82496C40);
REX_EXTERN(sub_82774B10);
REX_EXTERN(__imp__sub_82774B10);
REX_EXTERN(sub_824A5B90);
REX_EXTERN(__imp__sub_824A5B90);
REX_EXTERN(sub_826C9A70);
REX_EXTERN(__imp__sub_826C9A70);
REX_EXTERN(sub_826752F8);
REX_EXTERN(__imp__sub_826752F8);
REX_EXTERN(sub_82786AE0);
REX_EXTERN(__imp__sub_82786AE0);
REX_EXTERN(sub_8260F630);
REX_EXTERN(__imp__sub_8260F630);
REX_EXTERN(sub_82458500);
REX_EXTERN(__imp__sub_82458500);
REX_EXTERN(sub_8246BDA0);
REX_EXTERN(__imp__sub_8246BDA0);
REX_EXTERN(sub_824A6D10);
REX_EXTERN(__imp__sub_824A6D10);
REX_EXTERN(sub_826F67B8);
REX_EXTERN(__imp__sub_826F67B8);
REX_EXTERN(sub_82697098);
REX_EXTERN(__imp__sub_82697098);
REX_EXTERN(sub_8243F2B8);
REX_EXTERN(__imp__sub_8243F2B8);
REX_EXTERN(sub_8247BAA8);
REX_EXTERN(__imp__sub_8247BAA8);
REX_EXTERN(sub_825FB320);
REX_EXTERN(__imp__sub_825FB320);
REX_EXTERN(sub_82685F00);
REX_EXTERN(__imp__sub_82685F00);
REX_EXTERN(sub_8241BAD0);
REX_EXTERN(__imp__sub_8241BAD0);
REX_EXTERN(sub_825D8E20);
REX_EXTERN(__imp__sub_825D8E20);
REX_EXTERN(sub_826DA0E0);
REX_EXTERN(__imp__sub_826DA0E0);
REX_EXTERN(sub_82703600);
REX_EXTERN(__imp__sub_82703600);
REX_EXTERN(sub_82703610);
REX_EXTERN(__imp__sub_82703610);
REX_EXTERN(sub_82481810);
REX_EXTERN(__imp__sub_82481810);
REX_EXTERN(sub_826E08F8);
REX_EXTERN(__imp__sub_826E08F8);
REX_EXTERN(sub_82779F40);
REX_EXTERN(__imp__sub_82779F40);
REX_EXTERN(sub_8276ACF0);
REX_EXTERN(__imp__sub_8276ACF0);
REX_EXTERN(sub_8275D990);
REX_EXTERN(__imp__sub_8275D990);
REX_EXTERN(sub_8276AAA8);
REX_EXTERN(__imp__sub_8276AAA8);
REX_EXTERN(sub_82707B08);
REX_EXTERN(__imp__sub_82707B08);
REX_EXTERN(sub_827868D0);
REX_EXTERN(__imp__sub_827868D0);
REX_EXTERN(sub_828290B0);
REX_EXTERN(__imp__sub_828290B0);
REX_EXTERN(sub_82770198);
REX_EXTERN(__imp__sub_82770198);
REX_EXTERN(sub_827D30F8);
REX_EXTERN(__imp__sub_827D30F8);
REX_EXTERN(sub_82418CF8);
REX_EXTERN(__imp__sub_82418CF8);
REX_EXTERN(sub_826DA050);
REX_EXTERN(__imp__sub_826DA050);
REX_EXTERN(sub_826A8678);
REX_EXTERN(__imp__sub_826A8678);
REX_EXTERN(sub_826E08A8);
REX_EXTERN(__imp__sub_826E08A8);
REX_EXTERN(sub_8246B050);
REX_EXTERN(__imp__sub_8246B050);
REX_EXTERN(sub_8246B7B8);
REX_EXTERN(__imp__sub_8246B7B8);
REX_EXTERN(sub_826B7F00);
REX_EXTERN(__imp__sub_826B7F00);
REX_EXTERN(sub_826DDB68);
REX_EXTERN(__imp__sub_826DDB68);
REX_EXTERN(sub_826F40C8);
REX_EXTERN(__imp__sub_826F40C8);
REX_EXTERN(sub_8247BB58);
REX_EXTERN(__imp__sub_8247BB58);
REX_EXTERN(sub_825E1638);
REX_EXTERN(__imp__sub_825E1638);
REX_EXTERN(sub_82627A48);
REX_EXTERN(__imp__sub_82627A48);
REX_EXTERN(sub_824A6C20);
REX_EXTERN(__imp__sub_824A6C20);
REX_EXTERN(sub_824BD338);
REX_EXTERN(__imp__sub_824BD338);
REX_EXTERN(sub_825D7F98);
REX_EXTERN(__imp__sub_825D7F98);
REX_EXTERN(sub_826DAC98);
REX_EXTERN(__imp__sub_826DAC98);
REX_EXTERN(sub_826F23B8);
REX_EXTERN(__imp__sub_826F23B8);
REX_EXTERN(sub_8247BAC8);
REX_EXTERN(__imp__sub_8247BAC8);
REX_EXTERN(sub_8287A0F0);
REX_EXTERN(__imp__sub_8287A0F0);
REX_EXTERN(sub_824124E0);
REX_EXTERN(__imp__sub_824124E0);
REX_EXTERN(sub_82415240);
REX_EXTERN(__imp__sub_82415240);
REX_EXTERN(sub_8243EC48);
REX_EXTERN(__imp__sub_8243EC48);
REX_EXTERN(sub_8276ACC8);
REX_EXTERN(__imp__sub_8276ACC8);
REX_EXTERN(sub_826CF5B0);
REX_EXTERN(__imp__sub_826CF5B0);
REX_EXTERN(sub_8276AF08);
REX_EXTERN(__imp__sub_8276AF08);
REX_EXTERN(sub_825D8378);
REX_EXTERN(__imp__sub_825D8378);
REX_EXTERN(sub_825D8E00);
REX_EXTERN(__imp__sub_825D8E00);
REX_EXTERN(sub_82776AE8);
REX_EXTERN(__imp__sub_82776AE8);
REX_EXTERN(sub_824BD358);
REX_EXTERN(__imp__sub_824BD358);
REX_EXTERN(sub_8263D298);
REX_EXTERN(__imp__sub_8263D298);
REX_EXTERN(sub_826746D8);
REX_EXTERN(__imp__sub_826746D8);
REX_EXTERN(sub_82703608);
REX_EXTERN(__imp__sub_82703608);
REX_EXTERN(sub_82703780);
REX_EXTERN(__imp__sub_82703780);
REX_EXTERN(sub_825E3B50);
REX_EXTERN(__imp__sub_825E3B50);
REX_EXTERN(sub_82449FE8);
REX_EXTERN(__imp__sub_82449FE8);
REX_EXTERN(sub_824A5BC0);
REX_EXTERN(__imp__sub_824A5BC0);
REX_EXTERN(sub_824A4610);
REX_EXTERN(__imp__sub_824A4610);
REX_EXTERN(sub_824A5BD0);
REX_EXTERN(__imp__sub_824A5BD0);
REX_EXTERN(sub_824A6080);
REX_EXTERN(__imp__sub_824A6080);
REX_EXTERN(sub_8260F3B8);
REX_EXTERN(__imp__sub_8260F3B8);
REX_EXTERN(sub_826746E8);
REX_EXTERN(__imp__sub_826746E8);
REX_EXTERN(sub_82674768);
REX_EXTERN(__imp__sub_82674768);
REX_EXTERN(sub_825D7FD8);
REX_EXTERN(__imp__sub_825D7FD8);
REX_EXTERN(sub_82449540);
REX_EXTERN(__imp__sub_82449540);
REX_EXTERN(sub_826B5720);
REX_EXTERN(__imp__sub_826B5720);
REX_EXTERN(sub_826B7E48);
REX_EXTERN(__imp__sub_826B7E48);
REX_EXTERN(sub_827749F0);
REX_EXTERN(__imp__sub_827749F0);
REX_EXTERN(sub_82735528);
REX_EXTERN(__imp__sub_82735528);
REX_EXTERN(sub_8276DF18);
REX_EXTERN(__imp__sub_8276DF18);
REX_EXTERN(sub_82642238);
REX_EXTERN(__imp__sub_82642238);
REX_EXTERN(sub_8276AC90);
REX_EXTERN(__imp__sub_8276AC90);
REX_EXTERN(sub_8276ACE0);
REX_EXTERN(__imp__sub_8276ACE0);
REX_EXTERN(sub_8276ACE8);
REX_EXTERN(__imp__sub_8276ACE8);
REX_EXTERN(sub_8241E6A8);
REX_EXTERN(__imp__sub_8241E6A8);
REX_EXTERN(sub_826B8BD0);
REX_EXTERN(__imp__sub_826B8BD0);
REX_EXTERN(sub_828333E8);
REX_EXTERN(__imp__sub_828333E8);
REX_EXTERN(sub_824119A0);
REX_EXTERN(__imp__sub_824119A0);
REX_EXTERN(sub_824119B8);
REX_EXTERN(__imp__sub_824119B8);
REX_EXTERN(sub_82422C00);
REX_EXTERN(__imp__sub_82422C00);
REX_EXTERN(sub_824A4938);
REX_EXTERN(__imp__sub_824A4938);
REX_EXTERN(sub_824BD348);
REX_EXTERN(__imp__sub_824BD348);
REX_EXTERN(sub_824C01D8);
REX_EXTERN(__imp__sub_824C01D8);
REX_EXTERN(sub_8267EF90);
REX_EXTERN(__imp__sub_8267EF90);
REX_EXTERN(sub_8281D530);
REX_EXTERN(__imp__sub_8281D530);
REX_EXTERN(sub_824119D8);
REX_EXTERN(__imp__sub_824119D8);
REX_EXTERN(sub_82703618);
REX_EXTERN(__imp__sub_82703618);
REX_EXTERN(sub_8261D1A0);
REX_EXTERN(__imp__sub_8261D1A0);
REX_EXTERN(sub_824119C8);
REX_EXTERN(__imp__sub_824119C8);
REX_EXTERN(sub_8261D198);
REX_EXTERN(__imp__sub_8261D198);
REX_EXTERN(sub_82804948);
REX_EXTERN(__imp__sub_82804948);
REX_EXTERN(sub_824A4930);
REX_EXTERN(__imp__sub_824A4930);
REX_EXTERN(sub_825D7FF8);
REX_EXTERN(__imp__sub_825D7FF8);
REX_EXTERN(sub_826E08E8);
REX_EXTERN(__imp__sub_826E08E8);
REX_EXTERN(sub_82411A30);
REX_EXTERN(__imp__sub_82411A30);
REX_EXTERN(sub_824BBDB0);
REX_EXTERN(__imp__sub_824BBDB0);
REX_EXTERN(sub_825D8038);
REX_EXTERN(__imp__sub_825D8038);
REX_EXTERN(sub_82638770);
REX_EXTERN(__imp__sub_82638770);
REX_EXTERN(sub_826F4148);
REX_EXTERN(__imp__sub_826F4148);
REX_EXTERN(sub_827701A8);
REX_EXTERN(__imp__sub_827701A8);
REX_EXTERN(sub_827EF458);
REX_EXTERN(__imp__sub_827EF458);
REX_EXTERN(sub_824119F8);
REX_EXTERN(__imp__sub_824119F8);
REX_EXTERN(sub_825D8018);
REX_EXTERN(__imp__sub_825D8018);
REX_EXTERN(sub_82697818);
REX_EXTERN(__imp__sub_82697818);
REX_EXTERN(sub_82411988);
REX_EXTERN(__imp__sub_82411988);
REX_EXTERN(sub_8269A468);
REX_EXTERN(__imp__sub_8269A468);
REX_EXTERN(sub_82703A40);
REX_EXTERN(__imp__sub_82703A40);
REX_EXTERN(sub_825D7FB8);
REX_EXTERN(__imp__sub_825D7FB8);
REX_EXTERN(sub_825DD830);
REX_EXTERN(__imp__sub_825DD830);
REX_EXTERN(sub_824119E8);
REX_EXTERN(__imp__sub_824119E8);
REX_EXTERN(sub_8267EF80);
REX_EXTERN(__imp__sub_8267EF80);
REX_EXTERN(sub_82770670);
REX_EXTERN(__imp__sub_82770670);
REX_EXTERN(sub_8242A2D8);
REX_EXTERN(__imp__sub_8242A2D8);
REX_EXTERN(sub_825EA330);
REX_EXTERN(__imp__sub_825EA330);

namespace {
struct LoadField { uint8_t bits; uint8_t base_register; uint16_t offset; };
struct Function {
  const char* name;
  GuestFunction* original;
  GuestFunction* native;
  uint32_t pointer_registers;
  uint8_t load_count;
  std::array<LoadField, 4> loads;
};
const Function functions[] = {
  {"sub_8262E8F8", __imp__sub_8262E8F8, sub_8262E8F8, 0x8u, 0, {{}}},
  {"sub_827789A0", __imp__sub_827789A0, sub_827789A0, 0x8u, 1, {{{32, 3, 1416}}}},
  {"sub_8281E1F8", __imp__sub_8281E1F8, sub_8281E1F8, 0x20u, 0, {{}}},
  {"sub_825FB190", __imp__sub_825FB190, sub_825FB190, 0x8u, 0, {{}}},
  {"sub_827871A8", __imp__sub_827871A8, sub_827871A8, 0x38u, 2, {{{32, 3, 30672}, {32, 3, 30676}}}},
  {"sub_824A4AB0", __imp__sub_824A4AB0, sub_824A4AB0, 0x8u, 0, {{}}},
  {"sub_826CF0D8", __imp__sub_826CF0D8, sub_826CF0D8, 0x18u, 1, {{{32, 3, 8}}}},
  {"sub_82703778", __imp__sub_82703778, sub_82703778, 0x8u, 0, {{}}},
  {"sub_8242F430", __imp__sub_8242F430, sub_8242F430, 0x8u, 0, {{}}},
  {"sub_8243F2A8", __imp__sub_8243F2A8, sub_8243F2A8, 0x8u, 0, {{}}},
  {"sub_82441E08", __imp__sub_82441E08, sub_82441E08, 0x8u, 0, {{}}},
  {"sub_82411A50", __imp__sub_82411A50, sub_82411A50, 0x8u, 0, {{}}},
  {"sub_8247BAD8", __imp__sub_8247BAD8, sub_8247BAD8, 0x8u, 0, {{}}},
  {"sub_82481800", __imp__sub_82481800, sub_82481800, 0x8u, 0, {{}}},
  {"sub_826C9A90", __imp__sub_826C9A90, sub_826C9A90, 0x8u, 0, {{}}},
  {"sub_8276E040", __imp__sub_8276E040, sub_8276E040, 0x8u, 0, {{}}},
  {"sub_82774B20", __imp__sub_82774B20, sub_82774B20, 0x8u, 0, {{}}},
  {"sub_8260B3E0", __imp__sub_8260B3E0, sub_8260B3E0, 0x8u, 0, {{}}},
  {"sub_826F4140", __imp__sub_826F4140, sub_826F4140, 0x8u, 0, {{}}},
  {"sub_827073C8", __imp__sub_827073C8, sub_827073C8, 0x18u, 1, {{{64, 3, 288}}}},
  {"sub_8243AA18", __imp__sub_8243AA18, sub_8243AA18, 0x8u, 1, {{{32, 3, 4}}}},
  {"sub_824A6C30", __imp__sub_824A6C30, sub_824A6C30, 0x8u, 0, {{}}},
  {"sub_824D5C98", __imp__sub_824D5C98, sub_824D5C98, 0x8u, 0, {{}}},
  {"sub_827EF388", __imp__sub_827EF388, sub_827EF388, 0x8u, 0, {{}}},
  {"sub_824A5960", __imp__sub_824A5960, sub_824A5960, 0x8u, 0, {{}}},
  {"sub_825F74D0", __imp__sub_825F74D0, sub_825F74D0, 0x8u, 0, {{}}},
  {"sub_82415238", __imp__sub_82415238, sub_82415238, 0x8u, 0, {{}}},
  {"sub_8247BAB8", __imp__sub_8247BAB8, sub_8247BAB8, 0x8u, 0, {{}}},
  {"sub_82496C40", __imp__sub_82496C40, sub_82496C40, 0x8u, 0, {{}}},
  {"sub_82774B10", __imp__sub_82774B10, sub_82774B10, 0x8u, 0, {{}}},
  {"sub_824A5B90", __imp__sub_824A5B90, sub_824A5B90, 0x8u, 0, {{}}},
  {"sub_826C9A70", __imp__sub_826C9A70, sub_826C9A70, 0x8u, 0, {{}}},
  {"sub_826752F8", __imp__sub_826752F8, sub_826752F8, 0x8u, 0, {{}}},
  {"sub_82786AE0", __imp__sub_82786AE0, sub_82786AE0, 0x8u, 1, {{{32, 3, 31532}}}},
  {"sub_8260F630", __imp__sub_8260F630, sub_8260F630, 0x8u, 1, {{{32, 3, 1376}}}},
  {"sub_82458500", __imp__sub_82458500, sub_82458500, 0x8u, 0, {{}}},
  {"sub_8246BDA0", __imp__sub_8246BDA0, sub_8246BDA0, 0x8u, 0, {{}}},
  {"sub_824A6D10", __imp__sub_824A6D10, sub_824A6D10, 0x8u, 0, {{}}},
  {"sub_826F67B8", __imp__sub_826F67B8, sub_826F67B8, 0x8u, 0, {{}}},
  {"sub_82697098", __imp__sub_82697098, sub_82697098, 0x18u, 1, {{{32, 3, 28}}}},
  {"sub_8243F2B8", __imp__sub_8243F2B8, sub_8243F2B8, 0x8u, 0, {{}}},
  {"sub_8247BAA8", __imp__sub_8247BAA8, sub_8247BAA8, 0x8u, 0, {{}}},
  {"sub_825FB320", __imp__sub_825FB320, sub_825FB320, 0x8u, 0, {{}}},
  {"sub_82685F00", __imp__sub_82685F00, sub_82685F00, 0x8u, 0, {{}}},
  {"sub_8241BAD0", __imp__sub_8241BAD0, sub_8241BAD0, 0x8u, 0, {{}}},
  {"sub_825D8E20", __imp__sub_825D8E20, sub_825D8E20, 0x8u, 0, {{}}},
  {"sub_826DA0E0", __imp__sub_826DA0E0, sub_826DA0E0, 0x8u, 0, {{}}},
  {"sub_82703600", __imp__sub_82703600, sub_82703600, 0x8u, 0, {{}}},
  {"sub_82703610", __imp__sub_82703610, sub_82703610, 0x8u, 0, {{}}},
  {"sub_82481810", __imp__sub_82481810, sub_82481810, 0x8u, 0, {{}}},
  {"sub_826E08F8", __imp__sub_826E08F8, sub_826E08F8, 0x8u, 0, {{}}},
  {"sub_82779F40", __imp__sub_82779F40, sub_82779F40, 0x8u, 0, {{}}},
  {"sub_8276ACF0", __imp__sub_8276ACF0, sub_8276ACF0, 0x8u, 0, {{}}},
  {"sub_8275D990", __imp__sub_8275D990, sub_8275D990, 0x8u, 1, {{{32, 3, 248}}}},
  {"sub_8276AAA8", __imp__sub_8276AAA8, sub_8276AAA8, 0x8u, 0, {{}}},
  {"sub_82707B08", __imp__sub_82707B08, sub_82707B08, 0x8u, 0, {{}}},
  {"sub_827868D0", __imp__sub_827868D0, sub_827868D0, 0x8u, 0, {{}}},
  {"sub_828290B0", __imp__sub_828290B0, sub_828290B0, 0x8u, 0, {{}}},
  {"sub_82770198", __imp__sub_82770198, sub_82770198, 0x8u, 0, {{}}},
  {"sub_827D30F8", __imp__sub_827D30F8, sub_827D30F8, 0x8u, 0, {{}}},
  {"sub_82418CF8", __imp__sub_82418CF8, sub_82418CF8, 0x8u, 0, {{}}},
  {"sub_826DA050", __imp__sub_826DA050, sub_826DA050, 0x8u, 0, {{}}},
  {"sub_826A8678", __imp__sub_826A8678, sub_826A8678, 0x8u, 0, {{}}},
  {"sub_826E08A8", __imp__sub_826E08A8, sub_826E08A8, 0x8u, 0, {{}}},
  {"sub_8246B050", __imp__sub_8246B050, sub_8246B050, 0x8u, 0, {{}}},
  {"sub_8246B7B8", __imp__sub_8246B7B8, sub_8246B7B8, 0x8u, 0, {{}}},
  {"sub_826B7F00", __imp__sub_826B7F00, sub_826B7F00, 0x18u, 0, {{}}},
  {"sub_826DDB68", __imp__sub_826DDB68, sub_826DDB68, 0x8u, 1, {{{32, 3, 200}}}},
  {"sub_826F40C8", __imp__sub_826F40C8, sub_826F40C8, 0x8u, 0, {{}}},
  {"sub_8247BB58", __imp__sub_8247BB58, sub_8247BB58, 0x8u, 0, {{}}},
  {"sub_825E1638", __imp__sub_825E1638, sub_825E1638, 0x8u, 1, {{{32, 3, 13716}}}},
  {"sub_82627A48", __imp__sub_82627A48, sub_82627A48, 0x18u, 1, {{{32, 3, 0}}}},
  {"sub_824A6C20", __imp__sub_824A6C20, sub_824A6C20, 0x8u, 0, {{}}},
  {"sub_824BD338", __imp__sub_824BD338, sub_824BD338, 0x8u, 0, {{}}},
  {"sub_825D7F98", __imp__sub_825D7F98, sub_825D7F98, 0x8u, 1, {{{64, 3, 16}}}},
  {"sub_826DAC98", __imp__sub_826DAC98, sub_826DAC98, 0x8u, 0, {{}}},
  {"sub_826F23B8", __imp__sub_826F23B8, sub_826F23B8, 0x8u, 0, {{}}},
  {"sub_8247BAC8", __imp__sub_8247BAC8, sub_8247BAC8, 0x8u, 0, {{}}},
  {"sub_8287A0F0", __imp__sub_8287A0F0, sub_8287A0F0, 0x10u, 0, {{}}},
  {"sub_824124E0", __imp__sub_824124E0, sub_824124E0, 0x8u, 0, {{}}},
  {"sub_82415240", __imp__sub_82415240, sub_82415240, 0x8u, 0, {{}}},
  {"sub_8243EC48", __imp__sub_8243EC48, sub_8243EC48, 0x8u, 0, {{}}},
  {"sub_8276ACC8", __imp__sub_8276ACC8, sub_8276ACC8, 0x8u, 0, {{}}},
  {"sub_826CF5B0", __imp__sub_826CF5B0, sub_826CF5B0, 0x10u, 0, {{}}},
  {"sub_8276AF08", __imp__sub_8276AF08, sub_8276AF08, 0x8u, 0, {{}}},
  {"sub_825D8378", __imp__sub_825D8378, sub_825D8378, 0x8u, 0, {{}}},
  {"sub_825D8E00", __imp__sub_825D8E00, sub_825D8E00, 0x8u, 1, {{{64, 3, 16}}}},
  {"sub_82776AE8", __imp__sub_82776AE8, sub_82776AE8, 0x8u, 0, {{}}},
  {"sub_824BD358", __imp__sub_824BD358, sub_824BD358, 0x8u, 0, {{}}},
  {"sub_8263D298", __imp__sub_8263D298, sub_8263D298, 0x8u, 0, {{}}},
  {"sub_826746D8", __imp__sub_826746D8, sub_826746D8, 0x8u, 1, {{{32, 3, 152}}}},
  {"sub_82703608", __imp__sub_82703608, sub_82703608, 0x8u, 0, {{}}},
  {"sub_82703780", __imp__sub_82703780, sub_82703780, 0x8u, 0, {{}}},
  {"sub_825E3B50", __imp__sub_825E3B50, sub_825E3B50, 0x28u, 4, {{{32, 3, 13820}, {32, 3, 13824}, {32, 3, 21836}, {32, 3, 13828}}}},
  {"sub_82449FE8", __imp__sub_82449FE8, sub_82449FE8, 0x8u, 0, {{}}},
  {"sub_824A5BC0", __imp__sub_824A5BC0, sub_824A5BC0, 0x8u, 0, {{}}},
  {"sub_824A4610", __imp__sub_824A4610, sub_824A4610, 0x8u, 0, {{}}},
  {"sub_824A5BD0", __imp__sub_824A5BD0, sub_824A5BD0, 0x8u, 0, {{}}},
  {"sub_824A6080", __imp__sub_824A6080, sub_824A6080, 0x8u, 0, {{}}},
  {"sub_8260F3B8", __imp__sub_8260F3B8, sub_8260F3B8, 0x8u, 1, {{{32, 3, 1376}}}},
  {"sub_826746E8", __imp__sub_826746E8, sub_826746E8, 0x8u, 0, {{}}},
  {"sub_82674768", __imp__sub_82674768, sub_82674768, 0x8u, 0, {{}}},
  {"sub_825D7FD8", __imp__sub_825D7FD8, sub_825D7FD8, 0x8u, 1, {{{64, 3, 16}}}},
  {"sub_82449540", __imp__sub_82449540, sub_82449540, 0x8u, 0, {{}}},
  {"sub_826B5720", __imp__sub_826B5720, sub_826B5720, 0x8u, 0, {{}}},
  {"sub_826B7E48", __imp__sub_826B7E48, sub_826B7E48, 0x8u, 0, {{}}},
  {"sub_827749F0", __imp__sub_827749F0, sub_827749F0, 0x8u, 0, {{}}},
  {"sub_82735528", __imp__sub_82735528, sub_82735528, 0x8u, 0, {{}}},
  {"sub_8276DF18", __imp__sub_8276DF18, sub_8276DF18, 0x8u, 0, {{}}},
  {"sub_82642238", __imp__sub_82642238, sub_82642238, 0x8u, 0, {{}}},
  {"sub_8276AC90", __imp__sub_8276AC90, sub_8276AC90, 0x8u, 0, {{}}},
  {"sub_8276ACE0", __imp__sub_8276ACE0, sub_8276ACE0, 0x8u, 0, {{}}},
  {"sub_8276ACE8", __imp__sub_8276ACE8, sub_8276ACE8, 0x8u, 0, {{}}},
  {"sub_8241E6A8", __imp__sub_8241E6A8, sub_8241E6A8, 0x8u, 0, {{}}},
  {"sub_826B8BD0", __imp__sub_826B8BD0, sub_826B8BD0, 0x8u, 1, {{{32, 3, 60}}}},
  {"sub_828333E8", __imp__sub_828333E8, sub_828333E8, 0x8u, 0, {{}}},
  {"sub_824119A0", __imp__sub_824119A0, sub_824119A0, 0x8u, 0, {{}}},
  {"sub_824119B8", __imp__sub_824119B8, sub_824119B8, 0x8u, 0, {{}}},
  {"sub_82422C00", __imp__sub_82422C00, sub_82422C00, 0x8u, 0, {{}}},
  {"sub_824A4938", __imp__sub_824A4938, sub_824A4938, 0x8u, 0, {{}}},
  {"sub_824BD348", __imp__sub_824BD348, sub_824BD348, 0x8u, 0, {{}}},
  {"sub_824C01D8", __imp__sub_824C01D8, sub_824C01D8, 0x8u, 0, {{}}},
  {"sub_8267EF90", __imp__sub_8267EF90, sub_8267EF90, 0x8u, 0, {{}}},
  {"sub_8281D530", __imp__sub_8281D530, sub_8281D530, 0x8u, 0, {{}}},
  {"sub_824119D8", __imp__sub_824119D8, sub_824119D8, 0x8u, 0, {{}}},
  {"sub_82703618", __imp__sub_82703618, sub_82703618, 0x8u, 0, {{}}},
  {"sub_8261D1A0", __imp__sub_8261D1A0, sub_8261D1A0, 0x40u, 0, {{}}},
  {"sub_824119C8", __imp__sub_824119C8, sub_824119C8, 0x8u, 0, {{}}},
  {"sub_8261D198", __imp__sub_8261D198, sub_8261D198, 0x40u, 0, {{}}},
  {"sub_82804948", __imp__sub_82804948, sub_82804948, 0x18u, 1, {{{8, 3, 16}}}},
  {"sub_824A4930", __imp__sub_824A4930, sub_824A4930, 0x8u, 0, {{}}},
  {"sub_825D7FF8", __imp__sub_825D7FF8, sub_825D7FF8, 0x8u, 1, {{{64, 3, 16}}}},
  {"sub_826E08E8", __imp__sub_826E08E8, sub_826E08E8, 0x8u, 0, {{}}},
  {"sub_82411A30", __imp__sub_82411A30, sub_82411A30, 0x8u, 0, {{}}},
  {"sub_824BBDB0", __imp__sub_824BBDB0, sub_824BBDB0, 0x8u, 0, {{}}},
  {"sub_825D8038", __imp__sub_825D8038, sub_825D8038, 0x8u, 1, {{{64, 3, 16}}}},
  {"sub_82638770", __imp__sub_82638770, sub_82638770, 0x8u, 0, {{}}},
  {"sub_826F4148", __imp__sub_826F4148, sub_826F4148, 0x8u, 0, {{}}},
  {"sub_827701A8", __imp__sub_827701A8, sub_827701A8, 0x8u, 0, {{}}},
  {"sub_827EF458", __imp__sub_827EF458, sub_827EF458, 0x8u, 0, {{}}},
  {"sub_824119F8", __imp__sub_824119F8, sub_824119F8, 0x8u, 0, {{}}},
  {"sub_825D8018", __imp__sub_825D8018, sub_825D8018, 0x8u, 1, {{{64, 3, 16}}}},
  {"sub_82697818", __imp__sub_82697818, sub_82697818, 0x18u, 1, {{{16, 3, 8}}}},
  {"sub_82411988", __imp__sub_82411988, sub_82411988, 0x8u, 0, {{}}},
  {"sub_8269A468", __imp__sub_8269A468, sub_8269A468, 0x8u, 0, {{}}},
  {"sub_82703A40", __imp__sub_82703A40, sub_82703A40, 0x18u, 1, {{{32, 3, 56}}}},
  {"sub_825D7FB8", __imp__sub_825D7FB8, sub_825D7FB8, 0x8u, 1, {{{64, 3, 16}}}},
  {"sub_825DD830", __imp__sub_825DD830, sub_825DD830, 0x8u, 1, {{{64, 3, 16}}}},
  {"sub_824119E8", __imp__sub_824119E8, sub_824119E8, 0x8u, 0, {{}}},
  {"sub_8267EF80", __imp__sub_8267EF80, sub_8267EF80, 0x8u, 0, {{}}},
  {"sub_82770670", __imp__sub_82770670, sub_82770670, 0x8u, 0, {{}}},
  {"sub_8242A2D8", __imp__sub_8242A2D8, sub_8242A2D8, 0x8u, 0, {{}}},
  {"sub_825EA330", __imp__sub_825EA330, sub_825EA330, 0x8u, 1, {{{8, 3, 11071}}}},
};

GuestRegister& Register(GuestContext& ctx, unsigned index) {
  switch (index) {
    case 3: return ctx.r3;
    case 4: return ctx.r4;
    case 5: return ctx.r5;
    case 6: return ctx.r6;
    default: return ctx.r7;
  }
}

void CheckCase(NativeHelpersTest& test, const Function& function, uint64_t value,
               unsigned layout, uint32_t seed) {
  auto ctx = test.Context(seed);
  const uint32_t object = layout == 4 ? 0xE0010000u : NativeHelpersTest::kObject;
  const uint32_t outputs = layout == 4 ? 0xE0020000u : NativeHelpersTest::kData;
  test.Fill(object, 65536, seed);
  test.Fill(outputs, 16384, seed + 1);
  const uint32_t first_field = function.load_count ? function.loads[0].offset : 0;
  for (unsigned r = 3; r <= 7; ++r) {
    auto& reg = Register(ctx, r);
    const uint64_t high = uint64_t((seed & 1) ? 0xFEDCBA98u : 0x13579BDFu) << 32;
    if (function.pointer_registers & (1u << r)) {
      uint32_t address = r == 3 ? object : outputs + (r - 4) * 1024;
      if (r != 3 && layout == 1) address = object;
      if (r != 3 && layout == 2) address = object + first_field + (r - 4) * 4;
      if (r != 3 && layout == 3) address = object + first_field + 1 + (r - 4) * 2;
      reg.u64 = high | address;
    } else {
      const uint32_t scalar = uint32_t(value) ^
          uint32_t((value >> 32) * (r - 3) * 0x9E3779B1ull);
      reg.u64 = high | scalar;
    }
  }
  for (unsigned i = 0; i < function.load_count; ++i) {
    const auto field = function.loads[i];
    const uint32_t address = Register(ctx, field.base_register).u32 + field.offset;
    const uint64_t data = value ^ (uint64_t(i) * 0x13579BDF02468ACEull);
    if (field.bits == 8) test.Store8(address, uint8_t(data));
    else if (field.bits == 16) test.Store16(address, uint16_t(data));
    else if (field.bits == 32) test.Store32(address, uint32_t(data));
    else test.Store64(address, data);
  }
  test.Check(function.name, function.original, function.native, ctx,
             {{object, 65536}, {outputs, 16384}});
}
}  // namespace

int main() {
  NativeHelpersTest test;
  uint32_t seed = 0;
  constexpr std::array<uint64_t, 12> values = {
      0, 1, 0x7F, 0x80, 0xFF, 0x100, 0xFFFF, 0x10000,
      0x7FFFFFFF, 0x80000000, 0xFFFFFFFF, 0xFFFFFFFFFFFFFFFFull};
  std::mt19937_64 random(0x8262E8F8);
  for (const auto& function : functions) {
    for (unsigned layout = 0; layout < 5; ++layout)
      for (uint64_t value : values) CheckCase(test, function, value, layout, seed++);
    for (unsigned i = 0; i < 16; ++i)
      CheckCase(test, function, random(), i % 5, seed++);
  }
  test.Report("native field writers");
}
