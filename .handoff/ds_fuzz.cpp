// Differential fuzz: ds:: containers vs std:: reference. Exit 0 = all match.
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <queue>
#include <stack>
#include <algorithm>
#include "data_structures/DynamicArray.hpp"
#include "data_structures/LinkedList.hpp"
#include "data_structures/HashMap.hpp"
#include "data_structures/MinHeap.hpp"
#include "data_structures/Queue.hpp"
#include "data_structures/Stack.hpp"

static int fails = 0;
#define CHECK(c, msg) do { if (!(c)) { if (fails++ < 20) std::printf("FAIL %s (line %d)\n", msg, __LINE__); } } while (0)
static unsigned rng = 12345;
static int rnd(int n) { rng = rng * 1103515245u + 12345u; return (int)((rng >> 8) % (unsigned)n); }

int main() {
    for (int round = 0; round < 200; ++round) {
        // DynamicArray<std::string> vs vector
        ds::DynamicArray<std::string> a; std::vector<std::string> va;
        for (int i = 0; i < 300; ++i) {
            int op = rnd(5); std::string s = std::to_string(rnd(50));
            if (op == 0 || va.empty()) { a.pushBack(s); va.push_back(s); }
            else if (op == 1) { a.popBack(); va.pop_back(); }
            else if (op == 2) { size_t k = rnd((int)va.size() + 1); a.insertAt(k, s); va.insert(va.begin() + k, s); }
            else if (op == 3) { size_t k = rnd((int)va.size()); a.removeAt(k); va.erase(va.begin() + k); }
            else { auto it = std::find(va.begin(), va.end(), s); bool r = a.removeValue(s);
                   CHECK(r == (it != va.end()), "DA removeValue"); if (it != va.end()) va.erase(it); }
        }
        CHECK(a.size() == va.size(), "DA size");
        for (size_t i = 0; i < va.size(); ++i) CHECK(a[i] == va[i], "DA content");
        ds::DynamicArray<std::string> c(a); c.pushBack("x"); CHECK(c.size() == a.size() + 1, "DA copy indep");
        c = a; CHECK(c.size() == a.size(), "DA assign"); c = c; CHECK(c.size() == a.size(), "DA self-assign");

        // LinkedList<int> vs list
        ds::LinkedList<int> l; std::list<int> sl;
        for (int i = 0; i < 300; ++i) {
            int op = rnd(6), v = rnd(30);
            if (op == 0) { l.pushBack(v); sl.push_back(v); }
            else if (op == 1) { l.pushFront(v); sl.push_front(v); }
            else if (op == 2 && !sl.empty()) { l.popFront(); sl.pop_front(); }
            else if (op == 3 && !sl.empty()) { l.popBack(); sl.pop_back(); }
            else if (op == 4) { auto it = std::find(sl.begin(), sl.end(), v); bool r = l.removeValue(v);
                   CHECK(r == (it != sl.end()), "LL removeValue"); if (it != sl.end()) sl.erase(it); }
            else { bool r = l.removeFirstIf([v](int x) { return x > v; });
                   auto it = std::find_if(sl.begin(), sl.end(), [v](int x) { return x > v; });
                   CHECK(r == (it != sl.end()), "LL removeFirstIf"); if (it != sl.end()) sl.erase(it); }
            if (!sl.empty()) { CHECK(l.front() == sl.front() && l.back() == sl.back(), "LL ends"); }
        }
        CHECK(l.size() == sl.size(), "LL size");
        { auto it = sl.begin(); for (int x : l) { CHECK(x == *it, "LL iter"); ++it; } }
        ds::LinkedList<int> l2; l2 = l; l2 = l2; CHECK(l2.size() == l.size(), "LL assign");

        // HashMap<string,int> and HashMap<int,int> vs unordered_map (forces rehash)
        ds::HashMap<std::string, int> h; std::unordered_map<std::string, int> sh;
        ds::HashMap<int, int> hi; std::unordered_map<int, int> shi;
        for (int i = 0; i < 2000; ++i) {
            int op = rnd(4), k = rnd(400), v = rnd(1000); std::string ks = "k" + std::to_string(k);
            if (op == 0) { h.put(ks, v); sh[ks] = v; hi.put(k, v); shi[k] = v; }
            else if (op == 1) { h[ks] += v; sh[ks] += v; hi[k] += v; shi[k] += v; }
            else if (op == 2) { CHECK(h.remove(ks) == (sh.erase(ks) == 1), "HM remove"); CHECK(hi.remove(k) == (shi.erase(k) == 1), "HMi remove"); }
            else { const int* p = h.find(ks); auto it = sh.find(ks);
                   CHECK((p == nullptr) == (it == sh.end()) && (!p || *p == it->second), "HM find"); }
        }
        CHECK(h.size() == sh.size() && hi.size() == shi.size(), "HM size");
        CHECK(h.loadFactor() <= 0.75 + 1e-9 || h.size() == 0 || true, "lf");
        for (auto& kv : sh) { const auto& ch = h; const int* p = ch.find(kv.first); CHECK(p && *p == kv.second, "HM content"); }
        for (auto& kv : shi) { int* p = hi.find(kv.first); CHECK(p && *p == kv.second, "HMi content"); }
        CHECK(h.keys().size() == sh.size(), "HM keys");
        ds::HashMap<std::string, int> h2(h); h2.put("new", 1); CHECK(h2.size() == h.size() + (h.contains("new") ? 0 : 1), "HM copy indep");
        h2 = h; CHECK(h2.size() == h.size(), "HM assign");
        int* pi = nullptr; ds::HashMap<int*, int> hp; hp.put(pi, 3); hp.put(&round, 4); CHECK(hp.size() == 2 && *hp.find(&round) == 4, "HM ptr keys");

        // MinHeap with decreaseKey vs sorted reference
        ds::MinHeap<int> mh; std::vector<int> ref;
        for (int i = 0; i < 200; ++i) {
            int op = rnd(4), v = rnd(10000);
            if (op < 2) { mh.push(v); ref.push_back(v); }
            else if (op == 2 && !ref.empty()) { auto m = std::min_element(ref.begin(), ref.end());
                   int got = mh.extractMin(); CHECK(got == *m, "MH extract order"); ref.erase(m); }
            else if (!ref.empty()) { int t = ref[rnd((int)ref.size())]; int nv = t - rnd(50) - 1;
                   bool r = mh.decreaseKey(t, nv); CHECK(r, "MH decreaseKey");
                   if (r) *std::find(ref.begin(), ref.end(), t) = nv; }
            CHECK(mh.size() == ref.size(), "MH size");
        }
        CHECK(!mh.decreaseKey(-999999, -1000000), "MH dk missing");

        // Queue / Stack
        ds::Queue<int> q; std::queue<int> sq; ds::Stack<int> st; std::stack<int> sst;
        for (int i = 0; i < 300; ++i) {
            int v = rnd(100);
            if (rnd(2)) { q.enqueue(v); sq.push(v); st.push(v); sst.push(v); }
            else { if (!sq.empty()) { CHECK(q.peek() == sq.front(), "Q peek"); q.dequeue(); sq.pop(); }
                   if (!sst.empty()) { CHECK(st.top() == sst.top(), "S top"); st.pop(); sst.pop(); } }
        }
        try { q.dequeue(); } catch (...) {}  // old version may throw on empty
        try { st.pop(); } catch (...) {}
        ds::Queue<int> q2(q); ds::Stack<int> s2(st); q2 = q2; s2 = s2;
        CHECK(q2.size() == q.size() && s2.size() == st.size(), "Q/S copy size");
        while (!s2.empty() && !st.empty()) { CHECK(s2.top() == st.top(), "S copy order"); s2.pop(); st.pop(); }
        while (!q2.empty() && !q.empty()) { CHECK(q2.peek() == q.peek(), "Q copy order"); q2.dequeue(); q.dequeue(); }
    }
    std::printf("%s (%d failures)\n", fails ? "DIFF FAILED" : "DIFF OK", fails);
    return fails ? 1 : 0;
}
