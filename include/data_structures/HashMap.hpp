#pragma once
#include <utility>
#include "DynamicArray.hpp"
#include "LinkedList.hpp"
#include "Hash.hpp"

namespace ds {

template <typename K, typename V>
class HashMap {
    struct Pair {
        K key;
        V value;
    };

    using Bucket = LinkedList<Pair>;

public:
    HashMap() : buckets_(8, Bucket()), count_(0) {}

    V* find(const K& key) {
        for (Pair& p : buckets_[ds::hashOf(key) % buckets_.size()])
            if (p.key == key) return &p.value;
        return nullptr;
    }

    bool contains(const K& key) const {
        return const_cast<HashMap*>(this)->find(key) != nullptr;
    }

    void put(const K& key, const V& value) {
        V* v = find(key);
        if (v) {
            *v = value;
            return;
        }

        if (count_ >= buckets_.size() * 3 / 4)
            rehash();

        buckets_[ds::hashOf(key) % buckets_.size()]
            .pushBack(Pair{key, value});
        ++count_;
    }

    V& operator[](const K& key) {
        V* v = find(key);
        if (v) return *v;

        put(key, V());
        return *find(key);
    }

    bool remove(const K& key) {
        auto& bucket = buckets_[ds::hashOf(key) % buckets_.size()];
        bool removed = bucket.removeFirstIf(
            [&key](const Pair& p) { return p.key == key; });

        if (removed) --count_;
        return removed;
    }

    void clear() {
        for (Bucket& b : buckets_) b.clear();
        count_ = 0;
    }

    ds::DynamicArray<K> keys() const {
        ds::DynamicArray<K> result;
        for (const Bucket& b : buckets_)
            for (const Pair& p : b) result.pushBack(p.key);
        return result;
    }

    ds::DynamicArray<V> values() const {
        ds::DynamicArray<V> result;
        for (const Bucket& b : buckets_)
            for (const Pair& p : b) result.pushBack(p.value);
        return result;
    }

    size_t size() const { return count_; }
    bool empty() const { return count_ == 0; }

private:
    void rehash() {
        ds::DynamicArray<Pair> entries;

        for (Bucket& b : buckets_)
            for (Pair& p : b) entries.pushBack(std::move(p));

        buckets_ = ds::DynamicArray<Bucket>(buckets_.size() * 2, Bucket());

        for (Pair& p : entries)
            buckets_[ds::hashOf(p.key) % buckets_.size()]
                .pushBack(std::move(p));
    }

    ds::DynamicArray<Bucket> buckets_;
    size_t count_;
};

}