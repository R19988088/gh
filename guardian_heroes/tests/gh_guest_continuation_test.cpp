#include "gh_guest_continuation.h"
#include "gh_guest_continuation_title.h"
#include "gh_guest_continuation_support.h"
#include "original_continuation_fixture.h"
#include <array>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>

namespace cont = gh::guest::continuation;
namespace gh::guest::continuation { void RegisterGeneratedContinuation(Program&); }
void sub_8243AAE8(GuestContext& ctx, uint8_t*) { ctx.r9.u64 = ctx.r3.u64 ^ 0xDEAD; }
void DispatchFixtureIndirect(GuestContext& ctx, uint8_t* memory, uint32_t address) {
  if (address != 0x2000) throw std::invalid_argument("unsupported fixture target");
  sub_00002000(ctx, memory);
}
static cont::Machine* active;
static unsigned host_calls;
static unsigned title_safepoints;
static void TitleSafepoint() {
  auto* machine = cont::CurrentTitleExecution();
  assert(machine);
  machine->Restore(machine->Save());
  ++title_safepoints;
}
static void Hook(GuestContext& ctx, uint8_t*) {
  ++host_calls;
  auto* execution = cont::CurrentTitleExecution();
  auto* machine = execution ? execution : active;
  bool rejected = false;
  try { (void)machine->Save(); } catch (const std::logic_error&) { rejected = true; }
  assert(rejected);
  rejected = false;
  try { machine->Run(nullptr, 1); } catch (const std::logic_error&) { rejected = true; }
  assert(rejected);
  if (execution) {
    assert(cont::TitleProgram());
    assert(!cont::TryExecuteTitle(ctx, nullptr, 0x2000));
    assert(cont::CurrentTitleExecution() == execution);
  }
  ctx.r3.u64 = 777;
}
static cont::Transition MissingImport(GuestContext&, uint8_t*, cont::Frame&) {
  return cont::Transition::Call(0xDEAD, 0x3000);
}
static unsigned queued_begins;
class QueuedFixture final : public cont::PendingHost {
 public:
  bool Poll(GuestContext&, uint8_t*) override { return false; }
  std::vector<uint8_t> Save() const override { return {1}; }
  void WaitForWake(std::chrono::milliseconds) override {}
};
static std::unique_ptr<cont::PendingHost> BeginQueued(
    GuestContext&, uint8_t*, std::span<const uint8_t>) {
  ++queued_begins;
  return std::make_unique<QueuedFixture>();
}
static cont::Transition CallQueued(GuestContext&, uint8_t*, cont::Frame&) {
  return cont::Transition::Call(0x5000, 0x4000);
}
static cont::Transition ChoosePC(GuestContext& context, uint8_t*, cont::Frame&) {
  return cont::Transition::Next(context.r3.u32);
}
static void CheckPCMembership(std::vector<uint32_t> pcs,
                             std::initializer_list<uint32_t> accepted,
                             std::initializer_list<uint32_t> rejected) {
  const auto address = *std::min_element(pcs.begin(), pcs.end());
  cont::Program program;
  program.AddCompiled(address, std::move(pcs), ChoosePC);
  cont::Machine initial(program); initial.Start(address);
  const auto image = initial.Save();
  for (const auto pc : accepted) {
    cont::Machine machine(program); machine.Start(address); machine.context.r3.u32 = pc;
    const auto result = machine.Run(nullptr, 1);
    assert(result.status == cont::Status::Paused && result.steps == 1);
    assert(machine.frames().back().pc == pc);
    const auto saved = machine.Save();
    cont::Machine restored(program); restored.Restore(saved);
    assert(restored.Save() == saved && restored.frames().back().pc == pc);
    const auto zero = restored.Run(nullptr, 0);
    assert(zero.status == cont::Status::Paused && zero.steps == 0 && restored.Save() == saved);
  }
  for (const auto pc : rejected) {
    cont::Machine machine(program); machine.Start(address); machine.context.r3.u32 = pc;
    bool failed = false;
    try { machine.Run(nullptr, 1); } catch (const std::runtime_error&) { failed = true; }
    assert(failed);
    auto invalid = image;
    const auto pc_offset = 12 + sizeof(GuestContext) + 4;
    for (unsigned byte = 0; byte < 4; ++byte) invalid[pc_offset + byte] = uint8_t(pc >> (byte * 8));
    failed = false;
    try { machine.Restore(invalid); } catch (const std::invalid_argument&) { failed = true; }
    assert(failed);
  }
}
int main() {
  CheckPCMembership({0x10008, 0x10000, 0x1000C, 0x10004},
      {0x10000, 0x10004, 0x10008, 0x1000C},
      {0xFFFC, 0x10001, 0x10002, 0x10003, 0x10010});
  CheckPCMembership({0x20000, 0x20008, 0x20011}, {0x20000, 0x20008, 0x20011},
      {0x1FFFC, 0x20004, 0x2000C, 0x20010, 0x20015});
  CheckPCMembership({0x30000}, {0x30000}, {0x2FFFC, 0x30001, 0x30004});
  CheckPCMembership({0x40000, 0x40000, 0x40008}, {0x40000, 0x40008}, {0x40004});
  CheckPCMembership({0xFFFFFFF0, 0xFFFFFFF4, 0xFFFFFFF8, 0xFFFFFFFC},
      {0xFFFFFFF0, 0xFFFFFFFC}, {0, 0xFFFFFFEC, 0xFFFFFFFD});
  cont::Program program;
  cont::RegisterGeneratedContinuation(program);
  GuestContext initial{};
  initial.fpscr.csr = GuestFPSCRRegister::getHostCSR();
  initial.r1.u64 = 60000;
  initial.r3.u64 = 400;
  initial.r5.u64 = 81;
  std::array<uint8_t, 65536> expected_memory{}, memory{};
  auto expected = initial;
  sub_00001000(expected, expected_memory.data());
  auto machine = std::make_unique<cont::Machine>(program);
  machine->context = initial;
  machine->Start(0x1000);
  bool saw_deep_stack = false, saw_loop = false;
  unsigned snapshots = 0;
  while (true) {
    auto result = machine->Run(memory.data(), 17);
    assert(result.status == cont::Status::Complete || result.status == cont::Status::Paused);
    if (result.status == cont::Status::Complete) break;
    saw_deep_stack |= machine->frames().size() > 100;
    saw_loop |= machine->frames().size() == 1 && machine->frames().back().pc >= 0x100C;
    // Destroy the old machine after every pause. Guest memory is copied by the
    // caller at the same boundary, without retaining a native guest call stack.
    auto bytes = machine->Save();
    auto memory_copy = memory;
    machine.reset();
    machine = std::make_unique<cont::Machine>(program);
    machine->Restore(bytes);
    assert(machine->Save() == bytes);
    memory = memory_copy;
    ++snapshots;
  }
  assert(saw_deep_stack && saw_loop && snapshots > 100);
  assert(memory == expected_memory);
  assert(machine->context.r3.u64 == expected.r3.u64);
  assert(machine->context.r6.u64 == expected.r6.u64);
  assert(machine->context.r1.u64 == expected.r1.u64);
  assert(machine->context.lr == expected.lr);
  assert(ReadFixture32(memory.data(), 0) == 80200);
  assert(ReadFixture32(memory.data(), 4) == 3240);

  // Local register temporaries and effective addresses survive a guest call
  // and destruction of the entire native execution machine at every step.
  auto scratch_expected = initial;
  scratch_expected.r3.u64 = 5;
  auto scratch_memory = memory;
  sub_00005000(scratch_expected, scratch_memory.data());
  cont::Machine scratch(program);
  scratch.context = initial;
  scratch.context.r3.u64 = 5;
  scratch.Start(0x5000);
  while (scratch.Run(memory.data(), 1).status != cont::Status::Complete) {
    cont::Machine restored(program);
    restored.Restore(scratch.Save());
    scratch.Restore(restored.Save());
  }
  assert(memory == scratch_memory);
  assert(scratch.context.r9.u64 == scratch_expected.r9.u64);
  assert(scratch.context.r10.u64 == 99);
  for (unsigned index = 0; index < 2; ++index) {
    cont::Machine table(program);
    table.context = initial;
    table.context.r10.u64 = index;
    table.Start(0x6000);
    assert(table.Run(memory.data(), 20).status == cont::Status::Complete);
    assert(table.context.r3.u64 == (index ? 22 : 11));
    cont::Machine conditional(program);
    conditional.context = initial;
    conditional.context.r3.u64 = 5;
    conditional.context.ctr.u64 = 0x2000;
    conditional.context.cr0.eq = index;
    conditional.Start(0x7000);
    assert(conditional.Run(memory.data(), 1000).status == cont::Status::Complete);
    assert(conditional.context.r3.u64 == (index ? 15 : 42));
  }

  auto title_context = initial;
  assert(!cont::TryExecuteTitle(title_context, memory.data(), 0x1000));
  cont::InstallTitleProgram(&program);
  auto title_memory = std::array<uint8_t, 65536>{};
  assert(cont::TryExecuteTitle(title_context, title_memory.data(), 0x1000, TitleSafepoint));
  assert(!cont::CurrentTitleExecution() && title_safepoints > 1);
  assert(title_memory == expected_memory && title_context.r3.u64 == expected.r3.u64);
  cont::InstallTitleProgram(nullptr);

  // A real title-generated leaf uses its original PCs and memory operation.
  auto leaf_expected = initial;
  leaf_expected.r3.u64 = 64;
  auto leaf_memory = memory;
  sub_82421C70(leaf_expected, leaf_memory.data());
  cont::Machine leaf(program);
  leaf.context = initial;
  leaf.context.r3.u64 = 64;
  leaf.Start(0x82421C70);
  assert(leaf.Run(memory.data(), 1).status == cont::Status::Paused);
  assert(leaf.frames().back().pc == 0x82421C74);
  leaf.Restore(leaf.Save());
  assert(leaf.Run(memory.data(), 10).status == cont::Status::Complete);
  assert(memory == leaf_memory);
  assert(leaf.context.r10.u64 == leaf_expected.r10.u64);

  auto indirect_expected = initial;
  indirect_expected.ctr.u64 = 0x2000;
  auto indirect_memory = memory;
  sub_00004000(indirect_expected, indirect_memory.data());
  cont::Machine indirect(program);
  indirect.context = initial;
  indirect.context.ctr.u64 = 0x2000;
  indirect.Start(0x4000);
  assert(indirect.Run(memory.data(), 10000).status == cont::Status::Complete);
  assert(indirect.context.r3.u64 == indirect_expected.r3.u64);
  assert(indirect.context.lr == indirect_expected.lr);
  assert(memory == indirect_memory);

  // Exercise the actual title's tail-call wrapper with an atomic fixture
  // target. A tail call replaces its frame and preserves the caller's LR.
  program.AddAtomicHost(0x8243AAE8, sub_8243AAE8);
  auto tail_expected = initial;
  sub_82410108(tail_expected, memory.data());
  cont::Machine tail(program);
  tail.context = initial;
  tail.Start(0x82410108);
  assert(tail.Run(memory.data(), 5).status == cont::Status::Paused);
  assert(tail.frames().size() == 1 && tail.frames().back().function == 0x8243AAE8);
  tail.Restore(tail.Save());
  assert(tail.Run(memory.data(), 1).status == cont::Status::Complete);
  assert(tail.context.r9.u64 == tail_expected.r9.u64);
  assert(tail.context.lr == tail_expected.lr);

  // Hook dispatch is explicit and atomic; it must not silently run the
  // compiled original or expose a snapshot while its C++ frame is alive.
  program.AddAtomicHost(0x2000, Hook);
  cont::Machine hooked(program);
  active = &hooked;
  hooked.context = initial;
  hooked.Start(0x1000);
  assert(hooked.Run(memory.data(), 1000).status == cont::Status::Complete);
  assert(host_calls == 1 && hooked.context.r3.u64 == 777);
  cont::InstallTitleProgram(&program);
  auto hooked_title = initial;
  assert(cont::TryExecuteTitle(hooked_title, memory.data(), 0x1000));
  assert(host_calls == 2 && hooked_title.r3.u64 == 777);
  cont::InstallTitleProgram(nullptr);

  // A native hook can explicitly fall back to the original compiled body.
  // The entire host path must return before execution becomes serializable.
  program.AddAtomicHost(0x2000, sub_00002000);
  cont::Machine fallback(program);
  fallback.context = initial;
  fallback.Start(0x1000);
  auto fallback_memory = std::array<uint8_t, 65536>{};
  assert(fallback.Run(fallback_memory.data(), 1000).status == cont::Status::Complete);
  assert(fallback_memory == expected_memory);
  assert(fallback.context.r3.u64 == expected.r3.u64);

  program.AddCompiled(0x3000, {0x3000}, MissingImport);
  program.AddUnsupported(0xDEAD, "unsupported guest non-local control");
  cont::Machine missing(program);
  missing.Start(0x3000);
  const auto unsupported = missing.Run(memory.data(), 1);
  assert(unsupported.status == cont::Status::Unsupported);
  assert(unsupported.reason == "unsupported guest non-local control");
  bool rejected = false;
  try { missing.Save(); } catch (const std::logic_error&) { rejected = true; }
  assert(rejected);
  rejected = false;
  auto invalid = leaf.Save();
  invalid.pop_back();
  try { leaf.Restore(invalid); } catch (const std::invalid_argument&) { rejected = true; }
  assert(rejected);
  cont::Program queued_program;
  queued_program.AddCompiled(0x4000, {0x4000}, CallQueued);
  queued_program.AddQueuedHost(0x5000, BeginQueued);
  cont::Machine before_import(queued_program);
  before_import.Start(0x4000);
  assert(before_import.Run(nullptr, 1).status == cont::Status::Paused);
  cont::Machine restored_before_import(queued_program);
  restored_before_import.Restore(before_import.Save());
  restored_before_import.PreparePendingHost(nullptr, true);
  assert(queued_begins == 0);
  assert(restored_before_import.Run(nullptr, 1).status == cont::Status::Blocked);
  assert(queued_begins == 1);
  cont::Machine restored_pending(queued_program);
  restored_pending.Restore(restored_before_import.Save());
  restored_pending.PreparePendingHost(nullptr, true);
  assert(queued_begins == 2 && restored_pending.Save() == restored_before_import.Save());
  std::cout << "Guest PC dispatch, recursive and indirect frames, loop snapshots, real title leaf/tail parity, atomic hooks and unsupported-boundary rejection passed\n";
}
