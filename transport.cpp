
#include "transport.hpp"

#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace Socks {

// ==========================
// UnixSocketTransport
// ==========================

UnixSocketTransport::UnixSocketTransport(const std::string &socket_path)
    : socket_path_(socket_path) {}

void UnixSocketTransport::bind() {
    server_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd_ == -1)
        throw std::runtime_error("Failed to create UNIX socket");

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socket_path_.c_str(),
                 sizeof(addr.sun_path) - 1);
    unlink(socket_path_.c_str());

    if (::bind(server_fd_, (sockaddr *)&addr, sizeof(addr)) == -1)
        throw std::runtime_error("Failed to bind UNIX socket");

    if (listen(server_fd_, 5) == -1)
        throw std::runtime_error("Failed to listen on UNIX socket");
}

std::string UnixSocketTransport::receive(std::string &client_id) {
    int client_fd = accept(server_fd_, nullptr, nullptr);
    if (client_fd == -1)
        throw std::runtime_error("Failed to accept connection");

    char buffer[2048] = {};
    ssize_t len = read(client_fd, buffer, sizeof(buffer) - 1);
    if (len <= 0) {
        ::close(client_fd);
        throw std::runtime_error("Failed to read request");
    }

    client_id = std::to_string(client_fd);
    return std::string(buffer, len);
}

void UnixSocketTransport::send(const std::string &data,
                               const std::string &client_id) {
    int client_fd = std::stoi(client_id);
    write(client_fd, data.c_str(), data.size());
    ::close(client_fd);
}

std::string UnixSocketTransport::send(const std::string &data) {
    int client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd == -1)
        throw std::runtime_error("Failed to create UNIX client socket");

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socket_path_.c_str(),
                 sizeof(addr.sun_path) - 1);

    if (connect(client_fd, (sockaddr *)&addr, sizeof(addr)) == -1) {
        ::close(client_fd);
        throw std::runtime_error("Failed to connect to UNIX socket");
    }

    if (write(client_fd, data.c_str(), data.size()) == -1) {
        ::close(client_fd);
        throw std::runtime_error("Failed to send data to UNIX socket");
    }

    char buffer[2048] = {};
    ssize_t len = read(client_fd, buffer, sizeof(buffer) - 1);
    if (len <= 0) {
        ::close(client_fd);
        throw std::runtime_error("Failed to read response from UNIX socket");
    }

    ::close(client_fd);
    return std::string(buffer, len);
}

void UnixSocketTransport::close() {
    if (server_fd_ != -1) {
        ::close(server_fd_);
        unlink(socket_path_.c_str());
        server_fd_ = -1;
    }
}

// ==========================
// UdpTransport
// ==========================

UdpTransport::UdpTransport(int port) : port_(port), ip_("127.0.0.1") {}

UdpTransport::UdpTransport(const std::string &ip, int port)
    : port_(port), ip_(ip) {}

void UdpTransport::bind() {
    sock_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_ == -1)
        throw std::runtime_error("Failed to create UDP socket");

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_);

    if (::bind(sock_, (sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        throw std::runtime_error("Failed to bind UDP socket");
}

std::string UdpTransport::receive(std::string &client_id) {
    sockaddr_in client_addr{};
    socklen_t addr_len = sizeof(client_addr);
    char buffer[2048] = {};

    ssize_t len = recvfrom(sock_, buffer, sizeof(buffer) - 1, 0,
                           (sockaddr *)&client_addr, &addr_len);
    if (len <= 0)
        throw std::runtime_error("Failed to receive from UDP socket");

    client_id = std::string(inet_ntoa(client_addr.sin_addr)) + ":" +
                std::to_string(ntohs(client_addr.sin_port));
    return std::string(buffer, len);
}

void UdpTransport::send(const std::string &data, const std::string &client_id) {
    size_t delim = client_id.find(":");
    if (delim == std::string::npos)
        return;

    std::string ip = client_id.substr(0, delim);
    int port = std::stoi(client_id.substr(delim + 1));

    sockaddr_in client_addr{};
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &client_addr.sin_addr);

    sendto(sock_, data.c_str(), data.size(), 0, (sockaddr *)&client_addr,
           sizeof(client_addr));
}

std::string UdpTransport::send(const std::string &data) {
    int client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_fd == -1)
        throw std::runtime_error("Failed to create UDP client socket");

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip_.c_str());
    server_addr.sin_port = htons(port_);

    if (sendto(client_fd, data.c_str(), data.size(), 0,
               (sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        ::close(client_fd);
        throw std::runtime_error("Failed to send UDP packet");
    }

    char buffer[2048] = {};
    sockaddr_in from{};
    socklen_t from_len = sizeof(from);
    ssize_t len = recvfrom(client_fd, buffer, sizeof(buffer) - 1, 0,
                           (sockaddr *)&from, &from_len);
    if (len <= 0) {
        ::close(client_fd);
        throw std::runtime_error("Failed to receive UDP response");
    }

    ::close(client_fd);
    return std::string(buffer, len);
}

void UdpTransport::close() {
    if (sock_ != -1) {
        ::close(sock_);
        sock_ = -1;
    }
}

// ==========================
// TcpTransport
// ==========================

TcpTransport::TcpTransport(int port) : port_(port), ip_("127.0.0.1") {}

TcpTransport::TcpTransport(const std::string &ip, int port)
    : port_(port), ip_(ip) {}

void TcpTransport::bind() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ == -1)
        throw std::runtime_error("Failed to create TCP socket");

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_);

    if (::bind(server_fd_, (sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        throw std::runtime_error("Failed to bind TCP socket");

    if (listen(server_fd_, 5) == -1)
        throw std::runtime_error("Failed to listen on TCP socket");
}

std::string TcpTransport::receive(std::string &client_id) {
    sockaddr_in client_addr{};
    socklen_t addr_len = sizeof(client_addr);

    int client_fd = accept(server_fd_, (sockaddr *)&client_addr, &addr_len);
    if (client_fd == -1)
        throw std::runtime_error("Failed to accept TCP connection");

    char buffer[2048] = {};
    ssize_t len = read(client_fd, buffer, sizeof(buffer) - 1);
    if (len <= 0) {
        ::close(client_fd);
        throw std::runtime_error("Failed to read from TCP client");
    }

    client_id = std::to_string(client_fd);
    return std::string(buffer, len);
}

void TcpTransport::send(const std::string &data, const std::string &client_id) {
    int client_fd = std::stoi(client_id);
    write(client_fd, data.c_str(), data.size());
    ::close(client_fd);
}

std::string TcpTransport::send(const std::string &data) {
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1)
        throw std::runtime_error("Failed to create TCP client socket");

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    server_addr.sin_addr.s_addr = inet_addr(ip_.c_str());

    if (connect(client_fd, (sockaddr *)&server_addr, sizeof(server_addr)) ==
        -1) {
        ::close(client_fd);
        throw std::runtime_error("Failed to connect to TCP server");
    }

    if (write(client_fd, data.c_str(), data.size()) == -1) {
        ::close(client_fd);
        throw std::runtime_error("Failed to write to TCP server");
    }

    char buffer[2048] = {};
    ssize_t len = read(client_fd, buffer, sizeof(buffer) - 1);
    if (len <= 0) {
        ::close(client_fd);
        throw std::runtime_error("Failed to read from TCP server");
    }

    ::close(client_fd);
    return std::string(buffer, len);
}

void TcpTransport::close() {
    if (server_fd_ != -1) {
        ::close(server_fd_);
        server_fd_ = -1;
    }
}

} // namespace Socks
