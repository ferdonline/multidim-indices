#include <unordered_map>
#include <vector>

#include <fmt/ranges.h>
#include <small_vector.hpp>

#define ALLOW_UNORDERED_DIMENSIONS 0

using InputIndexT = gch::small_vector<size_t>;
using MDIndexArrayT = std::vector<InputIndexT>;
using DimensionsT = gch::small_vector<size_t>;

template <size_t NDim>
using IndexT = std::array<size_t, NDim>;

struct MultiDimIndices {
    MDIndexArrayT multidimensionalIndexArray;
    DimensionsT dimensionArray;
};

template <size_t NDim>
decltype(auto) expand_index(const InputIndexT& indices, const DimensionsT& dims) {
    IndexT<NDim> out;
    out.fill(0);
    size_t cur_i = 0;
    for (auto dim : dims) {
        out[dim] = indices[cur_i++];
    }
    return out;
}

struct DimCombination {
    gch::small_vector<size_t> dimensions;
    gch::small_vector<size_t> common;
};

DimCombination combine_dimensions(const DimensionsT& dims1, const DimensionsT& dims2) {
#if ALLOW_UNORDERED_DIMENSIONS
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

template <size_t NDim>
DimensionsT filter(const IndexT<NDim>& point, const DimensionsT& filter) {
    DimensionsT sub_point;
    sub_point.reserve(filter.size());
    for (const auto& dim : filter) {
        sub_point.push_back(point[dim]);
    }
    return sub_point;
}

template<size_t NDim>
struct HashByDim {
    HashByDim(const DimensionsT& key_dims)
        : key_dims_{key_dims} {}

    std::size_t operator()(const IndexT<NDim>& index) const { 
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

template <size_t NDim>
decltype(auto) index_indices(const MultiDimIndices& indices, const HashByDim<NDim>& hasher) {
    std::unordered_multimap<size_t, IndexT<NDim>> out_map;
    for (const auto& index : indices.multidimensionalIndexArray) {
        auto expanded_index = expand_index<NDim>(index, indices.dimensionArray);
        out_map.emplace(hasher(expanded_index), expanded_index);
    }
    return out_map;
}

#define LOW_DIM 3


MultiDimIndices combine_indices_f(const MultiDimIndices& a, const MultiDimIndices& b) {
    MultiDimIndices c;
    auto new_dims = combine_dimensions(a.dimensionArray, b.dimensionArray);
    c.dimensionArray = std::move(new_dims.dimensions);
    fmt::println("Common dimensions = [{}]", fmt::join(new_dims.common, ", "));
    fmt::println("New    dimensions = [{}]", fmt::join(c.dimensionArray, ", "));


    // operate in full width points
    auto largest_dim = c.dimensionArray.back();

    if (largest_dim < LOW_DIM) {
        // up to 3D - single specialization
        HashByDim<LOW_DIM> hasher(new_dims.common);
        auto index = index_indices<LOW_DIM>(a, hasher);

        for (const auto& [key, val] : index) {
            fmt::println(" - {} => [{}]", key, fmt::join(val, ", "));
        }

        for (const auto& b_index : b.multidimensionalIndexArray) {
            auto b_index_e = expand_index<LOW_DIM>(b_index, b.dimensionArray);
            fmt::println("   - getting indices with hash {}", hasher(b_index_e));
            auto [it_begin, it_end] = index.equal_range(hasher(b_index_e));
            for (auto a_index_e = it_begin; a_index_e != it_end; a_index_e++) {
                fmt::println("   - merging [{}] + [{}]", fmt::join(b_index_e, ", "), fmt::join(a_index_e->second, ", "));
                auto out_index = a_index_e->second;
                for (size_t cur_dim=0; cur_dim<LOW_DIM; cur_dim++) {
                    // Important: We use bitwise-or to combine values without IFs
                    //   - Common dimensions: values must be same -> bw-OR returns same value
                    //   - Not common dims: one value is 0 -> bw-OR returns the only value
                    out_index[cur_dim] |= b_index_e[cur_dim];
                }
                fmt::println("     Res = [{}]", fmt::join(out_index, ", "));
            }
        }
    }
     else if (largest_dim < 6) { // up to 6 dims?
        fmt::println("Larger array");
        auto index = index_indices<4>(a, new_dims.common);
        for (const auto& [key, val] : index) {
            fmt::println(" - {} => [{}]", key, fmt::join(val, ", "));
        }
    }

    // otherwise use small-vec, which might still keep things memory inlined if small nr of elems
    //    auto index = index_indices_multi(a, new_dims.common);
    // }

    return c;
}


void test1() {
    MultiDimIndices A, B;
    A.multidimensionalIndexArray = {{0, 0}, {0, 1}, {1, 0}};
    A.dimensionArray = {0, 1};
    B.multidimensionalIndexArray = {{0, 2}, {1, 3}};
    B.dimensionArray = {0, 2};

    auto C = combine_indices_f(A, B);

    auto out_arr = expand_index<3>({1,2,3}, {1,2});
    fmt::println("{}", fmt::join(out_arr, ", "));
}

void test2() {
    MultiDimIndices A, B;
    A.multidimensionalIndexArray = {{2, 0, 1}};
    A.dimensionArray = {1, 2, 3};
    B.multidimensionalIndexArray = {{2, 4, 7}};
    B.dimensionArray = {1, 4, 5};

    auto C = combine_indices_f(A, B);
}

int main() {
    test1();
    test2();
    return 0;
}