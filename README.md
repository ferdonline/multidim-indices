# Multi Dimension Index Folding

**Performance optimization project**

## 1. Description

Consider a structure which defines an array of sparse indices of N-Dimensional space. Sparse becuse we are lacking some
of the dimensions.

E.g. If the indices are known for dimensions 1 and 2 we may them represent as
```javascript
A = [
    indices: [{0, 0}, {0, 1}, {1, 0}],
    dimensions: {0, 1}  // 1 and 2 converted to base 0
]
```

consider now that we want to combine these indices with another set, to obtain an index of higher dimensionality.
```javascript
B = [
    indices: [{0, 2}, {1, 3}],
    dimensions: [1, 2]  // dims 2 and 3
]
```

Given the second set of indices refer to dimensions 2 and 3, one can possibly get a third index set C containing all
three dimensions. Note that nothing prevents the number of dimensions from being different.

The rules to combine the indices are as follows:

 - At least one common dimension
 - Values in common dimensions must be the same.

For example, by applying `C = combine(A, B)` one would get

```javascript
C = [
    indices: [{0, 0, 2}, {0, 1, 2}, {1, 0, 3}],
    dimensions: [0, 1, 2]  // dims 1, 2 and 3
]
```

Notice several combinations were discarded since values in common dimensions were not the same. If they agree the
remaining part of each index (from non-shared dimensions) are appended.

## 2. Compiling the project

Project is CMake based. Typically all you need to do is.

```sh
cmake . -B build
cd build && make
```

It will compile in RelWithDebInfo. However for attempting max performance, please use `cmake .
-DCMAKE_BUILD_TYPE=Release -B build` instead.

For building a very verbose debug version, call cmake with `-DMULTIDIM_DEBUG=1`

### Out binaries

When compiling as above, two main binaries are generated under build:

 - `test_unit` - A set of simple tests to assert basic functionality is correct
 - `benchmark` - A stress program which generates two random datasets of 4 million indices each and combines them. Due
  to the large amount of out data, the binary is compiled with `-DMULTIDIM_BENCHMARKING=1`, a flag that skips the final
vector append. Some stats are shown.

#### Benchmarking program

Under tests, `benchmark.cpp` implements a program which tests the `multidim` library with a relatively large data set.
It generates random index arrays, and possibly random dimensions (disabled for performance reproducibility).

The default parameters, hardcoded as `constexpr` are:
 - N_DIMENSIONS = 4           // 4-dimensional domain  (M)
 - MAX_INDEX_VALUE = 10000    // Each index for a single dimension in the range [0-10k]
 - MAX_INDICES_LEN = 1 << 22; // Each MultiDimIndexArray contains 4M indices (N)

- The benchmark runs in approximately 10 seconds in an x86_64 Intel CPU (Comet Lake). A typical run looks like
```sh
$ time ./benchmark
Indexing............................................ OK (32001 buckets)
Generating new indices...
[  0%] Generated 5 indices
[  5%] Generated 838560 indices
...
[ 98%] Generated 17163576 indices

real    0m9.735s
```

## 3. Algorithm Optimization

To improve the performance at the algorithm level some assumptions were made:
  1. The input indices arrays can be arbitrarily large (N), but fit in main memory
  2. The highest index value can be in the order of billions
  3. The number of dimensions (M) is much smaller than N, typically up to 10, even though it could be in the order of
     1000s.
  4. The highest dimension will be in the same order of magnitude as M, although it could  be in the order of millions.

To combine two arrays (size N) in a resource effective manner one must, in the first place, avoid comparing all-to-all
which would incur complexity O(N * N). Second, since we can't avoid considering an index, e.g. from A set, against a
number of elements from set B, we implement some preprocessing so it's applied only once per index, avoiding an
additional complexity factor O(N * M).

### 3.1 Mapping elements

To address the first concern one grouped all indices from one of the arrays (e.g. A) and indexed them by their values on
the common dimensions. This is possible given the common set of dimensions is static and can be computed ahead of time.
The mapping is an O(N*M) operation; however by considering M small, it can be accounted for a constant factor.

If A indices are mapped, one can go along indices from B and get all matching indices from A in a single O(1) map.get().
At this point it matters that the "merging" of two indices is fast as well. That's explained in the next point.

### 3.2 Preprocess for O(N) index merging

As one goes along indices from set B, for each we can obtain the bucket of A-indices whose common dimensions match. The
bucket can still hold many values, in particular if the domain of the keys is small (e.g. a single common dimension, or
the indices domain is small).

For maximum speed it would be great to "sum" the two indices in the simplest possible way, especially without
indirections or conditionals. However that can't be done in their original format. The "dimensions" array imposes an
indirection which we have to address, which we do via preprocessing.

At the time we map/bucket one of the arrays, instead of storing the original index, we can store a version with the
required output "dimensions". For dimensions which don't figure in the current index, a neutral 0 is used.

