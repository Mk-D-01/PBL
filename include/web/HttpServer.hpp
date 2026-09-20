#pragma once

#include <functional>
#include <string>

#include "data_structures/DynamicArray.hpp"
#include "data_structures/HashMap.hpp"

namespace web {

#ifdef _WIN32
using SocketHandle = unsigned long long;  // SOCKET
inline constexpr SocketHandle kInvalidSocket = ~0ULL;
#else
using SocketHandle = int;
inline constexpr SocketHandle kInvalidSocket = -1;
#endif

// HttpRequest: one parsed HTTP request.
struct HttpRequest {
    std::string method;                                // GET / POST / PUT / DELETE
    std::string path;                                  // decoded path, e.g. /api/map
    std::string rawQuery;                              // raw query string after '?'
    std::string body;
    ds::HashMap<std::string, std::string> headers;     // keys lower-cased

    // Returns the named cookie value, or "" when absent.
    std::string cookie(const std::string& name) const;
    // Returns a decoded query parameter value, or "" when absent.
    std::string queryParam(const std::string& name) const;
};

// HttpResponse: what the handler produced.
struct HttpResponse {
    int status = 200;
    std::string contentType = "application/json; charset=utf-8";
    std::string body;
    std::string setCookie;  // optional Set-Cookie value (single header)

    static HttpResponse json(int status, const std::string& body);
    static HttpResponse error(int status, const std::string& message);
    static HttpResponse text(int status, const std::string& body);
};

// HttpServer: single-threaded, blocking accept loop. One request per connection
// (Connection: close). Fine for a campus-wide demo; the engine needs no locks.
class HttpServer {
public:
    using Handler = std::function<HttpResponse(const HttpRequest&)>;

    HttpServer(int port, std::string docRoot, Handler handler);

    // Starts listening; blocks forever. Returns false when the socket cannot
    // be created/bound.
    bool run();

    // Attempts to serve `urlPath` from `docRoot` into `out`.
    // Returns false when the file does not exist / path is unsafe.
    static bool serveStatic(const std::string& docRoot, const std::string& urlPath,
                            HttpResponse& out);

private:
    void handleClient(SocketHandle sock);

    int port_;
    std::string docRoot_;
    Handler handler_;
};

// Percent-decodes a URL component (%xx and '+').
std::string urlDecode(const std::string& text);

// Returns the HTTP reason phrase for a status code.
const char* statusText(int status);

}  // namespace web
