#include "guardian_heroes_init.h"
#include "gh_guest_continuation_title.h"

#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>

namespace cont = gh::guest::continuation;

static void CheckParity(const cont::Program& program, uint32_t address,
                        cont::Host original, bool tail) {
  GuestContext initial{};
  initial.fpscr.csr = GuestFPSCRRegister::getHostCSR();
  initial.r1.u64 = 60000;
  initial.r3.u64 = 64;
  initial.r4.u64 = 123;
  initial.r5.u64 = 2;
  initial.lr = 0x12345678;
  auto expected = initial;
  alignas(32) std::array<uint8_t, 65536> memory{};
  alignas(32) std::array<uint8_t, 65536> expected_memory{};
  original(expected, expected_memory.data());
  expected.fpscr.csr = GuestFPSCRRegister::getHostCSR();

  auto machine = std::make_unique<cont::Machine>(program);
  machine->context = initial;
  machine->Start(address);
  size_t snapshots = 0;
  bool saw_tail = false;
  for (;;) {
    const auto result = machine->Run(memory.data(), 1);
    assert(result.status == cont::Status::Paused || result.status == cont::Status::Complete);
    if (result.status == cont::Status::Complete) break;
    saw_tail |= machine->frames().size() == 1 && machine->frames().back().function != address;
    const auto image = machine->Save();
    machine.reset();
    machine = std::make_unique<cont::Machine>(program);
    machine->Restore(image);
    assert(machine->Save() == image);
    ++snapshots;
    assert(snapshots < 100);
  }
  assert(snapshots > 0 && (!tail || saw_tail));
  assert(memory == expected_memory);
  assert(std::memcmp(&machine->context, &expected, sizeof(expected)) == 0);
}

int main() {
  const auto program = cont::CreateTitleContinuationProgram();
  size_t compiled = 0, atomic = 0, queued = 0, unsupported = 0;
  for (const auto* mapping = PPCFuncMappings; mapping->host; ++mapping) {
    const auto* entry = program.Find(static_cast<uint32_t>(mapping->guest));
    assert(entry);
    compiled += entry->step != nullptr;
    atomic += entry->host != nullptr;
    queued += entry->begin != nullptr;
    unsupported += !entry->unsupported.empty();
  }
  assert(compiled > 0 && atomic > 0 && queued == 5 && unsupported == 0);
  assert(cont::TitleProgram() == nullptr);
  CheckParity(program, 0x826E0908, &sub_826E0908, false);
  CheckParity(program, 0x8242C988, &sub_8242C988, true);
  std::cout << compiled << " compiled, " << atomic << " atomic, " << queued << " queued, " << unsupported
            << " unsupported; full registry link, real title leaf/tail parity and frame restore passed\n";
}