E.g. in the original example, if we were to index B indices, we would

 - take Index `{0, 2}` - transform to `{0, (0), 2}`  (expand dimensions from {1,2} to {0,1,2})
    - Assign it to bucket with key 0 (given the common dimension (1) has value 0)
 - take Index `{1, 3}` - transform to `{0, (1), 3}`
    - Assign to bucket with key 1

With the indices in this format one doesn't need indirections to combine them. Unfortunately we cannot simply sum them
due to the common dimensions - we don't want the value to double. However a bitwise-OR will do just what we want, since
we are either OR-ing the same number (from common dimensions), or a number with the neutral zero mentioned before.

### 3.3 Computing the merged dimensions

Given the current set of assumptions, where Dimensions arrays are very small, no special attention was given to this
step. Still, considering the arrays sorted (and sorting if needed), one implemented a "merge" algorithm featuring linear complexity (see `combine_dimensions()`).

## 4. Low-Level Optimization

To take the advantage of modern CPUs, in particular those based on recent x86_64 with vectorized instructions and large
cache sizes, attention was made to the implementation. Generally, it was attempted to have inner loops without
branching, applied over contiguous arrays. With that one targets a higher cache locality, and a higher chance for the
compiler to employ SIMD instructions.

To enable GCC and Clang compilers auto-vectorization, `-march=native` and `-O3` flag were enabled (with
`-DCMAKE_BUILD_TYPE=Release`). We can notice vectorization does happen as the compiler outputs messages like
`small_vector.hpp:2267:86: optimized: basic block part vectorized using 32 byte vectors`

### 4.1 Improving Memory locality

To improve performance, reducing memory indirections is of utmost importance. Without a-priory knowledge of the arrays
length, space must be allocated from the heap (std::vector behavior). However, as per assumption 3, we may be able to
optimize for a common case - up to a handful of dimensions. As so, one could use a structure that uses stack space first
until a certain number of elements, after which it would claim heap space. The concept is known as small_vector and
several open-source implementations exist. For simplicity we opted for a header only implementation from Gene Harvey
(gch/small_vector.hpp). By default it can hold 64 bytes within which translates to 8 * uint64_t (our IndexElemT) or 16 *
uint32_t (our DimensionT). We kept both counts consistent at 8.

When reading along vectors of these structures elements are guaranteed to be inlined and 100% contiguous, making
effective use of cache. The effects are expressive. For comparison, a version compiled using purely `std::vector`s
displayed the following timings:
```
real    1m2.193s
user    1m1.035s
sys     0m0.361s
```

Against the results from section [2] (9.7s), they run slower by a factor of 6x.

For that result contributed the sequential processing of `combine_index_arrays`, in particular the inner-most loop
(joining two indices with OR), operating on the contiguous elements.

### 4.2 Manual vectorization & loop unrolling

Compiler Automatic vectorization might not yield perfect vectorization. Therefore, in the separate `smalldim_opt.hpp`
source, some exploratory work was done to implement core routines in an optimized way.

The function `filter_index()` gets a specialization for the type `CompactIndexT` (aka an std::array of static size 4),
and attempts to aggressively optimize the most common case: up to 4 dimensions. Given a 4D index is composed of 64bit
unsigned values, we could in principle operate on the whole array with a single AVX2 instruction. Specifically
`_mm256_i32gather_epi64` could be useful to filter the index dimensions. However, due to limited applicability and the
existence of several corner cases that path was not pursued.

As an alternative, a manual loop unrolling was implemented to take advantage of the CPU multiple ALU ports.

## 5 Benchmarking Insights

Where is time being spent? Is it worth more vectorization?

We collected some profiling data using HPCtoolkit.
```sh
hpcrun -c f1000000 -e PERF_COUNT_HW_CPU_CYCLES -e REALTIME ./benchmark
hpcstruct hpctoolkit-benchmark-measurements/
hpcprof -S benchmark.hpcstruct hpctoolkit-benchmark-measurements/
```
![hpctoolkit](docs/profile-1a.png)

It is noticeable that the largest amount of time is spent in the main loop of `combine_index_arrays`. However, one fact
stand out: it is NOT directly code inside loop that take the most time (7.4%). Instead it is its children, in particular
line 167 for the retrieval of the range from the map (see [full_profile_image](docs/profile-1.png)), accounting for 62%
of the time.

With larger volumes of data is expected that retrieving data gets slower, as data does not fit in cache, and given the
access pattern is basically random we might be facing cache misses. To be sure about that, some perf analysis was
performed.

```
perf stat -e  cache-references,cache-misses,instructions ./benchmark
Indexing............................................ OK (32001 buckets)
 Performance counter stats for './benchmark':

        2055030136      cache-references
        1392850474      cache-misses                     #   67.78% of all cache refs
       50675335326      instructions

      10.518933821 seconds time elapsed

       9.710279000 seconds user
       0.404193000 seconds sys
```

Ouch! Even if anticipated, 67,8% of cache misses is an expressive value which aggressively bottlenecks performance.

Let's dig deeper.

