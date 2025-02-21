/// Performance optimization with inlined elements and vector instructions

#include <multidim.hpp>
#include "multidim_internal.hpp"

#include <immintrin.h>

namespace multidim {

#define OPTIMIZED_DIM 4

/// @brief Generic compact structure optimizing for low-dimensional indices
using CompactIndexT = std::array<uint64_t, OPTIMIZED_DIM>;

// Template version specialized for 4D (4*8 B = 256 bits)
// For reference version of any dimensionality see multidim.cpp
template <>
inline CompactIndexT expand_index(const MultiIndexT& index, const DimensionsT& dims, const DimensionsT& out_dims) {
    CompactIndexT out{0};
    // PART 1 - Full expansion
    // It would be great if we could  _mm256_i32scatter_epi64
    // However it only comes with AVX512 which is not widespread
    // unroll
    out[dims[0]] = index[0];
    if (dims.size() >= 2) out[dims[1]] = index[1];
    if (dims.size() >= 3) out[dims[2]] = index[2];
    if (dims.size() >= 4) out[dims[3]] = index[3];

    // PART 2 - Filter by required out dimensions / compact structure
    // __m256i gathered = _mm256_i32gather_epi64(out.data(), idx, 4);
    // Is it also unfortunate we dont know compile-time how many elements there are
    out[0] = out[out_dims[0]];
    out[1] = (out_dims.size() >= 2)? out[out_dims[1]] : 0;
    out[2] = (out_dims.size() >= 3)? out[out_dims[2]] : 0;
    out[3] = (out_dims.size() >= 4)? out[out_dims[3]] : 0;
    return out;
}

} // ns eof