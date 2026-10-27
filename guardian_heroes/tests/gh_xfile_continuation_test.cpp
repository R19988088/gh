#include "gh_system_kernel_state.h"
#include "gh_system_runtime.h"
#include "gh_system_xfile.h"
#include <rex/filesystem/devices/host_path_device.h>
#include <rex/filesystem/vfs.h>
#include <rex/stream.h>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace rex::system;
using rex::X_STATUS;
namespace fs = rex::filesystem;

namespace {
object_ref<XFile> Open(KernelState* kernel, std::string_view path, bool directory = false,
                       bool synchronous = false) {
  fs::File* host = nullptr;
  fs::FileAction action;
  assert(kernel->file_system()->OpenFile(nullptr, path, fs::FileDisposition::kOpen,
      fs::FileAccess::kFileReadData | (directory ? 0 : fs::FileAccess::kFileWriteData),
      directory, !directory, &host, &action) == X_STATUS_SUCCESS);
  return make_object<XFile>(kernel, host, synchronous);
}

object_ref<XFile> Roundtrip(KernelState* kernel, object_ref<XFile>& file, bool legacy = false) {
  std::array<uint8_t, 4096> bytes{};
  rex::stream::ByteStream stream(bytes.data(), bytes.size());
  assert(legacy ? file->Save(&stream) : file->SaveContinuation(&stream));
  const auto size = stream.offset();
  const auto handle = file->handle();
  stream.set_offset(0);
  auto restored = legacy ? XFile::Restore(kernel, &stream) : XFile::RestoreContinuation(kernel, &stream);
  assert(restored && stream.offset() == size && restored->handle() == handle);
  assert(kernel->object_table()->LookupObject<XFile>(handle).get() == restored.get());
  file.reset();
  return restored;
}

std::string Next(XFile& directory, std::string_view pattern = {}) {
  alignas(X_FILE_DIRECTORY_INFORMATION) std::array<uint8_t, 512> bytes{};
  auto* info = reinterpret_cast<X_FILE_DIRECTORY_INFORMATION*>(bytes.data());
  const auto status = directory.QueryDirectory(info, bytes.size(), pattern, false);
  if (status == X_STATUS_NO_MORE_FILES) return {};
  assert(status == X_STATUS_SUCCESS);
  return std::string(info->file_name, info->file_name_length);
}
}

