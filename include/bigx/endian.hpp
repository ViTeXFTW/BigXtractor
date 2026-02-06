#pragma once

#include <bit>
#include <cstdint>
#include <cstring>

namespace bigx {

// C++20-compatible byteswap (C++23 has std::byteswap)
namespace detail {

inline constexpr uint16_t byteswap(uint16_t value) noexcept {
  return static_cast<uint16_t>((value << 8) | (value >> 8));
}

inline constexpr uint32_t byteswap(uint32_t value) noexcept {
  return ((value & 0x000000FFu) << 24) | ((value & 0x0000FF00u) << 8) |
         ((value & 0x00FF0000u) >> 8) | ((value & 0xFF000000u) >> 24);
}

inline constexpr uint64_t byteswap(uint64_t value) noexcept {
  return ((value & 0x00000000000000FFull) << 56) | ((value & 0x000000000000FF00ull) << 40) |
         ((value & 0x0000000000FF0000ull) << 24) | ((value & 0x00000000FF000000ull) << 8) |
         ((value & 0x000000FF00000000ull) >> 8) | ((value & 0x0000FF0000000000ull) >> 24) |
         ((value & 0x00FF000000000000ull) >> 40) | ((value & 0xFF00000000000000ull) >> 56);
}

} // namespace detail

// Check if the system is little-endian at compile time
inline constexpr bool is_little_endian() noexcept {
  return std::endian::native == std::endian::little;
}

// Check if the system is big-endian at compile time
inline constexpr bool is_big_endian() noexcept {
  return std::endian::native == std::endian::big;
}

// Convert big-endian to host byte order
inline constexpr uint16_t betoh16(uint16_t value) noexcept {
  if constexpr (is_little_endian()) {
    return detail::byteswap(value);
  }
  return value;
}

inline constexpr uint32_t betoh32(uint32_t value) noexcept {
  if constexpr (is_little_endian()) {
    return detail::byteswap(value);
  }
  return value;
}

inline constexpr uint64_t betoh64(uint64_t value) noexcept {
  if constexpr (is_little_endian()) {
    return detail::byteswap(value);
  }
  return value;
}

// Convert host byte order to big-endian
inline constexpr uint16_t htobe16(uint16_t value) noexcept {
  if constexpr (is_little_endian()) {
    return detail::byteswap(value);
  }
  return value;
}

inline constexpr uint32_t htobe32(uint32_t value) noexcept {
  if constexpr (is_little_endian()) {
    return detail::byteswap(value);
  }
  return value;
}

inline constexpr uint64_t htobe64(uint64_t value) noexcept {
  if constexpr (is_little_endian()) {
    return detail::byteswap(value);
  }
  return value;
}

} // namespace bigx
