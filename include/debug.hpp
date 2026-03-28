#pragma once
#include <iostream>
#include <streambuf>
#include <syncstream>
namespace debug {
namespace {
class NullBuffer : public std::streambuf {
  public:
    int overflow(int c) override { return c; }
};
NullBuffer nb;
std::ostream null_out(&nb);
class SyncProxy {
    std::ostream &target;

  public:
    explicit SyncProxy(std::ostream &os) : target(os) {}
    template <typename T> auto operator<<(const T &msg) const { return std::osyncstream(target) << msg; }
    auto operator<<(std::ostream &(*manip)(std::ostream &)) const { return std::osyncstream(target) << manip; }
};
} // namespace
#ifdef DEBUG
inline auto &cout = std::cout;
inline auto &cerr = std::cerr;
#else
inline std::ostream &cout = null_out;
inline std::ostream &cerr = null_out;
#endif
namespace thread_safe {
#ifdef DEBUG
inline SyncProxy cout(std::cout);
inline SyncProxy cerr(std::cerr);
#else
inline std::ostream &cout = null_out;
inline std::ostream &cerr = null_out;
#endif
} // namespace thread_safe
} // namespace debug
