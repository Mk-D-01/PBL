#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>

namespace ds {

// FNV-1a 64-bit hash for string keys.
inline std::size_t hashOf(const std::string& text) {
    std::size_t hash = 14695981039346656037ULL;
    for (char ch : text) {
        hash ^= static_cast<unsigned char>(ch);
        hash *= 1099511628211ULL;
    }
    return hash;
}

// MurmurHash3 64-bit finalizer for integral keys.
template <typename T>
inline std::enable_if_t<std::is_integral<T>::value, std::size_t> hashOf(T value) {
    std::size_t x = static_cast<std::size_t>(value);
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return x;
}

// Identity-based hash for pointer keys.
template <typename T>
inline std::enable_if_t<std::is_pointer<T>::value, std::size_t> hashOf(T pointer) {
    return hashOf(reinterpret_cast<std::uintptr_t>(pointer));
}

}  // namespace ds
