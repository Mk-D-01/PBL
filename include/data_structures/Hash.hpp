#pragma once

#include <cstddef>
#include <string>
using namespace std;

namespace ds {

// Hash for string keys (FNV-1a): mixes every character into the result.
inline size_t hashOf(const string& text) {
    size_t hash = 14695981039346656037ULL;
    for (size_t i = 0; i < text.size(); i++) {
        hash = hash ^ (unsigned char)text[i];
        hash = hash * 1099511628211ULL;
    }
    return hash;
}

// Hash for integer keys (int, long, ...): scrambles the bits so that
// consecutive numbers do not land in consecutive buckets.
inline size_t hashOf(long long value) {
    size_t x = (size_t)value;
    x = x ^ (x >> 33);
    x = x * 0xff51afd7ed558ccdULL;
    x = x ^ (x >> 33);
    x = x * 0xc4ceb9fe1a85ec53ULL;
    x = x ^ (x >> 33);
    return x;
}

// Hash for pointer keys: uses the address as a number.
template <typename T>
inline size_t hashOf(T* pointer) {
    return hashOf((long long)(size_t)pointer);
}

}  // namespace ds
