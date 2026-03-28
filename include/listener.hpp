#pragma once
#include "netstream.hpp"
namespace net {
template <socket_type type> class listener;

template <> class listener<socket_type::TCP> {
    int32_t sock_fd = -1;
    uint16_t port;
    std::string ip;

  public:
    listener(uint16_t port, const std::string &ip = "0.0.0.0") : port(port), ip(ip) {
        sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fd < 0) {
            error("couldn't open listening socket");
        }

        int opt = 1;
        setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

        if (::bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            error("bind failed on " + ip + ":" + std::to_string(port));
        }
        if (::listen(sock_fd, 10) < 0) {
            error("listen failed");
        }

        debug::thread_safe::cout << "Listening on " << ip << ":" << port << "\n" << std::flush;
    }

    ~listener() {
        if (sock_fd >= 0) {
            close(sock_fd);
        }
    }

    iostream<socket_type::TCP> accept() {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = ::accept(sock_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            error(std::string("accept failed: ") + strerror(errno));
        }
        return iostream<socket_type::TCP>(client_fd, client_addr);
    }
};
template <> class listener<socket_type::UDP> {
    int32_t sock_fd = -1;
    uint16_t port;
    std::string ip;

  public:
    listener(uint16_t port, const std::string &ip = "0.0.0.0") : port(port), ip(ip) {
        sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock_fd < 0) {
            error("couldn't open UDP listening socket");
        }

        int opt = 1;
        setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

        if (::bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            error("bind failed on " + ip + ":" + std::to_string(port));
        }

        debug::thread_safe::cout << "UDP Listening on " << ip << ":" << port << "\n" << std::flush;
    }
    ~listener() {
        if (sock_fd >= 0) {
            close(sock_fd);
        }
    }
    iostream<socket_type::UDP> accept() {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        char dummy;
        ssize_t received = ::recvfrom(sock_fd, &dummy, 1, MSG_PEEK, (struct sockaddr *)&client_addr, &client_len);
        if (received < 0) {
            error(std::string("UDP accept/peek failed: ") + strerror(errno));
        }
        return iostream<socket_type::UDP>(sock_fd, client_addr, false);
    }
};
} // namespace net
