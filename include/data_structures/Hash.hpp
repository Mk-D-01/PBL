#pragma once
#include <string>
#include <functional>

namespace ds {

inline size_t hashOf(const std::string& text) {
    return std::hash<std::string>{}(text);
}

template <typename T>
inline size_t hashOf(T value) {
    return std::hash<T>{}(value);
}

}