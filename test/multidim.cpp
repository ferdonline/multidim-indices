#include <algorithm>
#include <unordered_map>
#include <vector>

#include "gch/small_vector.hpp"

#include "multidim.hpp"
#include "multidim_p.hpp"

#define ENABLE_UNORDERED_DIMENSIONS 0
#define LOW_DIM 4    // up to 4D: inline, vectorization friendly

#ifdef MULTIDIM_DEBUG
#include <fmt/ranges.h>
#define mdebug(...) { fmt::print(__VA_ARGS__); printf("\n"); }
#else
#define mdebug(...)
#endif

namespace multidim {


/// @brief Combines two dimension arrays.
/// @note This algorithm has linear O(N) complexity as we assume input arrays
///       are sorted. If not we should activate a compile-time flag for sorting to happen.
/// @note Dimension maps are not in the hot path, we use the generic smallvec
DimCombination combine_dimensions(const DimensionsT& dims1, const DimensionsT& dims2) {
#if ENABLE_UNORDERED_DIMENSIONS
    // sort dims. de-scoped for now
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


/// @brief A Hasher of a MultiIndex, given the relevant dimensions (the common ones)
/// @note See struct defined in multidim.hpp
/// This function takes a whole multi-dim index and hashes CONSIDERING ONLY 'key_dims_' given in the constructor.
/// As so, it doesn't require copying sub-selections of the index.
/// @note We purposely use the generated hash directly as the HashMap key as it proved
/// to be more than an order of magnitude faster. The slow version is in git branch `enh/index_by_smallvec`
inline std::size_t HashByDim::operator()(const IndexElemT *index) const {
    // Compute individual hash values to compose final hash
    // http://stackoverflow.com/a/1646913/126995
    size_t res = 17;
    for (const auto& dim : key_dims_) {
        res = res * 31 + std::hash<size_t>()(index[dim]);
    }
    return res;
}


/// @brief Fully expands an index from its original dimensions
/// The extended version has "holes", i.e. positions where the index is 0
/// @note Index being zero is an important design decision, since it will be "OR"ed with the second index
/// @param index The index to be "expanded"
/// @param in_dims The dimensions of the input index
/// @param out_vec A pointer to a buffer capable of holding the whole expanded index
/// @note: expand_index is templated and implemented in header multidim_p.hpp so it could be shared and inlined.
inline void expand_index(const MultiIndexT& index, const DimensionsT& in_dims, IndexElemT *const out_vec);


/// @brief Selects only a few dimensions of an index, as specified in out_dims.
/// @param in_vec: the expanded index array
/// @param out_dims: The dimensions we want to get from the index
/// @note: this routine is not bounds checked - the buffer should have all requested dimensions
/// @note: filter_index is templated and implemented in header multidim_p.hpp so it could be shared and inlined.
/// @note The default type 'MultiIndex' can hold up to 8 values within without reaching for the heap
// template <typename ArrT = MultiIndexT>
// inline MultiIndexT filter_index(const IndexElemT *const in_vec, const DimensionsT& out_dims);


/// @brief Function which creates a hash-map from all indices, indexed by the indices in common dimensions
/// @param indices The indices to be indexed
/// @param hasher The hasher object
/// @param out_dims The output dimensions, so that items are stored in the tree in their final shape
/// @return The hash map of the indices
IndicesMapT map_indices(const MultiDimIndices& indices, const HashByDim& hasher, const DimensionsT& out_dims) {
    IndicesMapT out_map{};

    // some really fast stack space (alloca) for the full expanded array, auto-reclaimed at the end
    // Full expanded array can be ~ large as it depends on the highest dimension value:
    //   e.g. out dimensions {10, 20, 30} require 30 + 1 elements
    // NOTE: out-dimensions is used to compute max_elems (instead of dims) so that the later loops have no conditions
    // NOTE: We must initialize the whole chunk to 0 to ensure no dirt data is there
    size_t max_elems = out_dims.back() + 1;
    auto expanded_index = static_cast<uint64_t*>(alloca(max_elems * sizeof(IndexElemT)));
    std::fill(expanded_index, expanded_index+max_elems, 0ul);

    fprintf(stderr, "Indexing...");
    size_t i=0;
    for (const auto& index : indices.multidimensionalIndexArray) {
        expand_index(index, indices.dimensionArray, expanded_index);
        auto hash = hasher(expanded_index);
        auto bucket = out_map.find(hash);
        if (bucket == out_map.end()) {
            auto [new_bucket, inserted] = out_map.emplace(hash, 0);
            new_bucket->second.push_back(filter_index(expanded_index, out_dims));
        } else {
            bucket->second.push_back(filter_index(expanded_index, out_dims));
        }
        if (++i % 100000 == 0) { fprintf(stderr, "."); }
    }
#ifdef MULTIDIM_DEBUG
    for (const auto& [key, val] : out_map) {
        mdebug(" - {} => {}", key, val);
    }
#endif
    fprintf(stderr, " OK (%ld buckets)\n", out_map.size());
    return out_map;
}

///
/// @brief Combines two arrays of multi-indices
///
/// This is the central function, which takes the indices (arrays) from the two sets
/// of MultiDimensionalArray, plus the output dimensions and does the matching and aggregation.
///
/// @param indices1: The array of indices from the first multi-dimensional-indices structure
/// @param indices2: The array of indices from the second multi-dimensional-indices structure
/// @param new_dims: The new set of dimensions the "joined" indices should feature
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

    // temp chunk. See rationale in `map_indices()`
    size_t max_elems = new_dims.dimensions.back() + 1;
    auto index2_exp = static_cast<uint64_t*>(alloca(max_elems * sizeof(IndexElemT)));
    std::fill(index2_exp, index2_exp+max_elems, 0ul);

    // Main processing loop
    // --------------------
    // We go along the second array
    //   - for each fetch the corresponding indices
    //      - for each possible pair, combine in a vectorization-friendly way:
    //      - No data indirection
    //      - contiguous elements processing with a single 'OR' instruction:
    //         - Common dimensions values: values are the same -> bw-OR returns same value
    //         - Otherwise: one of the values is 0 -> bw-OR returns the only value

    fprintf(stderr, "Generating new indices...\n");
    size_t arr2_len = indices2.multidimensionalIndexArray.size();
    size_t merges = 0;

    for (size_t i = 0; i < arr2_len; ++i) {
        auto index2 = indices2.multidimensionalIndexArray[i];
        mdebug(">> getting indices matching {}", index2);
        expand_index(index2, indices2.dimensionArray, index2_exp);
        const auto bucket = index.find(hasher(index2_exp));

        if (bucket == index.end()) {
            continue;  // no match. skip
        }

        // Now that we know there are corresponding indices, get this one in the right format
        const auto index2_final = filter_index(index2_exp, new_dims.dimensions);

        for (auto out_index : bucket->second) {
            mdebug("   - merging {} + {} ", final_index_2, out_index);

            for (size_t cur_dim=0; cur_dim<out_n_dimensions; cur_dim++) {
                // Due to hash collisions, we sadly have to filter.
                // Fortunately that proved to not impose any significant slowdown
                // (and is way faster than using the indexing dimensions as keys in the hashmap - over 10x)
                if (out_index[cur_dim] != 0 && index2_final[cur_dim] != 0 && out_index[cur_dim] != index2_final[cur_dim]) {
                    // in case of an error, break and continue outer loop for the next element
                    goto continue_outer_loop;
                }
                out_index[cur_dim] |= index2_final[cur_dim];
            }
            mdebug("     Res = {}", out_index);

            // Append to output array.
            // On large inputs this might exhaust memory so for benchmarks we skip it
            #ifndef MULTIDIM_BENCHMARKING
            index_arr_out.push_back(std::move(out_index));
            #endif
            merges += 1;

            continue_outer_loop:;
        }

        // Give user some feedback
        if (i % 100000 == 0) {
            auto progress_percent = i * 100.0 / indices2.multidimensionalIndexArray.size();
            fprintf(stderr, "[%3.0f%%] Generated %ld indices\n", progress_percent, merges);
        }
    }

    return index_arr_out;
}


/// @brief The top-level 'f' function, which combines indices structures.
/// @return A combined MultiDimIndices
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
