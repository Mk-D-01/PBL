#include "web/ApiController.hpp"
#include "web/HttpServer.hpp"

#include <cstdio>
#include <cstring>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

int main(int argc, char** argv) {
    int port = 18080;
    std::string docRoot = "web";
    for (int i = 1; i + 1 < argc; i += 2) {
        if (std::strcmp(argv[i], "--port") == 0) port = std::atoi(argv[i + 1]);
        if (std::strcmp(argv[i], "--root") == 0) docRoot = argv[i + 1];
    }

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    web::ApiController controller;
    web::HttpServer server(port, docRoot,
                           [&controller](const web::HttpRequest& request) {
                               return controller.handle(request);
                           });
    if (!server.run()) {
        std::fprintf(stderr, "failed to start server on port %d\n", port);
        return 1;
    }
    return 0;
}
