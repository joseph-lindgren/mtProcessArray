# mtProcessArray

Problem statement: 
Suppose you have a function processValues() - this function takes in as input a 2D float array and provides as output a 2D boolean array of the same size.  This function can only process strips of input data up to 128 rows x 1024 columns at a time.  Given a 2D array of floating point values, size 1024 x 1024, write C++ code that processes the entire array using this function.  When calling the function, subsequent strips must overlap by 50%, and the portions of overlap must be logically OR'd together in the resulting output.

Try to make the code cross-platform and make use of multiple threads and ensure thread-safety when combining overlapping regions (you may want to start with a non-threaded implementation).

========================================================================

Problem solution:
Currently implemented with std::thread, port to coda-oss (https://github.com/mdaus/coda-oss) multi-threading should be easy.

Array2d<bool> processValues(Array2d<float> & input) is a dummy function standing in for whatever the real thing will do (probably feature detection or filtering)

========================================================================

Notes:
File "multiProcessArray-stdthread.cxx" is the working prototype, while "multiProcessArray-2.cxx" is the beginning of my attempt to use the MDA CODA libraries.

I considered using the Blitz++ library (http://blitz.sourceforge.net/), but decided it would add undesirable complexity for a short project. It might be worthwhile for something more involved, however.
