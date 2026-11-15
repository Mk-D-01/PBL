#pragma once

#include <cstddef>
using namespace std;

namespace ds {

// Queue: FIFO (first in, first out) built on linked nodes.
// Note: call empty() before peek()/dequeue() - an empty queue has no front.
template <typename T>
class Queue {
    struct Node {
        T value;
        Node* behind;
    };

    Node* front_;
    Node* back_;
    size_t size_;

    void copyFrom(const Queue& other) {
        for (Node* node = other.front_; node != nullptr; node = node->behind)
            enqueue(node->value);
    }

public:
    Queue() {
        front_ = nullptr;
        back_ = nullptr;
        size_ = 0;
    }

    Queue(const Queue& other) {
        front_ = nullptr;
        back_ = nullptr;
        size_ = 0;
        copyFrom(other);
    }

    Queue& operator=(const Queue& other) {
        if (this != &other) {
            clear();
            copyFrom(other);
        }
        return *this;
    }

    ~Queue() {
        clear();
    }

    // adds at the back
    void enqueue(const T& value) {
        Node* node = new Node{value, nullptr};
        if (back_ == nullptr)
            front_ = node;          // queue was empty
        else
            back_->behind = node;
        back_ = node;
        size_++;
    }

    // removes from the front
    void dequeue() {
        if (front_ == nullptr)
            return;
        Node* node = front_;
        front_ = front_->behind;
        if (front_ == nullptr)
            back_ = nullptr;        // queue became empty
        delete node;
        size_--;
    }

    T& peek() { return front_->value; }
    const T& peek() const { return front_->value; }

    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    void clear() {
        while (!empty())
            dequeue();
    }
};

}  // namespace ds
