#pragma once
#include "utils.hpp"
// #define DEBUG
#include "debug.hpp"
#include <cstdint>
#include <endian.h>
#include <netinet/in.h>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include <concepts>
#include <iostream>
namespace net {
    using namespace utils;
    struct url {
        std::string protocol;
        std::string domain;
        int32_t port;
        std::string page;
        std::vector<std::string> params;
    };
    inline std::ostream& operator<<(std::ostream& out,const url& u) {
        out<<"protocol: "<<u.protocol<<std::endl;
        out<<"domain: "<<u.domain<<std::endl;
        out<<"port: "<<u.port<<std::endl;
        out<<"page: "<<u.page<<std::endl;
        for (auto&& i:u.params) {
            out<<"param: "<<i<<std::endl;
        }
        return out;
    }
    inline url getURL(const std::string& str) {
        url out;
        out.domain=str;
        size_t end=out.domain.find("://");
        if (end!=std::string::npos) {
            out.protocol=out.domain.substr(0,end);
            out.domain=out.domain.substr(end+3);
        }
        end=out.domain.find("/");
        if (end!=std::string::npos) {
            out.page=out.domain.substr(end+1);
            out.domain=out.domain.substr(0,end);
        }
        end=out.domain.find(":");
        if (end!=std::string::npos) {
            out.port=std::stoi(out.domain.substr(end+1));
            out.domain=out.domain.substr(0,end);
        } else {
            if (out.protocol == "http") {
                    out.port = 80;
            } else if (out.protocol == "https") {
                    out.port = 443;
            } else if (out.protocol == "ftp") {
                    out.port = 21;
            }
        }
        return out;
    }
    enum class socket_type {
        TCP,UDP
    };
    template<socket_type type>
    class iostream {
        sockaddr_in dest_addr;
        std::string dest;
        int32_t sock_fd=-1;
        int timeout=-1;
        size_t last_bytes_read=0;
        std::string buf;
    public:
        void flush() {
            send(buf.data(),buf.size());
            buf="";
        }
        bool binary=true;
        void setTimeout(int32_t t) {
                struct timeval tv;
                timeout = t;
                tv.tv_sec = t;
                tv.tv_usec = 0;
                setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        }
        bool setDestination(url u) {
            sockaddr_in new_dest_addr;
            std::memset(&new_dest_addr, 0, sizeof(new_dest_addr));
            new_dest_addr.sin_family = AF_INET;
            new_dest_addr.sin_port = htons(u.port);
            dest=u.domain;
            struct addrinfo hints, *res;
            std::memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_INET;
            hints.ai_socktype = (sock_fd == SOCK_DGRAM) ? SOCK_DGRAM : SOCK_STREAM;
            std::string portStr = std::to_string(u.port);
            int status = getaddrinfo(u.domain.c_str(), portStr.c_str(), &hints, &res);
            if (status != 0) {
                debug::thread_safe::cerr<<"DNS Lookup failed for "<<u.domain<<": "<<gai_strerror(status)<<std::endl;
                return false;
            } else debug::thread_safe::cout<<"DNS Lookup worked! "<<u.domain<<'\n'<<std::flush;
            std::memcpy(&new_dest_addr, res->ai_addr, res->ai_addrlen);
            freeaddrinfo(res);
            dest_addr = new_dest_addr;
            return true;
        }
        bool setDestination(const std::string& ip, uint16_t port) {
            sockaddr_in new_dest_addr;
            dest=ip;
            std::memset(&new_dest_addr, 0, sizeof(new_dest_addr));
            new_dest_addr.sin_family = AF_INET;
            new_dest_addr.sin_port = htons(port);
            if (inet_pton(AF_INET, ip.c_str(), &new_dest_addr.sin_addr) <= 0) {
                return 0;
            }
            dest_addr=new_dest_addr;
            return 1;
        }
        iostream(int32_t sock_fd): sock_fd(sock_fd) {}
        iostream(url u) {
            if constexpr (type==socket_type::TCP) {
                sock_fd=socket(AF_INET,SOCK_STREAM,0);
                if (sock_fd<0) {
                    error("couldn't open socket");
                }
                assert(setDestination(u));
                assert(connect(sock_fd,(struct sockaddr*)&dest_addr,sizeof(dest_addr))>=0);
            } else {
                sock_fd=socket(AF_INET,SOCK_DGRAM,0);
                if (sock_fd<0) {
                    error("couldn't open socket");
                }
                assert(setDestination(u));
            }
        }
        iostream(std::string ip, int32_t port) {
            if constexpr (type==socket_type::TCP) {
                sock_fd = socket(AF_INET,SOCK_STREAM,0);
                if (sock_fd<0) {
                    error("couldn't open socket");
                }
                assert(setDestination(ip,port));
                assert(connect(sock_fd,(struct sockaddr*)&dest_addr,sizeof(dest_addr))>=0);
            } else {
                sock_fd = socket(AF_INET,SOCK_DGRAM,0);
                if (sock_fd<0) {
                    error("couldn't open socket");
                }
                assert(setDestination(ip,port));
            }
        }
        iostream() {}
        ~iostream() {
            if (sock_fd>=0) {
                close(sock_fd);
            }
        }
        size_t bytes_read() {
            return last_bytes_read;
        }
        void send(const char* buffer,size_t length,int flags=0) {
            if constexpr (type == socket_type::TCP) {
                ::send(sock_fd, buffer, length, flags);
            } else if constexpr (type == socket_type::UDP) {
                ::sendto(sock_fd, buffer, length, flags, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
            }
            debug::thread_safe::cout<<dest<<" <- (sent "<<std::to_string(length)<<" bytes) "<<to_hex(std::string(buffer,length))<<std::endl;
        }
        ssize_t recv(char* buffer,size_t total,int flags=0) const {
            ssize_t received;
            if constexpr (type==socket_type::UDP) {
                socklen_t addr_len = sizeof(dest_addr);
                received=::recvfrom(sock_fd,buffer,total,flags,(struct sockaddr*)&dest_addr,&addr_len);
            } else received=::recv(sock_fd,buffer,total,flags);
            debug::thread_safe::cout<<dest<<" -> (recv "<<std::to_string(received)<<" bytes) "<<to_hex(std::string(buffer,total))<<std::endl;
            return received;
        }
        ssize_t recv_all(char* buffer,size_t total,int flags=0) const {
            size_t off=0;
            size_t left=total;
            while (left) {
                ssize_t received=recv(&buffer[off],left,flags);
                if (received<0) {
                    // using namespace std::chrono_literals;
                    debug::thread_safe::cerr<<std::string("recv error: ")<<strerror(errno)<<std::endl;
                    // std::this_thread::sleep_for(1ms);
                } else {
                    left-=received;
                    off+=received;
                }
            }
            debug::thread_safe::cout<<dest<<" -> (recv_all "<<std::to_string(total)<<" bytes) "<<to_hex(std::string(buffer,total))<<std::endl;
            return total;
        }
        void attach(int32_t fd) {
            sock_fd=fd;
        }
        int getTimeout() {
            return timeout;
        }
        template<typename T>
        requires std::invocable<T, iostream&> || 
                 std::is_convertible_v<T, std::string_view> || 
                 std::is_same_v<std::decay_t<T>, std::vector<char>> || 
                 std::is_integral_v<std::decay_t<T>> || 
                 std::is_floating_point_v<std::decay_t<T>>
        iostream& operator<<(const T& data) {
            const char* buffer = nullptr;
            size_t length = 0;
            uint16_t be16_buf;
            uint32_t be32_buf;
            uint64_t be64_buf;
            std::string string_buf;
            if constexpr (std::invocable<T, iostream&>) {
                return data(*this);
            } else if constexpr (std::is_convertible_v<T, std::string_view>) {
                std::string_view sv=data;
                buffer = sv.data();
                length = sv.size();
            } else if constexpr (std::is_same_v<std::decay_t<T>, std::vector<char>>) {
                buffer = data.data();
                length = data.size();
            } else if constexpr (std::is_integral_v<std::decay_t<T>>) {
                if (binary) {
                    length = sizeof(T);
                    if constexpr (sizeof(T) == 1) {
                        buffer = reinterpret_cast<const char*>(&data);
                    } else if constexpr (sizeof(T) == 2) {
                        be16_buf=htobe16(data);
                        buffer = reinterpret_cast<const char*>(&be16_buf);
                    } else if constexpr (sizeof(T) == 4) {
                        be32_buf=htobe32(data);
                        buffer = reinterpret_cast<const char*>(&be32_buf);
                    } else if constexpr (sizeof(T) == 8) {
                        be64_buf=htobe64(data);
                        buffer = reinterpret_cast<const char*>(&be64_buf);
                    }
                } else {
                    string_buf=std::to_string(data);
                    buffer=string_buf.data();
                    length=string_buf.size();
                }
            } else if constexpr (std::is_floating_point_v<std::decay_t<T>>) {
                buffer = reinterpret_cast<const char*>(&data);
                length = sizeof(T);
            } else {
                // should never happen
                static_assert(sizeof(T) == 0, "Unsupported type passed to net::iostream");
            }
            if (buffer && length > 0 && sock_fd != -1) {
                buf.append(buffer,length);
                // send(buffer,length);
            }
            return *this;
        }
        template<typename T>
        requires std::is_same_v<std::decay_t<T>, std::string> || 
                 std::is_same_v<std::decay_t<T>, std::vector<char>> || 
                 std::is_integral_v<std::decay_t<T>> || 
                 std::is_floating_point_v<std::decay_t<T>>
        iostream& operator>>(T& data) {
            ssize_t bytes_read = 0;
            if (sock_fd == -1) return *this;
            if constexpr (std::is_same_v<std::decay_t<T>, std::string> || 
                          std::is_same_v<std::decay_t<T>, std::vector<char>>) {
                bytes_read = recv(data.data(),data.size());
            } else if constexpr (std::is_integral_v<std::decay_t<T>>) {
                const constexpr int flags = (type == socket_type::TCP) ? MSG_WAITALL : 0;
                bytes_read = recv_all(&data,sizeof(data),flags);
                if constexpr (sizeof(T) == 2) {
                    data = static_cast<T>(be16toh(data));
                } else if constexpr (sizeof(T) == 4) {
                    data = static_cast<T>(be32toh(data));
                } else if constexpr (sizeof(T) == 8) {
                    data = static_cast<T>(be64toh(data));
                }
            } else if constexpr (std::is_floating_point_v<std::decay_t<T>>) {
                char buffer[sizeof(T)];
                int flags = (type == socket_type::TCP) ? MSG_WAITALL : 0;
                bytes_read = recv_all(buffer,sizeof(buffer),flags);
                std::memcpy(&data, buffer, sizeof(T));
            } else {
                // again, should never happen
                static_assert(sizeof(T) == 0, "Unsupported type passed to net::iostream");
            }
            last_bytes_read=bytes_read;
            return *this;
        }
    };
    inline auto setdestination(url u) {
        return [u](auto& stream) -> auto& {
            stream.setDestination(u);
            return stream;
        };
    }
    inline auto settimeout(int timeout) {
        return [timeout](auto& stream) -> auto& {
            stream.setTimeout(timeout);
            return stream;
        };
    }
    inline auto setdestination(std::string ip, uint16_t port) {
        return [ip, port](auto& stream) -> auto& {
            stream.setDestination(ip, port);
            return stream;
        };
    }
    inline constexpr auto send = [](auto& stream) -> auto& {
        stream.flush();
        return stream;
    };
    inline constexpr auto binary = [](auto& stream) -> auto& {
        stream.binary=true;
        return stream;
    };
    inline constexpr auto ascii = [](auto& stream) -> auto& {
        stream.binary=false;
        return stream;
    };
}
