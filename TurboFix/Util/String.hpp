#pragma once
#include <locale>
#include <vector>

namespace Util {
    constexpr unsigned long joaat(const char* s) {
        unsigned long hash = 0;
        for (; *s != '\0'; ++s) {
            auto c = *s;
            if (c >= 0x41 && c <= 0x5a) {
                c += 0x20;
            }
            hash += c;
            hash += hash << 10;
            hash ^= hash >> 6;
        }
        hash += hash << 3;
        hash ^= hash >> 11;
        hash += hash << 15;
        return hash;
    }

    std::string to_lower(std::string s);

    std::vector<std::string> split(const std::string& s, char delim);
}
