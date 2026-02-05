/***************************************************************************
 *   Copyright (C) 2026 by Niccolò Betto                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

/**
 * Barebone Single-Threaded HTTP Server
 *
 * Replies to "GET /" requests with a static HTML page, with basic request
 * validation for other requests.
 * Requires WITH_NETWORKING option enabled in `miosix_settings.h`.
 * Only uses POSIX-socket APIs.
 *
 * Compile for Linux hosts with: g++ -std=c++23 main.cpp -o main
 */

#include <netinet/in.h> // sockaddr_in
#include <sys/socket.h> // socket, bind, listen, accept
#include <unistd.h>     // close, read, write

#include <array>
#include <cstdio>
#include <memory>
#include <string_view>

// Configuration
constexpr int PORT = 80;
constexpr int BUFFER_SIZE = 4096;

// Request handling forward declaration
enum class ResponseType { OK, BadRequest, NotFound, MethodNotAllowed, Last };
extern std::array<std::string_view, (size_t)ResponseType::Last> staticResponses;
ResponseType handleRequest(std::string_view request);

int main() {
    // Create the socket
    // AF_INET = IPv4, SOCK_STREAM = TCP
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::perror("Socket creation failed");
        return 1;
    }

    // Configure socket options
    // SO_REUSEADDR prevents "Address already in use" errors if you restart the
    // server quickly
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        std::perror("setsockopt failed");
        return 1;
    }

    // Bind the socket to an address and port
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Listen on 0.0.0.0 (all interfaces)
    address.sin_port = htons(PORT);       // Host to Network Short

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::perror("Bind failed");
        return 1;
    }

    // Listen for incoming connections
    // The backlog is set to 10 (queue size for pending connections)
    if (listen(server_fd, 10) < 0) {
        std::perror("Listen failed");
        return 1;
    }

    std::printf("Server listening on port %d\n", PORT);

    auto buffer = std::make_unique<char[]>(BUFFER_SIZE);

    // Main event loop
    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);

        // Blocks until a client connects
        int client_fd =
            accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            std::perror("Accept failed");
            continue; // Try again on bad connection
        }

        // Read the request, blocks until data is available
        ssize_t bytes_read = read(client_fd, buffer.get(), BUFFER_SIZE - 1);

        if (bytes_read <= 0) {
            close(client_fd);
            continue; // Try again on read error
        }

        // Null-terminate for printing safety
        buffer[bytes_read] = '\0';
        std::string_view request(buffer.get(), bytes_read);

        auto responseType = handleRequest(request);

        // HTML Response
        auto response = staticResponses[(size_t)responseType];

        // Send the response
        send(client_fd, response.data(), response.size(), 0);

        // Close the connection
        close(client_fd);
    }

    // Unreachable code but good practice
    close(server_fd);
    return 0;
}

ResponseType handleRequest(std::string_view request) {
    auto result = ResponseType::OK;

    // Extract the method
    auto method_end = request.find(' ');
    if (method_end == std::string_view::npos) {
        std::printf("Received bad request: missing method/path separator\n");
        return ResponseType::BadRequest;
    }
    auto method = request.substr(0, method_end);

    // Extract the path
    auto path_start = method_end + 1;
    auto path_end = request.find(' ', path_start);
    if (path_end == std::string_view::npos) {
        std::printf("Received bad request: missing path/version separator\n");
        return ResponseType::BadRequest;
    }
    auto path = request.substr(path_start, path_end - path_start);

    std::printf("Received: %.*s %.*s\n", (int)method.size(), method.data(),
                (int)path.size(), path.data());

    // Validate the request
    if (method != "GET")
        return ResponseType::MethodNotAllowed;

    if (path.empty() || path[0] != '/')
        return ResponseType::BadRequest;

    if (path == "/")
        return ResponseType::OK;

    return ResponseType::NotFound;
}

decltype(staticResponses) staticResponses = {
    // ResponseType: OK
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    //"Content-Length: 213\r\n"
    "Connection: close\r\n"
    "\r\n"
    "<!DOCTYPE html>"
    "<html>"
    "<head><title>Miosix HTTP Server</title></head>"
    "<body>"
    "<h1>Miosix HTTP Server</h1>"
#ifdef MIOSIX
    "<p>This page was served from an embedded board running the Miosix "
    "HTTP server example.</p>"
#else
    "<p>This page was served from a Linux host running the Miosix "
    "HTTP server example.</p>"
#endif
    "</body>"
    "</html>",

    // ResponseType: BadRequest
    "HTTP/1.1 400 Bad Request\r\n"
    "Connection: close\r\n"
    "\r\n",

    // ResponseType: NotFound
    "HTTP/1.1 404 Not Found\r\n"
    "Connection: close\r\n"
    "\r\n",

    // ResponseType: MethodNotAllowed
    "HTTP/1.1 405 Method Not Allowed\r\n"
    "Connection: close\r\n"
    "\r\n",
};
