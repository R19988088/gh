#include "native_helpers_test.h"

#include <array>
#include <bit>
#include <cfenv>
#include <random>

#if defined(__clang__)
#pragma STDC FENV_ACCESS ON
#endif

REX_EXTERN(sub_8262AE58);
REX_EXTERN(__imp__sub_8262AE58);
REX_EXTERN(sub_8262AE40);
REX_EXTERN(__imp__sub_8262AE40);
REX_EXTERN(sub_826F15E8);
REX_EXTERN(__imp__sub_826F15E8);
REX_EXTERN(sub_82902D60);
REX_EXTERN(__imp__sub_82902D60);
REX_EXTERN(sub_82903118);
REX_EXTERN(__imp__sub_82903118);
REX_EXTERN(sub_828ED908);
REX_EXTERN(__imp__sub_828ED908);
REX_EXTERN(sub_82900418);
REX_EXTERN(__imp__sub_82900418);
REX_EXTERN(sub_82900558);
REX_EXTERN(__imp__sub_82900558);
REX_EXTERN(sub_82900628);
REX_EXTERN(__imp__sub_82900628);
REX_EXTERN(sub_82903158);
REX_EXTERN(__imp__sub_82903158);
REX_EXTERN(sub_82903570);
REX_EXTERN(__imp__sub_82903570);
REX_EXTERN(sub_8262AF00);
REX_EXTERN(__imp__sub_8262AF00);
REX_EXTERN(sub_828ED8B0);
REX_EXTERN(__imp__sub_828ED8B0);
REX_EXTERN(sub_82900CB0);
REX_EXTERN(__imp__sub_82900CB0);
REX_EXTERN(sub_82900EA8);
REX_EXTERN(__imp__sub_82900EA8);
REX_EXTERN(sub_829019E8);
REX_EXTERN(__imp__sub_829019E8);
REX_EXTERN(sub_82901BE8);
REX_EXTERN(__imp__sub_82901BE8);
REX_EXTERN(sub_82902298);
REX_EXTERN(__imp__sub_82902298);
REX_EXTERN(sub_829027A0);
REX_EXTERN(__imp__sub_829027A0);
REX_EXTERN(sub_82902AC0);
REX_EXTERN(__imp__sub_82902AC0);
REX_EXTERN(sub_82902E20);
REX_EXTERN(__imp__sub_82902E20);
REX_EXTERN(sub_82903218);
REX_EXTERN(__imp__sub_82903218);
REX_EXTERN(sub_82903650);
REX_EXTERN(__imp__sub_82903650);
REX_EXTERN(sub_82703788);
REX_EXTERN(__imp__sub_82703788);
REX_EXTERN(sub_828ED988);
REX_EXTERN(__imp__sub_828ED988);
REX_EXTERN(sub_829019D0);
REX_EXTERN(__imp__sub_829019D0);
REX_EXTERN(sub_82901BC8);
REX_EXTERN(__imp__sub_82901BC8);
REX_EXTERN(sub_82901E78);
REX_EXTERN(__imp__sub_82901E78);
REX_EXTERN(sub_82902780);
REX_EXTERN(__imp__sub_82902780);
REX_EXTERN(sub_82902AA0);
REX_EXTERN(__imp__sub_82902AA0);
REX_EXTERN(sub_82902E00);
REX_EXTERN(__imp__sub_82902E00);
REX_EXTERN(sub_829031F8);
REX_EXTERN(__imp__sub_829031F8);
REX_EXTERN(sub_82903630);
REX_EXTERN(__imp__sub_82903630);
REX_EXTERN(sub_82418D38);
REX_EXTERN(__imp__sub_82418D38);
REX_EXTERN(sub_82902A00);
REX_EXTERN(__imp__sub_82902A00);
REX_EXTERN(sub_82902D40);
REX_EXTERN(__imp__sub_82902D40);
REX_EXTERN(sub_829030F8);
REX_EXTERN(__imp__sub_829030F8);
REX_EXTERN(sub_825231D0);
REX_EXTERN(__imp__sub_825231D0);
REX_EXTERN(sub_8262AEE8);
REX_EXTERN(__imp__sub_8262AEE8);
REX_EXTERN(sub_82903828);
REX_EXTERN(__imp__sub_82903828);
REX_EXTERN(sub_828ED928);
REX_EXTERN(__imp__sub_828ED928);
REX_EXTERN(sub_8262AED0);
REX_EXTERN(__imp__sub_8262AED0);
REX_EXTERN(sub_828ED828);
REX_EXTERN(__imp__sub_828ED828);
REX_EXTERN(sub_82900498);
REX_EXTERN(__imp__sub_82900498);
REX_EXTERN(sub_82900640);
REX_EXTERN(__imp__sub_82900640);
REX_EXTERN(sub_82900B18);
REX_EXTERN(__imp__sub_82900B18);
REX_EXTERN(sub_82900D10);
REX_EXTERN(__imp__sub_82900D10);
REX_EXTERN(sub_82900F08);
REX_EXTERN(__imp__sub_82900F08);
REX_EXTERN(sub_82901B00);
REX_EXTERN(__imp__sub_82901B00);
REX_EXTERN(sub_82901DD8);
REX_EXTERN(__imp__sub_82901DD8);
REX_EXTERN(sub_829026E0);
REX_EXTERN(__imp__sub_829026E0);
REX_EXTERN(sub_829029E0);
REX_EXTERN(__imp__sub_829029E0);
REX_EXTERN(sub_82902D20);
REX_EXTERN(__imp__sub_82902D20);
REX_EXTERN(sub_829030D8);
REX_EXTERN(__imp__sub_829030D8);
REX_EXTERN(sub_82903480);
REX_EXTERN(__imp__sub_82903480);
REX_EXTERN(sub_8242A2C8);
REX_EXTERN(__imp__sub_8242A2C8);
REX_EXTERN(sub_8262AEA0);
REX_EXTERN(__imp__sub_8262AEA0);
REX_EXTERN(sub_82900BD0);
REX_EXTERN(__imp__sub_82900BD0);
REX_EXTERN(sub_82900DD0);
REX_EXTERN(__imp__sub_82900DD0);
REX_EXTERN(sub_82900FE8);
REX_EXTERN(__imp__sub_82900FE8);
REX_EXTERN(sub_82901470);
REX_EXTERN(__imp__sub_82901470);
REX_EXTERN(sub_829018B0);
REX_EXTERN(__imp__sub_829018B0);
REX_EXTERN(sub_82901A40);
REX_EXTERN(__imp__sub_82901A40);
REX_EXTERN(sub_82901D38);
REX_EXTERN(__imp__sub_82901D38);
REX_EXTERN(sub_82902660);
REX_EXTERN(__imp__sub_82902660);
REX_EXTERN(sub_82902960);
REX_EXTERN(__imp__sub_82902960);
REX_EXTERN(sub_82902CA0);
REX_EXTERN(__imp__sub_82902CA0);
REX_EXTERN(sub_82903058);
REX_EXTERN(__imp__sub_82903058);
REX_EXTERN(sub_829033F8);
REX_EXTERN(__imp__sub_829033F8);
REX_EXTERN(sub_829038C0);
REX_EXTERN(__imp__sub_829038C0);
REX_EXTERN(sub_824A6CD0);
REX_EXTERN(__imp__sub_824A6CD0);
REX_EXTERN(sub_82900C30);
REX_EXTERN(__imp__sub_82900C30);
REX_EXTERN(sub_82900E30);
REX_EXTERN(__imp__sub_82900E30);
REX_EXTERN(sub_82901068);
REX_EXTERN(__imp__sub_82901068);
REX_EXTERN(sub_829017C0);
REX_EXTERN(__imp__sub_829017C0);
REX_EXTERN(sub_82901810);
REX_EXTERN(__imp__sub_82901810);
REX_EXTERN(sub_82901AE0);
REX_EXTERN(__imp__sub_82901AE0);
REX_EXTERN(sub_82901DC0);
REX_EXTERN(__imp__sub_82901DC0);
REX_EXTERN(sub_829026C0);
REX_EXTERN(__imp__sub_829026C0);
REX_EXTERN(sub_829029C0);
REX_EXTERN(__imp__sub_829029C0);
REX_EXTERN(sub_82902D00);
REX_EXTERN(__imp__sub_82902D00);
REX_EXTERN(sub_829030B8);
REX_EXTERN(__imp__sub_829030B8);
REX_EXTERN(sub_82903460);
REX_EXTERN(__imp__sub_82903460);
REX_EXTERN(sub_82903900);
REX_EXTERN(__imp__sub_82903900);
REX_EXTERN(sub_828ED848);
REX_EXTERN(__imp__sub_828ED848);
REX_EXTERN(sub_82902640);
REX_EXTERN(__imp__sub_82902640);
REX_EXTERN(sub_82902940);
REX_EXTERN(__imp__sub_82902940);
REX_EXTERN(sub_82902C80);
REX_EXTERN(__imp__sub_82902C80);
REX_EXTERN(sub_82903038);
REX_EXTERN(__imp__sub_82903038);
REX_EXTERN(sub_829033D8);
REX_EXTERN(__imp__sub_829033D8);
REX_EXTERN(sub_82901938);
REX_EXTERN(__imp__sub_82901938);
REX_EXTERN(sub_82901E18);
REX_EXTERN(__imp__sub_82901E18);
REX_EXTERN(sub_82902700);
REX_EXTERN(__imp__sub_82902700);
REX_EXTERN(sub_82902A20);
REX_EXTERN(__imp__sub_82902A20);
REX_EXTERN(sub_82902D80);
REX_EXTERN(__imp__sub_82902D80);
REX_EXTERN(sub_82903138);
REX_EXTERN(__imp__sub_82903138);
REX_EXTERN(sub_82903550);
REX_EXTERN(__imp__sub_82903550);
REX_EXTERN(sub_828ED9A8);
REX_EXTERN(__imp__sub_828ED9A8);
REX_EXTERN(sub_82901048);
REX_EXTERN(__imp__sub_82901048);
REX_EXTERN(sub_829017A0);
REX_EXTERN(__imp__sub_829017A0);
REX_EXTERN(sub_82901900);
REX_EXTERN(__imp__sub_82901900);
REX_EXTERN(sub_82901C68);
REX_EXTERN(__imp__sub_82901C68);
REX_EXTERN(sub_82902318);
REX_EXTERN(__imp__sub_82902318);
REX_EXTERN(sub_82902820);
REX_EXTERN(__imp__sub_82902820);
REX_EXTERN(sub_82902B40);
REX_EXTERN(__imp__sub_82902B40);
REX_EXTERN(sub_82902EA0);
REX_EXTERN(__imp__sub_82902EA0);
REX_EXTERN(sub_82903298);
REX_EXTERN(__imp__sub_82903298);
REX_EXTERN(sub_829036D0);
REX_EXTERN(__imp__sub_829036D0);
REX_EXTERN(sub_82418DF8);
REX_EXTERN(__imp__sub_82418DF8);
REX_EXTERN(sub_829031D8);
REX_EXTERN(__imp__sub_829031D8);
REX_EXTERN(sub_82903610);
REX_EXTERN(__imp__sub_82903610);
REX_EXTERN(sub_82900B70);
REX_EXTERN(__imp__sub_82900B70);
REX_EXTERN(sub_82900D70);
REX_EXTERN(__imp__sub_82900D70);
REX_EXTERN(sub_82900F68);
REX_EXTERN(__imp__sub_82900F68);
REX_EXTERN(sub_829013F8);
REX_EXTERN(__imp__sub_829013F8);
REX_EXTERN(sub_82901830);
REX_EXTERN(__imp__sub_82901830);
REX_EXTERN(sub_82901B20);
REX_EXTERN(__imp__sub_82901B20);
REX_EXTERN(sub_82901DF0);
REX_EXTERN(__imp__sub_82901DF0);
REX_EXTERN(sub_82902840);
REX_EXTERN(__imp__sub_82902840);
REX_EXTERN(sub_82902B60);
REX_EXTERN(__imp__sub_82902B60);
REX_EXTERN(sub_82902EC0);
REX_EXTERN(__imp__sub_82902EC0);
REX_EXTERN(sub_829032B8);
REX_EXTERN(__imp__sub_829032B8);
REX_EXTERN(sub_829036F0);
REX_EXTERN(__imp__sub_829036F0);
REX_EXTERN(sub_824A69B0);
REX_EXTERN(__imp__sub_824A69B0);
REX_EXTERN(sub_828ED7E8);
REX_EXTERN(__imp__sub_828ED7E8);
REX_EXTERN(sub_828ED9E0);
REX_EXTERN(__imp__sub_828ED9E0);
REX_EXTERN(sub_829019B0);
REX_EXTERN(__imp__sub_829019B0);
REX_EXTERN(sub_82901CE8);
REX_EXTERN(__imp__sub_82901CE8);
REX_EXTERN(sub_82902620);
REX_EXTERN(__imp__sub_82902620);
REX_EXTERN(sub_82902920);
REX_EXTERN(__imp__sub_82902920);
REX_EXTERN(sub_82902C60);
REX_EXTERN(__imp__sub_82902C60);
REX_EXTERN(sub_82903018);
REX_EXTERN(__imp__sub_82903018);
REX_EXTERN(sub_829033B8);
REX_EXTERN(__imp__sub_829033B8);
REX_EXTERN(sub_82903848);
REX_EXTERN(__imp__sub_82903848);
REX_EXTERN(sub_8262A900);
REX_EXTERN(__imp__sub_8262A900);
REX_EXTERN(sub_8262AEB8);
REX_EXTERN(__imp__sub_8262AEB8);
REX_EXTERN(sub_82703638);
REX_EXTERN(__imp__sub_82703638);
REX_EXTERN(sub_82418DA8);
REX_EXTERN(__imp__sub_82418DA8);
REX_EXTERN(sub_829004D8);
REX_EXTERN(__imp__sub_829004D8);
REX_EXTERN(sub_82900C50);
REX_EXTERN(__imp__sub_82900C50);
REX_EXTERN(sub_82900E50);
REX_EXTERN(__imp__sub_82900E50);
REX_EXTERN(sub_82900F88);
REX_EXTERN(__imp__sub_82900F88);
REX_EXTERN(sub_82901418);
REX_EXTERN(__imp__sub_82901418);
REX_EXTERN(sub_829017D8);
REX_EXTERN(__imp__sub_829017D8);
REX_EXTERN(sub_82901990);
REX_EXTERN(__imp__sub_82901990);
REX_EXTERN(sub_82901CC8);
REX_EXTERN(__imp__sub_82901CC8);
REX_EXTERN(sub_82902600);
REX_EXTERN(__imp__sub_82902600);
REX_EXTERN(sub_82902900);
REX_EXTERN(__imp__sub_82902900);
REX_EXTERN(sub_82902C40);
REX_EXTERN(__imp__sub_82902C40);
REX_EXTERN(sub_82902FF8);
REX_EXTERN(__imp__sub_82902FF8);
REX_EXTERN(sub_82903398);
REX_EXTERN(__imp__sub_82903398);
REX_EXTERN(sub_82903808);
REX_EXTERN(__imp__sub_82903808);
REX_EXTERN(sub_8262A600);
REX_EXTERN(__imp__sub_8262A600);
REX_EXTERN(sub_82901C08);
REX_EXTERN(__imp__sub_82901C08);
REX_EXTERN(sub_829022B8);
REX_EXTERN(__imp__sub_829022B8);
REX_EXTERN(sub_829027C0);
REX_EXTERN(__imp__sub_829027C0);
REX_EXTERN(sub_82902AE0);
REX_EXTERN(__imp__sub_82902AE0);
REX_EXTERN(sub_82902E40);
REX_EXTERN(__imp__sub_82902E40);
REX_EXTERN(sub_82903238);
REX_EXTERN(__imp__sub_82903238);
REX_EXTERN(sub_82903670);
REX_EXTERN(__imp__sub_82903670);
REX_EXTERN(sub_825422B0);
REX_EXTERN(__imp__sub_825422B0);
REX_EXTERN(sub_82455A28);
REX_EXTERN(__imp__sub_82455A28);
REX_EXTERN(sub_82540070);
REX_EXTERN(__imp__sub_82540070);
REX_EXTERN(sub_829005B8);
REX_EXTERN(__imp__sub_829005B8);
REX_EXTERN(sub_82900A98);
REX_EXTERN(__imp__sub_82900A98);
REX_EXTERN(sub_82900C90);
REX_EXTERN(__imp__sub_82900C90);
REX_EXTERN(sub_82900E88);
REX_EXTERN(__imp__sub_82900E88);
REX_EXTERN(sub_82901970);
REX_EXTERN(__imp__sub_82901970);
REX_EXTERN(sub_82901CA8);
REX_EXTERN(__imp__sub_82901CA8);
REX_EXTERN(sub_829025E0);
REX_EXTERN(__imp__sub_829025E0);
REX_EXTERN(sub_829028E0);
REX_EXTERN(__imp__sub_829028E0);
REX_EXTERN(sub_82902C20);
REX_EXTERN(__imp__sub_82902C20);
REX_EXTERN(sub_82902FD8);
REX_EXTERN(__imp__sub_82902FD8);
REX_EXTERN(sub_82903378);
REX_EXTERN(__imp__sub_82903378);
REX_EXTERN(sub_829037E8);
REX_EXTERN(__imp__sub_829037E8);
REX_EXTERN(sub_8262A708);
REX_EXTERN(__imp__sub_8262A708);
REX_EXTERN(sub_8263F1D0);
REX_EXTERN(__imp__sub_8263F1D0);
REX_EXTERN(sub_82495160);
REX_EXTERN(__imp__sub_82495160);
REX_EXTERN(sub_8262AE88);
REX_EXTERN(__imp__sub_8262AE88);
REX_EXTERN(sub_828ED9C0);
REX_EXTERN(__imp__sub_828ED9C0);
REX_EXTERN(sub_82901A20);
REX_EXTERN(__imp__sub_82901A20);
REX_EXTERN(sub_82901D20);
REX_EXTERN(__imp__sub_82901D20);
REX_EXTERN(sub_829025C0);
REX_EXTERN(__imp__sub_829025C0);
REX_EXTERN(sub_829028C0);
REX_EXTERN(__imp__sub_829028C0);
REX_EXTERN(sub_82902C00);
REX_EXTERN(__imp__sub_82902C00);
REX_EXTERN(sub_82902FB8);
REX_EXTERN(__imp__sub_82902FB8);
REX_EXTERN(sub_82903358);
REX_EXTERN(__imp__sub_82903358);
REX_EXTERN(sub_828ED808);
REX_EXTERN(__imp__sub_828ED808);
REX_EXTERN(sub_828EDA40);
REX_EXTERN(__imp__sub_828EDA40);
REX_EXTERN(sub_82901890);
REX_EXTERN(__imp__sub_82901890);
REX_EXTERN(sub_82901C28);
REX_EXTERN(__imp__sub_82901C28);
REX_EXTERN(sub_829022D8);
REX_EXTERN(__imp__sub_829022D8);
REX_EXTERN(sub_829027E0);
REX_EXTERN(__imp__sub_829027E0);
REX_EXTERN(sub_82902B00);
REX_EXTERN(__imp__sub_82902B00);
REX_EXTERN(sub_82902E60);
REX_EXTERN(__imp__sub_82902E60);
REX_EXTERN(sub_82903258);
REX_EXTERN(__imp__sub_82903258);
REX_EXTERN(sub_82903690);
REX_EXTERN(__imp__sub_82903690);
REX_EXTERN(sub_8242A2F0);
REX_EXTERN(__imp__sub_8242A2F0);
REX_EXTERN(sub_8244AA80);
REX_EXTERN(__imp__sub_8244AA80);
REX_EXTERN(sub_82901950);
REX_EXTERN(__imp__sub_82901950);
REX_EXTERN(sub_82901C88);
REX_EXTERN(__imp__sub_82901C88);
REX_EXTERN(sub_829025A0);
REX_EXTERN(__imp__sub_829025A0);
REX_EXTERN(sub_829028A0);
REX_EXTERN(__imp__sub_829028A0);
REX_EXTERN(sub_82902BE0);
REX_EXTERN(__imp__sub_82902BE0);
REX_EXTERN(sub_82902F98);
REX_EXTERN(__imp__sub_82902F98);
REX_EXTERN(sub_82903338);
REX_EXTERN(__imp__sub_82903338);
REX_EXTERN(sub_82903770);
REX_EXTERN(__imp__sub_82903770);
REX_EXTERN(sub_828ED948);
REX_EXTERN(__imp__sub_828ED948);
REX_EXTERN(sub_82900458);
REX_EXTERN(__imp__sub_82900458);
REX_EXTERN(sub_829005F8);
REX_EXTERN(__imp__sub_829005F8);
REX_EXTERN(sub_82901AC0);
REX_EXTERN(__imp__sub_82901AC0);
REX_EXTERN(sub_82901DA8);
REX_EXTERN(__imp__sub_82901DA8);
REX_EXTERN(sub_829026A0);
REX_EXTERN(__imp__sub_829026A0);
REX_EXTERN(sub_829029A0);
REX_EXTERN(__imp__sub_829029A0);
REX_EXTERN(sub_82902CE0);
REX_EXTERN(__imp__sub_82902CE0);
REX_EXTERN(sub_82903098);
REX_EXTERN(__imp__sub_82903098);
REX_EXTERN(sub_828ED968);
REX_EXTERN(__imp__sub_828ED968);
REX_EXTERN(sub_82495170);
REX_EXTERN(__imp__sub_82495170);
REX_EXTERN(sub_824A69C0);
REX_EXTERN(__imp__sub_824A69C0);
REX_EXTERN(sub_828ED878);
REX_EXTERN(__imp__sub_828ED878);
REX_EXTERN(sub_828EDA00);
REX_EXTERN(__imp__sub_828EDA00);
REX_EXTERN(sub_82900478);
REX_EXTERN(__imp__sub_82900478);
REX_EXTERN(sub_82900610);
REX_EXTERN(__imp__sub_82900610);
REX_EXTERN(sub_82900CF0);
REX_EXTERN(__imp__sub_82900CF0);
REX_EXTERN(sub_82900EE8);
REX_EXTERN(__imp__sub_82900EE8);
REX_EXTERN(sub_82901A80);
REX_EXTERN(__imp__sub_82901A80);
REX_EXTERN(sub_82901D68);
REX_EXTERN(__imp__sub_82901D68);
REX_EXTERN(sub_82902720);
REX_EXTERN(__imp__sub_82902720);
REX_EXTERN(sub_82902A40);
REX_EXTERN(__imp__sub_82902A40);
REX_EXTERN(sub_82902DA0);
REX_EXTERN(__imp__sub_82902DA0);
REX_EXTERN(sub_82903178);
REX_EXTERN(__imp__sub_82903178);
REX_EXTERN(sub_829035B0);
REX_EXTERN(__imp__sub_829035B0);
REX_EXTERN(sub_82461748);
REX_EXTERN(__imp__sub_82461748);
REX_EXTERN(sub_8253CC18);
REX_EXTERN(__imp__sub_8253CC18);
REX_EXTERN(sub_82900538);
REX_EXTERN(__imp__sub_82900538);
REX_EXTERN(sub_82900778);
REX_EXTERN(__imp__sub_82900778);
REX_EXTERN(sub_82900B38);
REX_EXTERN(__imp__sub_82900B38);
REX_EXTERN(sub_82900D30);
REX_EXTERN(__imp__sub_82900D30);
REX_EXTERN(sub_82900F28);
REX_EXTERN(__imp__sub_82900F28);
REX_EXTERN(sub_829018E0);
REX_EXTERN(__imp__sub_829018E0);
REX_EXTERN(sub_82901C48);
REX_EXTERN(__imp__sub_82901C48);
REX_EXTERN(sub_829022F8);
REX_EXTERN(__imp__sub_829022F8);
REX_EXTERN(sub_82902800);
REX_EXTERN(__imp__sub_82902800);
REX_EXTERN(sub_82902B20);
REX_EXTERN(__imp__sub_82902B20);
REX_EXTERN(sub_82902E80);
REX_EXTERN(__imp__sub_82902E80);
REX_EXTERN(sub_82903278);
REX_EXTERN(__imp__sub_82903278);
REX_EXTERN(sub_829036B0);
REX_EXTERN(__imp__sub_829036B0);
REX_EXTERN(sub_82703620);
REX_EXTERN(__imp__sub_82703620);
REX_EXTERN(sub_829003F8);
REX_EXTERN(__imp__sub_829003F8);
REX_EXTERN(sub_82900518);
REX_EXTERN(__imp__sub_82900518);
REX_EXTERN(sub_82900758);
REX_EXTERN(__imp__sub_82900758);
REX_EXTERN(sub_82900C10);
REX_EXTERN(__imp__sub_82900C10);
REX_EXTERN(sub_82900E10);
REX_EXTERN(__imp__sub_82900E10);
REX_EXTERN(sub_82901028);
REX_EXTERN(__imp__sub_82901028);
REX_EXTERN(sub_82901780);
REX_EXTERN(__imp__sub_82901780);
REX_EXTERN(sub_829018C8);
REX_EXTERN(__imp__sub_829018C8);
REX_EXTERN(sub_82901A60);
REX_EXTERN(__imp__sub_82901A60);
REX_EXTERN(sub_82901D50);
REX_EXTERN(__imp__sub_82901D50);
REX_EXTERN(sub_82902680);
REX_EXTERN(__imp__sub_82902680);
REX_EXTERN(sub_82902980);
REX_EXTERN(__imp__sub_82902980);
REX_EXTERN(sub_82902CC0);
REX_EXTERN(__imp__sub_82902CC0);
REX_EXTERN(sub_82903078);
REX_EXTERN(__imp__sub_82903078);
REX_EXTERN(sub_82903418);
REX_EXTERN(__imp__sub_82903418);
REX_EXTERN(sub_829038E0);
REX_EXTERN(__imp__sub_829038E0);
REX_EXTERN(sub_82902F00);
REX_EXTERN(__imp__sub_82902F00);
REX_EXTERN(sub_829032F8);
REX_EXTERN(__imp__sub_829032F8);
REX_EXTERN(sub_82903730);
REX_EXTERN(__imp__sub_82903730);
REX_EXTERN(sub_8262AE70);
REX_EXTERN(__imp__sub_8262AE70);
REX_EXTERN(sub_828ED8D0);
REX_EXTERN(__imp__sub_828ED8D0);
REX_EXTERN(sub_829005A0);
REX_EXTERN(__imp__sub_829005A0);
REX_EXTERN(sub_82900718);
REX_EXTERN(__imp__sub_82900718);
REX_EXTERN(sub_82900BB0);
REX_EXTERN(__imp__sub_82900BB0);
REX_EXTERN(sub_82900DB0);
REX_EXTERN(__imp__sub_82900DB0);
REX_EXTERN(sub_82900FC8);
REX_EXTERN(__imp__sub_82900FC8);
REX_EXTERN(sub_82901450);
REX_EXTERN(__imp__sub_82901450);
REX_EXTERN(sub_82901870);
REX_EXTERN(__imp__sub_82901870);
REX_EXTERN(sub_82901BA8);
REX_EXTERN(__imp__sub_82901BA8);
REX_EXTERN(sub_829004B8);
REX_EXTERN(__imp__sub_829004B8);
REX_EXTERN(sub_82900680);
REX_EXTERN(__imp__sub_82900680);
REX_EXTERN(sub_82900B58);
REX_EXTERN(__imp__sub_82900B58);
REX_EXTERN(sub_82900CD0);
REX_EXTERN(__imp__sub_82900CD0);
REX_EXTERN(sub_82900EC8);
REX_EXTERN(__imp__sub_82900EC8);
REX_EXTERN(sub_82901A00);
REX_EXTERN(__imp__sub_82901A00);
REX_EXTERN(sub_82901D08);
REX_EXTERN(__imp__sub_82901D08);
REX_EXTERN(sub_82902338);
REX_EXTERN(__imp__sub_82902338);
REX_EXTERN(sub_82902880);
REX_EXTERN(__imp__sub_82902880);
REX_EXTERN(sub_82902BC0);
REX_EXTERN(__imp__sub_82902BC0);
REX_EXTERN(sub_82902F78);
REX_EXTERN(__imp__sub_82902F78);
REX_EXTERN(sub_82903318);
REX_EXTERN(__imp__sub_82903318);
REX_EXTERN(sub_82903750);
REX_EXTERN(__imp__sub_82903750);
REX_EXTERN(sub_82902BA0);
REX_EXTERN(__imp__sub_82902BA0);
REX_EXTERN(sub_82903920);
REX_EXTERN(__imp__sub_82903920);
REX_EXTERN(sub_824633D8);
REX_EXTERN(__imp__sub_824633D8);
REX_EXTERN(sub_828ED890);
REX_EXTERN(__imp__sub_828ED890);
REX_EXTERN(sub_82903590);
REX_EXTERN(__imp__sub_82903590);
REX_EXTERN(sub_82418D50);
REX_EXTERN(__imp__sub_82418D50);
REX_EXTERN(sub_82900438);
REX_EXTERN(__imp__sub_82900438);
REX_EXTERN(sub_82900570);
REX_EXTERN(__imp__sub_82900570);
REX_EXTERN(sub_82900668);
REX_EXTERN(__imp__sub_82900668);
REX_EXTERN(sub_82900A78);
REX_EXTERN(__imp__sub_82900A78);
REX_EXTERN(sub_82900C70);
REX_EXTERN(__imp__sub_82900C70);
REX_EXTERN(sub_82900E68);
REX_EXTERN(__imp__sub_82900E68);
REX_EXTERN(sub_82901920);
REX_EXTERN(__imp__sub_82901920);
REX_EXTERN(sub_82901B40);
REX_EXTERN(__imp__sub_82901B40);
REX_EXTERN(sub_82901E30);
REX_EXTERN(__imp__sub_82901E30);
REX_EXTERN(sub_82902860);
REX_EXTERN(__imp__sub_82902860);
REX_EXTERN(sub_82902B80);
REX_EXTERN(__imp__sub_82902B80);
REX_EXTERN(sub_82902EE0);
REX_EXTERN(__imp__sub_82902EE0);
REX_EXTERN(sub_829032D8);
REX_EXTERN(__imp__sub_829032D8);
REX_EXTERN(sub_82903710);
REX_EXTERN(__imp__sub_82903710);
REX_EXTERN(sub_828ED8F0);
REX_EXTERN(__imp__sub_828ED8F0);
REX_EXTERN(sub_828EDA20);
REX_EXTERN(__imp__sub_828EDA20);
REX_EXTERN(sub_82468DF8);
REX_EXTERN(__imp__sub_82468DF8);
REX_EXTERN(sub_82547720);
REX_EXTERN(__imp__sub_82547720);
REX_EXTERN(sub_82900588);
REX_EXTERN(__imp__sub_82900588);
REX_EXTERN(sub_82900D50);
REX_EXTERN(__imp__sub_82900D50);
REX_EXTERN(sub_82900F48);
REX_EXTERN(__imp__sub_82900F48);
REX_EXTERN(sub_829013D8);
REX_EXTERN(__imp__sub_829013D8);
REX_EXTERN(sub_829017F0);
REX_EXTERN(__imp__sub_829017F0);
REX_EXTERN(sub_82901AA0);
REX_EXTERN(__imp__sub_82901AA0);
REX_EXTERN(sub_82901D88);
REX_EXTERN(__imp__sub_82901D88);
REX_EXTERN(sub_82902740);
REX_EXTERN(__imp__sub_82902740);
REX_EXTERN(sub_82902A60);
REX_EXTERN(__imp__sub_82902A60);
REX_EXTERN(sub_82902DC0);
REX_EXTERN(__imp__sub_82902DC0);
REX_EXTERN(sub_82903198);
REX_EXTERN(__imp__sub_82903198);
REX_EXTERN(sub_829035D0);
REX_EXTERN(__imp__sub_829035D0);
REX_EXTERN(sub_829005D8);
REX_EXTERN(__imp__sub_829005D8);
REX_EXTERN(sub_82900B90);
REX_EXTERN(__imp__sub_82900B90);
REX_EXTERN(sub_82900D90);
REX_EXTERN(__imp__sub_82900D90);
REX_EXTERN(sub_82900FA8);
REX_EXTERN(__imp__sub_82900FA8);
REX_EXTERN(sub_82901430);
REX_EXTERN(__imp__sub_82901430);
REX_EXTERN(sub_82901850);
REX_EXTERN(__imp__sub_82901850);
REX_EXTERN(sub_82901B88);
REX_EXTERN(__imp__sub_82901B88);
REX_EXTERN(sub_82901E58);
REX_EXTERN(__imp__sub_82901E58);
REX_EXTERN(sub_82902760);
REX_EXTERN(__imp__sub_82902760);
REX_EXTERN(sub_82902A80);
REX_EXTERN(__imp__sub_82902A80);
REX_EXTERN(sub_82902DE0);
REX_EXTERN(__imp__sub_82902DE0);
REX_EXTERN(sub_829031B8);
REX_EXTERN(__imp__sub_829031B8);
REX_EXTERN(sub_829035F0);
REX_EXTERN(__imp__sub_829035F0);
REX_EXTERN(sub_829003D8);
REX_EXTERN(__imp__sub_829003D8);
REX_EXTERN(sub_829004F8);
REX_EXTERN(__imp__sub_829004F8);
REX_EXTERN(sub_82900738);
REX_EXTERN(__imp__sub_82900738);
REX_EXTERN(sub_82900BF0);
REX_EXTERN(__imp__sub_82900BF0);
REX_EXTERN(sub_82900DF0);
REX_EXTERN(__imp__sub_82900DF0);
REX_EXTERN(sub_82901008);
REX_EXTERN(__imp__sub_82901008);

