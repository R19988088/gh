#include "gh_kernel.h"

#include <array>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <thread>

#include <rex/chrono/clock.h>
#include <rex/runtime.h>
#include <rex/stream.h>
#include <rex/system/kernel_state.h>
#include <rex/system/util/object_table.h>
#include <rex/system/xevent.h>
#include <rex/system/xfile.h>
#include <rex/system/xmutant.h>
#include <rex/system/xnotifylistener.h>
#include <rex/system/xsemaphore.h>
#include <rex/system/xthread.h>
#include <rex/system/xtimer.h>

using rex::X_STATUS;
using namespace rex::system;
using namespace std::chrono_literals;

namespace {
uint64_t poll = 0;

void TestObjectTableRelease() {
  struct Object : XObject {
    explicit Object(bool& destroyed) : XObject(nullptr, Type::Timer), destroyed(destroyed) {}
    ~Object() override {
      // Model a timer destructor draining an APC that needs the kernel lock.
      std::thread callback([] { auto lock = rex::thread::global_critical_region::AcquireDirect(); });
      callback.join();
      destroyed = true;
    }
    bool& destroyed;
  };
  for (int operation = 0; operation < 3; ++operation) {
    bool destroyed = false;
    util::ObjectTable table;
    auto object = make_object<Object>(destroyed);
    rex::X_HANDLE handle = 0;
    assert(table.AddHandle(object.get(), &handle) == X_STATUS_SUCCESS);
    object.reset();
    if (operation == 0) {
      assert(table.ReleaseHandle(handle) == X_STATUS_SUCCESS);
    } else if (operation == 1) {
      table.Reset();
    } else {
      table.PurgeAllObjects();
    }
    assert(destroyed);
    assert(!table.LookupObject<XObject>(handle));
  }
}

void TestObjectTableLifecycle() {
  class NamedObject final : public XObject {
   public:
    explicit NamedObject(const char* name, bool host = false) : XObject(nullptr, Type::Event) {
      // Supply the name normally loaded from guest attributes without a runtime fixture.
      const_cast<std::string&>(this->name()) = name;
      set_host_object(host);
    }
  };
  util::ObjectTable table;
  auto guest = make_object<NamedObject>("guest");
  auto host = make_object<NamedObject>("host", true);
  rex::X_HANDLE guest_handle = 0, host_handle = 0, duplicate = 0;
  assert(table.AddHandle(guest.get(), &guest_handle) == X_STATUS_SUCCESS);
  assert(table.AddNameMapping("guest", guest_handle) == X_STATUS_SUCCESS);
  assert(table.DuplicateHandle(guest_handle, &duplicate) == X_STATUS_SUCCESS);
  assert(table.ReleaseHandle(guest_handle) == X_STATUS_SUCCESS);
  assert(table.RetainHandle(guest_handle) == X_STATUS_INVALID_HANDLE);
  assert(table.ReleaseHandle(guest_handle) == X_STATUS_INVALID_HANDLE);
  assert(guest->handles().size() == 1 && guest->handles()[0] == duplicate);
  assert(table.AddNameMapping("GUEST", duplicate) == X_STATUS_OBJECT_NAME_COLLISION);
  rex::X_HANDLE found = 0;
  assert(table.GetObjectByName("GUEST", &found) == X_STATUS_SUCCESS && found == duplicate);
  assert(table.ReleaseHandle(found) == X_STATUS_SUCCESS);
  assert(table.LookupObject<XObject>(duplicate).get() == guest.get());
  assert(table.ReleaseHandle(duplicate) == X_STATUS_SUCCESS);
  assert(guest->handles().empty());
  assert(!table.LookupObject<XObject>(duplicate));
  assert(table.GetObjectByName("guest", &found) == X_STATUS_OBJECT_NAME_NOT_FOUND);
  assert(table.AddHandle(guest.get(), &duplicate) == X_STATUS_SUCCESS);
  assert(table.AddNameMapping("guest", duplicate) == X_STATUS_SUCCESS);
  assert(table.AddHandle(host.get(), &host_handle) == X_STATUS_SUCCESS);
  assert(table.AddNameMapping("host", host_handle) == X_STATUS_SUCCESS);
  table.PurgeAllObjects();
  assert(guest->handles().empty());
  assert(!table.LookupObject<XObject>(duplicate));
  assert(table.LookupObject<XObject>(host_handle).get() == host.get());
  assert(host->handles().size() == 1);
  assert(table.GetObjectByName("guest", &found) == X_STATUS_OBJECT_NAME_NOT_FOUND);
  assert(table.AddNameMapping("HOST", host_handle) == X_STATUS_OBJECT_NAME_COLLISION);
  assert(table.AddHandle(guest.get(), &guest_handle) == X_STATUS_SUCCESS);
  assert(table.AddNameMapping("guest", guest_handle) == X_STATUS_SUCCESS);
  table.Reset();
  assert(guest->handles().empty());
  assert(host->handles().empty());
  guest->RetainHandle();
  guest->ReleaseHandle();
  assert(table.GetObjectByName("guest", &found) == X_STATUS_OBJECT_NAME_NOT_FOUND);
  assert(table.GetObjectByName("host", &found) == X_STATUS_OBJECT_NAME_NOT_FOUND);
  assert(table.AddHandle(guest.get(), &guest_handle) == X_STATUS_SUCCESS);
  assert(table.AddNameMapping("guest", guest_handle) == X_STATUS_SUCCESS);
  assert(table.RetainHandle(guest_handle) == X_STATUS_SUCCESS);
  assert(table.RemoveHandle(guest_handle) == X_STATUS_SUCCESS);
  assert(guest->handles().empty());
  assert(!table.LookupObject<XObject>(guest_handle));
  assert(table.RetainHandle(guest_handle) == X_STATUS_INVALID_HANDLE);
  assert(table.ReleaseHandle(guest_handle) == X_STATUS_INVALID_HANDLE);
  table.Reset();
}

void InitializeEvent(XEvent& event, bool manual = false, bool signaled = false) {
  X_DISPATCH_HEADER header{};
  header.type = manual ? 0 : 1;
  header.signal_state = signaled ? 1 : 0;
  event.InitializeNative(nullptr, &header);
}

void InitializeSemaphore(XSemaphore& semaphore, uint32_t count, uint32_t maximum) {
  X_KSEMAPHORE native{};
  native.header.signal_state = count;
  native.limit = maximum;
  assert(semaphore.InitializeNative(&native, &native.header));
}

void TestEventState() {
  XEvent event(nullptr);
  InitializeEvent(event);
  uint32_t state = 99;
  assert(event.Set(0, false) == 0);
  assert(event.Set(0, false) == 1);
  event.Query(nullptr, &state);
  assert(state == 1);
  event.Query(nullptr, &state);
  assert(state == 1);
  assert(event.Wait(0, 0, 0, &poll) == X_STATUS_SUCCESS);
  assert(event.Wait(0, 0, 0, &poll) == X_STATUS_TIMEOUT);
  assert(event.Reset() == 0);
}

void TestMixedWaits() {
  XEvent event(nullptr);
  XSemaphore semaphore(nullptr);
  InitializeEvent(event, false, true);
  InitializeSemaphore(semaphore, 0, 1);
  XObject* objects[] = {&event, &semaphore};
  assert(XObject::WaitMultiple(2, objects, 0, 0, 0, 0, &poll) == X_STATUS_TIMEOUT);
  assert(event.Wait(0, 0, 0, &poll) == X_STATUS_SUCCESS);
  event.Set(0, false);
  int32_t previous = -1;
  assert(semaphore.ReleaseSemaphore(1, &previous));
  assert(previous == 0);
  assert(XObject::WaitMultiple(2, objects, 0, 0, 0, 0, &poll) == X_STATUS_SUCCESS);
  assert(semaphore.Wait(0, 0, 0, &poll) == X_STATUS_TIMEOUT);
  assert(semaphore.ReleaseSemaphore(1, nullptr));
  assert(XObject::WaitMultiple(2, objects, 1, 0, 0, 0, &poll) == 1);
  assert(XObject::WaitMultiple(0, objects, 0, 0, 0, 0, &poll) == X_STATUS_INVALID_PARAMETER);
  XObject* duplicates[] = {&event, &event};
  assert(XObject::WaitMultiple(2, duplicates, 0, 0, 0, 0, &poll) == X_STATUS_INVALID_PARAMETER);
}

void TestSignalAndWait() {
  XEvent event(nullptr);
  XSemaphore semaphore(nullptr);
  InitializeEvent(event);
  InitializeSemaphore(semaphore, 0, 1);
  assert(XObject::SignalAndWait(&event, &event, 0, 0, 0, &poll) == X_STATUS_SUCCESS);
  assert(event.Wait(0, 0, 0, &poll) == X_STATUS_TIMEOUT);
  assert(XObject::SignalAndWait(&semaphore, &event, 0, 0, 0, &poll) == X_STATUS_TIMEOUT);
  assert(XObject::SignalAndWait(&semaphore, &event, 0, 0, 0, &poll) ==
         X_STATUS_SEMAPHORE_LIMIT_EXCEEDED);
  assert(semaphore.Wait(0, 0, 0, &poll) == X_STATUS_SUCCESS);
}

void TestMutantOwnership() {
  gh::kernel::SetThreadLocalValue(0xF8000100);
  XMutant mutant(nullptr);
  mutant.Initialize(false);
  assert(mutant.ReleaseMutant(0, false, false) == X_STATUS_MUTANT_NOT_OWNED);
  assert(mutant.Wait(0, 0, 0, &poll) == X_STATUS_SUCCESS);
  assert(mutant.Wait(0, 0, 0, &poll) == X_STATUS_SUCCESS);
  assert(mutant.ReleaseMutant(0, false, false) == X_STATUS_SUCCESS);
  assert(mutant.ReleaseMutant(0, false, false) == X_STATUS_SUCCESS);
  gh::kernel::SetThreadLocalValue(0);
}

void TestTimer() {
  XTimer timer(nullptr);
  timer.Initialize(1);
  assert(timer.SetTimer(0, 0, 0, 0, false) == X_STATUS_SUCCESS);
  uint64_t timeout = static_cast<uint64_t>(int64_t(-20'000'000));
  assert(timer.Wait(0, 0, 0, &timeout) == X_STATUS_SUCCESS);
  assert(timer.Wait(0, 0, 0, &poll) == X_STATUS_TIMEOUT);
  assert(timer.Cancel() == X_STATUS_SUCCESS);
  assert(timer.SetTimer(-10'000'000, 0, 0, 0, false) == X_STATUS_SUCCESS);
  assert(timer.Cancel() == X_STATUS_SUCCESS);
  assert(timer.Wait(0, 0, 0, &poll) == X_STATUS_TIMEOUT);
}

// A zero-byte read exercises XFile's real completion path without guest RAM.
class EmptyFile final : public rex::filesystem::File {
 public:
  EmptyFile() : File(0, nullptr) {}
  void Destroy() override {}
  X_STATUS ReadSync(std::span<uint8_t>, size_t, size_t*) override {
    assert(false);
    return X_STATUS_UNSUCCESSFUL;
  }
  X_STATUS WriteSync(std::span<const uint8_t>, size_t, size_t*) override {
    assert(false);
    return X_STATUS_UNSUCCESSFUL;
  }
};

void TestFileCompletionWait() {
  EmptyFile storage;
  XFile file(nullptr, &storage, false);
  XEvent event(nullptr);
  InitializeEvent(event, false, true);
  uint32_t read = 99;
  assert(file.Read(0, 0, 0, &read, 0) == X_STATUS_SUCCESS);
  assert(read == 0);
  XObject* objects[] = {&event, &file};
  assert(XObject::WaitMultiple(2, objects, 0, 0, 0, 0, &poll) == X_STATUS_SUCCESS);
  assert(file.Wait(0, 0, 0, &poll) == X_STATUS_TIMEOUT);
}

// Exercise XThread's real completion event and wait adapter without guest execution.
class CompletionThread final : public XThread {
 public:
  CompletionThread() : XThread(nullptr) {}
  void Complete() { kernel_thread_event_->Set(); }
};

void TestThreadCompletionWait() {
  CompletionThread thread;
  assert(thread.Wait(0, 0, 0, &poll) == X_STATUS_TIMEOUT);
  std::thread completion([&] { thread.Complete(); });
  uint64_t timeout = static_cast<uint64_t>(int64_t(-20'000'000));
  assert(thread.Wait(0, 0, 0, &timeout) == X_STATUS_SUCCESS);
  completion.join();
  assert(thread.Wait(0, 0, 0, &poll) == X_STATUS_SUCCESS);
}

void TestThreadDelay() {
  CompletionThread thread;
  assert(thread.Delay(0, 0, 0) == X_STATUS_SUCCESS);
  assert(thread.Delay(0, 0, static_cast<uint64_t>(int64_t(-1))) == X_STATUS_SUCCESS);
  assert(thread.Delay(0, 0, static_cast<uint64_t>(int64_t(-10'000))) == X_STATUS_SUCCESS);
  assert(thread.Delay(0, 0, 1) == X_STATUS_SUCCESS);
  assert(thread.Delay(0, 0, rex::chrono::Clock::QueryGuestSystemTime() + 10'000) ==
         X_STATUS_SUCCESS);
  bool callback_ran = false;
  auto worker = gh::kernel::HostThread::Create({.create_suspended = true}, [&] {
    assert(thread.Delay(0, 1, static_cast<uint64_t>(int64_t(-10'000'000))) == X_STATUS_USER_APC);
  });
  assert(worker);
  worker->QueueUserCallback([&] { callback_ran = true; });
  assert(worker->Resume());
  worker->Join();
  assert(callback_ran);
}

void TestGuestThreadTitleTermination() {
  class TerminationThread final : public XThread {
   public:
    TerminationThread(KernelState* kernel, bool suspended, bool return_normally,
                      gh::kernel::Event& reached_delay)
        : XThread(kernel, 16 * 1024, 0, 0, 0, suspended ? 1 : 0, true),
          return_normally_(return_normally),
          reached_delay_(reached_delay) {}

    void Execute() override {
      executed = true;
      if (return_normally_) return;
      reached_delay_.Set();
      Delay(0, 0, static_cast<uint64_t>(int64_t(-36'000'000'000)));
      delay_returned = true;
    }

    bool executed = false;
    bool delay_returned = false;

   private:
    bool return_normally_;
    gh::kernel::Event& reached_delay_;
  };

  // Each runtime owns real guest memory and starts with a fresh title lifecycle.
  for (int scenario = 0; scenario < 3; ++scenario) {
    const bool suspended = scenario == 0;
    const bool return_normally = scenario == 2;
    rex::Runtime runtime({});
    rex::RuntimeConfig config;
    config.tool_mode = true;
    assert(runtime.Setup(std::move(config)) == X_STATUS_SUCCESS);
    assert(!runtime.graphics_system() && !runtime.audio_system() && !runtime.input_system());
    gh::kernel::Event reached_delay(true);
    auto thread = make_object<TerminationThread>(runtime.kernel_state(), suspended,
                                                return_normally, reached_delay);
    assert(thread->Create() == X_STATUS_SUCCESS);
    if (suspended) {
      assert(!thread->is_running());
      assert(thread->is_alive());
      std::array<uint8_t, 65536> bytes{};
      rex::stream::ByteStream stream(bytes.data(), bytes.size());
      assert(thread->Save(&stream));
      assert(stream.offset() > 0);
      stream.set_offset(0);
      assert(!runtime.kernel_state()->Save(&stream));
      assert(stream.offset() == 0);
    } else if (!return_normally) {
      assert(reached_delay.Wait(2s) == gh::kernel::WaitResult::Signaled);
      assert(thread->is_running() && thread->is_alive());
    }
    if (!return_normally) runtime.kernel_state()->TerminateTitle();
    uint64_t timeout = static_cast<uint64_t>(int64_t(-20'000'000));
    assert(thread->Wait(0, 0, 0, &timeout) == X_STATUS_SUCCESS);
    thread->thread()->Join();
    assert(!thread->is_running() && !thread->is_alive());
    assert(thread->guest_object<X_KTHREAD>()->header.signal_state == 1);
    assert(thread->executed == !suspended);
    assert(!thread->delay_returned);
    thread.reset();
  }
}

void TestNotificationWaitAndRestore() {
  XNotifyListener listener(nullptr);
  listener.Initialize(1, 0);
  assert(listener.Wait(0, 0, 0, &poll) == X_STATUS_TIMEOUT);
  listener.EnqueueNotification(0x02000009, 90);  // Different mask index.
  listener.EnqueueNotification(0x00010009, 91);  // Newer version.
  assert(listener.Wait(0, 0, 0, &poll) == X_STATUS_TIMEOUT);
  listener.EnqueueNotification(9, 10);
  listener.EnqueueNotification(10, 20);
  assert(listener.Wait(0, 0, 0, &poll) == X_STATUS_SUCCESS);
  assert(listener.Wait(0, 0, 0, &poll) == X_STATUS_SUCCESS);

  std::array<uint8_t, 256> bytes{};
  rex::stream::ByteStream stream(bytes.data(), bytes.size());
  assert(listener.Save(&stream));
  stream.set_offset(0);
  auto restored = XNotifyListener::Restore(nullptr, &stream);
  assert(restored->mask() == 1 && restored->max_version() == 0);
  assert(restored->Wait(0, 0, 0, &poll) == X_STATUS_SUCCESS);
  uint32_t data = 0;
  assert(!restored->DequeueNotification(11, &data));
  assert(restored->Wait(0, 0, 0, &poll) == X_STATUS_SUCCESS);
  assert(restored->DequeueNotification(10, &data) && data == 20);
  assert(restored->Wait(0, 0, 0, &poll) == X_STATUS_SUCCESS);
  XNotificationID id = 0;
  assert(restored->DequeueNotification(&id, &data) && id == 9 && data == 10);
  assert(restored->Wait(0, 0, 0, &poll) == X_STATUS_TIMEOUT);
  assert(!restored->DequeueNotification(&id, &data));
  stream.set_offset(0);
  assert(restored->Save(&stream));
  stream.set_offset(0);
  auto empty = XNotifyListener::Restore(nullptr, &stream);
  assert(empty->Wait(0, 0, 0, &poll) == X_STATUS_TIMEOUT);
}

struct SevenWaitObjects {
  EmptyFile storage;
  XEvent event{nullptr};
  XSemaphore semaphore{nullptr};
  XTimer timer{nullptr};
  XFile file{nullptr, &storage, false};
  CompletionThread thread;
  XNotifyListener listener{nullptr};
  XMutant mutant{nullptr};
  std::array<XObject*, 7> objects{&event, &semaphore, &timer, &file,
                                 &thread, &listener, &mutant};

  SevenWaitObjects() {
    InitializeEvent(event);
    InitializeSemaphore(semaphore, 0, 1);
    timer.Initialize(0);
    listener.Initialize(1, 0);
    mutant.Initialize(false);
  }

  void Ready(size_t index) {
    switch (index) {
      case 0: event.Set(0, false); break;
      case 1: assert(semaphore.ReleaseSemaphore(1, nullptr)); break;
      case 2: {
        assert(timer.SetTimer(0, 0, 0, 0, false) == X_STATUS_SUCCESS);
        uint64_t timeout = static_cast<uint64_t>(int64_t(-20'000'000));
        assert(timer.Wait(0, 0, 0, &timeout) == X_STATUS_SUCCESS);
        break;
      }
      case 3: {
        uint32_t read = 99;
        assert(file.Read(0, 0, 0, &read, 0) == X_STATUS_SUCCESS && read == 0);
        break;
      }
      case 4: thread.Complete(); break;
      case 5: listener.EnqueueNotification(9, 10); break;
      case 6: break;  // An unowned mutant is already ready.
      default: assert(false);
    }
  }
};

void TestSevenObjectMixedWaits() {
  for (size_t ready = 0; ready < 7; ++ready) {
    SevenWaitObjects set;
    set.Ready(ready);
    assert(XObject::WaitMultiple(set.objects.size(), set.objects.data(), 1, 0, 0, 0, &poll) ==
           static_cast<X_STATUS>(ready));
    if (ready == 6) assert(set.mutant.ReleaseMutant(0, false, false) == X_STATUS_SUCCESS);
  }
  SevenWaitObjects set;
  for (size_t index = 0; index < 7; ++index) set.Ready(index);
  set.event.Reset();
  assert(XObject::WaitMultiple(7, set.objects.data(), 0, 0, 0, 0, &poll) == X_STATUS_TIMEOUT);
  // A failed WaitAll must leave consumable signals and mutant ownership intact.
  assert(set.mutant.ReleaseMutant(0, false, false) == X_STATUS_MUTANT_NOT_OWNED);
  set.event.Set(0, false);
  assert(XObject::WaitMultiple(7, set.objects.data(), 0, 0, 0, 0, &poll) == X_STATUS_SUCCESS);
  assert(set.event.Wait(0, 0, 0, &poll) == X_STATUS_TIMEOUT);
  assert(set.semaphore.Wait(0, 0, 0, &poll) == X_STATUS_TIMEOUT);
  assert(set.file.Wait(0, 0, 0, &poll) == X_STATUS_TIMEOUT);
  assert(set.thread.Wait(0, 0, 0, &poll) == X_STATUS_SUCCESS);
  assert(set.listener.Wait(0, 0, 0, &poll) == X_STATUS_SUCCESS);
  assert(set.timer.Wait(0, 0, 0, &poll) == X_STATUS_SUCCESS);
  assert(set.mutant.ReleaseMutant(0, false, false) == X_STATUS_SUCCESS);
}

void TestSignalAndWaitObjectCoverage() {
  for (size_t target = 0; target < 7; ++target) {
    for (int source = 0; source < 3; ++source) {
      SevenWaitObjects set;
      set.Ready(target);
      XEvent event(nullptr);
      XSemaphore semaphore(nullptr);
      XMutant mutant(nullptr);
      InitializeEvent(event);
      InitializeSemaphore(semaphore, 0, 1);
      mutant.Initialize(true);
      XObject* signals[] = {&event, &semaphore, &mutant};
      assert(XObject::SignalAndWait(signals[source], set.objects[target], 0, 0, 0, &poll) ==
             X_STATUS_SUCCESS);
      assert(signals[source]->Wait(0, 0, 0, &poll) == X_STATUS_SUCCESS);
      if (source == 2) assert(mutant.ReleaseMutant(0, false, false) == X_STATUS_SUCCESS);
      if (target == 6) assert(set.mutant.ReleaseMutant(0, false, false) == X_STATUS_SUCCESS);
    }
  }
  SevenWaitObjects set;
  for (size_t source = 2; source < 6; ++source) {
    assert(XObject::SignalAndWait(set.objects[source], &set.event, 0, 0, 0, &poll) ==
           X_STATUS_OBJECT_TYPE_MISMATCH);
  }
}
}  // namespace

int main() {
  TestEventState();
  TestMixedWaits();
  TestSignalAndWait();
  TestMutantOwnership();
  TestTimer();
  TestFileCompletionWait();
  TestObjectTableRelease();
  TestObjectTableLifecycle();
  TestThreadCompletionWait();
  TestThreadDelay();
  TestGuestThreadTitleTermination();
  TestNotificationWaitAndRestore();
  TestSevenObjectMixedWaits();
  TestSignalAndWaitObjectCoverage();
  std::puts("kernel adapters: 14 cases passed");
}
