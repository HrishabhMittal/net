// #define DEBUG
#include "../include/net.hpp"
int main() {
    net::iostream<net::socket_type::TCP> io(net::getURL("http://google.com"));
    std::string msg="GET / HTTP/1.1\r\n"
                    "Host: www.google.com\r\n"
                    "Connection: close\r\n\r\n";
    io<<msg<<net::send;
    std::string s;
    s.resize(1024);
    io>>s;
    s.resize(io.bytes_read());
    // debug::cout<<s<<std::endl;
    std::cout<<s<<std::endl;
}
