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
 * @file Simple Single-Threaded HTTP Server
 *
 * Replies to "GET /" requests with a templated HTML page, with basic request
 * validation for other requests.
 * See the code for available placeholders in the HTML template, the format is
 * similar to jinja2, but only supports simple {placeholder} syntax.
 *
 * Requires WITH_NETWORKING option enabled in `miosix_settings.h`. Only uses
 * POSIX-socket APIs.
 *
 * Compile for Linux hosts with: g++ -std=c++23 main.cpp -o main
 */

#include <array>
#include <chrono>
#include <cstdio>
#include <memory>
#include <string_view>

#ifdef _MIOSIX
#include <miosix.h>
#include <util/version.h>
#endif

#include <arpa/inet.h>  // sockaddr_in from <netinet/in.h>
#include <sys/socket.h> // socket, bind, listen, accept
#include <unistd.h>     // close, read, write

// Configuration
constexpr int PORT = 80;
constexpr int BUFFER_SIZE = 4096;

// Request handling forward declaration
enum class ResponseType { OK, BadRequest, NotFound, MethodNotAllowed, Last };
extern std::array<std::string_view, (size_t)ResponseType::Last> staticResponses;
ResponseType handleRequest(std::string_view request);
std::string renderBlueprint(std::string_view blueprint);

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
            std::perror("Read error");
            close(client_fd);
            continue; // Try again on read error
        }

        // Null-terminate for printing safety
        buffer[bytes_read] = '\0';
        std::string_view request(buffer.get(), bytes_read);

        auto responseType = handleRequest(request);

        // HTML Response
        auto response = staticResponses[(size_t)responseType];

        std::string rendered;
        if (responseType == ResponseType::OK) {
            rendered = renderBlueprint(response);
            response = rendered;
        }

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

std::string renderBlueprint(std::string_view blueprint) {
    std::string rendered;
    rendered.reserve(blueprint.size() + 512);

    size_t pos = 0;

    while (true) {
        // Find the next placeholder
        auto start = blueprint.find('{', pos);
        if (start == std::string_view::npos)
            break;

        // Append text before placeholder
        rendered += blueprint.substr(pos, start - pos);

        // Find the closing brace for the placeholder
        auto end = blueprint.find('}', start);
        if (end == std::string_view::npos)
            break;

        // Move past the current placeholder for the next iteration
        pos = end + 1;

        auto placeholder = blueprint.substr(start + 1, end - start - 1);
        if (placeholder == "build") {
#ifdef _MIOSIX
            rendered += miosix::getMiosixVersion();
#else
            rendered += "Linux (pc-linux-generic, " __DATE__ " " __TIME__
                        ", gcc " __VERSION__ ")";
#endif
        } else if (placeholder == "board") {
#ifdef _MIOSIX
            rendered += _MIOSIX_BOARDNAME;
#else
            rendered += "pc-linux-generic";
#endif
        } else if (placeholder == "uptime") {
            using namespace std::chrono;
            auto now = system_clock::now();
            auto epoch = now.time_since_epoch();
            rendered += std::to_string(epoch.count());
        } else {
            rendered += "{" + std::string(placeholder) + "}";
        }
    }

    // Append remaining text
    rendered += blueprint.substr(pos);
    return rendered;
}

decltype(staticResponses) staticResponses = {
    // ResponseType: OK
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Connection: close\r\n"
    "\r\n"
    "<!DOCTYPE html>"
    "<html>"
    "<head><title>Miosix HTTP Server</title></head>"
    "<body>"
    "<h1>Miosix HTTP Server</h1>"
    "<p>This page was served by the Miosix HTTP server example.</p>"
    "<p><code>"
    "<b>System information:</b><br />"
    "Board: {board} <br />"
    "Build: {build} <br />"
    "Uptime: {uptime} ns"
    "</code></p>"
    "</body>"
    "</html>"
    "\r\n",

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
