#include "String.hpp"

#include <sstream>
#include <algorithm>

namespace {
    template<typename Out>
    void split(const std::string& s, char delim, Out result) {
        std::stringstream ss;
        ss.str(s);
        std::string item;
        while (std::getline(ss, item, delim)) {
            *(result++) = item;
        }
    }
}

std::string Util::to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

std::vector<std::string> Util::split(const std::string& s, char delim) {
    std::vector<std::string> elems;
    ::split(s, delim, std::back_inserter(elems));
    return elems;
}
