#include "Utils.hpp"

std::string to_string(int n) {
    std::stringstream ss;
    ss << n;
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, struct pollfd pfd) {
    os << "fd: " << pfd.fd << std::endl;
    os << "events: " << pfd.events << std::endl;
    os << "revents: " << pfd.revents << std::endl;
    return os;
}
