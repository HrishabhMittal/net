#define DEBUG
#include "debug.hpp"
#include "net.hpp"
#include "netstream.hpp"
#include <asio.hpp>
#include <future>
#include <iostream>
#include <list>
#include <utility>
int main() {
    net::listener<net::socket_type::TCP> server(8080);
    std::vector<net::iostream<net::socket_type::TCP>> clients;

    // i know this isnt really right, i should prob remove clients which have disconnected, im just too lazy
    auto acceptor = [&]() {
        while (true) {
            auto client_stream = server.accept();
            debug::thread_safe::cout << "new client!" << std::endl;
            client_stream.setTimeout(1);
            clients.push_back(std::move(client_stream));
        }
    };
    std::string buffer(1000, '\0');
    auto reply = [&](net::iostream<net::socket_type::TCP> &i) {
        i >> buffer;
        std::string_view sv = std::string_view(buffer.data(), i.bytes_read());
        debug::thread_safe::cout << sv << std::endl;
        if (sv.substr(0, 4) == "ping") {
            i << "pong\n" << net::send;
        }
    };
    std::list<std::future<void>> futures;
    auto accept = std::async(std::launch::async, acceptor);
    while (true) {
        for (auto &i : clients) {
            futures.push_back(std::async(std::launch::async, [&]() { reply(i); }));
        }
        std::erase_if(futures,
                      [](auto &f) { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; });
    }
    std::unreachable();
}
