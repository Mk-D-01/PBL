#pragma once
#include <cstddef>
#include <utility>

namespace ds {

template <typename T>
class LinkedList {
    struct Node {
        T value;
        Node* prev;
        Node* next;

        Node(const T& v, Node* p, Node* n)
            : value(v), prev(p), next(n) {}
    };

public:
    LinkedList() : head_(nullptr), tail_(nullptr), size_(0) {}

    ~LinkedList() { clear(); }

    LinkedList(const LinkedList& other) : LinkedList() {
        for (const T& value : other) pushBack(value);
    }

    LinkedList& operator=(const LinkedList& other) {
        if (this != &other) {
            clear();
            for (const T& value : other) pushBack(value);
        }
        return *this;
    }

    LinkedList(LinkedList&& other) noexcept
        : head_(other.head_), tail_(other.tail_), size_(other.size_) {
        other.head_ = other.tail_ = nullptr;
        other.size_ = 0;
    }

    LinkedList& operator=(LinkedList&& other) noexcept {
        if (this != &other) {
            clear();
            head_ = other.head_;
            tail_ = other.tail_;
            size_ = other.size_;
            other.head_ = other.tail_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    void pushBack(const T& value) {
        Node* node = new Node(value, tail_, nullptr);
        if (tail_) tail_->next = node;
        else head_ = node;
        tail_ = node;
        ++size_;
    }

    void pushFront(const T& value) {
        Node* node = new Node(value, nullptr, head_);
        if (head_) head_->prev = node;
        else tail_ = node;
        head_ = node;
        ++size_;
    }

    void popFront() {
        if (!head_) return;
        Node* node = head_;
        head_ = head_->next;
        if (head_) head_->prev = nullptr;
        else tail_ = nullptr;
        delete node;
        --size_;
    }

    void popBack() {
        if (!tail_) return;
        Node* node = tail_;
        tail_ = tail_->prev;
        if (tail_) tail_->next = nullptr;
        else head_ = nullptr;
        delete node;
        --size_;
    }

    template <typename Pred>
    bool removeFirstIf(Pred pred) {
        for (Node* node = head_; node; node = node->next) {
            if (pred(node->value)) {
                if (node->prev) node->prev->next = node->next;
                else head_ = node->next;

                if (node->next) node->