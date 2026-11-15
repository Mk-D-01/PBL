#pragma once

#include <cstddef>
using namespace std;

namespace ds {

// LinkedList: doubly linked list built from scratch with raw nodes.
// Note: call empty() before front()/back()/popFront()/popBack().
template <typename T>
class LinkedList {
    struct Node {
        T value;
        Node* prev;
        Node* next;
    };

    Node* head_;
    Node* tail_;
    size_t size_;

    // removes one node from the list (fixes the neighbours' pointers)
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
        size_--;
    }

public:
    LinkedList() {
        head_ = nullptr;
        tail_ = nullptr;
        size_ = 0;
    }

    // copy constructor
    LinkedList(const LinkedList& other) {
        head_ = nullptr;
        tail_ = nullptr;
        size_ = 0;
        for (Node* node = other.head_; node != nullptr; node = node->next)
            pushBack(node->value);
    }

    // copy assignment
    LinkedList& operator=(const LinkedList& other) {
        if (this != &other) {
            clear();
            for (Node* node = other.head_; node != nullptr; node = node->next)
                pushBack(node->value);
        }
        return *this;
    }

    ~LinkedList() {
        clear();
    }

    // ---- modifiers ----
    void pushBack(const T& value) {
        Node* node = new Node{value, tail_, nullptr};
        if (tail_ == nullptr)
            head_ = node;           // list was empty
        else
            tail_->next = node;
        tail_ = node;
        size_++;
    }

    void pushFront(const T& value) {
        Node* node = new Node{value, nullptr, head_};
        if (head_ == nullptr)
            tail_ = node;           // list was empty
        else
            head_->prev = node;
        head_ = node;
        size_++;
    }

    void popFront() {
        if (head_ != nullptr)
            unlink(head_);
    }

    void popBack() {
        if (tail_ != nullptr)
            unlink(tail_);
    }

    // removes the first element for which pred(element) is true
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
        for (Node* node = head_; node != nullptr; node = node->next) {
            if (node->value == value) {
                unlink(node);
                return true;
            }
        }
        return false;
    }

    void clear() {
        while (head_ != nullptr)
            unlink(head_);
    }

    // ---- element access ----
    T& front() { return head_->value; }
    const T& front() const { return head_->value; }
    T& back() { return tail_->value; }
    const T& back() const { return tail_->value; }

    // ---- queries ----
    // true if pred(element) is true for at least one element
    template <typename Pred>
    bool containsIf(Pred pred) const {
        for (Node* node = head_; node != nullptr; node = node->next) {
            if (pred(node->value))
                return true;
        }
        return false;
    }

    bool contains(const T& value) const {
        for (Node* node = head_; node != nullptr; node = node->next) {
            if (node->value == value)
                return true;
        }
        return false;
    }

    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    // ---- iteration (just enough for range-for loops) ----
    class Iterator {
        Node* node_;
    public:
        Iterator(Node* node) { node_ = node; }
        T& operator*() const { return node_->value; }
        Iterator& operator++() {
            node_ = node_->next;
            return *this;
        }
        bool operator!=(const Iterator& other) const { return node_ != other.node_; }
        bool operator==(const Iterator& other) const { return node_ == other.node_; }
    };

    class ConstIterator {
        const Node* node_;
    public:
        ConstIterator(const Node* node) { node_ = node; }
        const T& operator*() const { return node_->value; }
        ConstIterator& operator++() {
            node_ = node_->next;
            return *this;
        }
        bool operator!=(const ConstIterator& other) const { return node_ != other.node_; }
        bool operator==(const ConstIterator& other) const { return node_ == other.node_; }
    };

    Iterator begin() { return Iterator(head_); }
    Iterator end() { return Iterator(nullptr); }
    ConstIterator begin() const { return ConstIterator(head_); }
    ConstIterator end() const { return ConstIterator(nullptr); }
};

}  // namespace ds
