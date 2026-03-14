#pragma once
#include <cctype>
#include <cstring>
#include <cstdint>

#include <source_location>
#include <stdexcept>

#include <iomanip>
#include <ios>
#include <iostream>
#include <ostream>
#include <sstream>

#include <string>
#include <vector>

#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <endian.h>
namespace net {
    inline std::string URLEncode(const std::string& input) {
        std::ostringstream escaped;
        escaped<<std::hex<<std::uppercase;
        for (uint8_t c:input) {
            if (std::isalnum(c)||c=='-'||c=='_'||c=='.'||c=='~') {
                escaped<<(int8_t)c;
            } else {
                escaped<<'%'<<std::setfill('0')<<std::setw(2)<<(int32_t)c;
            }
        }
        return escaped.str();
    }
}
namespace utils {
    inline void error(std::string msg) {
        std::cerr<<msg<<std::endl;
        exit(1);
    }
    inline void assert(bool condition, const std::source_location& loc = std::source_location::current()) {
        if (!condition) {
            std::string msg = "assertion failed at " + 
                              std::string(loc.file_name()) + ":" + 
                              std::to_string(loc.line()) + " in " + 
                              std::string(loc.function_name());
            throw std::runtime_error(msg);
        }
    }
    template<typename T>
    std::ostream& operator<<(std::ostream& out,const std::vector<T>& vec) {
        out<<"[ ";
        for (auto i:vec) {
            out<<i<<", ";
        }
        out<<"]";
        return out;
    }
    inline void printBinString(std::ostream& out,const std::string& str) {
        for (auto c:str) {
            if (std::isprint(c)) {
                out<<c;
            } else {
                out<<'.';
            }
        }
    }
    inline char hex_char(uint8_t c) {
        c &= 0xf;
        return "0123456789abcdef"[c];
    }
    inline uint8_t byte_from_hex(char c) {
        if (c>='0'&&c<='9') return c-'0';
        if (c>='a'&&c<='f') return c-'a'+10;
        if (c>='A'&&c<='F') return c-'A'+10;
        assert(0);
    }
    inline std::string to_hex(const std::string& byte_str) {
        std::string out;
        out.reserve(byte_str.size()*2);
        for (uint8_t c:byte_str) {
            out += hex_char(c>>4);
            out += hex_char(c);
        }
        return out;
    }
    inline std::string binString(const std::string& str) {
        std::string s;
        for (auto c:str) {
            if (std::isprint(c)) s+=c;
            else {
                s+="\\0x";
                s+=hex_char(c>>4);
                s+=hex_char(c);
            }
        }
        return s;
    }
    inline std::string hex_to_bytes(const std::string& hex_str) {
        if (hex_str.size()%2!=0) return ""; 
        std::string out;
        out.reserve(hex_str.size()/2);
        for (size_t i=0;i<hex_str.size();i+=2) {
            uint8_t high=byte_from_hex(hex_str[i]);
            uint8_t low=byte_from_hex(hex_str[i+1]);
            out+=static_cast<char>((high<<4)|low);
        }
        return out;
    }
}
