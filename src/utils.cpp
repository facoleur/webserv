// utils.cpp

#include "utils.hpp"

std::string to_string(int n) {
    std::stringstream ss;
    ss << n;
    return ss.str();
}
