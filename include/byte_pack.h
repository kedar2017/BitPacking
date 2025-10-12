#include <cstdio>
#include <array>
#include <iostream>

int out_bytes_needed (int bit_width, int num) {
    int num_bytes_needed = ((bit_width * num) + 7) >> 3;
    std::cout << "num_bytes_needed " << num_bytes_needed << " \n";
    return num_bytes_needed;
}

void byte_pack (const uint32_t* in, uint8_t* out, int bit_width, int in_num, int out_num_bytes) {
    int mask = (1 << bit_width) - 1;
    int bit_pos = 0;
    for (int i = 0; i < in_num; i++) {
        int temp = (in[i] & mask);

        int byte_index = (bit_pos >> 3);
        int offset_index = (bit_pos & 0b111);
        int right_offset = offset_index ^ 0b111;

        if (offset_index + bit_width > 8) {
            int spillage = (offset_index + bit_width) & 0b111;
            out[byte_index] = out[byte_index] | (temp >> spillage);
            out[byte_index + 1] = out[byte_index + 1] | ((temp & ((1 << spillage) - 1)) << (8 - spillage));
        } else {
            if (right_offset == 0) out[byte_index] = out[byte_index] | (temp);
            else out[byte_index] = out[byte_index] | (temp << (right_offset - bit_width + 1));
        }
        bit_pos = bit_pos + bit_width;
    }
}
