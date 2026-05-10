#pragma once

#include <cstdint>
#include <string>


inline int notation_to_square(const std::string& notation) {
    return ((notation[1] - '0') - 1) * 8 + std::tolower(notation[0]) - 'a';
}
inline int pop_lsb(uint64_t& bb) {
    int index = std::countr_zero(bb);
    bb &= bb - 1;
    return index;
}
