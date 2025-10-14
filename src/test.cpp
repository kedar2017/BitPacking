#include "../include/byte_pack.h"
#include <vector>
#include <random>

bool randomness_test (int bit_width, int in_num, int seed = 12345) {
    std::mt19937 rng(seed);
    uint32_t mask = ((1u << bit_width) - 1u);
    std::vector<uint32_t> in(in_num), out(in_num);
    for (auto& v : in) v = rng() & mask;

    int out_num = out_bytes_needed(bit_width, in_num);
    std::vector<uint8_t> packed(out_num, 0);

    byte_pack(in.data(), packed.data(), bit_width, in_num, out_num);
    byte_unpack(packed.data(), out.data(), bit_width, in_num);

    return in == out;
}

bool pattern_test (std::vector<uint32_t>& patterned_input, int bit_width) {
    std::vector<uint32_t> out(patterned_input.size());
    int out_num = out_bytes_needed(bit_width, patterned_input.size());
    std::vector<uint8_t> packed(out_num, 0);
    byte_pack(patterned_input.data(), packed.data(), bit_width, patterned_input.size(), out_num);
    byte_unpack(packed.data(), out.data(), bit_width, patterned_input.size());

    return patterned_input == out;
}

int main () {
    /*
    Currently supports bit width 1-8
    */
    std::vector<uint32_t> bit_widths{2,3,4,5,6,7,8};
    std::vector<uint32_t> in_nums{2,3,7,15,31,64,255,1024,2047};
    for (uint32_t bw: bit_widths) {
        for (uint32_t in_num: in_nums) {
            if (!randomness_test(bw, in_num)) {
                std::cout << "Randomness test failed!! \n";
                std::exit(1);
            }
        }
    }

    for (uint32_t bw: bit_widths) {
        uint32_t max_num = ((1u << bw) - 1u);
        std::vector<uint32_t> const_pattern(35, 0);
        std::vector<uint32_t> const_max_pattern(35, max_num);
        std::vector<uint32_t> inc_pattern(35,0);
        for (int i = 0; i < 35; i++) {
            inc_pattern[i] = i & max_num;
        }
        if (!pattern_test(const_pattern, bw) || !pattern_test(const_max_pattern, bw) || !pattern_test(inc_pattern, bw)) {
            std::cout << "Pattern test failed!! \n";
            std::exit(1);
        }
    }

    std::cout << "Correctness tests passed!! \n";
}