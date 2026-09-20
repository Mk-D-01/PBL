#pragma once

#include <cstddef>
#include <initializer_list>
#include <stdexcept>
#include <utility>

namespace ds {

// DynamicArray: a growable contiguous array implemented from scratch.
// Grows by doubling capacity; elements are shifted manually on insert/remove.
template <typename T>
class DynamicArray {
public:
    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    DynamicArray() : data_(nullptr), size_(0), capacity_(0) {}

    explicit DynamicArray(std::size_t initialCapacity) : DynamicArray() {
        if (initialCapacity > 0) reserve(initialCapacity);
    }

    DynamicArray(std::size_t count, const T& fillValue) : DynamicArray() {
        reserve(count);
        for (std::size_t i = 0; i < count; ++i) data_[size_++] = fillValue;
    }

    DynamicArray(std::initializer_list<T> init) : DynamicArray() {
        reserve(init.size());
        for (const T& value : init) data_[size_++] = value;
    }

    DynamicArray(const DynamicArray& other) : DynamicArray() {
        reserve(other.size_);
        for (std::size_t i = 0; i < other.size_; ++i) data_[size_++] = other.data_[i];
    }

    DynamicArray(DynamicArray&& other) noexcept
        : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }

    // Unified assignment (copy- and move-friendly via pass-by-value + swap).
    DynamicArray& operator=(DynamicArray other) {
        swap(other);
        return *this;
    }

    ~DynamicArray() { delete[] data_; }

    void swap(DynamicArray& other) noexcept {
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    // ---- element access ----
    T& at(std::size_t index) {
        if (index >= size_) throw std::out_of_range("DynamicArray::at: index out of range");
        return data_[index];
    }
    const T& at(std::size_t index) const {
        if (index >= size_) throw std::out_of_range("DynamicArray::at: index out of range");
        return data_[index];
    }
    T& operator[](std::size_t index) { return data_[index]; }
    const T& operator[](std::size_t index) const { return data_[index]; }

    T& front() {
        if (size_ == 0) throw std::out_of_range("DynamicArray::front on empty array");
        return data_[0];
    }
    const T& front() const {
        if (size_ == 0) throw std::out_of_range("DynamicArray::front on empty array");
        return data_[0];
    }
    T& back() {
        if (size_ == 0) throw std::out_of_range("DynamicArray::back on empty array");
        return data_[size_ - 1];
    }
    const T& back() const {
        if (size_ == 0) throw std::out_of_range("DynamicArray::back on empty array");
        return data_[size_ - 1];
    }

    // ---- modifiers ----
    void pushBack(const T& value) {
        if (size_ == capacity_) reserve(capacity_ == 0 ? 4 : capacity_ * 2);
        data_[size_++] = value;
    }
    void pushBack(T&& value) {
        if (size_ == capacity_) reserve(capacity_ == 0 ? 4 : capacity_ * 2);
        data_[size_++] = std::move(value);
    }

    void popBack() {
        if (size_ == 0) throw std::out_of_range("DynamicArray::popBack on empty array");
        --size_;
    }

    void insertAt(std::size_t index, const T& value) {
        if (index > size_) throw std::out_of_range("DynamicArray::insertAt: index out of range");
        if (size_ == capacity_) reserve(capacity_ == 0 ? 4 : capacity_ * 2);
        for (std::size_t i = size_; i > index; --i) data_[i] = std::move(data_[i - 1]);
        data_[index] = value;
        ++size_;
    }

    void removeAt(std::size_t index) {
        if (index >= size_) throw std::out_of_range("DynamicArray::removeAt: index out of range");
        for (std::size_t i = index; i + 1 < size_; ++i) data_[i] = std::move(data_[i + 1]);
        --size_;
    }

    bool removeValue(const T& value) {
        std::size_t index = indexOf(value);
        if (index == npos) return false;
        removeAt(index);
        return true;
    }

    void clear() { size_ = 0; }

    void reserve(std::size_t newCapacity) {
        if (newCapacity <= capacity_) return;
        std::size_t target = capacity_ == 0 ? 4 : capacity_;
        while (target < newCapacity) target *= 2;
        T* newData = new T[target];
        for (std::size_t i = 0; i < size_; ++i) newData[i] = std::move(data_[i]);
        delete[] data_;
        data_ = newData;
        capacity_ = target;
    }

    // ---- queries ----
    std::size_t size() const { return size_; }
    std::size_t capacity() const { return capacity_; }
    bool empty() const { return size_ == 0; }

    std::size_t indexOf(const T& value) const {
        for (std::size_t i = 0; i < size_; ++i)
            if (data_[i] == value) return i;
        return npos;
    }

    bool contains(const T& value) const { return indexOf(value) != npos; }

    // ---- iteration (raw pointers double as iterators) ----
    T* begin() { return data_; }
    T* end() { return data_ + size_; }
    const T* begin() const { return data_; }
    const T* end() const { return data_ + size_; }

private:
    T* data_;
    std::size_t size_;
    std::size_t capacity_;
};

}  // namespace ds