int main() {
  const auto path = std::filesystem::temp_directory_path() /
      ("gh-xfile-continuation-" + std::to_string(
          std::chrono::steady_clock::now().time_since_epoch().count()));
  assert(std::filesystem::create_directory(path));
  std::ofstream(path / "data.bin", std::ios::binary) << "abcdefghij";
  std::ofstream(path / "first.txt") << "first";
  std::ofstream(path / "second.txt") << "second";
  std::ofstream(path / "ignored.dat") << "ignored";
  rex::Runtime runtime({});
  rex::RuntimeConfig config;
  config.tool_mode = true;
  assert(runtime.Setup(std::move(config)) == X_STATUS_SUCCESS);
  auto* kernel = runtime.kernel_state();
  auto device = std::make_unique<fs::HostPathDevice>("\\Device\\XFileSnapshot", path, false);
  assert(device->Initialize());
  assert(kernel->file_system()->RegisterDevice(std::move(device)));
  assert(kernel->file_system()->RegisterSymbolicLink("xfile:", "\\Device\\XFileSnapshot"));
  assert(kernel->PauseDispatchForSnapshot(std::chrono::seconds(2)));
  const auto buffer = runtime.memory()->SystemHeapAlloc(32);
  assert(buffer);
  auto file = Open(kernel, "\\Device\\XFileSnapshot\\data.bin");
  uint64_t poll = 0;
  assert(file->Wait(0, 0, 0, &poll) == X_STATUS_TIMEOUT);
  uint32_t transferred = 0;
  assert(file->Read(buffer, 3, uint64_t(-1), &transferred, 0) == X_STATUS_SUCCESS);
  assert(transferred == 3 && file->position() == 3 && file->can_save_continuation());
  file = Roundtrip(kernel, file);
  assert(!file->is_synchronous() && file->position() == 3);
  assert(file->Wait(0, 0, 0, &poll) == X_STATUS_SUCCESS);
  assert(file->Wait(0, 0, 0, &poll) == X_STATUS_TIMEOUT);
  assert(file->Read(buffer, 2, uint64_t(-1), &transferred, 0) == X_STATUS_SUCCESS);
  assert(transferred == 2 && std::string(reinterpret_cast<char*>(runtime.memory()->TranslateVirtual(buffer)), 2) == "de");
  assert(file->Wait(0, 0, 0, &poll) == X_STATUS_SUCCESS);
  file = Roundtrip(kernel, file);
  assert(file->position() == 5 && file->Wait(0, 0, 0, &poll) == X_STATUS_TIMEOUT);
  std::memcpy(runtime.memory()->TranslateVirtual(buffer), "XY", 2);
  assert(file->Write(buffer, 2, uint64_t(-1), &transferred, 0) == X_STATUS_SUCCESS);
  file = Roundtrip(kernel, file);
  assert(file->position() == 7 && file->Wait(0, 0, 0, &poll) == X_STATUS_SUCCESS);
  assert(file->Read(buffer, 3, uint64_t(-1), &transferred, 0) == X_STATUS_SUCCESS);
  assert(std::string(reinterpret_cast<char*>(runtime.memory()->TranslateVirtual(buffer)), 3) == "hij");

  auto port = make_object<XIOCompletion>(kernel);
  file->RegisterIOCompletionPort(0x1234, port);
  std::array<uint8_t, 4096> rejected_bytes{};
  rex::stream::ByteStream rejected(rejected_bytes.data(), rejected_bytes.size());
  assert(!file->can_save_continuation() && !file->SaveContinuation(&rejected));
  assert(rejected.offset() == 0);
  file->RemoveIOCompletionPort(0x1234);
  file->entry()->SetForDeletion(true);
  assert(!file->can_save_continuation() && !file->SaveContinuation(&rejected));
  assert(rejected.offset() == 0);
  file->entry()->SetForDeletion(false);
  assert(file->can_save_continuation());

  auto directory = Open(kernel, "xfile:", true);
  const auto first = Next(*directory, "*.txt");
  assert(!first.empty());
  std::array<uint8_t, 4096> directory_bytes{};
  rex::stream::ByteStream directory_stream(directory_bytes.data(), directory_bytes.size());
  assert(directory->SaveContinuation(&directory_stream));
  const auto second = Next(*directory);
  assert(!second.empty() && second != first && Next(*directory).empty());
  directory_stream.set_offset(0);
  auto restored_directory = XFile::RestoreContinuation(kernel, &directory_stream);
  directory.reset();
  assert(restored_directory && Next(*restored_directory) == second);
  assert(Next(*restored_directory).empty());
  restored_directory->SetFindPattern("*.dat");
  restored_directory = Roundtrip(kernel, restored_directory);
  auto* entry = restored_directory->FindNext();
  assert(entry && entry->name() == "ignored.dat" && !restored_directory->FindNext());

  file->set_position(4);
  file = Roundtrip(kernel, file, true);
  assert(!file->is_synchronous() && file->position() == 4);
  assert(file->Read(buffer, 1, uint64_t(-1), &transferred, 0) == X_STATUS_SUCCESS);
  assert(*runtime.memory()->TranslateVirtual(buffer) == 'e');
  file->ReleaseHandle();
  file.reset();
  restored_directory->ReleaseHandle();
  restored_directory.reset();
  port->ReleaseHandle();
  port.reset();
  runtime.memory()->SystemHeapFree(buffer);
  kernel->ResumeDispatchAfterSnapshot();
  assert(kernel->file_system()->UnregisterSymbolicLink("xfile:"));
  assert(kernel->file_system()->UnregisterDevice("\\Device\\XFileSnapshot"));
  std::filesystem::remove_all(path);
  std::printf("XFile FIL1: quiescent async read/write position, auto-reset completion signal, "
              "directory pattern/cursor, completion-port/destructive-close rejection and legacy restore passed\n");
}
