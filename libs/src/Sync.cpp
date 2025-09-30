#include "../include/Sync.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

using namespace K;

uint16_t Sync::serve_once(const uint16_t port) {
    const int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) return 2;
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0) {
        close(server_fd);
        return 2;
    }

    if (listen(server_fd, 1) < 0) {
        close(server_fd);
        return 2;
    }

    const int client_socket = accept(server_fd, nullptr, nullptr);
    if (client_socket < 0) {
        close(server_fd);
        return 2;
    }

    char buffer[1024] = {};
    read(client_socket, buffer, 1024);

    close(client_socket);
    close(server_fd);
    return 0;
}

int Sync::client_send(const std::string &host, const uint16_t port, const std::string &message) {
    const int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return 2;

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &serv_addr.sin_addr) <= 0) {
        close(sock);
        return 2;
    }

    if (connect(sock, reinterpret_cast<sockaddr *>(&serv_addr), sizeof(serv_addr)) < 0) {
        close(sock);
        return 2;
    }
    send(sock, message.c_str(), message.length(), 0);
    close(sock);
    return 0;
}
