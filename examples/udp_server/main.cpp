#include <cstdio>
#include <cstdint>
#include <cstring>

#include <arpa/inet.h>  // sockaddr_in from <netinet/in.h>
#include <netinet/in.h>
#include <sys/socket.h> // socket, bind, listen, accept
#include <unistd.h>     // close, read, write

#ifdef _MIOSIX
#include <miosix.h>
#endif

#define RX
#define TX

constexpr int PORT = 1337;
constexpr int BUFFER_SIZE = 4096;

int main() {
    int server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    int client_fd = socket(AF_INET, SOCK_DGRAM, 0);

    // Bind the socket to an address and port
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Listen on 0.0.0.0 (all interfaces)
    address.sin_port = htons(PORT);       // Host to Network Short

    // TX address
    sockaddr_in tx_address{};
    tx_address.sin_family = AF_INET;
    tx_address.sin_addr.s_addr = inet_addr("192.168.1.1");
    tx_address.sin_port = htons(50000);
    const char TX_DATA[] = "Hello!";

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::perror("Bind failed");
        return 1;
    }

    auto buffer = new uint8_t[BUFFER_SIZE];

    while (1) {
        #ifdef RX
        sockaddr_in addr{};
        socklen_t len = sizeof(addr);
        auto count = recvfrom(server_fd, buffer, BUFFER_SIZE - 1, 0, (sockaddr*) &addr, &len);

        std::printf("UDP packet received from %s:%d, size: %d\n", inet_ntoa(addr.sin_addr), htons(addr.sin_port), count);
        buffer[count] = '\0';
        #ifdef _MIOSIX
        miosix::memDump(buffer, count);
        #else
        printf("Content: %s\n", buffer);
        #endif
        #endif

        #ifdef TX
        std::printf("Sending UDP packet to %s:%d\n", inet_ntoa(tx_address.sin_addr), htons(tx_address.sin_port));
        sendto(client_fd, TX_DATA, strlen(TX_DATA), 0, (sockaddr*)&tx_address, sizeof(tx_address));
        #endif
    }

    delete[] buffer;
    return 0;
}
