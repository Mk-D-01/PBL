#pragma once

#include <cstddef>
#include <stdexcept>
#include <utility>

#include "DynamicArray.hpp"

namespace ds {

// Stack: LIFO container on singly-linked nodes, implemented from scratch.
template <typename T>
class Stack {
    struct Node {
        T value;
        Node* below;
    };

public:
    Stack() : top_(nullptr), size_(0) {}

    Stack(const Stack& other) : Stack() { copyFrom(other); }

    Stack& operator=(const Stack& other) {
        if (this != &other) {
            Stack tmp(other);
            swap(tmp);
        }
        return *this;
    }

    Stack(Stack&& other) noexcept : top_(other.top_), size_(other.size_) {
        other.top_ = nullptr;
        other.size_ = 0;
    }

    Stack& operator=(Stack&& other) noexcept {
        if (this != &other) {
            clear();
            top_ = other.top_;
            size_ = other.size_;
            other.top_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    ~Stack() { clear(); }

    void push(const T& value) {
        top_ = new Node{value, top_};
        ++size_;
    }
    void push(T&& value) {
        top_ = new Node{std::move(value), top_};
        ++size_;
    }

    void pop() {
        if (top_ == nullptr) throw std::out_of_range("Stack::pop on empty stack");
        Node* node = top_;
        top_ = node->below;
        delete node;
        --size_;
    }

    T& top() {
        if (top_ == nullptr) throw std::out_of_range("Stack::top on empty stack");
        return top_->value;
    }
    const T& top() const {
        if (top_ == nullptr) throw std::out_of_range("Stack::top on empty stack");
        return top_->value;
    }

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    void clear() {
        while (!empty()) pop();
    }
    void swap(Stack& other) noexcept {
        std::swap(top_, other.top_);
        std::swap(size_, other.size_);
    }

private:
    void copyFrom(const Stack& other) {
        // Walk top-to-bottom, then re-push in reverse so the order is preserved.
        ds::DynamicArray<T> values;
        for (Node* node = other.top_; node != nullptr; node = node->below) values.pushBack(node->value);
        for (std::size_t i = values.size(); i-- > 0;) push(values[i]);
    }

    Node* top_;
    std::size_t size_;
};

}  // namespace ds
