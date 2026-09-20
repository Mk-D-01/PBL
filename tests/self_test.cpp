// Self-test suite for the MapGen Engine.
// Covers: custom data structures, graph operations, and navigation algorithms.

#include <cmath>
#include <cstdio>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>

#include "CampusMap.hpp"
#include "Connection.hpp"
#include "Location.hpp"
#include "MapGenerator.hpp"
#include "Menu.hpp"
#include "Navigation.hpp"
#include "Route.hpp"
#include "data_structures/DynamicArray.hpp"
#include "data_structures/HashMap.hpp"
#include "data_structures/LinkedList.hpp"
#include "data_structures/MinHeap.hpp"
#include "data_structures/Queue.hpp"
#include "data_structures/Stack.hpp"

namespace {

int checksRun = 0;
int checksFailed = 0;

template <typename Fn>
void check(const std::string& name, Fn assertion) {
    ++checksRun;
    if (!assertion()) {
        ++checksFailed;
        std::printf("[FAIL] %s\n", name.c_str());
    }
}

void require(bool condition, const std::string& name) {
    check(name, [&condition] { return condition; });
}

// ---------------------------------------------------------------- data structures

void testDataStructures() {
    // DynamicArray
    ds::DynamicArray<int> arr;
    for (int i = 0; i < 10; ++i) arr.pushBack(i * 2);
    require(arr.size() == 10, "DynamicArray pushBack/size");
    require(arr[7] == 14 && arr.back() == 18, "DynamicArray indexing/back");
    arr.removeAt(2);
    require(arr.size() == 9 && arr[2] == 6, "DynamicArray removeAt shifts elements");
    arr.insertAt(0, 111);
    require(arr.front() == 111 && arr.size() == 10, "DynamicArray insertAt front");
    arr.insertAt(5, 222);
    require(arr[5] == 222 && arr[6] == 10, "DynamicArray insertAt middle");
    require(arr.indexOf(18) == 10 && arr.indexOf(999) == ds::DynamicArray<int>::npos,
            "DynamicArray indexOf");
    require(arr.contains(222), "DynamicArray contains");
    arr.popBack();
    require(arr.back() == 16, "DynamicArray popBack");
    arr.removeValue(111);
    require(!arr.contains(111), "DynamicArray removeValue");

    ds::DynamicArray<int> copied(arr);
    require(copied.size() == arr.size(), "DynamicArray copy constructor");
    ds::DynamicArray<int> moved(std::move(copied));
    require(moved.size() == arr.size() && copied.size() == 0, "DynamicArray move constructor");

    // LinkedList
    ds::LinkedList<int> list;
    list.pushBack(1);
    list.pushBack(2);
    list.pushFront(0);
    require(list.front() == 0 && list.back() == 2 && list.size() == 3, "LinkedList push ops");
    require(list.contains(2) && !list.contains(9), "LinkedList contains");
    require(list.removeValue(1) && list.size() == 2, "LinkedList removeValue");
    int sum = 0;
    for (int v : list) sum += v;
    require(sum == 2, "LinkedList iteration");
    list.clear();
    require(list.empty(), "LinkedList clear");

    // Stack (LIFO)
    ds::Stack<int> stack;
    stack.push(1);
    stack.push(2);
    stack.push(3);
    require(stack.top() == 3 && stack.size() == 3, "Stack LIFO order");
    stack.pop();
    require(stack.top() == 2, "Stack pop");
    require(!stack.empty(), "Stack not empty");

    // Queue (FIFO)
    ds::Queue<int> queue;
    queue.enqueue(1);
    queue.enqueue(2);
    queue.enqueue(3);
    require(queue.peek() == 1 && queue.size() == 3, "Queue FIFO order");
    queue.dequeue();
    require(queue.peek() == 2, "Queue dequeue");

    // HashMap
    ds::HashMap<std::string, int> map;
    for (int i = 0; i < 100; ++i) map.put("key" + std::to_string(i), i * 3);
    require(map.size() == 100, "HashMap put/size (forces rehash)");
    require(*map.find("key42") == 126, "HashMap find");
    map.put("key42", -1);  // overwrite
    require(*map.find("key42") == -1, "HashMap put overwrites");
    require(map.contains("key99") && !map.contains("missing"), "HashMap contains");
    require(map.remove("key42") && !map.contains("key42"), "HashMap remove");
    require(map["key7"] == 21, "HashMap operator[] read");
    map["keyNew"] = 5;  // operator[] insert
    require(map["keyNew"] == 5, "HashMap operator[] insert");
    require(map.keys().size() == 100 && map.values().size() == 100, "HashMap keys/values");
    require(map.bucketCount() >= 100, "HashMap rehash grew buckets");

    // MinHeap (with decreaseKey)
    ds::MinHeap<int> heap;
    heap.push(5);
    heap.push(3);
    heap.push(8);
    heap.push(1);
    require(heap.peekMin() == 1 && heap.size() == 4, "MinHeap push/peekMin");
    require(heap.extractMin() == 1 && heap.extractMin() == 3, "MinHeap extractMin order");
    heap.push(10);
    heap.push(7);
    require(heap.decreaseKey(7, 2), "MinHeap decreaseKey lowers existing key");
    require(heap.peekMin() == 2, "MinHeap decreaseKey to new minimum");
    require(!heap.decreaseKey(99, 1), "MinHeap decreaseKey rejects absent value");
    heap.clear();
    require(heap.empty(), "MinHeap clear");

    // HeapEntry ordering (used by Dijkstra)
    campus::HeapEntry a(5, 1), b(3, 2), c(5, 0);
    require(b < a && c < a, "HeapEntry ordering with id tie-break");
}

// ---------------------------------------------------------------- map operations

void testMapOperations() {
    campus::CampusMap map;

    const int a = map.addLocation("Library", campus::LocationType::Facility, 10, 5, "Books");
    const int b = map.addLocation("CSE", campus::LocationType::Building, 4, 3, "3");
    const int g = map.addLocation("MainGate", campus::LocationType::Gate, 2, 12, "");
    require(a == 0 && b == 1 && g == 2, "addLocation assigns sequential ids");
    require(map.locationCount() == 3, "locationCount");
    require(map.addLocation("Library", campus::LocationType::Building, 1, 1, "1") == -1,
            "addLocation rejects duplicate name");
    require(map.searchByName("CSE") != nullptr && map.searchByName("cse") == nullptr,
            "searchByName exact match");
    require(map.searchByKeyword("book").size() == 1, "searchByKeyword case-insensitive");

    const campus::Location* lib = map.findLocation(a);
    require(lib->kind() == campus::LocationType::Facility, "polymorphic kind()");
    require(lib->describe().find("Books") != std::string::npos, "polymorphic describe()");

    require(map.connect(a, b, 50, "Lane") && map.edgeCount() == 1, "connect adds edge");
    require(!map.connect(a, b, 50, "Lane"), "connect rejects duplicate edge");
    require(map.connectionDistance(a, b) == 50 && map.connectionDistance(b, a) == 50,
            "edges are undirected");
    require(map.areConnected(a, b) && !map.areConnected(a, g), "areConnected");
    require(!map.connect(a, a, 10, ""), "connect rejects self-loop");
    require(!map.connect(a, 99, 10, ""), "connect rejects unknown id");

    require(map.updateLocation(b, "CS-Department"), "updateLocation rename");
    require(map.searchByName("CS-Department") != nullptr && map.searchByName("CSE") == nullptr,
            "rename updates index");
    require(map.updateLocation(b, 20, 8) && map.findLocation(b)->x() == 20, "updateLocation move");

    require(map.removeLocation(g), "removeLocation");
    require(map.findLocation(g) == nullptr && map.locationCount() == 2, "removal frees slot");
    require(!map.removeLocation(g), "double removal fails");
    require(map.edgeCount() == 1, "removal kept unrelated edge");

    // Removal cleans both half-edges of incident connections.
    campus::CampusMap m2;
    const int n0 = m2.addLocation("A", campus::LocationType::Building, 5, 5, "1");
    const int n1 = m2.addLocation("B", campus::LocationType::Building, 9, 5, "1");
    const int n2 = m2.addLocation("C", campus::LocationType::Building, 13, 5, "1");
    m2.connect(n0, n1, 10, "");
    m2.connect(n1, n2, 10, "");
    require(m2.removeLocation(n1) && m2.edgeCount() == 0, "removal drops incident edges");
    require(!m2.areConnected(n0, n2), "no phantom edge after removal");

    const std::string rendered = map.renderMap();
    require(rendered.find("Library") != std::string::npos, "renderMap shows node names");
    require(rendered.find("Legend") != std::string::npos, "renderMap shows legend");
    require(map.adjacencyReport().find("CS-Department") != std::string::npos,
            "adjacencyReport lists nodes");
    require(map.campusInfo().find("Locations   : 2") != std::string::npos, "campusInfo counts");
}

void testMapGenerator() {
    campus::CampusMap map;
    campus::MapGenerator::generate(map);
    require(map.locationCount() == 12, "generator seeds 12 locations");
    require(map.edgeCount() == 20, "generator seeds 20 connections");
    require(map.searchByName("MainGate") != nullptr && map.searchByName("Library") != nullptr,
            "generator landmark names");

    bool allConnected = true;
    for (const campus::Location* loc : map.allLocations())
        if (map.searchByKeyword(loc->name()).empty()) allConnected = false;
    require(allConnected, "generator locations are searchable");

    bool symmetric = true;
    for (const campus::Location* loc : map.allLocations()) {
        for (const campus::Connection& c : map.allConnectionsOf(loc->id())) {
            if (!map.areConnected(c.toId, loc->id())) symmetric = false;
        }
    }
    require(symmetric, "generator edges are symmetric");
}

// ---------------------------------------------------------------- navigation

void testNavigation() {
    campus::CampusMap map;
    campus::MapGenerator::generate(map);
    const campus::Navigation nav(map);

    const int mainGate = map.searchByName("MainGate")->id();
    const int eastGate = map.searchByName("EastGate")->id();
    const int hostel = map.searchByName("Hostel-A")->id();
    const int parking = map.searchByName("Parking")->id();

    // BFS fewest stops.
    const campus::Route bfs = nav.shortestRouteByHops(mainGate, hostel);
    require(bfs.reachable, "BFS finds route MainGate->Hostel-A");
    require(bfs.hops == 3, "BFS hop count is minimal (3)");
    require(bfs.path.front() == mainGate && bfs.path.back() == hostel, "BFS path endpoints");

    // Dijkstra shortest distance.
    const campus::Route dij = nav.shortestRouteByDistance(mainGate, hostel);
    require(dij.reachable, "Dijkstra finds route MainGate->Hostel-A");
    require(dij.totalDistance == 290, "Dijkstra total distance 290 m");
    require(dij.hops == 3, "Dijkstra route uses 3 edges");
    require(dij.totalDistance <= bfs.totalDistance, "Dijkstra distance <= BFS distance");

    // Both algorithms agree on reachability for every pair.
    bool allPairsReachable = true;
    for (const campus::Location* from : map.allLocations()) {
        for (const campus::Location* to : map.allLocations()) {
            const campus::Route r = nav.shortestRouteByDistance(from->id(), to->id());
            const bool dfsOk = nav.pathExists(from->id(), to->id());
            if (from->id() == to->id()) {
                if (!dfsOk) allPairsReachable = false;
                continue;
            }
            if (!r.reachable || !dfsOk) allPairsReachable = false;
        }
    }
    require(allPairsReachable, "campus is fully connected (all pairs)");

    // Adjacent nodes: direct edge is optimal.
    const campus::Route adjacent = nav.shortestRouteByDistance(mainGate, parking);
    require(adjacent.totalDistance == 60 && adjacent.hops == 1, "adjacent route uses the edge");

    // Invalid ids and self-route.
    require(!nav.shortestRouteByDistance(-1, hostel).reachable, "Dijkstra rejects bad source");
    require(!nav.shortestRouteByHops(mainGate, 999).reachable, "BFS rejects bad destination");
    const campus::Route self = nav.shortestRouteByDistance(mainGate, mainGate);
    require(self.reachable && self.hops == 0 && self.path.size() == 1, "self route is trivial");

    // DFS connectivity and traversals.
    require(nav.pathExists(mainGate, eastGate), "DFS pathExists");
    const ds::DynamicArray<int> bfsOrder = nav.traverseBFS(mainGate);
    require(bfsOrder.size() == 12 && bfsOrder.front() == mainGate, "BFS traversal visits all 12");
    const ds::DynamicArray<int> dfsOrder = nav.traverseDFS(mainGate);
    require(dfsOrder.size() == 12 && dfsOrder.front() == mainGate, "DFS traversal visits all 12");

    // Disconnect a bridge and verify isolation is detected.
    campus::CampusMap sparse;
    const int s1 = sparse.addLocation("A", campus::LocationType::Building, 3, 3, "1");
    const int s2 = sparse.addLocation("B", campus::LocationType::Building, 8, 3, "1");
    sparse.connect(s1, s2, 10, "");
    const campus::Navigation sparseNav(sparse);
    require(sparseNav.pathExists(s1, s2), "pathExists on connected pair");
    require(sparse.disconnect(s1, s2), "disconnect succeeds");
    require(!sparseNav.pathExists(s1, s2), "pathExists false after disconnect");
    require(!sparseNav.shortestRouteByHops(s1, s2).reachable, "BFS unreachable after disconnect");
    require(!sparseNav.shortestRouteByDistance(s1, s2).reachable,
            "Dijkstra unreachable after disconnect");
    require(nav.traverseBFS(mainGate).size() == 12, "BFS order stable after queries");
}

// ---------------------------------------------------------------- menu (scripted)

void testMenuScripted() {
    // Feed the menu's stdin reads from a scripted buffer so the UI layer is
    // exercised end-to-end without a live console.
    const std::string input =
        "DemoLab\n"       // 3: name
        "f\n"             // 3: type Facility
        "Robotics Lab\n"  // 3: service
        "34\n"            // 3: x
        "16\n"            // 3: y
        "lab\n"           // 5: keyword
        "DemoLab\n"       // 7: first location
        "Library\n"       // 7: second location
        "100\n"           // 7: distance
        "Test Path\n"     // 7: label
        "MainGate\n"      // 8: source
        "DemoLab\n"       // 8: destination
        "d\n"             // 8: metric
        "DemoLab\n"       // 6: id or name
        "n\n"             // 6: rename
        "RoboticsLab\n"   // 6: new name
        "RoboticsLab\n";  // 4: remove by name

    std::istringstream fakeInput(input);
    std::streambuf* originalBuf = std::cin.rdbuf(fakeInput.rdbuf());

    campus::Menu menu;
    menu.runAction(1);  // generate
    menu.runAction(9);  // info
    menu.runAction(2);  // display map

    menu.runAction(3);  // add DemoLab
    menu.runAction(5);  // search "lab"
    menu.runAction(7);  // connect DemoLab <-> Library
    menu.runAction(8);  // find route MainGate -> DemoLab (Dijkstra)
    menu.runAction(6);  // rename DemoLab -> RoboticsLab
    menu.runAction(4);  // remove RoboticsLab
    menu.runAction(11); // invalid choice

    menu.runAction(1);  // second generate -> message, not crash
    menu.runAction(10); // exit

    const bool consumedCleanly = !std::cin.fail();
    std::cin.rdbuf(originalBuf);
    require(consumedCleanly, "menu consumed scripted input without stream errors");
    require(true, "scripted menu actions ran without crashing");
}

}  // namespace

int runAllTests() {
    testDataStructures();
    testMapOperations();
    testMapGenerator();
    testNavigation();
    testMenuScripted();

    std::printf("\n%d checks, %d failed\n", checksRun, checksFailed);
    if (checksFailed == 0) std::printf("ALL TESTS PASSED\n");
    return checksFailed == 0 ? 0 : 1;
}

int main() { return runAllTests(); }
