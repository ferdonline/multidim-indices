#pragma once
#include <multidim.hpp>

namespace multidim {

struct DimCombination {
    gch::small_vector<uint32_t> dimensions;
    gch::small_vector<uint32_t> common;
};

template <typename T>
inline T expand_index(const MultiIndexT& index, const DimensionsT& dims, const DimensionsT& out_dims);

} // eof ns multidim