namespace {
struct Address {
  uint32_t absolute = 0;
  GuestRegister GuestContext::*reg = nullptr;
  int32_t offset = 0;
  uint32_t Get(const GuestContext& ctx) const {
    return reg ? (ctx.*reg).u32 + uint32_t(offset) : absolute;
  }
};

struct Query {
  const char* name;
  GuestFunction* original;
  GuestFunction* native;
  unsigned load_count;
  std::array<Address, 3> loads;
  unsigned store_count;
  std::array<Address, 4> stores;
};

const Query kQueries[] = {
  {"sub_8262AE58", __imp__sub_8262AE58, sub_8262AE58, 1, {{Address{0x82374998u}}}, 1, {{Address{0, &GuestContext::r5, 0}}}},
  {"sub_8262AE40", __imp__sub_8262AE40, sub_8262AE40, 1, {{Address{0x8237499Cu}}}, 1, {{Address{0, &GuestContext::r5, 0}}}},
  {"sub_826F15E8", __imp__sub_826F15E8, sub_826F15E8, 1, {{Address{0, &GuestContext::r3, 576}}}, 0, {{}}},
  {"sub_82902D60", __imp__sub_82902D60, sub_82902D60, 2, {{Address{0x82922540u}, Address{0x82334CD4u}}}, 1, {{Address{0x829CD1E8u}}}},
  {"sub_82903118", __imp__sub_82903118, sub_82903118, 2, {{Address{0x82922D30u}, Address{0x8233C34Cu}}}, 1, {{Address{0x829CD254u}}}},
  {"sub_828ED908", __imp__sub_828ED908, sub_828ED908, 2, {{Address{0x829123B0u}, Address{0x82000E5Cu}}}, 1, {{Address{0x829AB3CCu}}}},
  {"sub_82900418", __imp__sub_82900418, sub_82900418, 2, {{Address{0x82913C64u}, Address{0x82016F1Cu}}}, 1, {{Address{0x829B212Cu}}}},
  {"sub_82900558", __imp__sub_82900558, sub_82900558, 1, {{Address{0x82913C84u}}}, 1, {{Address{0x829B2154u}}}},
  {"sub_82900628", __imp__sub_82900628, sub_82900628, 1, {{Address{0x82913C94u}}}, 1, {{Address{0x829B2174u}}}},
  {"sub_82903158", __imp__sub_82903158, sub_82903158, 2, {{Address{0x82922D70u}, Address{0x8233C410u}}}, 1, {{Address{0x829CD25Cu}}}},
  {"sub_82903570", __imp__sub_82903570, sub_82903570, 2, {{Address{0x82923390u}, Address{0x8233F038u}}}, 1, {{Address{0x829CD2D4u}}}},
  {"sub_8262AF00", __imp__sub_8262AF00, sub_8262AF00, 1, {{Address{0x82000B48u}}}, 1, {{Address{0, &GuestContext::r5, 0}}}},
  {"sub_828ED8B0", __imp__sub_828ED8B0, sub_828ED8B0, 2, {{Address{0x829121C8u}, Address{0x82000E5Cu}}}, 1, {{Address{0x829AB3C0u}}}},
  {"sub_82900CB0", __imp__sub_82900CB0, sub_82900CB0, 2, {{Address{0x8291403Cu}, Address{0x82018DC8u}}}, 1, {{Address{0x829CCC40u}}}},
  {"sub_82900EA8", __imp__sub_82900EA8, sub_82900EA8, 2, {{Address{0x82914264u}, Address{0x82000E5Cu}}}, 1, {{Address{0x829CCC8Cu}}}},
  {"sub_829019E8", __imp__sub_829019E8, sub_829019E8, 1, {{Address{0x82000AE8u}}}, 1, {{Address{0x829CCE6Cu}}}},
  {"sub_82901BE8", __imp__sub_82901BE8, sub_82901BE8, 2, {{Address{0x82921478u}, Address{0x820183ACu}}}, 1, {{Address{0x829CCEBCu}}}},
  {"sub_82902298", __imp__sub_82902298, sub_82902298, 2, {{Address{0x829214DCu}, Address{0x82088FDCu}}}, 1, {{Address{0x829CD0A0u}}}},
  {"sub_829027A0", __imp__sub_829027A0, sub_829027A0, 2, {{Address{0x829218D4u}, Address{0x82328804u}}}, 1, {{Address{0x829CD124u}}}},
  {"sub_82902AC0", __imp__sub_82902AC0, sub_82902AC0, 2, {{Address{0x82921FC8u}, Address{0x8232FEC4u}}}, 1, {{Address{0x829CD188u}}}},
  {"sub_82902E20", __imp__sub_82902E20, sub_82902E20, 2, {{Address{0x829226B4u}, Address{0x82336764u}}}, 1, {{Address{0x829CD200u}}}},
  {"sub_82903218", __imp__sub_82903218, sub_82903218, 2, {{Address{0x82922EA4u}, Address{0x8233CB64u}}}, 1, {{Address{0x829CD274u}}}},
  {"sub_82903650", __imp__sub_82903650, sub_82903650, 2, {{Address{0x829234F0u}, Address{0x8233F900u}}}, 1, {{Address{0x829CD2F0u}}}},
  {"sub_82703788", __imp__sub_82703788, sub_82703788, 0, {{}}, 1, {{Address{0, &GuestContext::r3, 80}}}},
  {"sub_828ED988", __imp__sub_828ED988, sub_828ED988, 2, {{Address{0x829124F0u}, Address{0x82011668u}}}, 1, {{Address{0x829AB3DCu}}}},
  {"sub_829019D0", __imp__sub_829019D0, sub_829019D0, 1, {{Address{0x82000AE4u}}}, 1, {{Address{0x829CCE68u}}}},
  {"sub_82901BC8", __imp__sub_82901BC8, sub_82901BC8, 2, {{Address{0x82921474u}, Address{0x82016F1Cu}}}, 1, {{Address{0x829CCEB8u}}}},
  {"sub_82901E78", __imp__sub_82901E78, sub_82901E78, 2, {{Address{0x829214C0u}, Address{0x82018BACu}}}, 1, {{Address{0x829CCF14u}}}},
  {"sub_82902780", __imp__sub_82902780, sub_82902780, 2, {{Address{0x82921818u}, Address{0x823280D4u}}}, 1, {{Address{0x829CD120u}}}},
  {"sub_82902AA0", __imp__sub_82902AA0, sub_82902AA0, 2, {{Address{0x82921FC8u}, Address{0x8232FEC4u}}}, 1, {{Address{0x829CD184u}}}},
  {"sub_82902E00", __imp__sub_82902E00, sub_82902E00, 2, {{Address{0x82922638u}, Address{0x8233609Cu}}}, 1, {{Address{0x829CD1FCu}}}},
  {"sub_829031F8", __imp__sub_829031F8, sub_829031F8, 2, {{Address{0x82922EA4u}, Address{0x8233CB64u}}}, 1, {{Address{0x829CD270u}}}},
  {"sub_82903630", __imp__sub_82903630, sub_82903630, 2, {{Address{0x829234B0u}, Address{0x8233F828u}}}, 1, {{Address{0x829CD2ECu}}}},
  {"sub_82418D38", __imp__sub_82418D38, sub_82418D38, 1, {{Address{0x82000B48u}}}, 2, {{Address{0, &GuestContext::r3, 4}, Address{0, &GuestContext::r3, 0}}}},
  {"sub_82902A00", __imp__sub_82902A00, sub_82902A00, 2, {{Address{0x82921E44u}, Address{0x8232F0B4u}}}, 1, {{Address{0x829CD170u}}}},
  {"sub_82902D40", __imp__sub_82902D40, sub_82902D40, 2, {{Address{0x829224C4u}, Address{0x82334854u}}}, 1, {{Address{0x829CD1E4u}}}},
  {"sub_829030F8", __imp__sub_829030F8, sub_829030F8, 2, {{Address{0x82922D30u}, Address{0x8233C34Cu}}}, 1, {{Address{0x829CD250u}}}},
  {"sub_825231D0", __imp__sub_825231D0, sub_825231D0, 1, {{Address{0x82000DA8u}}}, 0, {{}}},
  {"sub_8262AEE8", __imp__sub_8262AEE8, sub_8262AEE8, 1, {{Address{0x8237498Cu}}}, 1, {{Address{0, &GuestContext::r5, 0}}}},
  {"sub_82903828", __imp__sub_82903828, sub_82903828, 2, {{Address{0x82923788u}, Address{0x82340ED4u}}}, 1, {{Address{0x829CD320u}}}},
  {"sub_828ED928", __imp__sub_828ED928, sub_828ED928, 2, {{Address{0x829124A0u}, Address{0x82000E5Cu}}}, 1, {{Address{0x829AB3D0u}}}},
  {"sub_8262AED0", __imp__sub_8262AED0, sub_8262AED0, 1, {{Address{0x8234EB34u}}}, 1, {{Address{0, &GuestContext::r5, 0}}}},
  {"sub_828ED828", __imp__sub_828ED828, sub_828ED828, 2, {{Address{0x82912058u}, Address{0x8200DEB0u}}}, 1, {{Address{0x829AB3B0u}}}},
  {"sub_82900498", __imp__sub_82900498, sub_82900498, 2, {{Address{0x829B2134u}, Address{0x820024FCu}}}, 1, {{Address{0x829B213Cu}}}},
  {"sub_82900640", __imp__sub_82900640, sub_82900640, 3, {{Address{0x82913C60u}, Address{0x82016F24u}, Address{0x829B2170u}}}, 1, {{Address{0x829B2178u}}}},
  {"sub_82900B18", __imp__sub_82900B18, sub_82900B18, 2, {{Address{0x82913E38u}, Address{0x82018BACu}}}, 1, {{Address{0x829CC9A4u}}}},
  {"sub_82900D10", __imp__sub_82900D10, sub_82900D10, 2, {{Address{0x82914074u}, Address{0x82018E24u}}}, 1, {{Address{0x829CCC4Cu}}}},
  {"sub_82900F08", __imp__sub_82900F08, sub_82900F08, 2, {{Address{0x82914314u}, Address{0x82019700u}}}, 1, {{Address{0x829CCC98u}}}},
  {"sub_82901B00", __imp__sub_82901B00, sub_82901B00, 2, {{Address{0x82921414u}, Address{0x82088008u}}}, 1, {{Address{0x829CCE94u}}}},
  {"sub_82901DD8", __imp__sub_82901DD8, sub_82901DD8, 1, {{Address{0x829214A4u}}}, 1, {{Address{0x829CCF00u}}}},
  {"sub_829026E0", __imp__sub_829026E0, sub_829026E0, 2, {{Address{0x82921770u}, Address{0x82089880u}}}, 1, {{Address{0x829CD10Cu}}}},
  {"sub_829029E0", __imp__sub_829029E0, sub_829029E0, 2, {{Address{0x82921E44u}, Address{0x8232F0B4u}}}, 1, {{Address{0x829CD16Cu}}}},
  {"sub_82902D20", __imp__sub_82902D20, sub_82902D20, 2, {{Address{0x829224C4u}, Address{0x82334854u}}}, 1, {{Address{0x829CD1E0u}}}},
  {"sub_829030D8", __imp__sub_829030D8, sub_829030D8, 2, {{Address{0x82922CB4u}, Address{0x8233B3FCu}}}, 1, {{Address{0x829CD24Cu}}}},
  {"sub_82903480", __imp__sub_82903480, sub_82903480, 2, {{Address{0x829232CCu}, Address{0x8233EEC4u}}}, 1, {{Address{0x829CD2CCu}}}},
  {"sub_8242A2C8", __imp__sub_8242A2C8, sub_8242A2C8, 0, {{}}, 3, {{Address{0, &GuestContext::r3, 0}, Address{0, &GuestContext::r3, 4}, Address{0, &GuestContext::r3, 8}}}},
  {"sub_8262AEA0", __imp__sub_8262AEA0, sub_8262AEA0, 1, {{Address{0x82374990u}}}, 1, {{Address{0, &GuestContext::r5, 0}}}},
  {"sub_82900BD0", __imp__sub_82900BD0, sub_82900BD0, 2, {{Address{0x82913FE8u}, Address{0x82018D60u}}}, 1, {{Address{0x829CCC24u}}}},
  {"sub_82900DD0", __imp__sub_82900DD0, sub_82900DD0, 2, {{Address{0x829140CCu}, Address{0x82018ED0u}}}, 1, {{Address{0x829CCC64u}}}},
  {"sub_82900FE8", __imp__sub_82900FE8, sub_82900FE8, 2, {{Address{0x82914520u}, Address{0x8201D998u}}}, 1, {{Address{0x829CCCC4u}}}},
  {"sub_82901470", __imp__sub_82901470, sub_82901470, 2, {{Address{0x82920BACu}, Address{0x82000E5Cu}}}, 1, {{Address{0x829CCE0Cu}}}},
  {"sub_829018B0", __imp__sub_829018B0, sub_829018B0, 1, {{Address{0x82000AE4u}}}, 1, {{Address{0x829CCE40u}}}},
  {"sub_82901A40", __imp__sub_82901A40, sub_82901A40, 2, {{Address{0x82921394u}, Address{0x82087F60u}}}, 1, {{Address{0x829CCE78u}}}},
  {"sub_82901D38", __imp__sub_82901D38, sub_82901D38, 1, {{Address{0x8292149Cu}}}, 1, {{Address{0x829CCEE8u}}}},
  {"sub_82902660", __imp__sub_82902660, sub_82902660, 2, {{Address{0x8292173Cu}, Address{0x820896CCu}}}, 1, {{Address{0x829CD0FCu}}}},
  {"sub_82902960", __imp__sub_82902960, sub_82902960, 2, {{Address{0x82921D2Cu}, Address{0x8232DA44u}}}, 1, {{Address{0x829CD15Cu}}}},
  {"sub_82902CA0", __imp__sub_82902CA0, sub_82902CA0, 2, {{Address{0x829223C8u}, Address{0x82333F18u}}}, 1, {{Address{0x829CD1D0u}}}},
  {"sub_82903058", __imp__sub_82903058, sub_82903058, 2, {{Address{0x82922B6Cu}, Address{0x82339974u}}}, 1, {{Address{0x829CD23Cu}}}},
  {"sub_829033F8", __imp__sub_829033F8, sub_829033F8, 2, {{Address{0x8292320Cu}, Address{0x8233E384u}}}, 1, {{Address{0x829CD2B0u}}}},
  {"sub_829038C0", __imp__sub_829038C0, sub_829038C0, 2, {{Address{0x8292384Cu}, Address{0x82341084u}}}, 1, {{Address{0x829CD328u}}}},
  {"sub_824A6CD0", __imp__sub_824A6CD0, sub_824A6CD0, 1, {{Address{0x82000B44u}}}, 1, {{Address{0, &GuestContext::r3, 1352}}}},
  {"sub_82900C30", __imp__sub_82900C30, sub_82900C30, 2, {{Address{0x82914004u}, Address{0x82018D7Cu}}}, 1, {{Address{0x829CCC30u}}}},
  {"sub_82900E30", __imp__sub_82900E30, sub_82900E30, 2, {{Address{0x82914100u}, Address{0x82018F30u}}}, 1, {{Address{0x829CCC7Cu}}}},
  {"sub_82901068", __imp__sub_82901068, sub_82901068, 2, {{Address{0x82914B30u}, Address{0x8201E068u}}}, 1, {{Address{0x829CCCD4u}}}},
  {"sub_829017C0", __imp__sub_829017C0, sub_829017C0, 1, {{Address{0x82000AE4u}}}, 1, {{Address{0x829CCE1Cu}}}},
  {"sub_82901810", __imp__sub_82901810, sub_82901810, 2, {{Address{0x8291088Cu}, Address{0x82013260u}}}, 1, {{Address{0x829CCE28u}}}},
  {"sub_82901AE0", __imp__sub_82901AE0, sub_82901AE0, 2, {{Address{0x82921414u}, Address{0x82088008u}}}, 1, {{Address{0x829CCE90u}}}},
  {"sub_82901DC0", __imp__sub_82901DC0, sub_82901DC0, 1, {{Address{0x829214A8u}}}, 1, {{Address{0x829CCEFCu}}}},
  {"sub_829026C0", __imp__sub_829026C0, sub_829026C0, 2, {{Address{0x82921754u}, Address{0x82089850u}}}, 1, {{Address{0x829CD108u}}}},
  {"sub_829029C0", __imp__sub_829029C0, sub_829029C0, 2, {{Address{0x82921DA8u}, Address{0x8232DF9Cu}}}, 1, {{Address{0x829CD168u}}}},
  {"sub_82902D00", __imp__sub_82902D00, sub_82902D00, 2, {{Address{0x82922408u}, Address{0x82333FE4u}}}, 1, {{Address{0x829CD1DCu}}}},
  {"sub_829030B8", __imp__sub_829030B8, sub_829030B8, 2, {{Address{0x82922CB4u}, Address{0x8233B3FCu}}}, 1, {{Address{0x829CD248u}}}},
  {"sub_82903460", __imp__sub_82903460, sub_82903460, 2, {{Address{0x829232CCu}, Address{0x8233EEC4u}}}, 1, {{Address{0x829CD2C8u}}}},
  {"sub_82903900", __imp__sub_82903900, sub_82903900, 2, {{Address{0x8292388Cu}, Address{0x82341148u}}}, 1, {{Address{0x829CD330u}}}},
  {"sub_828ED848", __imp__sub_828ED848, sub_828ED848, 2, {{Address{0x82912058u}, Address{0x8200DEB0u}}}, 1, {{Address{0x829AB3B4u}}}},
  {"sub_82902640", __imp__sub_82902640, sub_82902640, 2, {{Address{0x82921720u}, Address{0x820896B4u}}}, 1, {{Address{0x829CD0F8u}}}},
  {"sub_82902940", __imp__sub_82902940, sub_82902940, 2, {{Address{0x82921C70u}, Address{0x8232CB7Cu}}}, 1, {{Address{0x829CD158u}}}},
  {"sub_82902C80", __imp__sub_82902C80, sub_82902C80, 2, {{Address{0x829223ACu}, Address{0x82333E2Cu}}}, 1, {{Address{0x829CD1CCu}}}},
  {"sub_82903038", __imp__sub_82903038, sub_82903038, 2, {{Address{0x82922B6Cu}, Address{0x82339974u}}}, 1, {{Address{0x829CD238u}}}},
  {"sub_829033D8", __imp__sub_829033D8, sub_829033D8, 2, {{Address{0x829231CCu}, Address{0x8233E2ACu}}}, 1, {{Address{0x829CD2ACu}}}},
  {"sub_82901938", __imp__sub_82901938, sub_82901938, 1, {{Address{0x82000AE8u}}}, 1, {{Address{0x829CCE54u}}}},
  {"sub_82901E18", __imp__sub_82901E18, sub_82901E18, 1, {{Address{0x829214A4u}}}, 1, {{Address{0x829CCF08u}}}},
  {"sub_82902700", __imp__sub_82902700, sub_82902700, 2, {{Address{0x82921770u}, Address{0x82089880u}}}, 1, {{Address{0x829CD110u}}}},
  {"sub_82902A20", __imp__sub_82902A20, sub_82902A20, 2, {{Address{0x82921ED0u}, Address{0x8232F24Cu}}}, 1, {{Address{0x829CD174u}}}},
  {"sub_82902D80", __imp__sub_82902D80, sub_82902D80, 2, {{Address{0x82922540u}, Address{0x82334CD4u}}}, 1, {{Address{0x829CD1ECu}}}},
  {"sub_82903138", __imp__sub_82903138, sub_82903138, 2, {{Address{0x82922D70u}, Address{0x8233C410u}}}, 1, {{Address{0x829CD258u}}}},
  {"sub_82903550", __imp__sub_82903550, sub_82903550, 2, {{Address{0x82923390u}, Address{0x8233F038u}}}, 1, {{Address{0x829CD2D0u}}}},
  {"sub_828ED9A8", __imp__sub_828ED9A8, sub_828ED9A8, 1, {{Address{0x829125BCu}}}, 1, {{Address{0x829AB3E0u}}}},
  {"sub_82901048", __imp__sub_82901048, sub_82901048, 2, {{Address{0x82914B30u}, Address{0x8201E068u}}}, 1, {{Address{0x829CCCD0u}}}},
  {"sub_829017A0", __imp__sub_829017A0, sub_829017A0, 2, {{Address{0x829211B0u}, Address{0x82086778u}}}, 1, {{Address{0x829CCE18u}}}},
  {"sub_82901900", __imp__sub_82901900, sub_82901900, 2, {{Address{0x8291088Cu}, Address{0x82018650u}}}, 1, {{Address{0x829CCE4Cu}}}},
  {"sub_82901C68", __imp__sub_82901C68, sub_82901C68, 2, {{Address{0x829CCEC4u}, Address{0x820146D0u}}}, 1, {{Address{0x829CCECCu}}}},
  {"sub_82902318", __imp__sub_82902318, sub_82902318, 2, {{Address{0x8292155Cu}, Address{0x82089200u}}}, 1, {{Address{0x829CD0DCu}}}},
  {"sub_82902820", __imp__sub_82902820, sub_82902820, 2, {{Address{0x829219CCu}, Address{0x82329884u}}}, 1, {{Address{0x829CD134u}}}},
  {"sub_82902B40", __imp__sub_82902B40, sub_82902B40, 2, {{Address{0x82922110u}, Address{0x82330DACu}}}, 1, {{Address{0x829CD198u}}}},
  {"sub_82902EA0", __imp__sub_82902EA0, sub_82902EA0, 2, {{Address{0x829227ECu}, Address{0x82337254u}}}, 1, {{Address{0x829CD210u}}}},
  {"sub_82903298", __imp__sub_82903298, sub_82903298, 2, {{Address{0x82922F9Cu}, Address{0x8233D1F4u}}}, 1, {{Address{0x829CD284u}}}},
  {"sub_829036D0", __imp__sub_829036D0, sub_829036D0, 2, {{Address{0x829235B4u}, Address{0x8233FDF8u}}}, 1, {{Address{0x829CD300u}}}},
  {"sub_82418DF8", __imp__sub_82418DF8, sub_82418DF8, 1, {{Address{0x82000B48u}}}, 4, {{Address{0, &GuestContext::r3, 0}, Address{0, &GuestContext::r3, 4}, Address{0, &GuestContext::r3, 8}, Address{0, &GuestContext::r3, 12}}}},
  {"sub_829031D8", __imp__sub_829031D8, sub_829031D8, 2, {{Address{0x82922E64u}, Address{0x8233CA9Cu}}}, 1, {{Address{0x829CD26Cu}}}},
  {"sub_82903610", __imp__sub_82903610, sub_82903610, 2, {{Address{0x829234B0u}, Address{0x8233F828u}}}, 1, {{Address{0x829CD2E8u}}}},
  {"sub_82900B70", __imp__sub_82900B70, sub_82900B70, 2, {{Address{0x82913EA8u}, Address{0x82000E5Cu}}}, 1, {{Address{0x829CC9B0u}}}},
  {"sub_82900D70", __imp__sub_82900D70, sub_82900D70, 2, {{Address{0x82914090u}, Address{0x82018E3Cu}}}, 1, {{Address{0x829CCC58u}}}},
  {"sub_82900F68", __imp__sub_82900F68, sub_82900F68, 2, {{Address{0x82914504u}, Address{0x8201A308u}}}, 1, {{Address{0x829CCCB4u}}}},
  {"sub_829013F8", __imp__sub_829013F8, sub_829013F8, 2, {{Address{0x82920A44u}, Address{0x820856C4u}}}, 1, {{Address{0x829CCDFCu}}}},
  {"sub_82901830", __imp__sub_82901830, sub_82901830, 2, {{Address{0x829211C8u}, Address{0x82086D84u}}}, 1, {{Address{0x829CCE2Cu}}}},
  {"sub_82901B20", __imp__sub_82901B20, sub_82901B20, 2, {{Address{0x82921430u}, Address{0x82088034u}}}, 1, {{Address{0x829CCE98u}}}},
  {"sub_82901DF0", __imp__sub_82901DF0, sub_82901DF0, 3, {{Address{0x82921470u}, Address{0x82016F24u}, Address{0x829CCEFCu}}}, 1, {{Address{0x829CCF04u}}}},
  {"sub_82902840", __imp__sub_82902840, sub_82902840, 2, {{Address{0x829219CCu}, Address{0x82329884u}}}, 1, {{Address{0x829CD138u}}}},
  {"sub_82902B60", __imp__sub_82902B60, sub_82902B60, 2, {{Address{0x8292219Cu}, Address{0x823313BCu}}}, 1, {{Address{0x829CD19Cu}}}},
  {"sub_82902EC0", __imp__sub_82902EC0, sub_82902EC0, 2, {{Address{0x829227ECu}, Address{0x82337254u}}}, 1, {{Address{0x829CD214u}}}},
  {"sub_829032B8", __imp__sub_829032B8, sub_829032B8, 2, {{Address{0x82922FDCu}, Address{0x8233D2BCu}}}, 1, {{Address{0x829CD288u}}}},
  {"sub_829036F0", __imp__sub_829036F0, sub_829036F0, 2, {{Address{0x829235B4u}, Address{0x8233FDF8u}}}, 1, {{Address{0x829CD304u}}}},
  {"sub_824A69B0", __imp__sub_824A69B0, sub_824A69B0, 0, {{}}, 2, {{Address{0, &GuestContext::r3, 120}, Address{0, &GuestContext::r3, 124}}}},
  {"sub_828ED7E8", __imp__sub_828ED7E8, sub_828ED7E8, 2, {{Address{0x8291203Cu}, Address{0x8200DE20u}}}, 1, {{Address{0x829AAEE0u}}}},
  {"sub_828ED9E0", __imp__sub_828ED9E0, sub_828ED9E0, 2, {{Address{0x829126B0u}, Address{0x82000E5Cu}}}, 1, {{Address{0x829AB3E8u}}}},
  {"sub_829019B0", __imp__sub_829019B0, sub_829019B0, 2, {{Address{0x8292137Cu}, Address{0x82087CF4u}}}, 1, {{Address{0x829CCE64u}}}},
  {"sub_82901CE8", __imp__sub_82901CE8, sub_82901CE8, 2, {{Address{0x829CCEC4u}, Address{0x8200DE00u}}}, 1, {{Address{0x829CCEDCu}}}},
  {"sub_82902620", __imp__sub_82902620, sub_82902620, 2, {{Address{0x82921720u}, Address{0x820896B4u}}}, 1, {{Address{0x829CD0F4u}}}},
  {"sub_82902920", __imp__sub_82902920, sub_82902920, 2, {{Address{0x82921C70u}, Address{0x8232CB7Cu}}}, 1, {{Address{0x829CD154u}}}},
  {"sub_82902C60", __imp__sub_82902C60, sub_82902C60, 2, {{Address{0x829223ACu}, Address{0x82333E2Cu}}}, 1, {{Address{0x829CD1C8u}}}},
  {"sub_82903018", __imp__sub_82903018, sub_82903018, 2, {{Address{0x82922AC0u}, Address{0x82338A04u}}}, 1, {{Address{0x829CD234u}}}},
  {"sub_829033B8", __imp__sub_829033B8, sub_829033B8, 2, {{Address{0x829231CCu}, Address{0x8233E2ACu}}}, 1, {{Address{0x829CD2A8u}}}},
  {"sub_82903848", __imp__sub_82903848, sub_82903848, 2, {{Address{0x82923788u}, Address{0x82340ED4u}}}, 1, {{Address{0x829CD324u}}}},
  {"sub_8262A900", __imp__sub_8262A900, sub_8262A900, 1, {{Address{0x82000B44u}}}, 1, {{Address{0, &GuestContext::r5, 0}}}},
  {"sub_8262AEB8", __imp__sub_8262AEB8, sub_8262AEB8, 1, {{Address{0x8234EB44u}}}, 1, {{Address{0, &GuestContext::r5, 0}}}},
  {"sub_82703638", __imp__sub_82703638, sub_82703638, 1, {{Address{0, &GuestContext::r3, 72}}}, 0, {{}}},
  {"sub_82418DA8", __imp__sub_82418DA8, sub_82418DA8, 1, {{Address{0x82000B48u}}}, 4, {{Address{0, &GuestContext::r3, 12}, Address{0, &GuestContext::r3, 8}, Address{0, &GuestContext::r3, 4}, Address{0, &GuestContext::r3, 0}}}},
  {"sub_829004D8", __imp__sub_829004D8, sub_829004D8, 2, {{Address{0x829B2134u}, Address{0x820024FCu}}}, 1, {{Address{0x829B2144u}}}},
  {"sub_82900C50", __imp__sub_82900C50, sub_82900C50, 2, {{Address{0x82914020u}, Address{0x82018DA4u}}}, 1, {{Address{0x829CCC34u}}}},
  {"sub_82900E50", __imp__sub_82900E50, sub_82900E50, 1, {{Address{0x8291416Cu}}}, 1, {{Address{0x829CCC80u}}}},
  {"sub_82900F88", __imp__sub_82900F88, sub_82900F88, 2, {{Address{0x82000AE4u}, Address{0x82012ED0u}}}, 1, {{Address{0x829CCCB8u}}}},
  {"sub_82901418", __imp__sub_82901418, sub_82901418, 1, {{Address{0x82920AB4u}}}, 1, {{Address{0x829CCE00u}}}},
  {"sub_829017D8", __imp__sub_829017D8, sub_829017D8, 1, {{Address{0x82000AE8u}}}, 1, {{Address{0x829CCE20u}}}},
  {"sub_82901990", __imp__sub_82901990, sub_82901990, 2, {{Address{0x8292137Cu}, Address{0x82087CF4u}}}, 1, {{Address{0x829CCE60u}}}},
  {"sub_82901CC8", __imp__sub_82901CC8, sub_82901CC8, 2, {{Address{0x829CCEC0u}, Address{0x820024FCu}}}, 1, {{Address{0x829CCED8u}}}},
  {"sub_82902600", __imp__sub_82902600, sub_82902600, 2, {{Address{0x82921704u}, Address{0x820894BCu}}}, 1, {{Address{0x829CD0F0u}}}},
  {"sub_82902900", __imp__sub_82902900, sub_82902900, 2, {{Address{0x82921BD4u}, Address{0x8232C524u}}}, 1, {{Address{0x829CD150u}}}},
  {"sub_82902C40", __imp__sub_82902C40, sub_82902C40, 2, {{Address{0x82922330u}, Address{0x823338DCu}}}, 1, {{Address{0x829CD1B8u}}}},
  {"sub_82902FF8", __imp__sub_82902FF8, sub_82902FF8, 2, {{Address{0x82922AC0u}, Address{0x82338A04u}}}, 1, {{Address{0x829CD230u}}}},
  {"sub_82903398", __imp__sub_82903398, sub_82903398, 2, {{Address{0x82923144u}, Address{0x8233DF24u}}}, 1, {{Address{0x829CD2A4u}}}},
  {"sub_82903808", __imp__sub_82903808, sub_82903808, 2, {{Address{0x82923748u}, Address{0x82340E04u}}}, 1, {{Address{0x829CD31Cu}}}},
  {"sub_8262A600", __imp__sub_8262A600, sub_8262A600, 1, {{Address{0x823764A4u}}}, 1, {{Address{0, &GuestContext::r5, 0}}}},
  {"sub_82901C08", __imp__sub_82901C08, sub_82901C08, 2, {{Address{0x8292147Cu}, Address{0x82012ED0u}}}, 1, {{Address{0x829CCEC0u}}}},
  {"sub_829022B8", __imp__sub_829022B8, sub_829022B8, 2, {{Address{0x829214DCu}, Address{0x82088FDCu}}}, 1, {{Address{0x829CD0A4u}}}},
  {"sub_829027C0", __imp__sub_829027C0, sub_829027C0, 2, {{Address{0x829218D4u}, Address{0x82328804u}}}, 1, {{Address{0x829CD128u}}}},
  {"sub_82902AE0", __imp__sub_82902AE0, sub_82902AE0, 2, {{Address{0x82922074u}, Address{0x82330B04u}}}, 1, {{Address{0x829CD18Cu}}}},
  {"sub_82902E40", __imp__sub_82902E40, sub_82902E40, 2, {{Address{0x829226B4u}, Address{0x82336764u}}}, 1, {{Address{0x829CD204u}}}},
  {"sub_82903238", __imp__sub_82903238, sub_82903238, 2, {{Address{0x82922F14u}, Address{0x8233CE6Cu}}}, 1, {{Address{0x829CD278u}}}},
  {"sub_82903670", __imp__sub_82903670, sub_82903670, 2, {{Address{0x829234F0u}, Address{0x8233F900u}}}, 1, {{Address{0x829CD2F4u}}}},
  {"sub_825422B0", __imp__sub_825422B0, sub_825422B0, 1, {{Address{0x82016F24u}}}, 0, {{}}},
  {"sub_82455A28", __imp__sub_82455A28, sub_82455A28, 1, {{Address{0, &GuestContext::r3, 1000}}}, 0, {{}}},
  {"sub_82540070", __imp__sub_82540070, sub_82540070, 1, {{Address{0x8201D9D8u}}}, 0, {{}}},
  {"sub_829005B8", __imp__sub_829005B8, sub_829005B8, 2, {{Address{0x82913C7Cu}, Address{0x82000B44u}}}, 1, {{Address{0x829B2164u}}}},
  {"sub_82900A98", __imp__sub_82900A98, sub_82900A98, 2, {{Address{0x82913E18u}, Address{0x82018378u}}}, 1, {{Address{0x829CC940u}}}},
  {"sub_82900C90", __imp__sub_82900C90, sub_82900C90, 2, {{Address{0x8291403Cu}, Address{0x82018DC8u}}}, 1, {{Address{0x829CCC3Cu}}}},
  {"sub_82900E88", __imp__sub_82900E88, sub_82900E88, 2, {{Address{0x82914260u}, Address{0x82000E5Cu}}}, 1, {{Address{0x829CCC88u}}}},
  {"sub_82901970", __imp__sub_82901970, sub_82901970, 2, {{Address{0x8291088Cu}, Address{0x82018650u}}}, 1, {{Address{0x829CCE5Cu}}}},
  {"sub_82901CA8", __imp__sub_82901CA8, sub_82901CA8, 2, {{Address{0x829CCEC4u}, Address{0x8201238Cu}}}, 1, {{Address{0x829CCED4u}}}},
  {"sub_829025E0", __imp__sub_829025E0, sub_829025E0, 2, {{Address{0x82921704u}, Address{0x820894BCu}}}, 1, {{Address{0x829CD0ECu}}}},
  {"sub_829028E0", __imp__sub_829028E0, sub_829028E0, 2, {{Address{0x82921BD4u}, Address{0x8232C524u}}}, 1, {{Address{0x829CD14Cu}}}},
  {"sub_82902C20", __imp__sub_82902C20, sub_82902C20, 2, {{Address{0x82922330u}, Address{0x823338DCu}}}, 1, {{Address{0x829CD1B4u}}}},
  {"sub_82902FD8", __imp__sub_82902FD8, sub_82902FD8, 2, {{Address{0x82922A14u}, Address{0x82338454u}}}, 1, {{Address{0x829CD22Cu}}}},
  {"sub_82903378", __imp__sub_82903378, sub_82903378, 2, {{Address{0x82923144u}, Address{0x8233DF24u}}}, 1, {{Address{0x829CD2A0u}}}},
  {"sub_829037E8", __imp__sub_829037E8, sub_829037E8, 2, {{Address{0x82923748u}, Address{0x82340E04u}}}, 1, {{Address{0x829CD318u}}}},
  {"sub_8262A708", __imp__sub_8262A708, sub_8262A708, 1, {{Address{0x823764ACu}}}, 1, {{Address{0, &GuestContext::r5, 0}}}},
  {"sub_8263F1D0", __imp__sub_8263F1D0, sub_8263F1D0, 1, {{Address{0, &GuestContext::r3, 68}}}, 1, {{Address{0, &GuestContext::r4, 0}}}},
  {"sub_82495160", __imp__sub_82495160, sub_82495160, 1, {{Address{0x82000DACu}}}, 0, {{}}},
  {"sub_8262AE88", __imp__sub_8262AE88, sub_8262AE88, 1, {{Address{0x8234EB38u}}}, 1, {{Address{0, &GuestContext::r5, 0}}}},
  {"sub_828ED9C0", __imp__sub_828ED9C0, sub_828ED9C0, 2, {{Address{0x829125C0u}, Address{0x82000E5Cu}}}, 1, {{Address{0x829AB3E4u}}}},
  {"sub_82901A20", __imp__sub_82901A20, sub_82901A20, 2, {{Address{0x8291088Cu}, Address{0x82013260u}}}, 1, {{Address{0x829CCE74u}}}},
  {"sub_82901D20", __imp__sub_82901D20, sub_82901D20, 1, {{Address{0x82921498u}}}, 1, {{Address{0x829CCEE4u}}}},
  {"sub_829025C0", __imp__sub_829025C0, sub_829025C0, 2, {{Address{0x829216E8u}, Address{0x82089434u}}}, 1, {{Address{0x829CD0E8u}}}},
  {"sub_829028C0", __imp__sub_829028C0, sub_829028C0, 2, {{Address{0x82921B38u}, Address{0x8232B184u}}}, 1, {{Address{0x829CD148u}}}},
  {"sub_82902C00", __imp__sub_82902C00, sub_82902C00, 2, {{Address{0x829222B4u}, Address{0x8233305Cu}}}, 1, {{Address{0x829CD1B0u}}}},
  {"sub_82902FB8", __imp__sub_82902FB8, sub_82902FB8, 2, {{Address{0x82922A14u}, Address{0x82338454u}}}, 1, {{Address{0x829CD228u}}}},
  {"sub_82903358", __imp__sub_82903358, sub_82903358, 2, {{Address{0x829230C8u}, Address{0x8233DAE4u}}}, 1, {{Address{0x829CD29Cu}}}},
  {"sub_828ED808", __imp__sub_828ED808, sub_828ED808, 2, {{Address{0x8291203Cu}, Address{0x8200DE20u}}}, 1, {{Address{0x829AAEE4u}}}},
  {"sub_828EDA40", __imp__sub_828EDA40, sub_828EDA40, 2, {{Address{0x829126FCu}, Address{0x820121A0u}}}, 1, {{Address{0x829AB3F4u}}}},
  {"sub_82901890", __imp__sub_82901890, sub_82901890, 2, {{Address{0x82921364u}, Address{0x82087040u}}}, 1, {{Address{0x829CCE3Cu}}}},
  {"sub_82901C28", __imp__sub_82901C28, sub_82901C28, 2, {{Address{0x82921480u}, Address{0x82016F20u}}}, 1, {{Address{0x829CCEC4u}}}},
  {"sub_829022D8", __imp__sub_829022D8, sub_829022D8, 2, {{Address{0x82921538u}, Address{0x820890CCu}}}, 1, {{Address{0x829CD0A8u}}}},
  {"sub_829027E0", __imp__sub_829027E0, sub_829027E0, 2, {{Address{0x82921950u}, Address{0x82328CF4u}}}, 1, {{Address{0x829CD12Cu}}}},
  {"sub_82902B00", __imp__sub_82902B00, sub_82902B00, 2, {{Address{0x82922074u}, Address{0x82330B04u}}}, 1, {{Address{0x829CD190u}}}},
  {"sub_82902E60", __imp__sub_82902E60, sub_82902E60, 2, {{Address{0x82922760u}, Address{0x823370BCu}}}, 1, {{Address{0x829CD208u}}}},
  {"sub_82903258", __imp__sub_82903258, sub_82903258, 2, {{Address{0x82922F14u}, Address{0x8233CE6Cu}}}, 1, {{Address{0x829CD27Cu}}}},
  {"sub_82903690", __imp__sub_82903690, sub_82903690, 2, {{Address{0x82923574u}, Address{0x8233FD2Cu}}}, 1, {{Address{0x829CD2F8u}}}},
  {"sub_8242A2F0", __imp__sub_8242A2F0, sub_8242A2F0, 0, {{}}, 4, {{Address{0, &GuestContext::r3, 0}, Address{0, &GuestContext::r3, 4}, Address{0, &GuestContext::r3, 8}, Address{0, &GuestContext::r3, 12}}}},
  {"sub_8244AA80", __imp__sub_8244AA80, sub_8244AA80, 1, {{Address{0, &GuestContext::r3, 980}}}, 0, {{}}},
  {"sub_82901950", __imp__sub_82901950, sub_82901950, 2, {{Address{0x82000AECu}, Address{0x82018650u}}}, 1, {{Address{0x829CCE58u}}}},
  {"sub_82901C88", __imp__sub_82901C88, sub_82901C88, 2, {{Address{0x829CCEC0u}, Address{0x820024FCu}}}, 1, {{Address{0x829CCED0u}}}},
  {"sub_829025A0", __imp__sub_829025A0, sub_829025A0, 2, {{Address{0x829216E8u}, Address{0x82089434u}}}, 1, {{Address{0x829CD0E4u}}}},
  {"sub_829028A0", __imp__sub_829028A0, sub_829028A0, 2, {{Address{0x82921B38u}, Address{0x8232B184u}}}, 1, {{Address{0x829CD144u}}}},
  {"sub_82902BE0", __imp__sub_82902BE0, sub_82902BE0, 2, {{Address{0x829222B4u}, Address{0x8233305Cu}}}, 1, {{Address{0x829CD1ACu}}}},
  {"sub_82902F98", __imp__sub_82902F98, sub_82902F98, 2, {{Address{0x82922958u}, Address{0x82337C38u}}}, 1, {{Address{0x829CD224u}}}},
  {"sub_82903338", __imp__sub_82903338, sub_82903338, 2, {{Address{0x829230C8u}, Address{0x8233DAE4u}}}, 1, {{Address{0x829CD298u}}}},
  {"sub_82903770", __imp__sub_82903770, sub_82903770, 2, {{Address{0x82923664u}, Address{0x823401E4u}}}, 1, {{Address{0x829CD314u}}}},
  {"sub_828ED948", __imp__sub_828ED948, sub_828ED948, 2, {{Address{0x829124A4u}, Address{0x82000E5Cu}}}, 1, {{Address{0x829AB3D4u}}}},
  {"sub_82900458", __imp__sub_82900458, sub_82900458, 2, {{Address{0x82913C6Cu}, Address{0x82012ED0u}}}, 1, {{Address{0x829B2134u}}}},
  {"sub_829005F8", __imp__sub_829005F8, sub_829005F8, 1, {{Address{0x82913C94u}}}, 1, {{Address{0x829B216Cu}}}},
  {"sub_82901AC0", __imp__sub_82901AC0, sub_82901AC0, 2, {{Address{0x829213FCu}, Address{0x82088000u}}}, 1, {{Address{0x829CCE88u}}}},
  {"sub_82901DA8", __imp__sub_82901DA8, sub_82901DA8, 1, {{Address{0x829214A4u}}}, 1, {{Address{0x829CCEF8u}}}},
  {"sub_829026A0", __imp__sub_829026A0, sub_829026A0, 2, {{Address{0x82921754u}, Address{0x82089850u}}}, 1, {{Address{0x829CD104u}}}},
  {"sub_829029A0", __imp__sub_829029A0, sub_829029A0, 2, {{Address{0x82921DA8u}, Address{0x8232DF9Cu}}}, 1, {{Address{0x829CD164u}}}},
  {"sub_82902CE0", __imp__sub_82902CE0, sub_82902CE0, 2, {{Address{0x82922408u}, Address{0x82333FE4u}}}, 1, {{Address{0x829CD1D8u}}}},
  {"sub_82903098", __imp__sub_82903098, sub_82903098, 2, {{Address{0x82922BE8u}, Address{0x82339F7Cu}}}, 1, {{Address{0x829CD244u}}}},
  {"sub_828ED968", __imp__sub_828ED968, sub_828ED968, 2, {{Address{0x829124F0u}, Address{0x82011668u}}}, 1, {{Address{0x829AB3D8u}}}},
  {"sub_82495170", __imp__sub_82495170, sub_82495170, 1, {{Address{0x82000B48u}}}, 0, {{}}},
  {"sub_824A69C0", __imp__sub_824A69C0, sub_824A69C0, 0, {{}}, 1, {{Address{0, &GuestContext::r3, 132}}}},
  {"sub_828ED878", __imp__sub_828ED878, sub_828ED878, 1, {{Address{0x829120D4u}}}, 1, {{Address{0x829AB3B8u}}}},
  {"sub_828EDA00", __imp__sub_828EDA00, sub_828EDA00, 2, {{Address{0x829126B4u}, Address{0x82000E5Cu}}}, 1, {{Address{0x829AB3ECu}}}},
  {"sub_82900478", __imp__sub_82900478, sub_82900478, 2, {{Address{0x82913C70u}, Address{0x82016F20u}}}, 1, {{Address{0x829B2138u}}}},
  {"sub_82900610", __imp__sub_82900610, sub_82900610, 1, {{Address{0x82913C98u}}}, 1, {{Address{0x829B2170u}}}},
  {"sub_82900CF0", __imp__sub_82900CF0, sub_82900CF0, 2, {{Address{0x82914058u}, Address{0x82018E18u}}}, 1, {{Address{0x829CCC48u}}}},
  {"sub_82900EE8", __imp__sub_82900EE8, sub_82900EE8, 2, {{Address{0x829142B0u}, Address{0x820196F8u}}}, 1, {{Address{0x829CCC94u}}}},
  {"sub_82901A80", __imp__sub_82901A80, sub_82901A80, 2, {{Address{0x82000AECu}, Address{0x82013260u}}}, 1, {{Address{0x829CCE80u}}}},
  {"sub_82901D68", __imp__sub_82901D68, sub_82901D68, 2, {{Address{0x8292148Cu}, Address{0x82000B44u}}}, 1, {{Address{0x829CCEF0u}}}},
  {"sub_82902720", __imp__sub_82902720, sub_82902720, 2, {{Address{0x8292178Cu}, Address{0x82327CF0u}}}, 1, {{Address{0x829CD114u}}}},
  {"sub_82902A40", __imp__sub_82902A40, sub_82902A40, 2, {{Address{0x82921ED0u}, Address{0x8232F24Cu}}}, 1, {{Address{0x829CD178u}}}},
  {"sub_82902DA0", __imp__sub_82902DA0, sub_82902DA0, 2, {{Address{0x829225BCu}, Address{0x82334E2Cu}}}, 1, {{Address{0x829CD1F0u}}}},
  {"sub_82903178", __imp__sub_82903178, sub_82903178, 2, {{Address{0x82922DE0u}, Address{0x8233C714u}}}, 1, {{Address{0x829CD260u}}}},
  {"sub_829035B0", __imp__sub_829035B0, sub_829035B0, 2, {{Address{0x82923400u}, Address{0x8233F424u}}}, 1, {{Address{0x829CD2DCu}}}},
  {"sub_82461748", __imp__sub_82461748, sub_82461748, 1, {{Address{0, &GuestContext::r3, 1012}}}, 0, {{}}},
  {"sub_8253CC18", __imp__sub_8253CC18, sub_8253CC18, 1, {{Address{0x82000DA4u}}}, 0, {{}}},
  {"sub_82900538", __imp__sub_82900538, sub_82900538, 2, {{Address{0x829B2138u}, Address{0x8200DE00u}}}, 1, {{Address{0x829B2150u}}}},
  {"sub_82900778", __imp__sub_82900778, sub_82900778, 2, {{Address{0x82913CFCu}, Address{0x820179FCu}}}, 1, {{Address{0x829CBFE4u}}}},
  {"sub_82900B38", __imp__sub_82900B38, sub_82900B38, 2, {{Address{0x82913E38u}, Address{0x82018BACu}}}, 1, {{Address{0x829CC9A8u}}}},
  {"sub_82900D30", __imp__sub_82900D30, sub_82900D30, 2, {{Address{0x82914074u}, Address{0x82018E24u}}}, 1, {{Address{0x829CCC50u}}}},
  {"sub_82900F28", __imp__sub_82900F28, sub_82900F28, 2, {{Address{0x82914314u}, Address{0x82019700u}}}, 1, {{Address{0x829CCC9Cu}}}},
  {"sub_829018E0", __imp__sub_829018E0, sub_829018E0, 2, {{Address{0x82000AECu}, Address{0x82018650u}}}, 1, {{Address{0x829CCE48u}}}},
  {"sub_82901C48", __imp__sub_82901C48, sub_82901C48, 2, {{Address{0x829CCEC0u}, Address{0x820024FCu}}}, 1, {{Address{0x829CCEC8u}}}},
  {"sub_829022F8", __imp__sub_829022F8, sub_829022F8, 2, {{Address{0x82921538u}, Address{0x820890CCu}}}, 1, {{Address{0x829CD0ACu}}}},
  {"sub_82902800", __imp__sub_82902800, sub_82902800, 2, {{Address{0x82921950u}, Address{0x82328CF4u}}}, 1, {{Address{0x829CD130u}}}},
  {"sub_82902B20", __imp__sub_82902B20, sub_82902B20, 2, {{Address{0x82922110u}, Address{0x82330DACu}}}, 1, {{Address{0x829CD194u}}}},
  {"sub_82902E80", __imp__sub_82902E80, sub_82902E80, 2, {{Address{0x82922760u}, Address{0x823370BCu}}}, 1, {{Address{0x829CD20Cu}}}},
  {"sub_82903278", __imp__sub_82903278, sub_82903278, 2, {{Address{0x82922F9Cu}, Address{0x8233D1F4u}}}, 1, {{Address{0x829CD280u}}}},
  {"sub_829036B0", __imp__sub_829036B0, sub_829036B0, 2, {{Address{0x82923574u}, Address{0x8233FD2Cu}}}, 1, {{Address{0x829CD2FCu}}}},
  {"sub_82703620", __imp__sub_82703620, sub_82703620, 0, {{}}, 1, {{Address{0, &GuestContext::r3, 72}}}},
  {"sub_829003F8", __imp__sub_829003F8, sub_829003F8, 2, {{Address{0x82913C44u}, Address{0x82016EB0u}}}, 1, {{Address{0x829B2128u}}}},
  {"sub_82900518", __imp__sub_82900518, sub_82900518, 2, {{Address{0x829B2134u}, Address{0x820024FCu}}}, 1, {{Address{0x829B214Cu}}}},
  {"sub_82900758", __imp__sub_82900758, sub_82900758, 2, {{Address{0x82913CFCu}, Address{0x820179FCu}}}, 1, {{Address{0x829CBFE0u}}}},
  {"sub_82900C10", __imp__sub_82900C10, sub_82900C10, 2, {{Address{0x82914004u}, Address{0x82018D7Cu}}}, 1, {{Address{0x829CCC2Cu}}}},
  {"sub_82900E10", __imp__sub_82900E10, sub_82900E10, 2, {{Address{0x82914100u}, Address{0x82018F30u}}}, 1, {{Address{0x829CCC78u}}}},
  {"sub_82901028", __imp__sub_82901028, sub_82901028, 2, {{Address{0x82914B14u}, Address{0x8201DFFCu}}}, 1, {{Address{0x829CCCCCu}}}},
  {"sub_82901780", __imp__sub_82901780, sub_82901780, 2, {{Address{0x829211B0u}, Address{0x82086778u}}}, 1, {{Address{0x829CCE14u}}}},
  {"sub_829018C8", __imp__sub_829018C8, sub_829018C8, 1, {{Address{0x82000AE8u}}}, 1, {{Address{0x829CCE44u}}}},
  {"sub_82901A60", __imp__sub_82901A60, sub_82901A60, 2, {{Address{0x82921394u}, Address{0x82087F60u}}}, 1, {{Address{0x829CCE7Cu}}}},
  {"sub_82901D50", __imp__sub_82901D50, sub_82901D50, 1, {{Address{0x829214A0u}}}, 1, {{Address{0x829CCEECu}}}},
  {"sub_82902680", __imp__sub_82902680, sub_82902680, 2, {{Address{0x8292173Cu}, Address{0x820896CCu}}}, 1, {{Address{0x829CD100u}}}},
  {"sub_82902980", __imp__sub_82902980, sub_82902980, 2, {{Address{0x82921D2Cu}, Address{0x8232DA44u}}}, 1, {{Address{0x829CD160u}}}},
  {"sub_82902CC0", __imp__sub_82902CC0, sub_82902CC0, 2, {{Address{0x829223C8u}, Address{0x82333F18u}}}, 1, {{Address{0x829CD1D4u}}}},
  {"sub_82903078", __imp__sub_82903078, sub_82903078, 2, {{Address{0x82922BE8u}, Address{0x82339F7Cu}}}, 1, {{Address{0x829CD240u}}}},
  {"sub_82903418", __imp__sub_82903418, sub_82903418, 2, {{Address{0x8292320Cu}, Address{0x8233E384u}}}, 1, {{Address{0x829CD2B4u}}}},
  {"sub_829038E0", __imp__sub_829038E0, sub_829038E0, 2, {{Address{0x8292384Cu}, Address{0x82341084u}}}, 1, {{Address{0x829CD32Cu}}}},
  {"sub_82902F00", __imp__sub_82902F00, sub_82902F00, 2, {{Address{0x82922898u}, Address{0x823379E4u}}}, 1, {{Address{0x829CD21Cu}}}},
  {"sub_829032F8", __imp__sub_829032F8, sub_829032F8, 2, {{Address{0x8292301Cu}, Address{0x8233D37Cu}}}, 1, {{Address{0x829CD290u}}}},
  {"sub_82903730", __imp__sub_82903730, sub_82903730, 2, {{Address{0x82923624u}, Address{0x8234011Cu}}}, 1, {{Address{0x829CD30Cu}}}},
  {"sub_8262AE70", __imp__sub_8262AE70, sub_8262AE70, 1, {{Address{0x82374994u}}}, 1, {{Address{0, &GuestContext::r5, 0}}}},
  {"sub_828ED8D0", __imp__sub_828ED8D0, sub_828ED8D0, 2, {{Address{0x829121CCu}, Address{0x82000E5Cu}}}, 1, {{Address{0x829AB3C4u}}}},
  {"sub_829005A0", __imp__sub_829005A0, sub_829005A0, 1, {{Address{0x82913C90u}}}, 1, {{Address{0x829B2160u}}}},
  {"sub_82900718", __imp__sub_82900718, sub_82900718, 2, {{Address{0x82913CC4u}, Address{0x82016F30u}}}, 1, {{Address{0x829B245Cu}}}},
  {"sub_82900BB0", __imp__sub_82900BB0, sub_82900BB0, 2, {{Address{0x82913F9Cu}, Address{0x82000E5Cu}}}, 1, {{Address{0x829CC9B8u}}}},
  {"sub_82900DB0", __imp__sub_82900DB0, sub_82900DB0, 2, {{Address{0x829140B0u}, Address{0x82018EC0u}}}, 1, {{Address{0x829CCC60u}}}},
  {"sub_82900FC8", __imp__sub_82900FC8, sub_82900FC8, 2, {{Address{0x82914520u}, Address{0x8201D998u}}}, 1, {{Address{0x829CCCC0u}}}},
  {"sub_82901450", __imp__sub_82901450, sub_82901450, 2, {{Address{0x82920BA8u}, Address{0x82000E5Cu}}}, 1, {{Address{0x829CCE08u}}}},
  {"sub_82901870", __imp__sub_82901870, sub_82901870, 2, {{Address{0x82921364u}, Address{0x82087040u}}}, 1, {{Address{0x829CCE38u}}}},
  {"sub_82901BA8", __imp__sub_82901BA8, sub_82901BA8, 2, {{Address{0x82921458u}, Address{0x820882B8u}}}, 1, {{Address{0x829CCEB4u}}}},
  {"sub_829004B8", __imp__sub_829004B8, sub_829004B8, 2, {{Address{0x829B2138u}, Address{0x820146D0u}}}, 1, {{Address{0x829B2140u}}}},
  {"sub_82900680", __imp__sub_82900680, sub_82900680, 3, {{Address{0x82913C60u}, Address{0x82016F24u}, Address{0x829B2178u}}}, 1, {{Address{0x829B2180u}}}},
  {"sub_82900B58", __imp__sub_82900B58, sub_82900B58, 1, {{Address{0x82913EA4u}}}, 1, {{Address{0x829CC9ACu}}}},
  {"sub_82900CD0", __imp__sub_82900CD0, sub_82900CD0, 2, {{Address{0x82914058u}, Address{0x82018E18u}}}, 1, {{Address{0x829CCC44u}}}},
  {"sub_82900EC8", __imp__sub_82900EC8, sub_82900EC8, 2, {{Address{0x829142B0u}, Address{0x820196F8u}}}, 1, {{Address{0x829CCC90u}}}},
  {"sub_82901A00", __imp__sub_82901A00, sub_82901A00, 2, {{Address{0x82000AECu}, Address{0x82018650u}}}, 1, {{Address{0x829CCE70u}}}},
  {"sub_82901D08", __imp__sub_82901D08, sub_82901D08, 1, {{Address{0x82921494u}}}, 1, {{Address{0x829CCEE0u}}}},
  {"sub_82902338", __imp__sub_82902338, sub_82902338, 2, {{Address{0x8292155Cu}, Address{0x82089200u}}}, 1, {{Address{0x829CD0E0u}}}},
  {"sub_82902880", __imp__sub_82902880, sub_82902880, 2, {{Address{0x82921AACu}, Address{0x8232AFECu}}}, 1, {{Address{0x829CD140u}}}},
  {"sub_82902BC0", __imp__sub_82902BC0, sub_82902BC0, 2, {{Address{0x82922238u}, Address{0x823321E4u}}}, 1, {{Address{0x829CD1A8u}}}},
  {"sub_82902F78", __imp__sub_82902F78, sub_82902F78, 2, {{Address{0x82922958u}, Address{0x82337C38u}}}, 1, {{Address{0x829CD220u}}}},
  {"sub_82903318", __imp__sub_82903318, sub_82903318, 2, {{Address{0x8292301Cu}, Address{0x8233D37Cu}}}, 1, {{Address{0x829CD294u}}}},
  {"sub_82903750", __imp__sub_82903750, sub_82903750, 2, {{Address{0x82923664u}, Address{0x823401E4u}}}, 1, {{Address{0x829CD310u}}}},
  {"sub_82902BA0", __imp__sub_82902BA0, sub_82902BA0, 2, {{Address{0x82922238u}, Address{0x823321E4u}}}, 1, {{Address{0x829CD1A4u}}}},
  {"sub_82903920", __imp__sub_82903920, sub_82903920, 2, {{Address{0x8292388Cu}, Address{0x82341148u}}}, 1, {{Address{0x829CD334u}}}},
  {"sub_824633D8", __imp__sub_824633D8, sub_824633D8, 1, {{Address{0, &GuestContext::r3, 1140}}}, 0, {{}}},
  {"sub_828ED890", __imp__sub_828ED890, sub_828ED890, 2, {{Address{0x829120D8u}, Address{0x82000E5Cu}}}, 1, {{Address{0x829AB3BCu}}}},
  {"sub_82903590", __imp__sub_82903590, sub_82903590, 2, {{Address{0x82923400u}, Address{0x8233F424u}}}, 1, {{Address{0x829CD2D8u}}}},
  {"sub_82418D50", __imp__sub_82418D50, sub_82418D50, 0, {{}}, 2, {{Address{0, &GuestContext::r3, 0}, Address{0, &GuestContext::r3, 4}}}},
  {"sub_82900438", __imp__sub_82900438, sub_82900438, 2, {{Address{0x82913C68u}, Address{0x820183ACu}}}, 1, {{Address{0x829B2130u}}}},
  {"sub_82900570", __imp__sub_82900570, sub_82900570, 1, {{Address{0x82913C88u}}}, 1, {{Address{0x829B2158u}}}},
  {"sub_82900668", __imp__sub_82900668, sub_82900668, 1, {{Address{0x82913C94u}}}, 1, {{Address{0x829B217Cu}}}},
  {"sub_82900A78", __imp__sub_82900A78, sub_82900A78, 2, {{Address{0x82913E18u}, Address{0x82018378u}}}, 1, {{Address{0x829CC93Cu}}}},
  {"sub_82900C70", __imp__sub_82900C70, sub_82900C70, 2, {{Address{0x82914020u}, Address{0x82018DA4u}}}, 1, {{Address{0x829CCC38u}}}},
  {"sub_82900E68", __imp__sub_82900E68, sub_82900E68, 2, {{Address{0x82914170u}, Address{0x82000E5Cu}}}, 1, {{Address{0x829CCC84u}}}},
  {"sub_82901920", __imp__sub_82901920, sub_82901920, 1, {{Address{0x82000AE4u}}}, 1, {{Address{0x829CCE50u}}}},
  {"sub_82901B40", __imp__sub_82901B40, sub_82901B40, 2, {{Address{0x82921430u}, Address{0x82088034u}}}, 1, {{Address{0x829CCE9Cu}}}},
  {"sub_82901E30", __imp__sub_82901E30, sub_82901E30, 3, {{Address{0x82921470u}, Address{0x82016F24u}, Address{0x829CCF04u}}}, 1, {{Address{0x829CCF0Cu}}}},
  {"sub_82902860", __imp__sub_82902860, sub_82902860, 2, {{Address{0x82921AACu}, Address{0x8232AFECu}}}, 1, {{Address{0x829CD13Cu}}}},
  {"sub_82902B80", __imp__sub_82902B80, sub_82902B80, 2, {{Address{0x8292219Cu}, Address{0x823313BCu}}}, 1, {{Address{0x829CD1A0u}}}},
  {"sub_82902EE0", __imp__sub_82902EE0, sub_82902EE0, 2, {{Address{0x82922898u}, Address{0x823379E4u}}}, 1, {{Address{0x829CD218u}}}},
  {"sub_829032D8", __imp__sub_829032D8, sub_829032D8, 2, {{Address{0x82922FDCu}, Address{0x8233D2BCu}}}, 1, {{Address{0x829CD28Cu}}}},
  {"sub_82903710", __imp__sub_82903710, sub_82903710, 2, {{Address{0x82923624u}, Address{0x8234011Cu}}}, 1, {{Address{0x829CD308u}}}},
  {"sub_828ED8F0", __imp__sub_828ED8F0, sub_828ED8F0, 1, {{Address{0x829123ACu}}}, 1, {{Address{0x829AB3C8u}}}},
  {"sub_828EDA20", __imp__sub_828EDA20, sub_828EDA20, 2, {{Address{0x829126FCu}, Address{0x820121A0u}}}, 1, {{Address{0x829AB3F0u}}}},
  {"sub_82468DF8", __imp__sub_82468DF8, sub_82468DF8, 1, {{Address{0x82000B44u}}}, 0, {{}}},
  {"sub_82547720", __imp__sub_82547720, sub_82547720, 1, {{Address{0x82017458u}}}, 0, {{}}},
  {"sub_82900588", __imp__sub_82900588, sub_82900588, 1, {{Address{0x82913C8Cu}}}, 1, {{Address{0x829B215Cu}}}},
  {"sub_82900D50", __imp__sub_82900D50, sub_82900D50, 2, {{Address{0x82914090u}, Address{0x82018E3Cu}}}, 1, {{Address{0x829CCC54u}}}},
  {"sub_82900F48", __imp__sub_82900F48, sub_82900F48, 2, {{Address{0x82914504u}, Address{0x8201A308u}}}, 1, {{Address{0x829CCCB0u}}}},
  {"sub_829013D8", __imp__sub_829013D8, sub_829013D8, 2, {{Address{0x82920A44u}, Address{0x820856C4u}}}, 1, {{Address{0x829CCDF8u}}}},
  {"sub_829017F0", __imp__sub_829017F0, sub_829017F0, 2, {{Address{0x82000AECu}, Address{0x82018650u}}}, 1, {{Address{0x829CCE24u}}}},
  {"sub_82901AA0", __imp__sub_82901AA0, sub_82901AA0, 2, {{Address{0x829213FCu}, Address{0x82088000u}}}, 1, {{Address{0x829CCE84u}}}},
  {"sub_82901D88", __imp__sub_82901D88, sub_82901D88, 2, {{Address{0x82921490u}, Address{0x82000B44u}}}, 1, {{Address{0x829CCEF4u}}}},
  {"sub_82902740", __imp__sub_82902740, sub_82902740, 2, {{Address{0x8292178Cu}, Address{0x82327CF0u}}}, 1, {{Address{0x829CD118u}}}},
  {"sub_82902A60", __imp__sub_82902A60, sub_82902A60, 2, {{Address{0x82921F4Cu}, Address{0x8232F55Cu}}}, 1, {{Address{0x829CD17Cu}}}},
  {"sub_82902DC0", __imp__sub_82902DC0, sub_82902DC0, 2, {{Address{0x829225BCu}, Address{0x82334E2Cu}}}, 1, {{Address{0x829CD1F4u}}}},
  {"sub_82903198", __imp__sub_82903198, sub_82903198, 2, {{Address{0x82922DE0u}, Address{0x8233C714u}}}, 1, {{Address{0x829CD264u}}}},
  {"sub_829035D0", __imp__sub_829035D0, sub_829035D0, 2, {{Address{0x82923440u}, Address{0x8233F508u}}}, 1, {{Address{0x829CD2E0u}}}},
  {"sub_829005D8", __imp__sub_829005D8, sub_829005D8, 2, {{Address{0x82913C80u}, Address{0x82000B44u}}}, 1, {{Address{0x829B2168u}}}},
  {"sub_82900B90", __imp__sub_82900B90, sub_82900B90, 2, {{Address{0x82913F98u}, Address{0x82000E5Cu}}}, 1, {{Address{0x829CC9B4u}}}},
  {"sub_82900D90", __imp__sub_82900D90, sub_82900D90, 2, {{Address{0x829140B0u}, Address{0x82018EC0u}}}, 1, {{Address{0x829CCC5Cu}}}},
  {"sub_82900FA8", __imp__sub_82900FA8, sub_82900FA8, 2, {{Address{0x82000AECu}, Address{0x82012ED0u}}}, 1, {{Address{0x829CCCBCu}}}},
  {"sub_82901430", __imp__sub_82901430, sub_82901430, 2, {{Address{0x82920AB8u}, Address{0x82000E5Cu}}}, 1, {{Address{0x829CCE04u}}}},
  {"sub_82901850", __imp__sub_82901850, sub_82901850, 2, {{Address{0x829211C8u}, Address{0x82086D84u}}}, 1, {{Address{0x829CCE30u}}}},
  {"sub_82901B88", __imp__sub_82901B88, sub_82901B88, 2, {{Address{0x82921458u}, Address{0x820882B8u}}}, 1, {{Address{0x829CCEB0u}}}},
  {"sub_82901E58", __imp__sub_82901E58, sub_82901E58, 2, {{Address{0x829214C0u}, Address{0x82018BACu}}}, 1, {{Address{0x829CCF10u}}}},
  {"sub_82902760", __imp__sub_82902760, sub_82902760, 2, {{Address{0x82921818u}, Address{0x823280D4u}}}, 1, {{Address{0x829CD11Cu}}}},
  {"sub_82902A80", __imp__sub_82902A80, sub_82902A80, 2, {{Address{0x82921F4Cu}, Address{0x8232F55Cu}}}, 1, {{Address{0x829CD180u}}}},
  {"sub_82902DE0", __imp__sub_82902DE0, sub_82902DE0, 2, {{Address{0x82922638u}, Address{0x8233609Cu}}}, 1, {{Address{0x829CD1F8u}}}},
  {"sub_829031B8", __imp__sub_829031B8, sub_829031B8, 2, {{Address{0x82922E64u}, Address{0x8233CA9Cu}}}, 1, {{Address{0x829CD268u}}}},
  {"sub_829035F0", __imp__sub_829035F0, sub_829035F0, 2, {{Address{0x82923440u}, Address{0x8233F508u}}}, 1, {{Address{0x829CD2E4u}}}},
  {"sub_829003D8", __imp__sub_829003D8, sub_829003D8, 2, {{Address{0x82913C44u}, Address{0x82016EB0u}}}, 1, {{Address{0x829B2124u}}}},
  {"sub_829004F8", __imp__sub_829004F8, sub_829004F8, 2, {{Address{0x829B2138u}, Address{0x8201238Cu}}}, 1, {{Address{0x829B2148u}}}},
  {"sub_82900738", __imp__sub_82900738, sub_82900738, 2, {{Address{0x82913CC4u}, Address{0x82016F30u}}}, 1, {{Address{0x829B2460u}}}},
  {"sub_82900BF0", __imp__sub_82900BF0, sub_82900BF0, 2, {{Address{0x82913FE8u}, Address{0x82018D60u}}}, 1, {{Address{0x829CCC28u}}}},
  {"sub_82900DF0", __imp__sub_82900DF0, sub_82900DF0, 2, {{Address{0x829140CCu}, Address{0x82018ED0u}}}, 1, {{Address{0x829CCC68u}}}},
  {"sub_82901008", __imp__sub_82901008, sub_82901008, 2, {{Address{0x82914B14u}, Address{0x8201DFFCu}}}, 1, {{Address{0x829CCCC8u}}}},
};

GuestFunction* original_function;
GuestFunction* native_function;
uint32_t original_host_csr;
int original_exceptions;
const char* current_name;

void Original(GuestContext& ctx, uint8_t* base) {
  ctx.fpscr.setcsr(ctx.fpscr.csr);
  std::feclearexcept(FE_ALL_EXCEPT);
  original_function(ctx, base);
  original_host_csr = ctx.fpscr.getcsr();
  original_exceptions = std::fetestexcept(FE_ALL_EXCEPT);
}

void Native(GuestContext& ctx, uint8_t* base) {
  ctx.fpscr.setcsr(ctx.fpscr.csr);
  std::feclearexcept(FE_ALL_EXCEPT);
  native_function(ctx, base);
  const uint32_t native_host_csr = ctx.fpscr.getcsr();
  if (native_host_csr != original_host_csr ||
      std::fetestexcept(FE_ALL_EXCEPT) != original_exceptions) {
    std::fprintf(stderr, "%s: host CSR %08x / %08x, exceptions %x / %x\n",
                 current_name, original_host_csr, native_host_csr,
                 original_exceptions, std::fetestexcept(FE_ALL_EXCEPT));
    std::exit(1);
  }
}

struct Inputs { uint32_t a, b, c; };
constexpr Inputs kEdges[] = {
  {0, 0, 0}, {0, 0x80000000, 0x80000000},
  {0x80000000, 0, 0}, {0x80000000, 0x80000000, 0x80000000},
  {0x3F800000, 0x3F800000, 0xBF800000},
  {0x3F800000, 0xBF800000, 0x3F800000},
  {0xBF800000, 0x3F800000, 0xBF800000},
  {0xBF800000, 0xBF800000, 0x3F800000},
  {1, 1, 0x80000001}, {1, 0x80000001, 1},
  {0x80000001, 0x80000001, 0x80000001},
  {0x007FFFFF, 1, 0x00800000}, {0x00800000, 0x007FFFFF, 0x80000001},
  {0x00800000, 0x3F000000, 0x80000001},
  {0x80800000, 0x3F000000, 1},
  {0x7F7FFFFF, 0x40000000, 0xFF7FFFFF},
  {0xFF7FFFFF, 0x40000000, 0x7F7FFFFF},
  {0x7F7FFFFF, 0x7F7FFFFF, 0},
  {0x3F800000, 0x33800000, 0},
  {0x3F800001, 0x33800000, 0xBF800000},
  {0xBF800000, 0xB3800000, 0},
  {0x3F800001, 0x3F7FFFFF, 0xBF800000},
  {0x7F800000, 0xFF800000, 0},
  {0xFF800000, 0x7F800000, 0x80000000},
  {0x7FC12345, 0xFFC54321, 0x7FC23456},
  {0x7FA12345, 0x3F800000, 0x7FC23456},
  {0x3F800000, 0xFFA12345, 0x7FC23456},
  {0x3F800000, 0x3F800000, 0x7FA12345},
  {0x3EAAAAAB, 0x3F555555, 0xBEAAAAAB},
  {0x3F000001, 0x3EFFFFFF, 0xBE800000},
  {0x00800001, 0x3F000000, 0x80000001},
  {0x80000001, 0x3F000000, 1},
  {0x3F800000, 1, 0}, {0xBF800000, 0x80000001, 0},
  {0x00800000, 0x3F000001, 0}, {0x7F7FFFFF, 1, 0},
};

constexpr uint64_t kDoubleInputs[] = {
  0, 0x8000000000000000ull,
  0x3FF0000000000000ull, 0xBFF0000000000000ull,
  0x3FF0000010000000ull, 0x3FF000000FFFFFFFull, 0x3FF0000010000001ull,
  0xBFF0000010000000ull,
  0x3690000000000000ull, 0xB690000000000000ull,
  0x36A0000000000000ull, 1,
  0x47F0000000000000ull, 0xC7F0000000000000ull,
  0x7FEFFFFFFFFFFFFFull, 0xFFEFFFFFFFFFFFFFull,
  0x7FF0000000000000ull, 0xFFF0000000000000ull,
  0x7FF8123456789ABCull, 0xFFF8FEDCBA987654ull,
  0x7FF0123456789ABCull, 0xFFF0123456789ABCull,
};

void CheckCase(NativeHelpersTest& test, const Query& query, Inputs values,
               uint32_t rounding, uint32_t seed, unsigned placement) {
  auto context = test.Context(seed);
  uint32_t object = NativeHelpersTest::kObject;
  if (placement == 3) object = 0xA0100000u;
  if (placement == 4) object = 0xE0100000u;
  if (placement == 5) object = 0x7F001000u;
  if (placement == 6) object = 0xFFFFFFF0u;
  context.r3.u64 = 0x1234567800000000ull | object;
  context.r4.u64 = 0x8765432100000000ull | (object + 4096u);
  context.r5.u64 = 0xFEDCBA9800000000ull | (object + 8192u);
  if (placement == 1 || placement == 2)
    context.r4.u32 = object + (placement == 1 ? 68u : 69u);
  if (placement == 7 && query.load_count)
    context.r5.u32 = query.loads[0].Get(context);
  constexpr size_t count = std::size(kDoubleInputs);
  context.f1.u64 = kDoubleInputs[(seed + 0) % count];
  context.f2.u64 = kDoubleInputs[(seed + 1) % count];
  context.f3.u64 = kDoubleInputs[(seed + 2) % count];
  context.f4.u64 = kDoubleInputs[(seed + 3) % count];
  std::feclearexcept(FE_ALL_EXCEPT);
  context.fpscr.InitHost();
  context.fpscr.storeFromGuest(rounding);
  if (seed & 1) context.fpscr.enableFlushMode();
  else context.fpscr.disableFlushMode();

  std::array<NativeTestRegion, 7> observed{};
  for (unsigned i = 0; i < query.load_count; ++i)
    observed[i] = {query.loads[i].Get(context) - 4u, 12};
  for (unsigned i = 0; i < query.store_count; ++i)
    observed[3 + i] = {query.stores[i].Get(context) - 4u, 12};
  for (const auto& region : observed)
    if (region.size) test.Fill(region.address, region.size, seed);
  const uint32_t bits[] = {values.a, values.b, values.c};
  for (unsigned i = 0; i < query.load_count; ++i)
    test.Store32(query.loads[i].Get(context), bits[i]);

  current_name = query.name;
  original_function = query.original;
  native_function = query.native;
  test.Check(query.name, Original, Native, context,
             {observed[0], observed[1], observed[2], observed[3],
              observed[4], observed[5], observed[6]});
}
}  // namespace

int main() {
  PPCFPSCRRegister host;
  const uint32_t saved_host = host.getcsr();
  NativeHelpersTest test;
  std::mt19937 random(0xF10A7E17u);
  uint32_t seed = 0;
  for (const auto& query : kQueries) {
    for (uint32_t rounding = 0; rounding < 4; ++rounding) {
      for (const auto& edge : kEdges)
        CheckCase(test, query, edge, rounding, seed++, 0);
      for (unsigned i = 0; i < 8; ++i) {
        Inputs values{uint32_t(random()), uint32_t(random()), uint32_t(random())};
        CheckCase(test, query, values, rounding, seed++, 0);
      }
      // Dynamic addresses cover overlapping fields, aliases, MMIO and wrapping.
      bool dynamic = false;
      for (const auto& address : query.loads) dynamic |= address.reg != nullptr;
      for (const auto& address : query.stores) dynamic |= address.reg != nullptr;
      if (dynamic)
        for (unsigned placement = 1; placement <= 7; ++placement)
          CheckCase(test, query, {0x3F800001u, 0x3F7FFFFFu, 0xBF800000u},
                    rounding, seed++, placement);
    }
  }
  test.Report("345 floating leaf helpers");
  host.setcsr(saved_host);
}
