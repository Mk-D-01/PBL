#pragma once

#include <cstddef>
#include <stdexcept>
#include <utility>

namespace ds {

// Queue: FIFO container on singly-linked nodes, implemented from scratch.
template <typename T>
class Queue {
    struct Node {
        T value;
        Node* behind;
    };

public:
    Queue() : front_(nullptr), back_(nullptr), size_(0) {}

    Queue(const Queue& other) : Queue() { copyFrom(other); }

    Queue& operator=(const Queue& other) {
        if (this != &other) {
            Queue tmp(other);
            swap(tmp);
        }
        return *this;
    }

    Queue(Queue&& other) noexcept
        : front_(other.front_), back_(other.back_), size_(other.size_) {
        other.front_ = nullptr;
        other.back_ = nullptr;
        other.size_ = 0;
    }

    Queue& operator=(Queue&& other) noexcept {
        if (this != &other) {
            clear();
            front_ = other.front_;
            back_ = other.back_;
            size_ = other.size_;
            other.front_ = nullptr;
            other.back_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    ~Queue() { clear(); }

    void enqueue(const T& value) {
        Node* node = new Node{value, nullptr};
        if (back_ != nullptr)
            back_->behind = node;
        else
            front_ = node;
        back_ = node;
        ++size_;
    }

    void enqueue(T&& value) {
        Node* node = new Node{std::move(value), nullptr};
        if (back_ != nullptr)
            back_->behind = node;
        else
            front_ = node;
        back_ = node;
        ++size_;
    }

    void dequeue() {
        if (front_ == nullptr) throw std::out_of_range("Queue::dequeue on empty queue");
        Node* node = front_;
        front_ = node->behind;
        if (front_ == nullptr) back_ = nullptr;
        delete node;
        --size_;
    }

    T& peek() {
        if (front_ == nullptr) throw std::out_of_range("Queue::peek on empty queue");
        return front_->value;
    }
    const T& peek() const {
        if (front_ == nullptr) throw std::out_of_range("Queue::peek on empty queue");
        return front_->value;
    }

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    void clear() {
        while (!empty()) dequeue();
    }
    void swap(Queue& other) noexcept {
        std::swap(front_, other.front_);
        std::swap(back_, other.back_);
        std::swap(size_, other.size_);
    }

private:
    void copyFrom(const Queue& other) {
        for (Node* node = other.front_; node != nullptr; node = node->behind) enqueue(node->value);
    }

    Node* front_;
    Node* back_;
    std::size_t size_;
};

}  // namespace ds
