#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <vector>

namespace gh::kernel {

// The adapter supplies object lifetime and locking; this table owns handle
// allocation, references and retirement independently of runtime object types.
class HandleTable {
 public:
  static constexpr std::uint32_t kHandleBase = 0xF8000000;
  struct Record {
    std::uint32_t handle;
    std::shared_ptr<void> object;
  };
  struct ReleaseResult {
    bool valid;
    std::shared_ptr<void> removed;
  };

  std::uint32_t Add(std::shared_ptr<void> object);
  void* Lookup(std::uint32_t handle) const;
  bool Retain(std::uint32_t handle);
  ReleaseResult Release(std::uint32_t handle);
  std::shared_ptr<void> Remove(std::uint32_t handle);
  std::int32_t References(std::uint32_t handle) const;
  std::vector<Record> Objects() const;
  std::vector<Record> Reset();
  std::vector<Record> Purge(const std::function<bool(void*)>& is_host);
  std::vector<std::int32_t> SaveReferences() const;
  std::vector<Record> RestoreReferences(std::span<const std::int32_t> counts);
  std::shared_ptr<void> Restore(std::uint32_t handle, std::shared_ptr<void> object);

 private:
  struct Entry {
    std::shared_ptr<void> object;
    std::int32_t references = 0;
  };
  Entry* Find(std::uint32_t handle);
  const Entry* Find(std::uint32_t handle) const;
  std::vector<Record> Extract(const std::function<bool(void*)>& remove);
  std::vector<Entry> entries_;
  std::uint32_t last_free_entry_ = 0;
};

}  // namespace gh::kernel
