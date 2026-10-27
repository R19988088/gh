#include "gh_kernel_objects.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace gh::kernel {
namespace {
std::uint32_t Slot(std::uint32_t handle) { return (handle - HandleTable::kHandleBase) >> 2; }
std::uint32_t Handle(std::uint32_t slot) { return HandleTable::kHandleBase + (slot << 2); }
}

const HandleTable::Entry* HandleTable::Find(std::uint32_t handle) const {
  const auto slot = Slot(handle);
  return slot && slot < entries_.size() && entries_[slot].object ? &entries_[slot] : nullptr;
}
HandleTable::Entry* HandleTable::Find(std::uint32_t handle) {
  return const_cast<Entry*>(std::as_const(*this).Find(handle));
}

std::uint32_t HandleTable::Add(std::shared_ptr<void> object) {
  auto slot = last_free_entry_;
  auto scanned = std::size_t{0};
  while (scanned < entries_.size()) {
    if (slot && !entries_[slot].object) break;
    ++scanned;
    slot = (slot + 1) % entries_.size();
    if (!slot) { ++scanned; ++slot; }
  }
  if (scanned >= entries_.size()) {
    const auto old_capacity = static_cast<std::uint32_t>(entries_.size());
    entries_.resize(std::max<std::size_t>(16 * 1024, entries_.size() * 2));
    last_free_entry_ = old_capacity;
    slot = ++last_free_entry_;
  }
  entries_[slot] = {std::move(object), 1};
  return Handle(slot);
}

void* HandleTable::Lookup(std::uint32_t handle) const {
  const auto* entry = Find(handle);
  return entry ? entry->object.get() : nullptr;
}
bool HandleTable::Retain(std::uint32_t handle) {
  auto* entry = Find(handle);
  if (!entry) return false;
  ++entry->references;
  return true;
}
HandleTable::ReleaseResult HandleTable::Release(std::uint32_t handle) {
  auto* entry = Find(handle);
  if (!entry) return {false, {}};
  if (--entry->references) return {true, {}};
  return {true, std::move(entry->object)};
}
std::shared_ptr<void> HandleTable::Remove(std::uint32_t handle) {
  auto* entry = Find(handle);
  if (!entry) return {};
  entry->references = 0;
  return std::move(entry->object);
}
std::int32_t HandleTable::References(std::uint32_t handle) const {
  const auto* entry = Find(handle);
  return entry ? entry->references : 0;
}
std::vector<HandleTable::Record> HandleTable::Objects() const {
  std::vector<Record> result;
  for (std::uint32_t slot = 0; slot < entries_.size(); ++slot) {
    if (entries_[slot].object) result.push_back({Handle(slot), entries_[slot].object});
  }
  return result;
}
std::vector<HandleTable::Record> HandleTable::Extract(const std::function<bool(void*)>& remove) {
  std::vector<Record> result;
  for (std::uint32_t slot = 0; slot < entries_.size(); ++slot) {
    auto& entry = entries_[slot];
    if (entry.object && remove(entry.object.get())) {
      result.push_back({Handle(slot), std::move(entry.object)});
      entry.references = 0;
    }
  }
  return result;
}
std::vector<HandleTable::Record> HandleTable::Reset() {
  auto result = Extract([](void*) { return true; });
  entries_.clear();
  last_free_entry_ = 0;
  return result;
}
std::vector<HandleTable::Record> HandleTable::Purge(const std::function<bool(void*)>& is_host) {
  return Extract([&](void* object) { return !is_host(object); });
}
std::vector<std::int32_t> HandleTable::SaveReferences() const {
  std::vector<std::int32_t> result;
  result.reserve(entries_.size());
  for (const auto& entry : entries_) result.push_back(entry.references);
  return result;
}
std::vector<HandleTable::Record> HandleTable::RestoreReferences(
    std::span<const std::int32_t> counts) {
  std::vector<Record> removed;
  for (std::size_t slot = 0; slot < entries_.size(); ++slot) {
    if (entries_[slot].object && (slot >= counts.size() || counts[slot] == 0))
      removed.push_back({Handle(static_cast<std::uint32_t>(slot)),
                         std::move(entries_[slot].object)});
  }
  last_free_entry_ = 0;
  entries_.resize(counts.size());
  for (std::size_t i = 0; i < counts.size(); ++i) entries_[i].references = counts[i];
  return removed;
}
std::shared_ptr<void> HandleTable::Restore(std::uint32_t handle,
                                          std::shared_ptr<void> object) {
  const auto slot = Slot(handle);
  if (!slot || slot >= entries_.size()) throw std::out_of_range("restore handle slot");
  return std::exchange(entries_[slot].object, std::move(object));
}
}  // namespace gh::kernel
