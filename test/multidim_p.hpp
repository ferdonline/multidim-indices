/// Header containing the private API of MultiDim

#pragma once
#include <unordered_map>
#include "multidim.hpp"

namespace multidim {

/// @brief Relevant Information after dimension array being combined
struct DimCombination {
    DimensionsT dimensions;
    DimensionsT common;
};

/// @brief The type for the map, indexed by the common index hash
using IndicesMapT = std::unordered_map<size_t, std::vector<MultiIndexT>>;

/// @brief Combine two sets of dimensions. See full description in implementation
DimCombination combine_dimensions(const DimensionsT& dims1, const DimensionsT& dims2);

/// @brief Expands an index to its full length
/// expand_index is a 'small_vector<uint64>' which can hold up to 8 values without reaching for the heap
inline void expand_index(const MultiIndexT& index, const DimensionsT& in_dims, IndexElemT *const out_vec) {
    size_t max_elems = in_dims.back() + 1;
    std::fill(out_vec, out_vec+max_elems, 0ul);
    size_t cur_i = 0;
    for (auto dim : in_dims) {
        out_vec[dim] = index[cur_i++];
    }
}

/// @brief Selects only a few dimensions of an index, as specified in out_dims.
/// MultiIndexT is a 'small_vector<uint64>' which can hold up to 8 values without reaching for the heap
template <typename ArrT = MultiIndexT>
inline ArrT filter_index(const IndexElemT *const in_vec, const DimensionsT& out_dims) {
    ArrT out_index(out_dims.size());
    size_t cur_i = 0;
    for (auto dim : out_dims) {
        out_index[cur_i++] = in_vec[dim];
    }
    return out_index;
}

/// @brief A Hasher of a MultiIndex, given the relevant dimensions
/// @note Relevant dimensions (key_dims) are the common ones between two MultiDimIndices
struct HashByDim {
    HashByDim(const DimensionsT& key_dims)
        : key_dims_{key_dims} {}

    // The hash computing. See impl in respective .cpp
    inline std::size_t operator()(const IndexElemT *index) const;

private:
    const DimensionsT key_dims_;
};


} // eof ns multidim
