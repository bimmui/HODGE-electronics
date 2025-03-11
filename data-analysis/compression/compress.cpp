#include <bitset>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "huffman.h"

int main(void) {
    std::vector<uint32_t> freqs = {40, 35, 20, 5};
    std::unordered_map<char, Pattern> map = create_table(
        std::vector<char> {'a', 'b', 'c', 'd'}, 
        freqs
    );
    for (auto const& [key, val]  : map) {
        std::cout << key << ": " << std::bitset<4>(val.pattern) << "\n";
    }

    return EXIT_SUCCESS;
}