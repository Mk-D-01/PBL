#pragma once
#include <cstddef>
#include <utility>

namespace ds {

template <typename T>
class DynamicArray {
public:
    DynamicArray() : data_(nullptr), size_(0), capacity_(0) {}

    explicit DynamicArray(size_t capacity) : DynamicArray() {
        reserve(capacity);
    }

    DynamicArray(size_t count, const T& value) : DynamicArray() {
        reserve(count);
        for (size_t i = 0; i < count; i++)
            data_[size_++] = value;
    }

    ~DynamicArray() {
        delete[] data_;
    }

    DynamicArray(const DynamicArray& other) : DynamicArray() {
        reserve(other.size_);
        for (size_t i = 0; i < other.size_; i++)
            data_[size_++] = other.data_[i];
    }

    DynamicArray& operator=(const DynamicArray& other) {
        if (this != &other) {
            DynamicArray temp(other);
            swap(temp);
        }
        return *this;
    }

    void swap(DynamicArray& other) {
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    T& operator[](size_t i) { return data_[i]; }
    const T& operator[](size_t i) const { return data_[i]; }

    T& back() { return data_[size_ - 1]; }

    void pushBack(const T& value) {
        if (size_ == capacity_)
            reserve(capacity_ == 0 ? 4 : capacity_ * 2);
        data_[size_++] = value;
    }

    void pushBack(T&& value) {
        if (size_ == capacity_)
            reserve(capacity_ == 0 ? 4 : capacity_ * 2);
        data_[size_++] = std::move(value);
    }

    void clear() { size_ = 0; }

    void reserve(size_t capacity) {
        if (capacity <= capacity_) return;

        T* temp = new T[capacity];
        for (size_t i = 0; i < size_; i++)
            temp[i] = std::move(data_[i]);

        delete[] data_;
        data_ = temp;
        capacity_ = capacity;
    }

    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    T* begin() { return data_; }
    T* end() { return data_ + size_; }
    const T* begin() const { return data_; }
    const T* end() const { return data_ + size_; }

private:
    T* data_;
    size_t size_;
    size_t capacity_;
};

}