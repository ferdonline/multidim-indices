/// Performance optimization with inlined elements and vector instructions

#include <multidim.hpp>
#include "multidim_internal.hpp"

namespace multidim {

/// @brief Generic compact structure optimizing for low-dimensional indices
template <size_t NDim>
using IndexT = std::array<size_t, NDim>;

// /// Template version specialized for 4D (4*8 B = 256 bits)
// template <>
// inline IndexT<4> expand_index(const InputIndexT& indices, const DimensionsT& dims) {
//     IndexT<4> out = {0, 0, 0, 0};
//     size_t cur_i = 0;
//     for (auto dim : dims) {
//         out[dim] = indices[cur_i++];
//     }
//     return out;
// }

}