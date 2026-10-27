#include "../src/guest_context.h"
#include "../src/guest_dispatch.h"

#include <cassert>
#include <cstdint>
#include <string>

namespace {
void AddOne(GuestContext& context, uint8_t*) { ++context.r3.u32; }

void SetMarker(GuestContext& context, uint8_t*) { context.r4.u32 = 0xC0DEC0DEu; }

struct RuntimeContext {
  uint32_t marker = 0;
};

void RuntimeSetMarker(RuntimeContext& context, uint8_t*) { context.marker = 0xA11CEu; }

struct Registrar {
  int calls = 0;
  bool SetFunction(uint32_t, guest::Function<GuestContext>) {
    ++calls;
    return true;
  }
};

struct RuntimeRegistrar {
  int calls = 0;
  bool SetFunction(uint32_t, guest::Function<RuntimeContext>) {
    ++calls;
    return true;
  }
};
}  // namespace

int main() {
  auto& registry = guest::FunctionRegistry<GuestContext>::Instance();
  Registrar registrar;
  assert(guest::RegisterFunction(&registrar, 0x3000, AddOne));
  assert(registrar.calls == 1);
  RuntimeRegistrar runtime_registrar;
  assert(guest::RegisterFunction(&runtime_registrar, 0x3004, SetMarker,
                                 RuntimeSetMarker));
  assert(runtime_registrar.calls == 1);
  assert(registry.Lookup(0x3004) == SetMarker);
  GuestContext adapted_context{};
  registry.Dispatch(0x3004, adapted_context, nullptr);
  assert(adapted_context.r4.u32 == 0xC0DEC0DEu);
  assert(guest::RegisterFunction<GuestContext>(0x1000, AddOne));
  assert(guest::RegisterFunction<GuestContext>(0x2000, SetMarker));
  // Small PPC thunk entry points are registered exactly like other generated
  // guest functions, even though their bodies only branch elsewhere.
  assert(guest::RegisterFunction<GuestContext>(0x82420000, AddOne));
  // Import stubs are ordinary guest addresses in the table.
  assert(guest::RegisterFunction<GuestContext>(0x82905D6C, SetMarker));
  assert(registry.Lookup(0x1000) == AddOne);

  GuestContext context{};
  registry.Dispatch(0x1000, context, nullptr);
  assert(context.r3.u32 == 1);

  guest::DispatchIndirect(context, nullptr, 0x2000,
                          [](uint32_t) -> guest::Function<GuestContext> { return nullptr; });
  assert(context.r4.u32 == 0xC0DEC0DEu);

  // A generated thunk is dispatched from the project-owned table.
  context.r3.u32 = 40;
  guest::DispatchIndirect(context, nullptr, 0x82420000, nullptr);
  assert(context.r3.u32 == 41);

  context.r4.u32 = 0;
  guest::DispatchIndirect(context, nullptr, 0x82905D6C, nullptr);
  assert(context.r4.u32 == 0xC0DEC0DEu);

  bool trapped = false;
  context.lr = 0x12345678u;
  context.ctr.u32 = 0x23456789u;
  context.r1.u32 = 0x3456789Au;
  context.r3.u32 = 0x456789ABu;
  context.r4.u32 = 0x56789ABCu;
  try {
    guest::DispatchIndirect(context, nullptr, 0xDEADBEAF, nullptr);
  } catch (const guest::InvalidFunctionTrap& error) {
    trapped = error.address() == 0xDEADBEAF &&
              std::string(error.what()) ==
                  "Call to invalid or unregistered function at guest address 0xDEADBEAF "
                  "(lr=0x12345678, ctr=0x23456789, r1=0x3456789A, r3=0x456789AB, "
                  "r4=0x56789ABC)";
  }
  assert(trapped);
  assert(context.last_indirect_target == 0xDEADBEAF);
  return 0;
}
