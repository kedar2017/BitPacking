#include "../include/byte_pack.h"
#include <vector>
#include <random>
#include <chrono>
#include <cstring>

using clk = std::chrono::steady_clock;


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

void wall_time_compare_memcpy (std::vector<uint32_t>& patterned_input, std::vector<uint32_t>& bit_widths) {
    std::vector<uint32_t> packed(patterned_input.size(), 0);
    for (uint32_t bw: bit_widths) {
        for (int i = 0; i < 1000000; i++) {
            memcpy(packed.data(), patterned_input.data(), patterned_input.size() * sizeof(uint32_t));
            memcpy(patterned_input.data(), packed.data(), patterned_input.size() * sizeof(uint32_t));
        }
    }
}

void wall_time_compare_cache_resident (std::vector<uint32_t>& patterned_input, std::vector<uint32_t>& bit_widths) {
    for (uint32_t bw: bit_widths) {
        int out_num = out_bytes_needed(bw, patterned_input.size());
        std::vector<uint8_t> packed(out_num, 0);
        for (int i = 0; i < 1000000; i++) {
            byte_pack(patterned_input.data(), packed.data(), bw, patterned_input.size(), out_num);
            byte_unpack(packed.data(), patterned_input.data(), bw, patterned_input.size());
        }
    }
}

void wall_time_large_array_compare_memcpy (std::vector<uint32_t>& patterned_input, std::vector<uint32_t>& bit_widths) {
    std::vector<uint32_t> packed(patterned_input.size(), 0);
    for (uint32_t bw: bit_widths) {
        memcpy(packed.data(), patterned_input.data(), patterned_input.size() * sizeof(uint32_t));
        memcpy(patterned_input.data(), packed.data(), patterned_input.size() * sizeof(uint32_t));
    }
}

void wall_time_large_array_compare_pack_unpack (std::vector<uint32_t>& patterned_input, std::vector<uint32_t>& bit_widths) {
    for (uint32_t bw: bit_widths) {
        int out_num = out_bytes_needed(bw, patterned_input.size());
        std::vector<uint8_t> packed(out_num, 0);
        byte_pack(patterned_input.data(), packed.data(), bw, patterned_input.size(), out_num);
        byte_unpack(packed.data(), patterned_input.data(), bw, patterned_input.size());
    }
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

    std::vector<uint32_t> const_pattern(35, 1);
    auto t0 = clk::now();
    wall_time_compare_cache_resident (const_pattern, bit_widths);
    auto t1 = clk::now();
    double wall_time_cache_resident = std::chrono::duration<double, std::milli>(t1 - t0).count();

    uint32_t pack_unpack_bytes_touched = 0;
    double pack_unpack_memory_throughput = 0;

    for (uint32_t bw: bit_widths) {
        pack_unpack_bytes_touched += (sizeof(uint32_t) * const_pattern.size() * 2 + 2 * out_bytes_needed(bw, const_pattern.size()));
    }

    pack_unpack_memory_throughput = (pack_unpack_bytes_touched) / (wall_time_cache_resident);

    std::vector<uint32_t> const_pattern_memcpy(35, 1);
    auto t2 = clk::now();
    wall_time_compare_memcpy (const_pattern_memcpy, bit_widths);
    auto t3 = clk::now();
    double wall_time_memcpy = std::chrono::duration<double, std::milli>(t3 - t2).count();

    double memcpy_memory_throughput = 0;
    memcpy_memory_throughput = (bit_widths.size()) * (4 * sizeof(uint32_t) * const_pattern_memcpy.size()) / wall_time_memcpy;

    std::cout << "Memory throughput (pack/unpack): " << pack_unpack_memory_throughput << " GB/s \n";
    std::cout << "Memory throughput (memcpy): " << memcpy_memory_throughput << " GB/s \n";    


    std::vector<uint32_t> const_pattern_large_array(350000, 1);
    auto t4 = clk::now();
    wall_time_large_array_compare_pack_unpack (const_pattern_large_array, bit_widths);
    auto t5 = clk::now();
    double wall_time_pack_unpack_large_array = std::chrono::duration<double, std::milli>(t5 - t4).count();

    uint32_t pack_unpack_bytes_touched_large_array = 0;
    double pack_unpack_memory_throughput_large_array = 0;

    for (uint32_t bw: bit_widths) {
        pack_unpack_bytes_touched_large_array += (sizeof(uint32_t) * const_pattern_large_array.size() * 2 + 2 * out_bytes_needed(bw, const_pattern_large_array.size()));
    }

    pack_unpack_memory_throughput_large_array = (pack_unpack_bytes_touched_large_array * 1000) / (wall_time_pack_unpack_large_array * 1e9);

    std::vector<uint32_t> const_pattern_memcpy_large_array(350000, 1);
    auto t6 = clk::now();
    wall_time_large_array_compare_memcpy (const_pattern_memcpy_large_array, bit_widths);
    auto t7 = clk::now();
    double wall_time_memcpy_large_array = std::chrono::duration<double, std::milli>(t7 - t6).count();

    double memcpy_memory_throughput_large_array = 0;
    memcpy_memory_throughput_large_array = (bit_widths.size()) * (4 * sizeof(uint32_t) * const_pattern_memcpy_large_array.size() * 1000) / (wall_time_memcpy_large_array * 1e9);

    std::cout << "Memory throughput - large array (pack/unpack): " << pack_unpack_memory_throughput_large_array << " GB/s \n";
    std::cout << "Memory throughput - large array (memcpy): " << memcpy_memory_throughput_large_array << " GB/s \n";    

}