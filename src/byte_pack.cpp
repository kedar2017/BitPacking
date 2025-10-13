#include "../include/byte_pack.h"
#include <vector>

int main () {
    std::vector<uint32_t> in(10);

    in[0] = 2;
    in[1] = 4;
    in[2] = 1;
    in[3] = 1;
    in[4] = 1;
    in[5] = 100;
    in[6] = 2;
    in[7] = 3;
    in[8] = 3;
    in[9] = 3;

    int bit_width = 7;
    int out_num = out_bytes_needed(bit_width, in.size());
    std::vector<uint8_t> out(out_num, 0);
    byte_pack (in.data(), out.data(), bit_width, in.size(), out_num);

    for (int i = 0; i < out.size(); i++) {
        std::cout << "out index " << i << " value is " << (int) out[i] << " \n";
    }

    std::vector<uint32_t> in_final(10);
    byte_unpack (out.data(), in_final.data(), bit_width, in.size());

    for (int i = 0; i < in_final.size(); i++) {
        std::cout << "in_final index " << i << " value is " << (uint32_t) in_final[i] << " \n";
    }
}