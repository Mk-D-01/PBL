#pragma once

#include <cstddef>
#include "DynamicArray.hpp"
using namespace std;

namespace ds {

// MinHeap: binary min-heap stored in an array (priority queue for Dijkstra).
// For the node at index i: parent = (i-1)/2, children = 2i+1 and 2i+2.
//
// T needs operator< (smaller = higher priority) and operator== (so that
// decreaseKey can find the entry).
// Note: call empty() before extractMin()/peekMin().
template <typename T>
class MinHeap {
    DynamicArray<T> data_;

    void swapItems(size_t a, size_t b) {
        T temp = data_[a];
        data_[a] = data_[b];
        data_[b] = temp;
    }

    // moves the item at index up until its parent is not bigger
    void siftUp(size_t index) {
        while (index > 0) {
            size_t parent = (index - 1) / 2;
            if (!(data_[index] < data_[parent]))
                break;
            swapItems(index, parent);
            index = parent;
        }
    }

    // moves the item at index down until both children are not smaller
    void siftDown(size_t index) {
        while (true) {
            size_t smallest = index;
            size_t left = 2 * index + 1;
            size_t right = 2 * index + 2;
            if (left < data_.size() && data_[left] < data_[smallest])
                smallest = left;
            if (right < data_.size() && data_[right] < data_[smallest])
                smallest = right;
            if (smallest == index)
                break;
            swapItems(index, smallest);
            index = smallest;
        }
    }

public:
    MinHeap() {}

    MinHeap(size_t initialCapacity) {
        data_.reserve(initialCapacity);
    }

    void push(const T& value) {
        data_.pushBack(value);
        siftUp(data_.size() - 1);
    }

    // removes and returns the smallest element
    T extractMin() {
        T minValue = data_[0];
        data_[0] = data_.back();
        data_.popBack();
        if (!data_.empty())
            siftDown(0);
        return minValue;
    }

    const T& peekMin() const { return data_[0]; }

    // Replaces the entry equal to target with newValue (which must be smaller)
    // and fixes the heap. Returns false if target is missing or newValue is
    // not smaller - the heap is left unchanged in that case.
    bool decreaseKey(const T& target, const T& newValue) {
        size_t index = data_.indexOf(target);
        if (index == DynamicArray<T>::npos)
            return false;
        if (!(newValue < target))
            return false;
        data_[index] = newValue;
        siftUp(index);
        return true;
    }

    bool contains(const T& value) const { return data_.contains(value); }

    size_t size() const { return data_.size(); }
    bool empty() const { return data_.empty(); }
    void clear() { data_.clear(); }
};

}  // namespace ds
