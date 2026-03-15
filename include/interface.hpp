#pragma once
#include "utils.hpp"
namespace net {
    struct interface {
        std::string name;
        std::string ipv4 = "N/A";
        std::string ipv6 = "N/A";
        bool is_up = false;
        bool is_loopback = false;
    };
    inline std::vector<std::string> interfaces() {
        std::vector<std::string> interface_names;
        struct ifaddrs *ifaddr_list = nullptr;
        if (getifaddrs(&ifaddr_list) == -1) {
            perror("getifaddrs");
            return interface_names;
        }
        std::set<std::string> unique_names;
        for (struct ifaddrs *ifa = ifaddr_list; ifa != nullptr; ifa = ifa->ifa_next) {
            if (ifa->ifa_name != nullptr) {
                unique_names.insert(ifa->ifa_name);
            }
        }
        for (const auto& name : unique_names) {
            interface_names.push_back(name);
        }
        freeifaddrs(ifaddr_list);
        return interface_names;
    }
    inline interface get_interface(std::string target_name) {
        interface result;
        result.name = target_name;
        struct ifaddrs *interfaces = nullptr;
        if (getifaddrs(&interfaces) == -1) {
            return result; 
        }
        for (struct ifaddrs *ifa = interfaces; ifa != nullptr; ifa = ifa->ifa_next) {
            if (!ifa->ifa_addr || target_name != ifa->ifa_name) {
                continue;
            }
            result.is_up = (ifa->ifa_flags & IFF_UP);
            result.is_loopback = (ifa->ifa_flags & IFF_LOOPBACK);
            int family = ifa->ifa_addr->sa_family;
            if (family == AF_INET) {
                char addr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr, addr, INET_ADDRSTRLEN);
                result.ipv4 = addr;
            } 
            else if (family == AF_INET6) {
                char addr[INET6_ADDRSTRLEN];
                inet_ntop(AF_INET6, &((struct sockaddr_in6*)ifa->ifa_addr)->sin6_addr, addr, INET6_ADDRSTRLEN);
                result.ipv6 = addr;
            }
        }
        freeifaddrs(interfaces);
        return result;
    }
}
