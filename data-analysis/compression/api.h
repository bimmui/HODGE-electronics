#pragma once

#include <cstdint>
#include <vector>

template <typename T, typename U = float, typename V = uint8_t>
class Compressor {
public:
    static T* create(std::vector<std::vector<U>>) = 0;
    static T* create_max_bits(std::vector<std::vector<U>>) = 0;
    static T* create_max_rmse(std::vector<std::vector<U>>) = 0;
    vector<>* compress(vector) = 0;
};