/// Performance optimization with inlined elements and vector instructions

#include <array>
#include <immintrin.h>

#include <multidim.hpp>
#include "multidim_p.hpp"


namespace multidim {

#define OPTIMIZED_DIM 4

/// @brief Generic compact structure optimizing for low-dimensional indices
using CompactIndexT = std::array<uint64_t, OPTIMIZED_DIM>;

// Template version specialized for 4D (4*8 B = 256 bits)
// For reference version of any dimensionality see multidim_p.hpp
template <>
inline CompactIndexT filter_index(const IndexElemT *const in_vec, const DimensionsT& out_dims) {
    CompactIndexT out{0};
    // Filter by required out dimensions / compact structure
    // Ideally we would use _mm256_i32gather_epi64(out.data(), idx, 4);
    // However that is only applicable with at least 4 dims out, and the need to make
    // memory aligned to 16 bytes is not worth it.
    // We take advantage of the constant size to unroll
    out[0] = in_vec[out_dims[0]];
    out[1] = (out_dims.size() >= 2)? in_vec[out_dims[1]] : 0;
    out[2] = (out_dims.size() >= 3)? in_vec[out_dims[2]] : 0;
    out[3] = (out_dims.size() >= 4)? in_vec[out_dims[3]] : 0;
    return out;
}

} // ns eof
