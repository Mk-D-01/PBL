// Web API self-test suite.
// Builds the HTTP layer in-process and drives it through real loopback
// sockets: login/whoami, static files, map/search/route correctness,
// role guards (401/403), and admin CRUD round-trips.

#include <cstdio>
#include <cstring>
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
    web::ApiController controller;
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

    // ---- logout ----
    resp = http("POST", "/logout", "", adminCookie);
    CHECK(bodyOf(resp).find("\"ok\":true") != std::string::npos, "logout works");
    resp = http("GET", "/api/whoami", "", adminCookie);
    CHECK(bodyOf(resp).find("guest") != std::string::npos, "session invalidated after logout");

    std::printf("\n%d checks, %d failures\n", gChecks, gFailures);
    return gFailures == 0 ? 0 : 1;
}
