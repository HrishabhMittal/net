#pragma once
#include "netstream.hpp"
namespace net {
class network_object {
  public:
    virtual ~network_object() = default;
    virtual void send(iostream<socket_type::TCP> &stream) const {}
    virtual void send(iostream<socket_type::UDP> &stream) const {}
    virtual void recv(iostream<socket_type::TCP> &stream) {}
    virtual void recv(iostream<socket_type::UDP> &stream) {}
};
template <socket_type type> iostream<type> &operator<<(iostream<type> &stream, const network_object &obj) {
    obj.send(stream);
    return stream;
}
template <socket_type type> iostream<type> &operator>>(iostream<type> &stream, network_object &obj) {
    obj.recv(stream);
    return stream;
}
} // namespace net
