#pragma once
#include <multidim.hpp>

namespace multidim {

struct DimCombination {
    DimensionsT dimensions;
    DimensionsT common;
};

template <typename T>
inline T expand_index(const MultiIndexT& index, const DimensionsT& dims, const DimensionsT& out_dims);

} // eof ns multidim
