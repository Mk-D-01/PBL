#pragma once

#include <cstddef>
#include "DynamicArray.hpp"
#include "LinkedList.hpp"
#include "Hash.hpp"
using namespace std;

namespace ds {

// HashMap: hash table with separate chaining, built from scratch.
// - Each bucket is a linked list of (key, value) pairs.
// - The table doubles in size when the load factor goes above 0.75.
// - K needs ds::hashOf() and operator==.
template <typename K, typename V>
class HashMap {
    struct Pair {
        K key;
        V value;
        bool operator==(const Pair& other) const { return key == other.key; }
    };
    typedef LinkedList<Pair> Bucket;

    DynamicArray<Bucket> buckets_;
    size_t bucketCount_;
    size_t entryCount_;

    size_t indexOf(const K& key) const {
        return hashOf(key) % bucketCount_;
    }

    // if the table is too full, makes it twice as big and re-places every pair
    void maybeRehash() {
        if (loadFactor() <= 0.75)
            return;

        // 1. take all pairs out of the old buckets
        DynamicArray<Pair> entries;
        for (size_t i = 0; i < bucketCount_; i++) {
            for (Pair& pair : buckets_[i])
                entries.pushBack(pair);
        }

        // 2. build the bigger, empty table
        bucketCount_ = bucketCount_ * 2;
        buckets_ = DynamicArray<Bucket>(bucketCount_, Bucket());

        // 3. put every pair into its new bucket
        for (size_t i = 0; i < entries.size(); i++)
            buckets_[indexOf(entries[i].key)].pushBack(entries[i]);
    }

public:
    HashMap() : buckets_(8, Bucket()) {
        bucketCount_ = 8;
        entryCount_ = 0;
    }

    HashMap(size_t initialBuckets) : buckets_(initialBuckets == 0 ? 8 : initialBuckets, Bucket()) {
        bucketCount_ = (initialBuckets == 0) ? 8 : initialBuckets;
        entryCount_ = 0;
    }

    // (the default copy constructor / assignment copy all three members, which is what we want)

    // Returns a pointer to the value for key, or nullptr if the key is absent.
    V* find(const K& key) {
        Bucket& bucket = buckets_[indexOf(key)];
        for (Pair& pair : bucket) {
            if (pair.key == key)
                return &pair.value;
        }
        return nullptr;
    }

    const V* find(const K& key) const {
        const Bucket& bucket = buckets_[indexOf(key)];
        for (const Pair& pair : bucket) {
            if (pair.key == key)
                return &pair.value;
        }
        return nullptr;
    }

    bool contains(const K& key) const { return find(key) != nullptr; }

    // Adds (key, value); if the key already exists its value is replaced.
    void put(const K& key, const V& value) {
        V* existing = find(key);
        if (existing != nullptr) {
            *existing = value;
            return;
        }
        maybeRehash();
        buckets_[indexOf(key)].pushBack(Pair{key, value});
        entryCount_++;
    }

    // map[key]: gives the value, creating a default one if the key is new.
    V& operator[](const K& key) {
        V* existing = find(key);
        if (existing != nullptr)
            return *existing;
        maybeRehash();
        Bucket& bucket = buckets_[indexOf(key)];
        bucket.pushBack(Pair{key, V()});
        entryCount_++;
        return bucket.back().value;
    }

    bool remove(const K& key) {
        Bucket& bucket = buckets_[indexOf(key)];
        // Pairs compare equal when their keys match, so removeValue finds the right one
        if (bucket.removeValue(Pair{key, V()})) {
            entryCount_--;
            return true;
        }
        return false;
    }

    void clear() {
        for (size_t i = 0; i < bucketCount_; i++)
            buckets_[i].clear();
        entryCount_ = 0;
    }

    // all keys (in bucket order, not insertion order)
    DynamicArray<K> keys() const {
        DynamicArray<K> result;
        for (size_t i = 0; i < bucketCount_; i++) {
            for (const Pair& pair : buckets_[i])
                result.pushBack(pair.key);
        }
        return result;
    }

    // all values (same order as keys())
    DynamicArray<V> values() const {
        DynamicArray<V> result;
        for (size_t i = 0; i < bucketCount_; i++) {
            for (const Pair& pair : buckets_[i])
                result.pushBack(pair.value);
        }
        return result;
    }

    size_t size() const { return entryCount_; }
    bool empty() const { return entryCount_ == 0; }
    size_t bucketCount() const { return bucketCount_; }
    double loadFactor() const { return (double)entryCount_ / (double)bucketCount_; }
};

}  // namespace ds
