#pragma once

#include <cstddef>
#include "DynamicArray.hpp"
using namespace std;

namespace ds {

// Stack: LIFO (last in, first out) built on linked nodes.
// Note: call empty() before top()/pop() - an empty stack has no top.
template <typename T>
class Stack {
    struct Node {
        T value;
        Node* below;
    };

    Node* top_;
    size_t size_;

    // copies other into this (empty) stack, keeping the same order
    void copyFrom(const Stack& other) {
        // collect top-to-bottom, then push in reverse so the order is the same
        DynamicArray<T> values;
        for (Node* node = other.top_; node != nullptr; node = node->below)
            values.pushBack(node->value);
        for (size_t i = values.size(); i > 0; i--)
            push(values[i - 1]);
    }

public:
    Stack() {
        top_ = nullptr;
        size_ = 0;
    }

    Stack(const Stack& other) {
        top_ = nullptr;
        size_ = 0;
        copyFrom(other);
    }

    Stack& operator=(const Stack& other) {
        if (this != &other) {
            clear();
            copyFrom(other);
        }
        return *this;
    }

    ~Stack() {
        clear();
    }

    void push(const T& value) {
        Node* node = new Node{value, top_};
        top_ = node;
        size_++;
    }

    void pop() {
        if (top_ == nullptr)
            return;
        Node* node = top_;
        top_ = top_->below;
        delete node;
        size_--;
    }

    T& top() { return top_->value; }
    const T& top() const { return top_->value; }

    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    void clear() {
        while (!empty())
            pop();
    }
};

}  // namespace ds
