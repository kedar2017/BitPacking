# BitPacking

> Attempting high-speed bit packing and unpacking — take an array of small integers and convert it into a very compact bitstream.

Bit-packing = take integers that fit in, say, 3–7 bits and densely stuff them into bytes/words to cut memory and bandwidth. This repo is a **learning + benchmarking playground** for that inner loop:

- A **scalar bit-packing / unpacking implementation** for fixed bit-width integers.
- A **micro-benchmark harness** that compares pack/unpack throughput against `memcpy` for both small (L1-hot) and large (streaming) arrays.
- A concrete roadmap toward **branchless 64-bit implementations** and **SIMD-style wide ops** (AVX2 / NEON) in future phases.

---

## Table of contents

- [Motivation](#motivation)
- [What this project does](#what-this-project-does)
- [Project roadmap](#project-roadmap)
- [Repository layout](#repository-layout)
- [Building & running](#building--running)
- [Current benchmark snapshot](#current-benchmark-snapshot)
- [API & example usage](#api--example-usage)
- [Design notes](#design-notes)
- [Future work](#future-work)

---

## Motivation

Bit-packing shows up everywhere:

- Storing dense bit arrays (1-bit flags, masks, occupancy maps, compressed posting lists).
- Compressing integer IDs, run-lengths, or deltas into minimal bits.
- Columnar formats (Parquet/ORC-style) that pack levels or dictionary IDs.
- Any place you want to **trade CPU for memory and bandwidth** in a controlled way.

Fast, predictable pack/unpack code is a fundamental primitive, especially if you care about:

- **Throughput:** GB/s of pack/unpack, not just compression ratio.
- **CPU vs memory bottlenecks:** Is your code limited by compute or by DRAM/L3?
- **Vectorization:** How far you can push scalar C++ vs wide ops (64-bit, AVX2, NEON).

This repo is a concrete exploration of those questions.

---

## What this project does

### Core functionality

**Phase 1 (implemented):**

- Pack fixed bit-width integers `bw ∈ {1…8,16}` into a byte stream.
- Unpack back into 32-bit or 16-bit integers.
- Validate with:
  - Random inputs
  - Patterned inputs (e.g., ramp, repeating patterns)

The implementation is currently **scalar**, designed to be readable and “obviously correct” before being aggressively optimized.

### Benchmark harness

**Phase 2 (implemented):**

- Small helper functions to compute **pack/unpack throughput (GB/s)**.
- Two regimes:
  - **L1-hot:** small arrays that fit in cache → compute vs `memcpy`.
  - **Streaming:** large arrays pulled from memory → memory bandwidth vs `memcpy`.
- Compares:
  - Pack+unpack cycle vs a bare `memcpy` of similar volume.

---

## Project roadmap

You can think of this repo as evolving through clear phases:

- **Phase 1 – Baseline scalar implementation ✅**
  - Correct pack/unpack for fixed bit widths.
  - Thorough tests with random and structured inputs.

- **Phase 2 – Benchmark harness & profiling ✅**
  - Throughput measurements vs `memcpy` (small vs large arrays).
  - Identify when the code is compute-bound vs memory-bound.

- **Phase 3 – Branchless 64-bit accumulator (next step)**
  - Specialize per bit-width (`bw`) so shift counts are compile-time constants.
  - Use a 64-bit accumulator: accumulate bits, flush full bytes/words.
  - Make inner loops **branchless** and unrolled.

- **Phase 4 – Wide ops / SIMD (planned)**
  - Use wide integer operations (64/128-bit) and eventually AVX2/NEON intrinsics.
  - Pack/unpack multiple values in parallel.

The repo’s current README numbers and comments come from Phases 1–2.

---

## Repository layout

Rough structure (may change slightly over time):

- `include/`
  - Public headers for bit-packing / unpacking functions.
- `src/`
  - Implementation and benchmark driver (`main` that runs pack/unpack vs `memcpy`).
- `README.md`
  - This document, including benchmark snapshots and roadmap.

---

## Building & running

### Requirements

- C++17 (or later) compiler (GCC/Clang/MSVC should all work).
- A POSIX-like environment is easiest for now (Linux/macOS), though Windows should be fine with minor tweaks.

### Build

Example using `g++`:

```bash
git clone https://github.com/kedar2017/BitPacking.git
cd BitPacking

g++ -std=c++17 -O3 -march=native \
    src/*.cpp \
    -o bitpack_bench
```

### Run 

```
./bitpack_bench
```

This will:

- Generate random/patterned integer arrays.
- Pack them into a bitstream and unpack back.
- Check correctness.
- Print throughput numbers for:

  - Pack+unpack (small arrays)
  - memcpy on similar small arrays
  - Pack+unpack (large arrays)
  - memcpy on similar large arrays

## Current benchmark snapshot

From one run of the current scalar implementation:

| Scenario                             | Operation      | Throughput (GB/s) |
|--------------------------------------|----------------|-------------------|
| Small arrays (L1-hot)                | Pack + unpack  | 1.37              |
| Small arrays (L1-hot)                | memcpy         | 23.78             |
| Large arrays (streaming from memory) | Pack + unpack  | 0.077             |
| Large arrays (streaming from memory) | memcpy         | 24.06             |

---

### Interpretation (why this is interesting)

- **Compute-limited for small arrays**  
  Even when data is in L1, memcpy is ~23× faster than the scalar pack+unpack loop → the bit-packing math itself is the bottleneck.

- **Memory-limited for large arrays**  
  For big arrays, memcpy is ~300× faster, showing that the current implementation issues lots of small, inefficient loads/stores and doesn’t fully exploit memory bandwidth.

This is exactly what the next phases are aimed at improving.



## API & example usage

The exact function names may evolve, but the core API is conceptually:

```
// Pack `in_num` 32-bit integers (each in [0, 2^bit_width)) into `out`.
void byte_pack(const uint32_t* in,
               uint8_t*        out,
               int             bit_width,
               int             in_num,
               int             out_num_bytes);

// Unpack from `in` bitstream into `out_num` 32-bit integers.
void byte_unpack(const uint8_t* in,
                 uint32_t*      out,
                 int            bit_width,
                 int            out_num);

```

how you’d use it:

```

#include "byte_pack.h"
#include <vector>
#include <cstdint>
#include <cassert>
#include <iostream>

int main() {
    const int bit_width = 6;         // values must be in [0, 2^6)
    const int N         = 1024;

    std::vector<uint32_t> input(N);
    for (int i = 0; i < N; ++i) {
        input[i] = i & ((1u << bit_width) - 1);  // some pattern that fits in 6 bits
    }

    // Compute how many bytes we need for N values at `bit_width` bits.
    const int total_bits      = N * bit_width;
    const int out_num_bytes   = (total_bits + 7) / 8;

    std::vector<uint8_t> packed(out_num_bytes, 0);
    std::vector<uint32_t> output(N, 0);

    byte_pack(input.data(), packed.data(), bit_width, N, out_num_bytes);
    byte_unpack(packed.data(), output.data(), bit_width, N);

    // Validate
    for (int i = 0; i < N; ++i) {
        assert(input[i] == output[i]);
    }

    std::cout << "Pack/unpack successful for bit_width = "
              << bit_width << ", N = " << N << "\n";
    return 0;
}
```

## Design notes

Why pack in the first place?
- Reduce memory footprint for arrays of small integers.
- Improve cache behavior by fitting more logical values per cache line.
- When combined with other tricks (delta encoding, RLE), this becomes a building block for more advanced compression.

Current scalar design

- Uses bit shifts and masks to insert/extract values into/from a byte buffer.
- Supports a parameterized bit_width, so the same code handles 3, 6, 7, 8, 16-bit cases, etc.




### Identified bottlenecks

From the benchmark numbers and code inspection:

Compute bottleneck (L1-hot data):

- Lots of scalar shift/mask operations per value.
- Some branching / non-linear control flow in inner loops.
- No unrolling and limited use of 64-bit accumulators.

Memory bottleneck (large arrays):

- Byte-wise stores that cause read-for-ownership and write-allocate traffic.
- Possibly suboptimal layout/tiling wrt last-level cache.
- Pack+unpack on the same region back-to-back → store→load stalls.


## Future work 


For memory behavior:
- Tile to LLC size (e.g., 1–2 MiB): pack/unpack in tiles to stay closer to cache.
- Widen stores: build 64-bit (or 128-bit) words and store them as whole words; avoid scattered byte writes.
- Avoid immediate reread: don’t unpack the same region right after pack when benchmarking; separate write-heavy and read-heavy phases.
- Alignment & prefetch:
    - Align buffers to 64-byte cache lines.
    - Prefetch input a few cache lines ahead for large arrays.

For compute behavior:

- Specialize per bit-width: Use templates or switch-based dispatch so each bit_width has its own tight inner loop.
- Branchless 64-bit accumulator loop: Accumulate bits in a 64-bit register, flush full bytes/words when ready.
- Wide integer / SIMD ops: Use 64/128-bit ops or AVX2/NEON intrinsics to pack/unpack multiple values in parallel.
