#include "web/HttpServer.hpp"

#include "web/Json.hpp"
#include <cstdio>
#include <cstring>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif
using SockLen = int;
inline int closeSocket(web::SocketHandle s) { return ::closesocket(s); }
inline int sendAll(web::SocketHandle s, const char* data, int len) { return ::send(s, data, len, 0); }
inline int recvSome(web::SocketHandle s, char* buf, int len) { return static_cast<int>(::recv(s, buf, len, 0)); }
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using SockLen = socklen_t;
inline int closeSocket(web::SocketHandle s) { return ::close(s); }
inline int sendAll(web::SocketHandle s, const char* data, int len) { return static_cast<int>(::write(s, data, static_cast<size_t>(len))); }
inline int recvSome(web::SocketHandle s, char* buf, int len) { return static_cast<int>(::read(s, buf, static_cast<size_t>(len))); }
#endif

#include <algorithm>
#include <fstream>
#include <sstream>

namespace web {

std::string urlDecode(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        if (c == '+') {
            out.push_back(' ');
        } else if (c == '%' && i + 2 < text.size()) {
            auto hexVal = [](char h) -> int {
                if (h >= '0' && h <= '9') return h - '0';
                if (h >= 'a' && h <= 'f') return h - 'a' + 10;
                if (h >= 'A' && h <= 'F') return h - 'A' + 10;
                return -1;
            };
            int hi = hexVal(text[i + 1]);
            int lo = hexVal(text[i + 2]);
            if (hi >= 0 && lo >= 0) {
                out.push_back(static_cast<char>((hi << 4) | lo));
                i += 2;
            } else {
                out.push_back(c);
            }
        } else {
            out.push_back(c);
        }
    }
    return out;
}

const char* statusText(int status) {
    switch (status) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 500: return "Internal Server Error";
        default: return "OK";
    }
}

std::string HttpRequest::cookie(const std::string& name) const {
    const std::string* header = headers.find("cookie");
    if (header == nullptr) return "";
    std::size_t pos = 0;
    while (pos < header->size()) {
        std::size_t end = header->find(';', pos);
        if (end == std::string::npos) end = header->size();
        std::string pair = header->substr(pos, end - pos);
        std::size_t eq = pair.find('=');
        if (eq != std::string::npos && pair.substr(0, eq) == name) {
            std::string value = pair.substr(eq + 1);
            while (!value.empty() && (value.front() == ' ')) value.erase(value.begin());
            return value;
        }
        pos = end + 1;
    }
    return "";
}

std::string HttpRequest::queryParam(const std::string& name) const {
    std::size_t pos = 0;
    while (pos < rawQuery.size()) {
        std::size_t end = rawQuery.find('&', pos);
        if (end == std::string::npos) end = rawQuery.size();
        std::string pair = rawQuery.substr(pos, end - pos);
        std::size_t eq = pair.find('=');
        if (eq != std::string::npos && pair.substr(0, eq) == name) {
            return urlDecode(pair.substr(eq + 1));
        }
        pos = end + 1;
    }
    return "";
}

HttpResponse HttpResponse::json(int status, const std::string& body) {
    HttpResponse r;
    r.status = status;
    r.contentType = "application/json; charset=utf-8";
    r.body = body;
    return r;
}

HttpResponse HttpResponse::error(int status, const std::string& message) {
    return json(status, "{\"error\":\"" + Json::escape(message) + "\"}");
}

HttpResponse HttpResponse::text(int status, const std::string& body) {
    HttpResponse r;
    r.status = status;
    r.contentType = "text/plain; charset=utf-8";
    r.body = body;
    return r;
}

HttpServer::HttpServer(int port, std::string docRoot, Handler handler)
    : port_(port), docRoot_(std::move(docRoot)), handler_(std::move(handler)) {}

bool HttpServer::run() {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::fprintf(stderr, "WSAStartup failed\n");
        return false;
    }
#endif
    SocketHandle listener = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listener == kInvalidSocket) {
        std::fprintf(stderr, "socket() failed\n");
        return false;
    }
    int reuse = 1;
    ::setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ::htonl(INADDR_LOOPBACK);  // localhost only
    addr.sin_port = ::htons(static_cast<unsigned short>(port_));

    if (::bind(listener, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        std::fprintf(stderr, "bind() failed on port %d\n", port_);
        closeSocket(listener);
        return false;
    }
    if (::listen(listener, 8) != 0) {
        std::fprintf(stderr, "listen() failed\n");
        closeSocket(listener);
        return false;
    }
    std::printf("MapGen web server listening on http://localhost:%d\n", port_);
    std::fflush(stdout);

    while (true) {
        SocketHandle client = ::accept(listener, nullptr, nullptr);
        if (client == kInvalidSocket) continue;
        handleClient(client);
        closeSocket(client);
    }
}

