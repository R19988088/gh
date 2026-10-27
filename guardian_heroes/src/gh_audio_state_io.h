#pragma once
#include <cstdint>
#include <cstring>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace gh::audio::state_io {
// Versioned local snapshot records use this build's scalar/float ABI.
struct Writer {
  std::vector<uint8_t> bytes;
  template<class T> void Put(const T& value) {
    static_assert(std::is_trivially_copyable_v<T>);
    auto start = reinterpret_cast<const uint8_t*>(&value);
    bytes.insert(bytes.end(), start, start + sizeof(T));
  }
  void Blob(std::span<const uint8_t> value) {
    Put(static_cast<uint32_t>(value.size()));
    bytes.insert(bytes.end(), value.begin(), value.end());
  }
};
struct Reader {
  std::span<const uint8_t> bytes;
  template<class T> T Get() {
    static_assert(std::is_trivially_copyable_v<T>);
    if (bytes.size() < sizeof(T)) throw std::runtime_error("Truncated audio state");
    T value; std::memcpy(&value, bytes.data(), sizeof(T));
    bytes = bytes.subspan(sizeof(T)); return value;
  }
  std::vector<uint8_t> Blob() {
    const auto size = Get<uint32_t>();
    if (size > bytes.size()) throw std::runtime_error("Truncated audio payload");
    std::vector<uint8_t> value(bytes.begin(), bytes.begin() + size);
    bytes = bytes.subspan(size); return value;
  }
};
}  // namespace gh::audio::state_io
