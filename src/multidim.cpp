#include <algorithm>
#include <unordered_map>
#include <vector>

#include <fmt/ranges.h>
#include <small_vector.hpp>

#include <multidim.hpp>
#include "multidim_internal.hpp"
#include "smalldim_opt.hpp"

#define ENABLE_UNORDERED_DIMENSIONS 0
#define LOW_DIM 4    // up to 4D: inline, vectorization friendly

#ifdef MULTIDIM_DEBUG
#define mdebug(...) fmt::println(__VA_ARGS__)
#else
#define mdebug(...)
#endif

namespace multidim {


template <typename T=MultiIndexT>
inline T expand_index(const MultiIndexT& index, const DimensionsT& dims) {
    T out(dims.size());
    std::fill(out.begin(), out.end(), 0);
    size_t cur_i = 0;
    for (auto dim : dims) {
        out[dim] = index[cur_i++];
    }
    return out;
}

/// @brief Combines two dimension maps.
/// @note Dimension maps are not in the hot path, we use the generic smallvec
DimCombination combine_dimensions(const DimensionsT& dims1, const DimensionsT& dims2) {
#if ENABLE_UNORDERED_DIMENSIONS
    // sort dims
#endif
    // go through the dimensions, which are ordered
    DimCombination new_dims;
    new_dims.dimensions.reserve(dims1.size() + dims2.size() - 1);
    for (size_t i=0, j=0;;) {
        if (i >= dims1.size()) {
            if (j >= dims2.size()) {
                break;
            }
            new_dims.dimensions.push_back(dims2[j]);
            j++;
            continue;
        }
        if (j >= dims2.size()) {
            new_dims.dimensions.push_back(dims1[i]);
            i++;
            continue;
        }
        if (dims1[i] == dims2[j]) {
            new_dims.common.push_back(dims1[i]);
            new_dims.dimensions.push_back(dims1[i]);
            i++;
            j++;
            continue;
        }
        if (dims1[i] < dims2[j]) {
            new_dims.dimensions.push_back(dims1[i]);
            i++;
        } else {
            new_dims.dimensions.push_back(dims2[j]);
            j++;
        }
    }
    return new_dims;
}

template <typename T=MultiIndexT>
DimensionsT filter(const T& point, const DimensionsT& filter) {
    DimensionsT sub_point;
    sub_point.reserve(filter.size());
    for (const auto& dim : filter) {
        sub_point.push_back(point[dim]);
    }
    return sub_point;
}

template <typename T=MultiIndexT>
struct HashByDim {
    HashByDim(const DimensionsT& key_dims)
        : key_dims_{key_dims} {}

    std::size_t operator()(const T& index) const { 
        // Compute individual hash values to compose final hash
        // http://stackoverflow.com/a/1646913/126995
        size_t res = 17;
        for (const auto& dim : key_dims_) {
            res = res * 31 + std::hash<size_t>()(index[dim]);
        }
        return res;
     }
  
private: 
    const DimensionsT key_dims_;
};

template <typename T=MultiIndexT>
decltype(auto) map_indices(const MultiDimIndices& indices, const HashByDim<T>& hasher) {
    std::unordered_multimap<size_t, T> out_map;
    for (const auto& index : indices.multidimensionalIndexArray) {
        auto expanded_index = expand_index<T>(index, indices.dimensionArray);
        out_map.emplace(hasher(expanded_index), expanded_index);
    }

#ifdef MULTIDIM_DEBUG
    for (const auto& [key, val] : out_map) {
        fmt::println(" - {} => {}", key, val);
    }
#endif
    return out_map;
}

/// @brief Combines two arrays of multi-indices
template <typename T=MultiIndexT>
MDIndexArrayT combine_index_arrays(const MultiDimIndices& indices1, 
                                   const MultiDimIndices& indices2,
                                   const DimensionsT& common_dims) {
    MDIndexArrayT index_arr_out{};
    if (common_dims.empty()) {
        return index_arr_out;
    }
    const auto larger_dim = common_dims.back();
    
    // Create a hasher for our type, considering only the important dimensions
    HashByDim<T> hasher(common_dims);

    // indices 1 are those which get mapped
    auto index = map_indices(indices1, hasher);

    // We then go along the second array
    // Important: We use bitwise-or to combine indices with matching dims 
    // in a vectorization-friendly way:
    //   - No conditionals and no data indirection 
    //   - Common dimensions values: values must be same -> bw-OR returns same value
    //   -   Otherwise: one value is 0 -> bw-OR returns the only value
    
    for (const auto& index2 : indices2.multidimensionalIndexArray) {
        auto index2_exp = expand_index(index2, indices2.dimensionArray);
        mdebug("   - getting indices with hash {}", hasher(index2_exp));
        
        auto [it_begin, it_end] = index.equal_range(hasher(index2_exp));
        for (auto index1_exp = it_begin; index1_exp != it_end; index1_exp++) {
            mdebug("   - merging {} + {} ", index2_exp, index1_exp->second);
            
            auto out_index = index1_exp->second;
            for (size_t cur_dim=0; cur_dim<larger_dim; cur_dim++) {
                out_index[cur_dim] |= index2_exp[cur_dim];
            }
            mdebug("     Res = {}", out_index);
            index_arr_out.emplace_back(std::move(out_index));
        }
    }
    return index_arr_out;
}

MultiDimIndices combine_indices_f(const MultiDimIndices& a, const MultiDimIndices& b) {
    MultiDimIndices multidim_out;
    auto new_dims = combine_dimensions(a.dimensionArray, b.dimensionArray);
    multidim_out.dimensionArray = std::move(new_dims.dimensions);
    mdebug("Common dimensions = {}", new_dims.common);
    mdebug("New    dimensions = {}", c.dimensionArray);

    multidim_out.multidimensionalIndexArray = combine_index_arrays(a, b, new_dims.common);
    // operate in full width points
    // auto largest_dim = c.dimensionArray.back();

    // if (largest_dim < LOW_DIM) {
    //     // up to 3D - single specialization
    //     combine_index_arrays()
        
    // else if (largest_dim < 8) { // up to 8 dims?
    //     fmt::println("Larger array");
    //     auto index = index_indices<4>(a, new_dims.common);
    //     for (const auto& [key, val] : index) {
    //         fmt::println(" - {} => [{}]", key, val);
    //     }
    // } 
    // else {
    //     //otherwise use small-vec, which might still keep things memory inlined if small nr of elems
    //     auto index = index_indices_multi(a, new_dims.common);
    // }

    return multidim_out;
}

} // multi-dim namespace
