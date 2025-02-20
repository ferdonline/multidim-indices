#pragma once
#include <vector>
#include <small_vector.hpp>

namespace multidim {

/// @brief Generic structure for highly dimensional indices
using MultiIndexT = gch::small_vector<size_t>;

/// @brief Type of the dimensions array
/// This structure is not in the critical path so we use
/// a `small_vector` which can grow onto the heap 
using DimensionsT = gch::small_vector<size_t>;

/// @brief The type of the Multi-Dimensional-Index-Array
/// We consider it arbitrarily large, therefore it's
/// heap-allocated (vector), and read/written once
using MDIndexArrayT = std::vector<MultiIndexT>;

/// @brief The MultiDimensionalIndices structure
/// The main program structure keeps the indices and their 
/// corresponding dimensions.
struct MultiDimIndices {
    MDIndexArrayT multidimensionalIndexArray{};
    DimensionsT dimensionArray{};
};

/// @brief The "f" function, which combines two MultiDimIndices 
MultiDimIndices combine_indices_f(const MultiDimIndices& a, const MultiDimIndices& b);

} // namespace multidim eof
