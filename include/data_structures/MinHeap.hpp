#pragma once

#include <cstddef>
#include <stdexcept>
#include <utility>

#include "DynamicArray.hpp"
#include "Hash.hpp"

namespace ds {

// MinHeap: array-backed binary min-heap implemented from scratch.
// Used as the priority queue for Dijkstra's shortest-path algorithm.
//
// Requirements on T:
//   - comparable with < (smaller = higher priority), and
//   - equality-comparable with == so decreaseKey can locate entries.
template <typename T>
class MinHeap {
public:
    MinHeap() = default;

    explicit MinHeap(std::size_t initialCapacity) { data_.reserve(initialCapacity); }

    void push(const T& value) {
        data_.pushBack(value);
        siftUp(data_.size() - 1);
    }
    void push(T&& value) {
        data_.pushBack(std::move(value));
        siftUp(data_.size() - 1);
    }

    // Empties the heap and returns its minimum element.
    T extractMin() {
        if (data_.empty()) throw std::out_of_range("MinHeap::extractMin on empty heap");
        T minValue = std::move(data_[0]);
        data_[0] = std::move(data_.back());
        data_.popBack();
        if (!data_.empty()) siftDown(0);
        return minValue;
    }

    const T& peekMin() const {
        if (data_.empty()) throw std::out_of_range("MinHeap::peekMin on empty heap");
        return data_[0];
    }

    // Replaces the existing entry equal to `target` with `newValue` and
    // restores heap order (used to lower a key). Returns false when `target`
    // is absent or `newValue` would not lower the key (heap is left intact).
    bool decreaseKey(const T& target, const T& newValue) {
        const std::size_t index = indexOf(target);
        if (index == DynamicArray<T>::npos) return false;
        if (!(newValue < target)) return false;  // only lowering is allowed
        data_[index] = newValue;
        siftUp(index);
        return true;
    }

    bool contains(const T& value) const { return indexOf(value) != DynamicArray<T>::npos; }

    std::size_t size() const { return data_.size(); }
    bool empty() const { return data_.empty(); }
    void clear() { data_.clear(); }

private:
    static std::size_t parent(std::size_t i) { return (i - 1) / 2; }
    static std::size_t leftChild(std::size_t i) { return 2 * i + 1; }
    static std::size_t rightChild(std::size_t i) { return 2 * i + 2; }

    std::size_t indexOf(const T& value) const { return data_.indexOf(value); }

    void siftUp(std::size_t index) {
        while (index > 0 && data_[index] < data_[parent(index)]) {
            std::swap(data_[index], data_[parent(index)]);
            index = parent(index);
        }
    }

    void siftDown(std::size_t index) {
        for (;;) {
            std::size_t smallest = index;
            std::size_t left = leftChild(index);
            std::size_t right = rightChild(index);
            if (left < data_.size() && data_[left] < data_[smallest]) smallest = left;
            if (right < data_.size() && data_[right] < data_[smallest]) smallest = right;
            if (smallest == index) break;
            std::swap(data_[index], data_[smallest]);
            index = smallest;
        }
    }

    ds::DynamicArray<T> data_;
};

}  // namespace ds
