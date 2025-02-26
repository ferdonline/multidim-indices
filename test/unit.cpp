#include <gch/small_vector.hpp>
#include <multidim.hpp>
#include "../src/smalldim_opt.hpp"

#include <acutest.h> // add last

using namespace multidim;

#ifdef MULTIDIM_DEBUG
#include <fmt/ranges.h>
#define mdebug(...) { fmt::print(__VA_ARGS__); printf("\n"); }
#else
#define mdebug(...)
#endif


void test_index_basic() {
    IndexElemT buff[10]{};
    MultiIndexT in_index{{5, 1, 3}};
    DimensionsT in_dims{1, 3, 5};

    expand_index(in_index, in_dims, buff);
    TEST_CHECK(buff[1] == 5);
    TEST_CHECK(buff[3] == 1);
    TEST_CHECK(buff[5] == 3);

    DimensionsT out_dims{{0, 1, 2, 3, 5, 6}};

    auto out_index = filter_index(buff, out_dims);
    mdebug("out Index = {}", out_index);
    auto expected_index = MultiIndexT{0, 5, 0, 1, 3, 0};
    TEST_CHECK(out_index == expected_index);
}

void test_example1() {
    MultiDimIndices A, B;
    A.multidimensionalIndexArray = {{0, 0}, {0, 1}, {1, 0}};
    A.dimensionArray = {0, 1};
    B.multidimensionalIndexArray = {{0, 2}, {1, 3}};
    B.dimensionArray = {0, 2};

    auto C = combine_indices_f(A, B);
    mdebug("C.indices = {}", C.multidimensionalIndexArray);
    mdebug("C.dimensions = {}", C.dimensionArray);
    DimensionsT expected_dims{0, 1, 2};
    TEST_CHECK(C.dimensionArray == expected_dims);
    MDIndexArrayT expected_indices{{0, 0, 2}, {0, 1, 2}, {1, 0, 3}};
    std::sort(C.multidimensionalIndexArray.begin(), C.multidimensionalIndexArray.end());
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

    DimensionsT expected_dims{1, 2, 3, 4, 5};
    TEST_CHECK(C.dimensionArray == expected_dims);
    MDIndexArrayT expected_indices{{2, 0, 1, 4, 7}};
    TEST_CHECK(C.multidimensionalIndexArray == expected_indices);
}


void test_speedy_filter() {
    /// value [2,4,6] to be mapped to new dims. Drop dim 0, fill with 0 at end
    IndexElemT buff[8]{4, 5, 6, 7, 8, 0, 0, 0};
    auto out = filter_index<CompactIndexT>(buff, {1, 3, 5});
    CompactIndexT expected_index{5, 7, 0};
    mdebug("Exp index = {}", out);
    TEST_CHECK(out == expected_index);
}


TEST_LIST = {
    {"test_index_basic", test_index_basic},
    {"test_example1", test_example1},
    {"test2", test_example2},
    {"test_speedy_filter", test_speedy_filter},
    { NULL, NULL }     /* zeroed record marking the end of the list */
};
