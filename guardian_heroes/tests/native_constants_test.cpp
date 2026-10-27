#include "native_helpers_test.h"

REX_EXTERN(sub_82495B30);
REX_EXTERN(__imp__sub_82495B30);
REX_EXTERN(sub_82495B50);
REX_EXTERN(__imp__sub_82495B50);
REX_EXTERN(sub_82495B00);
REX_EXTERN(__imp__sub_82495B00);
REX_EXTERN(sub_82495B70);
REX_EXTERN(__imp__sub_82495B70);
REX_EXTERN(sub_82495AE0);
REX_EXTERN(__imp__sub_82495AE0);
REX_EXTERN(sub_82410120);
REX_EXTERN(__imp__sub_82410120);
REX_EXTERN(sub_82495AF0);
REX_EXTERN(__imp__sub_82495AF0);
REX_EXTERN(sub_82495B80);
REX_EXTERN(__imp__sub_82495B80);
REX_EXTERN(sub_82495B90);
REX_EXTERN(__imp__sub_82495B90);
REX_EXTERN(sub_824118C8);
REX_EXTERN(__imp__sub_824118C8);
REX_EXTERN(sub_82495B40);
REX_EXTERN(__imp__sub_82495B40);
REX_EXTERN(sub_82495B10);
REX_EXTERN(__imp__sub_82495B10);
REX_EXTERN(sub_8288C3B8);
REX_EXTERN(__imp__sub_8288C3B8);
REX_EXTERN(sub_82410130);
REX_EXTERN(__imp__sub_82410130);
REX_EXTERN(sub_82495B60);
REX_EXTERN(__imp__sub_82495B60);
REX_EXTERN(sub_82495B20);
REX_EXTERN(__imp__sub_82495B20);
REX_EXTERN(sub_8248A6B0);
REX_EXTERN(__imp__sub_8248A6B0);
REX_EXTERN(sub_8265CBD8);
REX_EXTERN(__imp__sub_8265CBD8);
REX_EXTERN(sub_82435B88);
REX_EXTERN(__imp__sub_82435B88);
REX_EXTERN(sub_826E9910);
REX_EXTERN(__imp__sub_826E9910);
REX_EXTERN(sub_8256D9B8);
REX_EXTERN(__imp__sub_8256D9B8);
REX_EXTERN(sub_8256D9D8);
REX_EXTERN(__imp__sub_8256D9D8);
REX_EXTERN(sub_8256D9E8);
REX_EXTERN(__imp__sub_8256D9E8);
REX_EXTERN(sub_8256D9F8);
REX_EXTERN(__imp__sub_8256D9F8);
REX_EXTERN(sub_8256DA08);
REX_EXTERN(__imp__sub_8256DA08);
REX_EXTERN(sub_8256DA18);
REX_EXTERN(__imp__sub_8256DA18);
REX_EXTERN(sub_8256DA28);
REX_EXTERN(__imp__sub_8256DA28);
REX_EXTERN(sub_8256DA38);
REX_EXTERN(__imp__sub_8256DA38);
REX_EXTERN(sub_8256DA48);
REX_EXTERN(__imp__sub_8256DA48);
REX_EXTERN(sub_82570C08);
REX_EXTERN(__imp__sub_82570C08);
REX_EXTERN(sub_82570C28);
REX_EXTERN(__imp__sub_82570C28);
REX_EXTERN(sub_82570C38);
REX_EXTERN(__imp__sub_82570C38);
REX_EXTERN(sub_82570C48);
REX_EXTERN(__imp__sub_82570C48);
REX_EXTERN(sub_82570C58);
REX_EXTERN(__imp__sub_82570C58);
REX_EXTERN(sub_82570C68);
REX_EXTERN(__imp__sub_82570C68);
REX_EXTERN(sub_82570C78);
REX_EXTERN(__imp__sub_82570C78);
REX_EXTERN(sub_82570C88);
REX_EXTERN(__imp__sub_82570C88);
REX_EXTERN(sub_82570C98);
REX_EXTERN(__imp__sub_82570C98);
REX_EXTERN(sub_825721C8);
REX_EXTERN(__imp__sub_825721C8);
REX_EXTERN(sub_825721D8);
REX_EXTERN(__imp__sub_825721D8);
REX_EXTERN(sub_825721E8);
REX_EXTERN(__imp__sub_825721E8);
REX_EXTERN(sub_825721F8);
REX_EXTERN(__imp__sub_825721F8);
REX_EXTERN(sub_82572208);
REX_EXTERN(__imp__sub_82572208);
REX_EXTERN(sub_82572218);
REX_EXTERN(__imp__sub_82572218);

