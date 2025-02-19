Consider a typed data structure "MultidimensionalIndexCollection" that contains the following information:

```c
typedef struct

{
        size_t **       multidimensionalIndexArray;
        size_t          multidimensionalIndexArrayLength;
        size_t *        dimensionArray;
        size_t          dimensionArrayLength;
} MultidimensionalIndexCollection;

```

The `multidimensionalIndexArray` is an array that holds `multidimensionalIndexArrayLength` many multidimensional indices, where a multidimensional index is a k-tuple of non-negative integers. Each multidimensional index in the `multidimensionalIndexArray` has the same dimensionality which is `dimensionArrayLength`. The `dimensionArray` is an array that holds `dimensionArrayLength` many dimensions (non-negative integers) and specifies to which dimension the entries of a multidimensional index in `multidimensionalIndexArray` corresponds to. For instance, if `dimensionArray = {2, 4, 7}`, then the first, second, and third entires of all multidimensional indices in `multidimensionalIndexArray` correspond to the dimensions `2`, `4`, and `7`, respectively.

Given two instances `A` and `B` of "MultidimensionalIndexCollection", we define a binary operation `f` between `A` and `B` to yield another instance of "MultidimensionalIndexCollection", `C = f(A, B)`, that is obtained via the following rules:

- The array of dimensions of `C` is the union of the array of dimensions of `A` and `B`. For instance, if `A.dimensionArray = {1, 2, 3}` and `B.dimensionArray = {1, 4, 5}`, then `C.dimensionArray = {1, 2, 3, 4, 5}`.

- For each possible pair of multidimensional indices with one from `A.multidimensionalIndexArray` and the other from `B.multidimensionalIndexArray`, add a multidimensional index to `C.multidimensionalIndexArray` such that its entries for each dimension in `C.dimensionArray` agrees with the entries of the input multidimensional indices from  `A` and `B` corresponding to the same dimension. For instance, if `A.dimensionArray = {1, 2, 3}` and `B.dimensionArray = {1, 4, 5}`, and we have the `A` multidimensional index `{2, 0, 1}` and the `B` multidimensional index `{2, 4, 7}`, then the corresponding `C` multidimensional index to add to `C.multidimensionalIndexArray` is `{2, 0, 1, 4, 7}`. Note that, if `A` and `B` have common dimensions, then the multidimensional indices from `A` and `B` must have the same entry for the common dimension for you to be able to add a multidimensional index to `C.multidimensionalIndexArray` unambiguously. If the multidimensional indices from `A` and `B` do not have the same entry for their common dimension (e.g., for the `A.dimensionArray` and `B.dimensionArray` given above, and the same `A` multidimensional index of `{2, 0, 1}`, if we had the `B` multidimensional index `{3, 4, 7}`, so that the `A` index and `B` index corresponding to the dimension `1` are `2` and `3`, respectively, which are not equal), then nothing should be added to `C.multidimensionalIndexArray`.

Example (`C = f(A, B)`):

```c

A.multidimensionalIndexArray = {{0, 0}, {0, 1}, {1, 0}};

A.multidimensionalIndexArrayLength = 3;

A.dimensionArray = {0, 1}

A.dimensionArrayLength = 2

B.multidimensionalIndexArray = {{0, 2}, {1, 3}};

B.multidimensionalIndexArrayLength = 2;

B.dimensionArray = {0, 2}

B.dimensionArrayLength = 2

C.multidimensionalIndexArray = {{0, 0, 2}, {0, 1, 2}, {1, 0, 3}};

C.multidimensionalIndexArrayLength = 3;

C.dimensionArray = {0, 1, 2}

C.dimensionArrayLength = 3

```

Write a C/C++ code that implements the operation described above with an emphasis on performance. You may hardcode your inputs and you are free to implement and organise the data structures in any way you think will help boost the performance of your code. Please also consider how your program might leverage multi-threading and use it if you think it will help boost the performance of your code.

Document the example by providing clearly-documented source code as well as a well-written but brief README.md and place it in a public GitHub repository. In the README.md, please highlight all your efforts towards improving the performance of your code and specify the properties of the hardware your code is targeting. You can find some examples (ref. 0 below) of documented repositories that you can easily run on the Signaloid Cloud Developer Platform at Signaloid's GitHub page.


You can use one of our existing example repositories (ref. 1 below) as a template and follow the directions in that repository's README.md if you choose to run it on the Signaloid Cloud Developer Platform. We will not provide any additional help or feedback in the process, although, like any other user of the platform, you can send mail to developer-support@signaloid.com if you have questions about problems with the platform. We will not answer any questions specific to the exercise.