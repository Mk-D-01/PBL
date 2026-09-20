#pragma once

#include <cstddef>
#include <stdexcept>
#include <utility>

#include "DynamicArray.hpp"
#include "LinkedList.hpp"
#include "Hash.hpp"

namespace ds {

// HashMap: separate-chaining hash table implemented from scratch.
// - Buckets are DynamicArray<LinkedList<Pair>>.
// - Resizes (rehashes) when load factor exceeds 0.75.
// - Key must be hashable by ds::hashOf and equality-comparable.
template <typename K, typename V>
class HashMap {
    struct Pair {
        K key;
        V value;
    };
    using Bucket = LinkedList<Pair>;

public:
    HashMap() : buckets_(8, Bucket()), bucketCount_(8), entryCount_(0) {}

    explicit HashMap(std::size_t initialBuckets)
        : buckets_(initialBuckets == 0 ? 8 : initialBuckets, Bucket()),
          bucketCount_(initialBuckets == 0 ? 8 : initialBuckets),
          entryCount_(0) {}

    HashMap(const HashMap& other)
        : buckets_(other.buckets_), bucketCount_(other.bucketCount_), entryCount_(other.entryCount_) {}

    HashMap& operator=(const HashMap& other) {
        if (this != &other) {
            HashMap tmp(other);
            swap(tmp);
        }
        return *this;
    }

    HashMap(HashMap&& other) noexcept
        : buckets_(std::move(other.buckets_)),
          bucketCount_(other.bucketCount_),
          entryCount_(other.entryCount_) {
        other.bucketCount_ = 8;
        other.entryCount_ = 0;
    }

    HashMap& operator=(HashMap&& other) noexcept {
        if (this != &other) {
            buckets_ = std::move(other.buckets_);
            bucketCount_ = other.bucketCount_;
            entryCount_ = other.entryCount_;
            other.bucketCount_ = 8;
            other.entryCount_ = 0;
        }
        return *this;
    }

    // ---- core operations ----

    // Returns pointer to the value for key, or nullptr when absent.
    V* find(const K& key) {
        Bucket& bucket = buckets_[indexOf(key)];
        for (Pair& pair : bucket)
            if (pair.key == key) return &pair.value;
        return nullptr;
    }
    const V* find(const K& key) const {
        const Bucket& bucket = buckets_[indexOf(key)];
        for (const Pair& pair : bucket)
            if (pair.key == key) return &pair.value;
        return nullptr;
    }

    bool contains(const K& key) const { return find(key) != nullptr; }

    // Inserts (key, value); overwrites the value when the key already exists.
    void put(const K& key, const V& value) {
        if (contains(key)) {
            *find(key) = value;
            return;
        }
        maybeRehash();
        buckets_[indexOf(key)].pushBack(Pair{key, value});
        ++entryCount_;
    }

    // Access-by-key with default construction on first use.
    V& operator[](const K& key) {
        V* found = find(key);
        if (found != nullptr) return *found;
        maybeRehash();
        Bucket& bucket = buckets_[indexOf(key)];
        bucket.pushBack(Pair{key, V()});
        ++entryCount_;
        return bucket.back().value;
    }

    bool remove(const K& key) {
        Bucket& bucket = buckets_[indexOf(key)];
        bool removed = bucket.removeFirstIf(
            [&key](const Pair& pair) { return pair.key == key; });
        if (removed) --entryCount_;
        return removed;
    }

    void clear() {
        for (Bucket& bucket : buckets_) bucket.clear();
        entryCount_ = 0;
    }

    // Collects all keys (order follows bucket layout, not insertion order).
    ds::DynamicArray<K> keys() const {
        ds::DynamicArray<K> result;
        for (const Bucket& bucket : buckets_)
            for (const Pair& pair : bucket) result.pushBack(pair.key);
        return result;
    }

    ds::DynamicArray<V> values() const {
        ds::DynamicArray<V> result;
        for (const Bucket& bucket : buckets_)
            for (const Pair& pair : bucket) result.pushBack(pair.value);
        return result;
    }

    std::size_t size() const { return entryCount_; }
    bool empty() const { return entryCount_ == 0; }
    std::size_t bucketCount() const { return bucketCount_; }
    double loadFactor() const {
        return bucketCount_ == 0 ? 0.0 : static_cast<double>(entryCount_) / static_cast<double>(bucketCount_);
    }

    void swap(HashMap& other) noexcept {
        buckets_.swap(other.buckets_);
        std::swap(bucketCount_, other.bucketCount_);
        std::swap(entryCount_, other.entryCount_);
    }

private:
    std::size_t indexOf(const K& key) const { return ds::hashOf(key) % bucketCount_; }

    void maybeRehash() {
        if (bucketCount_ == 0 || loadFactor() <= 0.75) return;
        ds::DynamicArray<Pair> entries;
        for (Bucket& bucket : buckets_)
            for (Pair& pair : bucket) entries.pushBack(std::move(pair));
        bucketCount_ *= 2;
        buckets_ = ds::DynamicArray<Bucket>(bucketCount_, Bucket());
        for (Pair& pair : entries) {
            buckets_[indexOf(pair.key)].pushBack(std::move(pair));
        }
    }

    ds::DynamicArray<Bucket> buckets_;
    std::size_t bucketCount_;
    std::size_t entryCount_;
};

}  // namespace ds