struct ConstantQuery { const char* name; GuestFunction* original; GuestFunction* native; };

int main() {
  NativeHelpersTest test;
  const ConstantQuery queries[] = {
    {"sub_82495B30", __imp__sub_82495B30, sub_82495B30},
    {"sub_82495B50", __imp__sub_82495B50, sub_82495B50},
    {"sub_82495B00", __imp__sub_82495B00, sub_82495B00},
    {"sub_82495B70", __imp__sub_82495B70, sub_82495B70},
    {"sub_82495AE0", __imp__sub_82495AE0, sub_82495AE0},
    {"sub_82410120", __imp__sub_82410120, sub_82410120},
    {"sub_82495AF0", __imp__sub_82495AF0, sub_82495AF0},
    {"sub_82495B80", __imp__sub_82495B80, sub_82495B80},
    {"sub_82495B90", __imp__sub_82495B90, sub_82495B90},
    {"sub_824118C8", __imp__sub_824118C8, sub_824118C8},
    {"sub_82495B40", __imp__sub_82495B40, sub_82495B40},
    {"sub_82495B10", __imp__sub_82495B10, sub_82495B10},
    {"sub_8288C3B8", __imp__sub_8288C3B8, sub_8288C3B8},
    {"sub_82410130", __imp__sub_82410130, sub_82410130},
    {"sub_82495B60", __imp__sub_82495B60, sub_82495B60},
    {"sub_82495B20", __imp__sub_82495B20, sub_82495B20},
    {"sub_8248A6B0", __imp__sub_8248A6B0, sub_8248A6B0},
    {"sub_8265CBD8", __imp__sub_8265CBD8, sub_8265CBD8},
    {"sub_82435B88", __imp__sub_82435B88, sub_82435B88},
    {"sub_826E9910", __imp__sub_826E9910, sub_826E9910},
    {"sub_8256D9B8", __imp__sub_8256D9B8, sub_8256D9B8},
    {"sub_8256D9D8", __imp__sub_8256D9D8, sub_8256D9D8},
    {"sub_8256D9E8", __imp__sub_8256D9E8, sub_8256D9E8},
    {"sub_8256D9F8", __imp__sub_8256D9F8, sub_8256D9F8},
    {"sub_8256DA08", __imp__sub_8256DA08, sub_8256DA08},
    {"sub_8256DA18", __imp__sub_8256DA18, sub_8256DA18},
    {"sub_8256DA28", __imp__sub_8256DA28, sub_8256DA28},
    {"sub_8256DA38", __imp__sub_8256DA38, sub_8256DA38},
    {"sub_8256DA48", __imp__sub_8256DA48, sub_8256DA48},
    {"sub_82570C08", __imp__sub_82570C08, sub_82570C08},
    {"sub_82570C28", __imp__sub_82570C28, sub_82570C28},
    {"sub_82570C38", __imp__sub_82570C38, sub_82570C38},
    {"sub_82570C48", __imp__sub_82570C48, sub_82570C48},
    {"sub_82570C58", __imp__sub_82570C58, sub_82570C58},
    {"sub_82570C68", __imp__sub_82570C68, sub_82570C68},
    {"sub_82570C78", __imp__sub_82570C78, sub_82570C78},
    {"sub_82570C88", __imp__sub_82570C88, sub_82570C88},
    {"sub_82570C98", __imp__sub_82570C98, sub_82570C98},
    {"sub_825721C8", __imp__sub_825721C8, sub_825721C8},
    {"sub_825721D8", __imp__sub_825721D8, sub_825721D8},
    {"sub_825721E8", __imp__sub_825721E8, sub_825721E8},
    {"sub_825721F8", __imp__sub_825721F8, sub_825721F8},
    {"sub_82572208", __imp__sub_82572208, sub_82572208},
    {"sub_82572218", __imp__sub_82572218, sub_82572218},
  };
  uint32_t seed = 0;
  for (const auto& query : queries) {
    for (uint32_t sample = 0; sample < 8; ++sample) {
      auto ctx = test.Context(seed++);
      ctx.r3.u64 = uint64_t(0x9ABCD123u + sample) << 32 | (sample * 0x3210987u);
      ctx.r11.u64 = ~ctx.r3.u64;
      test.Check(query.name, query.original, query.native, ctx, {});
    }
  }
  test.Report("native constant queries (44 functions)");
}
