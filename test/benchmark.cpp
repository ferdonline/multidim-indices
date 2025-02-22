#include <random>
#include <iostream>

#include <gch/small_vector.hpp>
#include <multidim.hpp>

#ifdef MULTIDIM_DEBUG
#include <fmt/ranges.h>
#define mdebug(...) fmt::println(__VA_ARGS__)
#else
#define mdebug(...)
#endif

namespace md = multidim;

constexpr size_t MAX_DIM_LEN = 16;
constexpr size_t MAX_INDEX_VALUE = 100;
constexpr size_t MAX_INDICES_LEN = 1 << 20; // > MAX_INDEX_VALUE so that there is better probability of repeated
// constexpr size_t MAX_DIM_VALUE // impl specific. We randomly increase dim value

// Create very large random input arrays
struct InputArrays {

    InputArrays()
        : A{}, B{}
    {
        std::mt19937 rng(std::random_device{}());
        // Generate dimensions
        gen_dimensions(A.dimensionArray, rng);        
        gen_dimensions(B.dimensionArray, rng);

        // Generate indices
        gen_indices(A.multidimensionalIndexArray, rng);
        gen_indices(B.multidimensionalIndexArray, rng);
    }

    inline void gen_dimensions(md::DimensionsT& arr, std::mt19937& rng) {
        std::poisson_distribution poisson_1(1);  // 1 is the reference, but there could jumps
        arr.resize(MAX_DIM_LEN);
        for (uint32_t next = poisson_1(rng), i = 0; i < MAX_DIM_LEN; i++) {
            arr[i] = next;
            next += std::max(poisson_1(rng), 1);
        }
    }

    inline void gen_indices(md::MDIndexArrayT& arr, std::mt19937& rng) {
        std::uniform_int_distribution<std::mt19937::result_type> randint(0, MAX_INDEX_VALUE); // for indices
        arr.resize(MAX_INDICES_LEN);
        for (auto& index : arr) {
            index.resize(MAX_DIM_LEN);
            for (size_t i = 0; i < MAX_DIM_LEN; i++) {
                index[i] = randint(rng);
            }
        }
    }

    md::MultiDimIndices A;
    md::MultiDimIndices B;
};

static const InputArrays input{};

int main() {

    mdebug("Arr A Dims = {}", input.A.dimensionArray);
    mdebug("Arr B Dims = {}", input.B.dimensionArray);

    auto C = md::combine_indices_f(input.A, input.B);
    mdebug("Arr C Dims = {}", C.dimensionArray);
    mdebug("Arr C len = {}", C.multidimensionalIndexArray.size());

    return 0;
}