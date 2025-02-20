#include <fmt/ranges.h>

#include <multidim.hpp>

using namespace multidim;

void test1() {
    MultiDimIndices A, B;
    A.multidimensionalIndexArray = {{0, 0}, {0, 1}, {1, 0}};
    A.dimensionArray = {0, 1};
    B.multidimensionalIndexArray = {{0, 2}, {1, 3}};
    B.dimensionArray = {0, 2};

    auto C = combine_indices_f(A, B);
    fmt::println("C.indices = {}", C.multidimensionalIndexArray);
    fmt::println("C.dimensions = {}", C.dimensionArray);

    assert((C.dimensionArray[0] == std::vector{0, 0, 0}));

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