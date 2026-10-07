// Web API self-test suite.
// Builds the HTTP layer in-process and drives it through real loopback
// sockets: login/whoami, static files, map/search/route correctness,
// role guards (401/403), and admin CRUD round-trips.

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>

#include "web/ApiController.hpp"
#include "web/HttpServer.hpp"
#include "web/Json.hpp"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif
using RawSocket = SOCKET;
#define BAD_SOCK INVALID_SOCKET
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using RawSocket = int;
#define BAD_SOCK (-1)
#endif

static int gChecks = 0;
static int gFailures = 0;

#define CHECK(cond, name)                                                        \
    do {                                                                         \
        ++gChecks;                                                               \
        if (!(cond)) {                                                           \
            ++gFailures;                                                         \
            std::printf("  FAIL %s (line %d)\n", name, __LINE__);                \
        }                                                                        \
    } while (0)

namespace {

constexpr int kPort = 18099;  // test-only port

// Sends one raw request and returns the FULL raw response (headers + body).
std::string sendRequest(const std::string& rawRequest, std::string& statusLine) {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
    RawSocket sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock == BAD_SOCK) return "";

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = ::htons(kPort);
    addr.sin_addr.s_addr = ::htonl(INADDR_LOOPBACK);
    if (::connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        ::closesocket(sock);
        return "";
    }
    ::send(sock, rawRequest.c_str(), static_cast<int>(rawRequest.size()), 0);

    std::string response;
    char buf[4096];
    while (true) {
        int n = ::recv(sock, buf, sizeof(buf), 0);
        if (n <= 0) break;
        response.append(buf, static_cast<std::size_t>(n));
    }
    ::closesocket(sock);

    std::size_t firstLineEnd = response.find("\r\n");
    statusLine = firstLineEnd == std::string::npos ? "" : response.substr(0, firstLineEnd);
    return response;
}

// Performs one request and returns the FULL raw response (headers + body).
// Use bodyOf() when only the payload is needed.
std::string http(const std::string& method, const std::string& target,
                 const std::string& body = "", const std::string& cookie = "",
                 std::string* statusLineOut = nullptr) {
    std::string req = method + " " + target + " HTTP/1.1\r\nHost: localhost\r\n";
    if (!cookie.empty()) req += "Cookie: " + cookie + "\r\n";
    if (!body.empty()) {
        req += "Content-Type: application/json\r\n";
        req += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    }
    req += "Connection: close\r\n\r\n" + body;

    std::string statusLine;
    std::string full = sendRequest(req, statusLine);
    if (statusLineOut != nullptr) *statusLineOut = statusLine;
    return full;
}

std::string bodyOf(const std::string& fullResponse) {
    std::size_t bodyStart = fullResponse.find("\r\n\r\n");
    return bodyStart == std::string::npos ? "" : fullResponse.substr(bodyStart + 4);
}

std::string cookieValue(const std::string& setCookieHeader) {
    // setCookieHeader looks like "session=tok-...; Path=/; HttpOnly"
    std::size_t end = setCookieHeader.find(';');
    return end == std::string::npos ? setCookieHeader : setCookieHeader.substr(0, end);
}

std::string headerValue(const std::string& response, const std::string& name) {
    std::string needle = name + ": ";
    std::size_t pos = response.find(needle);
    if (pos == std::string::npos) return "";
    std::size_t end = response.find("\r\n", pos);
    return response.substr(pos + needle.size(), end - pos - needle.size());
}

}  // namespace

