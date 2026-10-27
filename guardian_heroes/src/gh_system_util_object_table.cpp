/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include <algorithm>
#include <memory>
#include <new>

#include <rex/logging.h>
#include <rex/stream.h>
#include "gh_system_util_object_table.h"
#include "gh_system_xobject.h"
#include "gh_system_xthread.h"

#include "gh_kernel_objects.h"

namespace rex::system::util {
static_assert(XObject::kHandleBase == gh::kernel::HandleTable::kHandleBase);
namespace {
std::shared_ptr<void> OwnObject(XObject* object) {
  object->Retain();
  return {object, [](void* pointer) { static_cast<XObject*>(pointer)->Release(); }};
}
}

ObjectTable::ObjectTable() : handles_(std::make_unique<gh::kernel::HandleTable>()) {}
ObjectTable::~ObjectTable() { Reset(); }

void ObjectTable::UnlinkHandle(XObject* object, X_HANDLE handle) {
  handle &= ~3u;
  std::erase(object->handles(), handle);
  if (object->name().empty()) return;
  std::lock_guard name_lock(name_mutex_);
  auto mapping = name_table_.find(string::string_key_case(object->name()));
  if (mapping != name_table_.end() && mapping->second == handle) {
    if (object->handles().empty()) name_table_.erase(mapping);
    else mapping->second = object->handles().front();
  }
}

void ObjectTable::Reset() {
  // Retired objects are destroyed after releasing the global lock.
  std::vector<gh::kernel::HandleTable::Record> removed;
  auto lock = global_critical_region_.Acquire();
  removed = handles_->Reset();
  for (const auto& record : removed)
    UnlinkHandle(static_cast<XObject*>(record.object.get()), record.handle);
  std::lock_guard name_lock(name_mutex_);
  name_table_.clear();
}

X_STATUS ObjectTable::AddHandle(XObject* object, X_HANDLE* out_handle) {
  auto lock = global_critical_region_.Acquire();
  try {
    object->handles().reserve(object->handles().size() + 1);
    auto handle = handles_->Add(OwnObject(object));
    object->handles().push_back(handle);
    if (out_handle) *out_handle = handle;
    return X_STATUS_SUCCESS;
  } catch (const std::bad_alloc&) {
    return X_STATUS_NO_MEMORY;
  }
}

X_STATUS ObjectTable::DuplicateHandle(X_HANDLE handle, X_HANDLE* out_handle) {
  auto object = LookupObject<XObject>(handle);
  return object ? AddHandle(object.get(), out_handle) : X_STATUS_INVALID_HANDLE;
}

X_STATUS ObjectTable::RetainHandle(X_HANDLE handle) {
  handle = TranslateHandle(handle);
  auto lock = global_critical_region_.Acquire();
  return handles_->Retain(handle) ? X_STATUS_SUCCESS : X_STATUS_INVALID_HANDLE;
}

X_STATUS ObjectTable::ReleaseHandle(X_HANDLE handle) {
  handle = TranslateHandle(handle);
  gh::kernel::HandleTable::ReleaseResult result;
  auto lock = global_critical_region_.Acquire();
  result = handles_->Release(handle);
  if (!result.valid) return X_STATUS_INVALID_HANDLE;
  if (result.removed) UnlinkHandle(static_cast<XObject*>(result.removed.get()), handle);
  return X_STATUS_SUCCESS;
}

X_STATUS ObjectTable::RemoveHandle(X_HANDLE handle) {
  handle = TranslateHandle(handle);
  std::shared_ptr<void> removed;
  auto lock = global_critical_region_.Acquire();
  if (!handles_->Lookup(handle)) return X_STATUS_INVALID_HANDLE;
  assert_zero(handles_->References(handle));
  removed = handles_->Remove(handle);
  UnlinkHandle(static_cast<XObject*>(removed.get()), handle);
  return X_STATUS_SUCCESS;
}

std::vector<object_ref<XObject>> ObjectTable::GetAllObjects() {
  auto lock = global_critical_region_.Acquire();
  std::vector<object_ref<XObject>> result;
  for (const auto& record : handles_->Objects()) {
    auto* object = static_cast<XObject*>(record.object.get());
    if (std::find(result.begin(), result.end(), object) == result.end())
      result.push_back(retain_object(object));
  }
  return result;
}

void ObjectTable::PurgeAllObjects() {
  std::vector<gh::kernel::HandleTable::Record> removed;
  auto lock = global_critical_region_.Acquire();
  removed = handles_->Purge([](void* object) {
    return static_cast<XObject*>(object)->is_host_object();
  });
  for (const auto& record : removed)
    UnlinkHandle(static_cast<XObject*>(record.object.get()), record.handle);
  std::lock_guard name_lock(name_mutex_);
  std::erase_if(name_table_, [this](const auto& mapping) {
    return !handles_->Lookup(mapping.second);
  });
}

template <>
object_ref<XObject> ObjectTable::LookupObject<XObject>(X_HANDLE handle) {
  return object_ref<XObject>(LookupObject(handle, false));
}

XObject* ObjectTable::LookupObject(X_HANDLE handle, bool already_locked) {
  handle = TranslateHandle(handle);
  std::unique_lock<std::recursive_mutex> lock(global_critical_region_.mutex(), std::defer_lock);
  if (!already_locked) lock.lock();
  auto* object = static_cast<XObject*>(handles_->Lookup(handle));
  if (object) object->Retain();
  return object;
}

void ObjectTable::GetObjectsByType(XObject::Type type, std::vector<object_ref<XObject>>* results) {
  auto lock = global_critical_region_.Acquire();
  for (const auto& record : handles_->Objects()) {
    auto* object = static_cast<XObject*>(record.object.get());
    if (object->type() == type) results->push_back(retain_object(object));
  }
}

X_HANDLE ObjectTable::TranslateHandle(X_HANDLE handle) {
  if (handle == 0xFFFFFFFF) return 0;
  if (handle == 0xFFFFFFFE) return XThread::GetCurrentThreadHandle();
  return handle;
}

X_STATUS ObjectTable::AddNameMapping(const std::string_view name, X_HANDLE handle) {
  std::lock_guard name_lock(name_mutex_);
  if (name_table_.count(string::string_key_case(name))) return X_STATUS_OBJECT_NAME_COLLISION;
  name_table_.insert({string::string_key_case::create(name), handle});
  return X_STATUS_SUCCESS;
}

void ObjectTable::RemoveNameMapping(const std::string_view name) {
  std::lock_guard name_lock(name_mutex_);
  name_table_.erase(string::string_key_case(name));
}

X_STATUS ObjectTable::GetObjectByName(const std::string_view name, X_HANDLE* out_handle) {
  auto lock = global_critical_region_.Acquire();
  X_HANDLE handle;
  {
    std::lock_guard name_lock(name_mutex_);
    auto it = name_table_.find(string::string_key_case(name));
    if (it == name_table_.end()) {
      *out_handle = X_INVALID_HANDLE_VALUE;
      return X_STATUS_OBJECT_NAME_NOT_FOUND;
    }
    handle = it->second;
  }
  if (!handles_->Retain(handle)) {
    *out_handle = X_INVALID_HANDLE_VALUE;
    return X_STATUS_OBJECT_NAME_NOT_FOUND;
  }
  *out_handle = handle;
  return X_STATUS_SUCCESS;
}

bool ObjectTable::Save(stream::ByteStream* stream) {
  auto lock = global_critical_region_.Acquire();
  const auto counts = handles_->SaveReferences();
  stream->Write(static_cast<uint32_t>(counts.size()));
  for (const auto count : counts) stream->Write(count);
  return true;
}

bool ObjectTable::Restore(stream::ByteStream* stream) {
  std::vector<int32_t> counts(stream->Read<uint32_t>());
  for (auto& count : counts) count = stream->Read<int32_t>();
  std::vector<gh::kernel::HandleTable::Record> removed;
  auto lock = global_critical_region_.Acquire();
  removed = handles_->RestoreReferences(counts);
  for (const auto& record : removed)
    UnlinkHandle(static_cast<XObject*>(record.object.get()), record.handle);
  return true;
}

X_STATUS ObjectTable::RestoreHandle(X_HANDLE handle, XObject* object) {
  std::shared_ptr<void> replaced;
  auto lock = global_critical_region_.Acquire();
  replaced = handles_->Restore(handle, OwnObject(object));
  if (replaced && replaced.get() != object)
    UnlinkHandle(static_cast<XObject*>(replaced.get()), handle);
  return X_STATUS_SUCCESS;
}

}  // namespace rex::system::util
