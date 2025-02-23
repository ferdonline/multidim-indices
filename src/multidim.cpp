#include <algorithm>
#include <unordered_map>
#include <vector>

#include <gch/small_vector.hpp>

#include <multidim.hpp>
#include "multidim_internal.hpp"

#define ENABLE_UNORDERED_DIMENSIONS 0
#define LOW_DIM 4    // up to 4D: inline, vectorization friendly

#ifdef MULTIDIM_DEBUG
#include <fmt/format.h>
#define mdebug(...) ::fmt::println(__VA_ARGS__)
#else
#define mdebug(...)
#endif

namespace multidim {


/// @brief Expands an index from its original dimensions to the combined ones
/// The extended version has "holes", i.e. positions where the index is 0
/// @note Index being zero is an important design decision, since it will be "OR"ed with the second index
/// @param index The index to be "expanded"
/// @param dims The dimensions of the input index
/// @param out_dims The output dimens
/// @return  The expanded
template <typename ArrayT>
inline ArrayT expand_index(const MultiIndexT& index, const DimensionsT& dims, const DimensionsT& out_dims) {
    /// First do a full-width expansion
    /// @todo use tmp stack space: alloca
    ArrayT out_index(out_dims.back() + 1); // max size
    std::fill(out_index.begin(), out_index.end(), 0);
    // first pass - expand the index to full dimensions
    size_t cur_i = 0;
    for (auto dim : dims) {
        out_index[dim] = index[cur_i++];
    }
    // second pass - select only the out dimensions (make "sparse")
    // Note Can can reuse the buffer since it will be shorter
    cur_i = 0;
    for (auto dim : out_dims) {
        out_index[cur_i++] = out_index[dim];
    }
    out_index.resize(cur_i);
    return out_index;
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

/// @brief A Hasher of a MultiIndex, given the relevant dimensions
/// @note Relevant dimensions are the common ones. We
struct HashByDim {
    HashByDim(const DimensionsT& key_dims)
        : key_dims_{key_dims} {}

    std::size_t operator()(const MultiIndexT& index) const {
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


/// @brief Function which creates a hash-map from all indices, indexed by the indices in common dimensions
/// @param indices The indices to be indexed
/// @param hasher The hasher object
/// @param out_dims The output dimensions, so that items are stored in the tree in their final shape
/// @return The hash map of the indices
decltype(auto) map_indices(const MultiDimIndices& indices, const HashByDim& hasher, const DimensionsT& out_dims) {
    std::unordered_multimap<size_t, MultiIndexT> out_map;
    fprintf(stderr, "Indexing...");
    size_t i=0;
    for (const auto& index : indices.multidimensionalIndexArray) {
        MultiIndexT expanded_index = expand_index<MultiIndexT>(index, indices.dimensionArray, out_dims);
        mdebug("in: {} out: {}", index, expanded_index);
        out_map.emplace(hasher(expanded_index), expanded_index);
        if (++i % 100000 == 0) { fprintf(stderr, "."); }
    }
#ifdef MULTIDIM_DEBUG
    for (const auto& [key, val] : out_map) {
        fmt::println(" - {} => {}", key, val);
    }
#endif
    return out_map;
}

/// @brief Combines two arrays of multi-indices
MDIndexArrayT combine_index_arrays(const MultiDimIndices& indices1,
                                   const MultiDimIndices& indices2,
                                   const DimCombination& new_dims) {
    MDIndexArrayT index_arr_out{};
    if (new_dims.common.empty()) {
        return index_arr_out;
    }
    const auto out_n_dimensions = new_dims.dimensions.size();

    // Create a hasher for our type, considering only the important dimensions
    HashByDim hasher(new_dims.common);

    // indices 1 are those which get mapped
    auto index = map_indices(indices1, hasher, new_dims.dimensions);

    // We then go along the second array
    // Important: We use bitwise-or to combine indices with matching dims
    // in a vectorization-friendly way:
    //   - No conditionals and no data indirection
    //   - Common dimensions values: values must be same -> bw-OR returns same value
    //   -   Otherwise: one value is 0 -> bw-OR returns the only value

    fprintf(stderr, "Matching indices...\n");
    size_t i=0;
    size_t merges = 0;

    for (const auto& index2 : indices2.multidimensionalIndexArray) {
        auto index2_exp = expand_index<MultiIndexT>(index2, indices2.dimensionArray, new_dims.dimensions);
        mdebug("   - getting indices with hash {}", hasher(index2_exp));

        auto [it_begin, it_end] = index.equal_range(hasher(index2_exp));
        for (auto index1_exp = it_begin; index1_exp != it_end; index1_exp++) {
            mdebug("   - merging {} + {} ", index2_exp, index1_exp->second);

            auto out_index = index1_exp->second;
            for (size_t cur_dim=0; cur_dim<out_n_dimensions; cur_dim++) {
                out_index[cur_dim] |= index2_exp[cur_dim];
            }
            mdebug("     Res = {}", out_index);

            index_arr_out.push_back(std::move(out_index));
            merges += 1;
        }
        // Give user some feedback
        if (++i % 100000 == 0) {
            fprintf(stderr, "Merged: %ld\n", merges);
        }
    }

    return index_arr_out;
}

MultiDimIndices combine_indices_f(const MultiDimIndices& a, const MultiDimIndices& b) {
    MultiDimIndices multidim_out;
    auto new_dims = combine_dimensions(a.dimensionArray, b.dimensionArray);
    mdebug("Common dimensions = {}", new_dims.common);
    mdebug("New    dimensions = {}", new_dims.dimensions);

    multidim_out.multidimensionalIndexArray = combine_index_arrays(a, b, new_dims);
    multidim_out.dimensionArray = std::move(new_dims.dimensions);

    return multidim_out;
}

} // multi-dim namespace
