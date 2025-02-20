#pragma once
#include <multidim.hpp>

namespace multidim {

struct DimCombination {
    gch::small_vector<size_t> dimensions;
    gch::small_vector<size_t> common;
};

// DimCombination combine_dimensions(const DimensionsT& dims1, const DimensionsT& dims2);

} // eof ns multidim
