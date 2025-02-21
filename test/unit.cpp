#include <acutest.h>
#include <gch/small_vector.hpp>
#include <multidim.hpp>

using namespace multidim;

#ifdef MULTIDIM_DEBUG
#include <fmt/ranges.h>
#define mdebug(...) fmt::println(__VA_ARGS__)
#else
#define mdebug(...)
#endif

void test_example1() {
    MultiDimIndices A, B;
    A.multidimensionalIndexArray = {{0, 0}, {0, 1}, {1, 0}};
    A.dimensionArray = {0, 1};
    B.multidimensionalIndexArray = {{0, 2}, {1, 3}};
    B.dimensionArray = {0, 2};

    auto C = combine_indices_f(A, B);
    // fmt::println("C.indices = {}", C.multidimensionalIndexArray);
    // fmt::println("C.dimensions = {}", C.dimensionArray);
    gch::small_vector<size_t> expected_dims{0, 1, 2};
    TEST_CHECK(C.dimensionArray == expected_dims);
    std::vector<gch::small_vector<size_t>> expected_indices{{0, 0, 2}, {0, 1, 2}, {1, 0, 3}};
    TEST_CHECK(C.multidimensionalIndexArray == expected_indices);
}

void test_example2() {
    MultiDimIndices A, B;
    A.multidimensionalIndexArray = {{2, 0, 1}};
    A.dimensionArray = {1, 2, 3};
    B.multidimensionalIndexArray = {{2, 4, 7}};
    B.dimensionArray = {1, 4, 5};

    auto C = combine_indices_f(A, B);
    mdebug("C.dimensions = {}", C.dimensionArray);
    mdebug("C.indices = {}", C.multidimensionalIndexArray);

    gch::small_vector<size_t> expected_dims{1, 2, 3, 4, 5};
    TEST_CHECK(C.dimensionArray == expected_dims);
    std::vector<gch::small_vector<size_t>> expected_indices{{2, 0, 1, 4, 7}};
    TEST_CHECK(C.multidimensionalIndexArray == expected_indices);
}

TEST_LIST = {
    {"test_example1", test_example1},
    {"test2", test_example2}
};