bool HttpServer::serveStatic(const std::string& docRoot, const std::string& urlPath,
                             HttpResponse& out) {
    if (urlPath.find("..") != std::string::npos) return false;
    std::string rel = (urlPath == "/" || urlPath.empty()) ? "/index.html" : urlPath;
    // Security: refuse anything not directly under docRoot.
    if (rel.front() != '/') return false;

    std::string filePath = docRoot + rel;
    std::ifstream file(filePath, std::ios::binary);
    if (!file) return false;

    std::ostringstream buffer;
    buffer << file.rdbuf();
    out.status = 200;
    out.body = buffer.str();

    if (rel.size() >= 5 && rel.compare(rel.size() - 5, 5, ".html") == 0) {
        out.contentType = "text/html; charset=utf-8";
    } else if (rel.size() >= 4 && rel.compare(rel.size() - 4, 4, ".css") == 0) {
        out.contentType = "text/css; charset=utf-8";
    } else if (rel.size() >= 3 && rel.compare(rel.size() - 3, 3, ".js") == 0) {
        out.contentType = "application/javascript; charset=utf-8";
    } else if (rel.size() >= 4 && rel.compare(rel.size() - 4, 4, ".svg") == 0) {
        out.contentType = "image/svg+xml";
    } else if (rel.size() >= 4 && rel.compare(rel.size() - 4, 4, ".png") == 0) {
        out.contentType = "image/png";
    } else if (rel.size() >= 4 && rel.compare(rel.size() - 4, 4, ".ico") == 0) {
        out.contentType = "image/x-icon";
    } else {
        out.contentType = "text/plain; charset=utf-8";
    }
    return true;
}

void HttpServer::handleClient(SocketHandle sock) {
    std::string raw;
    char buf[4096];
    std::size_t headerEnd = std::string::npos;

    // Read until the end of the headers (and a little body).
    while (raw.size() < 1u << 20) {  // 1 MiB cap
        int n = recvSome(sock, buf, sizeof(buf));
        if (n <= 0) break;
        raw.append(buf, static_cast<std::size_t>(n));
        headerEnd = raw.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            // Check whether Content-Length body bytes have fully arrived.
            std::size_t clPos = raw.find("Content-Length:");
            if (clPos == std::string::npos) clPos = raw.find("content-length:");
            bool bodyComplete = true;
            if (clPos != std::string::npos && clPos < headerEnd) {
                int contentLength = std::atoi(raw.c_str() + clPos + 15);
                if (static_cast<std::size_t>(contentLength) > raw.size() - (headerEnd + 4)) {
                    bodyComplete = false;
                }
            }
            if (bodyComplete) break;
        }
    }
    if (raw.empty()) return;
    if (headerEnd == std::string::npos) {
        HttpResponse resp = HttpResponse::error(400, "malformed request");
        std::string out = "HTTP/1.1 400 Bad Request\r\nContent-Type: application/json\r\nContent-Length: " +
                          std::to_string(resp.body.size()) + "\r\nConnection: close\r\n\r\n" + resp.body;
        sendAll(sock, out.c_str(), static_cast<int>(out.size()));
        return;
    }

    // ---- request line ----
    HttpRequest req;
    std::istringstream stream(raw.substr(0, headerEnd));
    std::string line;
    std::getline(stream, line);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    {
        std::istringstream rl(line);
        std::string target;
        rl >> req.method >> target;
        std::size_t q = target.find('?');
        if (q != std::string::npos) {
            req.rawQuery = target.substr(q + 1);
            req.path = urlDecode(target.substr(0, q));
        } else {
            req.path = urlDecode(target);
        }
    }

    // ---- headers ----
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) break;
        std::size_t colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string key = line.substr(0, colon);
        std::transform(key.begin(), key.end(), key.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        std::string value = line.substr(colon + 1);
        while (!value.empty() && value.front() == ' ') value.erase(value.begin());
        req.headers.put(key, value);
    }

    // ---- body ----
    req.body = raw.substr(headerEnd + 4);

    // ---- dispatch ----
    HttpResponse resp;
    if (handler_) {
        resp = handler_(req);
    } else {
        resp = HttpResponse::error(500, "no handler configured");
    }

    std::string head = "HTTP/1.1 " + std::to_string(resp.status) + " " + statusText(resp.status) +
                       "\r\nContent-Type: " + resp.contentType +
                       "\r\nContent-Length: " + std::to_string(resp.body.size()) +
                       "\r\nConnection: close\r\n";
    if (!resp.setCookie.empty()) {
        head += "Set-Cookie: " + resp.setCookie + "\r\n";
    }
    head += "\r\n";
    sendAll(sock, head.c_str(), static_cast<int>(head.size()));
    sendAll(sock, resp.body.data(), static_cast<int>(resp.body.size()));
}

}  // namespace web