```
sudo perf stat -M TopdownL1 ./benchmark
(...)
       50603231646      INST_RETIRED.ANY                 #     35.4 %  tma_retiring
         224410793      CPU_CLK_UNHALTED.REF_XCLK        #      2.6 %  tma_frontend_bound
                                                         #     60.0 %  tma_backend_bound
                                                         #      2.1 %  tma_bad_speculation
        3221252522      IDQ_UOPS_NOT_DELIVERED.CORE
         159418755      INT_MISC.RECOVERY_CYCLES_ANY
         218940885      CPU_CLK_UNHALTED.ONE_THREAD_ACTIVE
       31282403841      CPU_CLK_UNHALTED.THREAD
        7341695615      UOPS_RETIRED.MACRO_FUSED
       43694353015      UOPS_RETIRED.RETIRE_SLOTS
       45936574074      UOPS_ISSUED.ANY

      10.333821638 seconds time elapsed

       9.479807000 seconds user
       0.411749000 seconds sys


sudo perf stat -M TopdownL2 ./benchmark
(...)
 Performance counter stats for './benchmark':

       49512005695      INST_RETIRED.ANY                 #      2.8 %  tma_heavy_operations
                                                         #     34.0 %  tma_light_operations     (29.19%)
         222978644      CPU_CLK_UNHALTED.REF_XCLK        #      1.5 %  tma_fetch_bandwidth
                                                         #      1.2 %  tma_fetch_latency
                                                         #      0.2 %  tma_branch_mispredicts
                                                         #      0.0 %  tma_machine_clears
                                                         #     11.4 %  tma_core_bound
                                                         #     48.8 %  tma_memory_bound
        3381641335      IDQ_UOPS_NOT_DELIVERED.CORE
         406136936      EXE_ACTIVITY.BOUND_ON_STORES
        1994383700      EXE_ACTIVITY.1_PORTS_UTIL
          24107861      BR_MISP_RETIRED.ALL_BRANCHES
         156868609      INT_MISC.RECOVERY_CYCLES_ANY
         220331304      CPU_CLK_UNHALTED.ONE_THREAD_ACTIVE
       30958604463      CPU_CLK_UNHALTED.THREAD
       45333783693      UOPS_RETIRED.RETIRE_SLOTS
       16521345428      CYCLE_ACTIVITY.STALLS_MEM_ANY
        7670902688      UOPS_RETIRED.MACRO_FUSED
         372767819      IDQ_UOPS_NOT_DELIVERED.CYCLES_0_UOPS_DELIV.CORE
        2772832240      EXE_ACTIVITY.2_PORTS_UTIL
       17463753247      CYCLE_ACTIVITY.STALLS_TOTAL
           1120541      MACHINE_CLEARS.COUNT
       45256790039      UOPS_ISSUED.ANY

      10.311962068 seconds time elapsed

# At the third level one can understand the memory bounds
sudo perf stat -M TopdownL3 ./benchmark

Performance counter stats for './benchmark':
(...)
         249258919      CPU_CLK_UNHALTED.REF_XCLK        #     11.2 %  tma_l2_bound
                                                         #     34.0 %  tma_dram_bound
(...)
       44789935013      UOPS_EXECUTED.THREAD
       12391490790      CYCLE_ACTIVITY.STALLS_L2_MISS
       16372851331      CYCLE_ACTIVITY.STALLS_L1D_MISS
       44698394115      UOPS_RETIRED.RETIRE_SLOTS
         104110272      MEM_LOAD_RETIRED.L1_MISS
          33816850      MEM_LOAD_RETIRED.L2_HIT
       45622746450      UOPS_ISSUED.ANY
         240362187      CPU_CLK_UNHALTED.REF_XCLK        #      0.0 %  tma_store_bound
                                                         #      0.3 %  tma_divider
                                                         #      9.0 %  tma_ports_utilization
(...)
                                                         #      6.9 %  tma_l3_bound
                                                         #      2.2 %  tma_l1_bound
```

From TopdownL2 Metrics one can directly observe we are memory bandwidth bound, as tma_memory_bound amounts to 48.8%,
whereas "core-bound" operations only amount to 11%. From the TopdownL3 metrics one can observe memory contention comes
mostly from DRAM (34%), which is expected since the whole HashMap won't fit in CPU L2/L3 caches and it is being randomly
accessed.

## Multi-Threading, an option?

These observations suggest that introducing multi-threading in the current implementation might not help:
 - Contention happens mostly at DRAM level, and more threads competing for the same resource might worsen the situation.
 - Physical cores share L3 cache, so running on several of them would likely lead to cache trashing and more L3 misses.
 - The added complexity for multi-threading would further negatively affect performance, particularly if any sort of
   synchronization was required

The current implementation supposes large data sets, in which "only" one of MultiDimensionalIndexArrays requires
indexing. This might be an acceptable compromise, in particular if one of the datasets is substantially smaller than the
other. In case it was acceptable both indices to be held in memory, a possibility would be to create clusters (shards)
where aggregation could then be among related work units. If such technique showed to exhibit less cache misses, then
one could potentially consider multiple cores. However such technique is more complex and requires additional upfront
time which might be difficult to recover later on.
