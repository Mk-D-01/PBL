#pragma once

#include <cstddef>
#include <stdexcept>
#include <utility>

namespace ds {

// LinkedList: doubly linked list implemented from scratch with raw nodes.
template <typename T>
class LinkedList {
    struct Node {
        T value;
        Node* prev;
        Node* next;
    };

public:
    LinkedList() : head_(nullptr), tail_(nullptr), size_(0) {}

    LinkedList(const LinkedList& other) : LinkedList() {
        for (Node* node = other.head_; node != nullptr; node = node->next) pushBack(node->value);
    }

    LinkedList& operator=(const LinkedList& other) {
        if (this != &other) {
            LinkedList tmp(other);
            swap(tmp);
        }
        return *this;
    }

    LinkedList(LinkedList&& other) noexcept
        : head_(other.head_), tail_(other.tail_), size_(other.size_) {
        other.head_ = nullptr;
        other.tail_ = nullptr;
        other.size_ = 0;
    }

    LinkedList& operator=(LinkedList&& other) noexcept {
        if (this != &other) {
            clear();
            head_ = other.head_;
            tail_ = other.tail_;
            size_ = other.size_;
            other.head_ = nullptr;
            other.tail_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    ~LinkedList() { clear(); }

    void swap(LinkedList& other) noexcept {
        std::swap(head_, other.head_);
        std::swap(tail_, other.tail_);
        std::swap(size_, other.size_);
    }

    // ---- modifiers ----
    void pushBack(const T& value) {
        Node* node = new Node{value, tail_, nullptr};
        if (tail_ != nullptr)
            tail_->next = node;
        else
            head_ = node;
        tail_ = node;
        ++size_;
    }

    void pushFront(const T& value) {
        Node* node = new Node{value, nullptr, head_};
        if (head_ != nullptr)
            head_->prev = node;
        else
            tail_ = node;
        head_ = node;
        ++size_;
    }

    void popFront() {
        if (head_ == nullptr) throw std::out_of_range("LinkedList::popFront on empty list");
        Node* node = head_;
        head_ = node->next;
        if (head_ != nullptr)
            head_->prev = nullptr;
        else
            tail_ = nullptr;
        delete node;
        --size_;
    }

    void popBack() {
        if (tail_ == nullptr) throw std::out_of_range("LinkedList::popBack on empty list");
        Node* node = tail_;
        tail_ = node->prev;
        if (tail_ != nullptr)
            tail_->next = nullptr;
        else
            head_ = nullptr;
        delete node;
        --size_;
    }

    template <typename Pred>
    bool removeFirstIf(Pred pred) {
        for (Node* node = head_; node != nullptr; node = node->next) {
            if (pred(node->value)) {
                unlink(node);
                return true;
            }
        }
        return false;
    }

    bool removeValue(const T& value) {
        return removeFirstIf([&value](const T& candidate) { return candidate == value; });
    }

    void clear() {
        Node* node = head_;
        while (node != nullptr) {
            Node* next = node->next;
            delete node;
            node = next;
        }
        head_ = nullptr;
        tail_ = nullptr;
        size_ = 0;
    }

    // ---- element access ----
    T& front() {
        if (head_ == nullptr) throw std::out_of_range("LinkedList::front on empty list");
        return head_->value;
    }
    const T& front() const {
        if (head_ == nullptr) throw std::out_of_range("LinkedList::front on empty list");
        return head_->value;
    }
    T& back() {
        if (tail_ == nullptr) throw std::out_of_range("LinkedList::back on empty list");
        return tail_->value;
    }
    const T& back() const {
        if (tail_ == nullptr) throw std::out_of_range("LinkedList::back on empty list");
        return tail_->value;
    }

    // ---- queries ----
    template <typename Pred>
    bool containsIf(Pred pred) const {
        for (Node* node = head_; node != nullptr; node = node->next)
            if (pred(node->value)) return true;
        return false;
    }

    bool contains(const T& value) const {
        return containsIf([&value](const T& candidate) { return candidate == value; });
    }

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    // ---- iteration ----
    class Iterator {
    public:
        explicit Iterator(Node* node) : node_(node) {}
        T& operator*() const { return node_->value; }
        Iterator& operator++() {
            node_ = node_->next;
            return *this;
        }
        bool operator!=(const Iterator& other) const { return node_ != other.node_; }
        bool operator==(const Iterator& other) const { return node_ == other.node_; }

    private:
        Node* node_;
    };

    class ConstIterator {
    public:
        explicit ConstIterator(const Node* node) : node_(node) {}
        const T& operator*() const { return node_->value; }
        ConstIterator& operator++() {
            node_ = node_->next;
            return *this;
        }
        bool operator!=(const ConstIterator& other) const { return node_ != other.node_; }
        bool operator==(const ConstIterator& other) const { return node_ == other.node_; }

    private:
        const Node* node_;
    };

    Iterator begin() { return Iterator(head_); }
    Iterator end() { return Iterator(nullptr); }
    ConstIterator begin() const { return ConstIterator(head_); }
    ConstIterator end() const { return ConstIterator(nullptr); }

private:
    void unlink(Node* node) {
        if (node->prev != nullptr)
            node->prev->next = node->next;
        else
            head_ = node->next;
        if (node->next != nullptr)
            node->next->prev = node->prev;
        else
            tail_ = node->prev;
        delete node;
        --size_;
    }

    Node* head_;
    Node* tail_;
    std::size_t size_;
};

}  // namespace ds