int main() {
    // Run the server on a background thread, like a real deployment.
    // Saved maps go to a throw-away directory so the tests never touch the real maps/.
    const std::string kMapsDir = "maps_test_tmp";
    std::filesystem::remove_all(kMapsDir);
    web::ApiController controller(kMapsDir);
    web::HttpServer server(kPort, "web",
                           [&controller](const web::HttpRequest& request) {
                               return controller.handle(request);
                           });
    std::thread serverThread([&server] { server.run(); });
    serverThread.detach();  // run() loops forever; detached on purpose

    // Wait for the listener to come up.
    std::string statusLine;
    for (int i = 0; i < 50; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        (void)sendRequest("GET /api/whoami HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n", statusLine);
        if (!statusLine.empty()) break;
    }
    CHECK(!statusLine.empty(), "server comes up");

    // ---- static front end ----
    std::string index = bodyOf(http("GET", "/", "", "", &statusLine));
    CHECK(statusLine.find("200") != std::string::npos, "GET / serves index.html");
    CHECK(index.find("MapGen Engine") != std::string::npos, "index contains app title");
    std::string appJs = bodyOf(http("GET", "/app.js", "", "", &statusLine));
    CHECK(statusLine.find("200") != std::string::npos, "GET /app.js serves javascript");
    CHECK(appJs.find("api(") != std::string::npos || !appJs.empty(), "app.js body is non-empty");
    http("GET", "/nope.html", "", "", &statusLine);
    CHECK(statusLine.find("404") != std::string::npos, "unknown static file is 404");

    // ---- auth ----
    std::string resp = http("POST", "/login", R"({"username":"user","password":"user123"})");
    std::string userCookie = cookieValue(headerValue(resp, "Set-Cookie"));
    CHECK(!userCookie.empty(), "user login sets session cookie");

    resp = http("POST", "/login", R"({"username":"admin","password":"admin123"})");
    std::string adminCookie = cookieValue(headerValue(resp, "Set-Cookie"));
    CHECK(!adminCookie.empty(), "admin login sets session cookie");

    resp = bodyOf(http("POST", "/login", R"({"username":"user","password":"wrong"})"));
    CHECK(resp.find("error") != std::string::npos, "bad credentials rejected");

    resp = bodyOf(http("GET", "/api/whoami", "", userCookie));
    CHECK(resp.find("\"user\"") != std::string::npos, "whoami reports user role");
    resp = bodyOf(http("GET", "/api/whoami", "", adminCookie));
    CHECK(resp.find("\"admin\"") != std::string::npos, "whoami reports admin role");
    resp = bodyOf(http("GET", "/api/whoami"));
    CHECK(resp.find("guest") != std::string::npos, "whoami without cookie is guest");
    // Regression: browsers send "a=1; session=..." (cookies on localhost are shared across ports).
    resp = bodyOf(http("GET", "/api/whoami", "", "other=1; " + adminCookie));
    CHECK(resp.find("\"admin\"") != std::string::npos, "session cookie found after another cookie");

    // ---- read-only endpoints ----
    resp = bodyOf(http("GET", "/api/map"));
    CHECK(resp.find("\"locations\"") != std::string::npos, "GET /api/map has locations");
    CHECK(resp.find("\"connections\"") != std::string::npos, "GET /api/map has connections");
    CHECK(resp.find("MainGate") != std::string::npos, "map contains MainGate");
    CHECK(resp.find("Hostel-A") != std::string::npos, "map contains Hostel-A");

    resp = bodyOf(http("GET", "/api/search?q=lib"));
    CHECK(resp.find("Library") != std::string::npos, "search 'lib' finds Library");
    resp = bodyOf(http("GET", "/api/search?q=zzz"));
    CHECK(resp.find("\"results\":[]") != std::string::npos, "search with no matches returns []");

    resp = bodyOf(http("GET", "/api/route?from=MainGate&to=Hostel-A&metric=distance"));
    CHECK(resp.find("\"reachable\":true") != std::string::npos, "Dijkstra finds MainGate->Hostel-A");
    CHECK(resp.find("290") != std::string::npos, "Dijkstra total distance is 290 m");
    resp = bodyOf(http("GET", "/api/route?from=MainGate&to=Hostel-A&metric=hops"));
    CHECK(resp.find("\"reachable\":true") != std::string::npos, "BFS finds MainGate->Hostel-A");
    resp = bodyOf(http("GET", "/api/route?from=Nowhere&to=Hostel-A"));
    CHECK(resp.find("error") != std::string::npos, "route with unknown source is 404");

    resp = bodyOf(http("GET", "/api/traverse?from=MainGate&order=bfs"));
    CHECK(resp.find("\"visit\"") != std::string::npos, "BFS traversal works");
    resp = bodyOf(http("GET", "/api/traverse?from=MainGate&order=dfs"));
    CHECK(resp.find("\"visit\"") != std::string::npos, "DFS traversal works");

    resp = bodyOf(http("GET", "/api/info"));
    CHECK(resp.find("\"info\"") != std::string::npos, "GET /api/info works");

    // ---- role guards ----
    std::string status;
    bodyOf(http("POST", "/api/locations", R"({"name":"X","x":1,"y":1})", "", &status));
    CHECK(status.find("401") != std::string::npos, "admin endpoint without login is 401");
    bodyOf(http("POST", "/api/locations", R"({"name":"X","x":1,"y":1})", userCookie, &status));
    CHECK(status.find("403") != std::string::npos, "admin endpoint as user is 403");
    bodyOf(http("POST", "/api/generate", "", userCookie, &status));
    CHECK(status.find("403") != std::string::npos, "generate as user is 403");
    bodyOf(http("DELETE", "/api/locations/0", "", userCookie, &status));
    CHECK(status.find("403") != std::string::npos, "delete location as user is 403");

    // ---- admin CRUD round-trip ----
    resp = bodyOf(http("POST", "/api/locations", R"({"name":"RoboticsLab","type":"F","detail":"Robotics Lab","x":40,"y":6})", adminCookie));
    CHECK(resp.find("\"id\"") != std::string::npos, "admin adds a location");
    std::string newId;
    {
        web::Json parsed;
        if (web::Json::parse(resp, parsed)) newId = std::to_string(parsed.getInt("id", -1));
    }
    CHECK(newId != "-1" && !newId.empty(), "new location gets an id");

    resp = bodyOf(http("GET", "/api/search?q=RoboticsLab"));
    CHECK(resp.find("RoboticsLab") != std::string::npos, "new location is searchable");

    resp = bodyOf(http("PUT", "/api/locations",
                        "{\"id\":" + newId + ",\"name\":\"Robotics Lab\"}", adminCookie));
    CHECK(resp.find("\"ok\":true") != std::string::npos, "admin renames a location");

    resp = bodyOf(http("POST", "/api/connections",
                       "{\"from\":" + newId + ",\"to\":2,\"d\":60,\"label\":\"TestPath\"}", adminCookie));
    CHECK(resp.find("\"ok\":true") != std::string::npos, "admin adds a connection");

    resp = bodyOf(http("GET", "/api/route?from=Robotics%20Lab&to=Library&metric=distance"));
    CHECK(resp.find("\"reachable\":true") != std::string::npos, "route through new connection works");

    resp = bodyOf(http("DELETE", "/api/connections?from=" + newId + "&to=2", "", adminCookie));
    CHECK(resp.find("\"ok\":true") != std::string::npos, "admin removes a connection");

    resp = bodyOf(http("DELETE", "/api/locations/" + newId, "", adminCookie));
    CHECK(resp.find("\"ok\":true") != std::string::npos, "admin removes a location");
    resp = bodyOf(http("GET", "/api/search?q=Robotics"));
    CHECK(resp.find("\"results\":[]") != std::string::npos, "removed location is gone from search");

    resp = bodyOf(http("POST", "/api/generate", "", adminCookie));
    CHECK(resp.find("\"ok\":true") != std::string::npos, "admin regenerates the campus");

    // ---- multiple maps: list, create, save, switch ----
    auto has = [](const std::string& text, const std::string& piece) {
        return text.find(piece) != std::string::npos;
    };
    resp = bodyOf(http("GET", "/api/maps"));
    CHECK(has(resp, "\"active\":\"default\"") && has(resp, "\"saved\":false"),
          "map list is public and shows the unsaved default map");

    http("POST", "/api/maps", R"({"name":"x"})", "", &statusLine);
    CHECK(has(statusLine, "401"), "create map without login is 401");
    http("POST", "/api/maps/select", R"({"name":"default"})", userCookie, &statusLine);
    CHECK(has(statusLine, "403"), "select map as user is 403");
    http("POST", "/api/maps/save", "", userCookie, &statusLine);
    CHECK(has(statusLine, "403"), "save map as user is 403");

    // The built-in default must be saved first, or switching away would lose it.
    resp = bodyOf(http("POST", "/api/maps/save", "", adminCookie));
    CHECK(has(resp, "\"ok\":true") && has(resp, "\"name\":\"default\""), "admin saves the default map");
    CHECK(std::filesystem::exists(kMapsDir + "/default.json"), "save writes maps/<name>.json");

    // Names become file names: path tricks and reserved device names must be refused.
    http("POST", "/api/maps", R"({"name":"../evil","width":40,"height":15})", adminCookie, &statusLine);
    CHECK(has(statusLine, "400"), "map name with path characters is rejected");
    http("POST", "/api/maps", R"({"name":"CON","width":40,"height":15})", adminCookie, &statusLine);
    CHECK(has(statusLine, "400"), "reserved device map name is rejected");
    http("POST", "/api/maps", R"({"name":"default","width":40,"height":15})", adminCookie, &statusLine);
    CHECK(has(statusLine, "400"), "duplicate map name is rejected");
    http("POST", "/api/maps", R"({"name":"huge","width":5000,"height":15})", adminCookie, &statusLine);
    CHECK(has(statusLine, "400"), "oversized map is rejected");
    CHECK(!std::filesystem::exists("evil.json") && !std::filesystem::exists("huge.json"),
          "rejected names create no files");

    resp = bodyOf(http("POST", "/api/maps", R"({"name":"scratch","width":40,"height":15})", adminCookie));
    CHECK(has(resp, "\"ok\":true"), "admin creates a map from scratch");
    resp = bodyOf(http("GET", "/api/map"));
    CHECK(has(resp, "\"name\":\"scratch\"") && has(resp, "\"width\":40") && has(resp, "\"height\":15") &&
              has(resp, "\"locations\":[]"),
          "new map is active, empty and has the requested size");

    // Coordinates are clamped to the map's own size (40 wide -> maxX 37), not the old 62.
    http("POST", "/api/locations", R"({"name":"A","type":"F","detail":"a","x":10,"y":5})", adminCookie);
    http("POST", "/api/locations", R"({"name":"B","type":"F","detail":"b","x":20,"y":5})", adminCookie);
    http("POST", "/api/locations", R"({"name":"C","type":"B","detail":"3","x":100,"y":5})", adminCookie);
    resp = bodyOf(http("GET", "/api/map"));
    CHECK(has(resp, "\"x\":37"), "coordinates clamp to the active map's width");
    resp = bodyOf(http("GET", "/api/maps"));
    CHECK(has(resp, "\"dirty\":true"), "edits mark the map dirty");

    http("POST", "/api/connections", R"({"from":0,"to":2,"d":50,"label":"walk"})", adminCookie);
    http("DELETE", "/api/locations/1", "", adminCookie);  // leaves a hole at id 1
    resp = bodyOf(http("POST", "/api/maps/save", "", adminCookie));
    CHECK(has(resp, "\"ok\":true"), "admin saves the scratch map");
    resp = bodyOf(http("GET", "/api/maps"));
    CHECK(has(resp, "\"dirty\":false") && has(resp, "\"scratch\""), "saving clears dirty; map is listed");

    http("POST", "/api/maps/save", R"({"name":"default"})", adminCookie, &statusLine);
    CHECK(has(statusLine, "400"), "Save As never overwrites a different map");

    resp = bodyOf(http("POST", "/api/maps/select", R"({"name":"default"})", adminCookie));
    CHECK(has(resp, "\"ok\":true"), "admin switches to the default map");
    resp = bodyOf(http("GET", "/api/map"));
    CHECK(has(resp, "\"name\":\"default\"") && has(resp, "MainGate") && !has(resp, "\"name\":\"A\""),
          "switching shows the other map's data");
    resp = bodyOf(http("POST", "/api/maps/select", R"({"name":"scratch"})", adminCookie));
    resp = bodyOf(http("GET", "/api/map"));
    CHECK(has(resp, "\"width\":40") && has(resp, "\"name\":\"A\"") && has(resp, "\"name\":\"C\"") &&
              !has(resp, "\"name\":\"B\""),
          "saved map reloads with its size and locations");
    CHECK(has(resp, "\"id\":2"), "reload keeps ids stable across the deleted slot");
    // Regression: ids {0,2} with only 2 live locations - the old Navigation::valid()
    // compared ids to the live count and rejected id 2.
    resp = bodyOf(http("GET", "/api/route?from=A&to=C&metric=distance"));
    CHECK(has(resp, "\"reachable\":true"), "routing works after a deletion left a gap in ids");

    // A corrupt file must be refused without disturbing the active map.
    {
        std::ofstream bad(kMapsDir + "/broken.json");
        bad << "{ this is not json";
    }
    http("POST", "/api/maps/select", R"({"name":"broken"})", adminCookie, &statusLine);
    CHECK(has(statusLine, "400"), "corrupt map file is rejected");
    resp = bodyOf(http("GET", "/api/maps"));
    CHECK(has(resp, "\"active\":\"scratch\""), "failed switch keeps the active map");
    http("POST", "/api/maps/select", R"({"name":"missing"})", adminCookie, &statusLine);
    CHECK(has(statusLine, "404"), "selecting an unknown map is 404");

    // The built-in campus must stay reachable even if it was never saved (or its file is gone).
    http("POST", "/api/maps/select", R"({"name":"scratch"})", adminCookie);
    std::filesystem::remove(kMapsDir + "/default.json");
    resp = bodyOf(http("GET", "/api/maps"));
    CHECK(has(resp, "\"name\":\"default\""), "built-in default stays listed without a saved file");
    http("POST", "/api/maps/select", R"({"name":"default"})", adminCookie, &statusLine);
    CHECK(has(statusLine, "200"), "built-in default can be selected without a saved file");
    resp = bodyOf(http("GET", "/api/map"));
    CHECK(has(resp, "MainGate") && has(resp, "\"width\":62"), "selecting default rebuilds the built-in campus");

    // ---- viewers: any visitor can look at another saved map without switching the shared one ----
    resp = bodyOf(http("GET", "/api/map?map=scratch"));  // no cookie at all: a guest
    CHECK(has(resp, "\"name\":\"scratch\"") && has(resp, "\"width\":40") && has(resp, "\"name\":\"A\""),
          "guest can view another saved map with ?map=");
    resp = bodyOf(http("GET", "/api/maps"));
    CHECK(has(resp, "\"active\":\"default\""), "viewing another map leaves the shared active map alone");
    resp = bodyOf(http("GET", "/api/map"));
    CHECK(has(resp, "\"name\":\"default\"") && has(resp, "MainGate"), "no ?map= still shows the active map");
    resp = bodyOf(http("GET", "/api/route?from=A&to=C&map=scratch", "", userCookie));
    CHECK(has(resp, "\"reachable\":true"), "routing works inside a viewed map");
    resp = bodyOf(http("GET", "/api/route?from=A&to=C"));
    CHECK(has(resp, "error"), "same names do not exist in the active map");
    resp = bodyOf(http("GET", "/api/search?q=A&map=scratch"));
    CHECK(has(resp, "\"name\":\"A\""), "search works inside a viewed map");
    resp = bodyOf(http("GET", "/api/info?map=scratch"));
    CHECK(has(resp, "Locations   : 2"), "info works inside a viewed map");
    http("GET", "/api/map?map=..%2Fevil", "", "", &statusLine);
    CHECK(has(statusLine, "400"), "viewing a map with path characters is rejected");
    http("GET", "/api/map?map=nosuchmap", "", "", &statusLine);
    CHECK(has(statusLine, "404"), "viewing an unknown map is 404");
    http("POST", "/api/locations", R"({"name":"Z","type":"F","detail":"z","x":5,"y":5})", userCookie, &statusLine);
    CHECK(has(statusLine, "403"), "viewing is read-only: users still cannot edit");

    // Leave the default campus active for the remaining checks.

    // ---- logout ----
    resp = http("POST", "/logout", "", adminCookie);
    CHECK(bodyOf(resp).find("\"ok\":true") != std::string::npos, "logout works");
    resp = http("GET", "/api/whoami", "", adminCookie);
    CHECK(bodyOf(resp).find("guest") != std::string::npos, "session invalidated after logout");

    std::filesystem::remove_all(kMapsDir);
    std::printf("\n%d checks, %d failures\n", gChecks, gFailures);
    return gFailures == 0 ? 0 : 1;
}
