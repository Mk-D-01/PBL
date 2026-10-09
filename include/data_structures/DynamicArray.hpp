#pragma once

#include <cstddef>
using namespace std;

namespace ds {

// DynamicArray: a growable array (like vector), written from scratch.
// When it is full, the capacity doubles and the old elements are copied over.
// Note: the caller must check size()/empty() first - no bounds checking is done.
template <typename T>
class DynamicArray {
    T* data_;
    size_t size_;
    size_t capacity_;

public:
    static const size_t npos = (size_t)-1;   // "not found" value for indexOf

    DynamicArray() {
        data_ = nullptr;
        size_ = 0;
        capacity_ = 0;
    }

    DynamicArray(size_t initialCapacity) {
        data_ = nullptr;
        size_ = 0;
        capacity_ = 0;
        reserve(initialCapacity);
    }

    // n copies of fillValue
    DynamicArray(size_t count, const T& fillValue) {
        data_ = nullptr;
        size_ = 0;
        capacity_ = 0;
        reserve(count);
        for (size_t i = 0; i < count; i++)
            pushBack(fillValue);
    }

    // copy constructor
    DynamicArray(const DynamicArray& other) {
        data_ = nullptr;
        size_ = 0;
        capacity_ = 0;
        reserve(other.size_);
        for (size_t i = 0; i < other.size_; i++)
            pushBack(other.data_[i]);
    }

    // copy assignment
    DynamicArray& operator=(const DynamicArray& other) {
        if (this != &other) {
            clear();
            reserve(other.size_);
            for (size_t i = 0; i < other.size_; i++)
                pushBack(other.data_[i]);
        }
        return *this;
    }

    ~DynamicArray() {
        delete[] data_;
    }

    // ---- element access ----
    T& operator[](size_t index) { return data_[index]; }
    const T& operator[](size_t index) const { return data_[index]; }
    T& front() { return data_[0]; }
    const T& front() const { return data_[0]; }
    T& back() { return data_[size_ - 1]; }
    const T& back() const { return data_[size_ - 1]; }

    // ---- modifiers ----
    void pushBack(const T& value) {
        if (size_ == capacity_) {
            if (capacity_ == 0)
                reserve(4);
            else
                reserve(capacity_ * 2);
        }
        data_[size_] = value;
        size_++;
    }

    void popBack() {
        if (size_ > 0)
            size_--;
    }

    // inserts value at index, shifting later elements right
    void insertAt(size_t index, const T& value) {
        if (index > size_)
            return;
        if (size_ == capacity_) {
            if (capacity_ == 0)
                reserve(4);
            else
                reserve(capacity_ * 2);
        }
        for (size_t i = size_; i > index; i--)
            data_[i] = data_[i - 1];
        data_[index] = value;
        size_++;
    }

    // removes the element at index, shifting later elements left
    void removeAt(size_t index) {
        if (index >= size_)
            return;
        for (size_t i = index; i + 1 < size_; i++)
            data_[i] = data_[i + 1];
        size_--;
    }

    bool removeValue(const T& value) {
        size_t index = indexOf(value);
        if (index == npos)
            return false;
        removeAt(index);
        return true;
    }

    void clear() { size_ = 0; }

    // makes sure there is room for at least newCapacity elements
    void reserve(size_t newCapacity) {
        if (newCapacity <= capacity_)
            return;
        T* newData = new T[newCapacity];
        for (size_t i = 0; i < size_; i++)
            newData[i] = data_[i];
        delete[] data_;
        data_ = newData;
        capacity_ = newCapacity;
    }

    // ---- queries ----
    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    size_t indexOf(const T& value) const {
        for (size_t i = 0; i < size_; i++) {
            if (data_[i] == value)
                return i;
        }
        return npos;
    }

    bool contains(const T& value) const { return indexOf(value) != npos; }

    // plain pointers work as iterators, so range-for loops work
    T* begin() { return data_; }
    T* end() { return data_ + size_; }
    const T* begin() const { return data_; }
    const T* end() const { return data_ + size_; }
};

}  // namespace ds
