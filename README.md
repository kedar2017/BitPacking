# BitPacking

Bit-packing = take integers that fit in, say, 3–7 bits and densely stuff them into bytes/words to cut memory & bandwidth.

That’s one of the fundamental inner loops you’d need when:

- Storing dense bit arrays (1-bit flags, masks, occupancy maps, compressed posting lists);
- Compressing integer IDs, run-lengths, or deltas into minimal bits;
- Doing vectorized count, and, or, xor, or population-count operations.

Phases of this project: 

Phase1: 
- Pack fixed-bit-width integers (bw ∈ {1…8,16}) into a byte stream.
- Unpack back to 32-bit or 16-bit integers.
- Validated thoroughly: create random inputs, patterned inputs 

Phase 2 – Benchmark Harness
- Small functions to help calculate the pack/unpack throughput 
- Time both the L1-hot (small size data) and streaming (large data) to give insights into compute versus memory bandwidth bottlenecks 
- Comparison in each case is made against memcpy 


```
Memory throughput (pack/unpack): 1.37452 GB/s 
Memory throughput (memcpy): 23.7836 GB/s 
Memory throughput - large array (pack/unpack): 0.077187 GB/s 
Memory throughput - large array (memcpy): 24.0649 GB/s 
```


Based on the above, it's realized that the current implementation is both compute limited and memory limited. 
Compute limited because even with very small data size, we can be assured that the data resides in the L1-cache and still memcpy is 23x faster 
Memory limited because in case of large arrays where the data needs to be fetched from the memory, memcpy is 300x faster 

For memory limitation, I will be optimizing:
- Tile to LLC (e.g., 1–2 MiB). Do pack for a tile (and, if needed, unpack later), then move on.
- Widen stores: build 64-bit (or 128-bit) chunks and store whole words; avoid byte stores that trigger read-for-ownership.
- Avoid immediate reread: don’t unpack the same region right after pack; that creates store→load stalls.
- Consider non-temporal stores for big, write-only outputs (esp. unpack’s 4N writes).
- First-touch and align buffers to 64 B; prefetch input a few lines ahead.

For compute limitation, I will be optimizing:
- Specialize per bw (constant shift counts), unroll, and make the inner loop branchless
- Use wide ops (64/128-bit) or intrinsics (AVX2/NEON) to assemble multiple outputs at once