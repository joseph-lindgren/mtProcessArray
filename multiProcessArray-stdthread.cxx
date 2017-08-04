/*
 * multiProcessArray-2.cxx
 * 
 * Copyright 2017 Joseph <joseph<dot>lindgren<at>aol<dot>com>
 * 
 * sys-c++ is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program; If not, 
 * see <http://www.gnu.org/licenses/>.
 *
 */

/* Suppose you have a function processValues() - this function takes in
 * as input a 2D float array and provides as output a 2D boolean array
 * of the same size.
 * This function can only process strips of input data up to 128 rows
 * x 1024 columns at a time.
 * Given a 2D array of floating point values, size 1024 x 1024, write
 * C++ code that processes the entire array using this function.
 * When calling the function, subsequent strips must overlap by 50%, and
 * the portions of overlap must be logically OR'd together in the
 * resulting output.
 *
 * Try to make the code cross-platform and make use of multiple threads
 * and ensure thread-safety when combining overlapping regions (you may
 * want to start with a non-threaded implementation).
 *
 * The STL and third-party libraries may be used. The threading
 * primitives available here may also be used (the “sys” module/library
 * in particular, and perhaps the “mt” module/library if desired).
 *
 * If using the coda-oss libraries referenced above, here are some basic
 * build instructions:
 * python waf configure --prefix=/your/install/location
 * python waf install
 */
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <cstdlib>

#define	DEFAULT_SIDE_LEN	32
#define	MAX_SIDE_LEN	1024
#define	MAX_VIEWABLE	80

//// BEGIN TEMPLATE CLASS Array2d =======================================

template<class T>
class Array2d 
{
	friend Array2d<bool> processValues(Array2d<float> & input);
private:
	size_t rowNum, colNum;
	std::vector<T> a;
public:
	// Member functions
	Array2d<T>() : a(0,0) { rowNum = 0; colNum = 0; }
	Array2d<T>( size_t rows, size_t cols, T fill = 0 ) : rowNum(rows), colNum(cols), a(rowNum*colNum,fill) {}
	Array2d<T>( size_t rows, size_t cols, std::vector<T> array ) : rowNum(rows), colNum(cols), a(array) {}
	void SetAllVals(T func(T) )
	{
		for ( size_t i = 0 ; i < a.size() ; i++ )
			a[i] = func(i);
	}

	inline size_t GetRowNum() { return rowNum; }
	inline size_t GetColNum() { return colNum; }
	inline T Get(size_t row, size_t col) { return a[colNum*row+col]; }
	inline void Set(size_t row, size_t col, T val) { a[colNum*row+col] = val; }
	inline T * operator[](size_t row) { return &a[colNum*row]; }

	void Print(std::string name = "Array");
	Array2d<T> SubArray( size_t startRow, size_t startCol, size_t endRow, size_t endCol );
	void EntrywiseOpImport(Array2d<T> & src, T op(T,T), size_t startRow, size_t startCol);

	// Big Three are default due to container construction:
	//~ Array2d( const Array2d<T> & src );
	//~ ~Array2d();
	//~ Array2d<T> & operator=( const Array2d<T> & src );
};

template<class T>
void Array2d<T>::Print(std::string name)
{	/// Print function for debugging
	std::cout << name;
	if ( a.size() == 0 or rowNum == 0 or colNum == 0 )
		std::cout << " is empty." << std::endl;
	else
	{
		printf(" has %d rows and %d cols:\n", int(rowNum), int(colNum) );
		for (size_t i = 0 ; i < rowNum ; i++)
		{
			for (size_t j = 0 ; j < colNum ; j++)
			{
				std::cout << Get(i,j) << ",";
			}
			std::cout << std::endl;
		}
	}
}

template<class T>
Array2d<T> Array2d<T>::SubArray( size_t startRow, size_t startCol, size_t endRow, size_t endCol )
{	/// Allocates and returns a subset of the array
	size_t rows = endRow - startRow, cols = endCol - startCol;

	// Repair index overflow
	if ( startRow + rows > rowNum )
		rows = rowNum - startRow;
	if ( startCol + cols > colNum )
		cols = colNum - startCol;

	// Prepare return array
	Array2d<T> result( rows, cols, T(0) );

	// Copies in sub-array data
	for ( size_t i = 0 ; i < rows ; i++ )
	{
		for ( size_t j = 0 ; j < cols ; j++ )
			(result)[i][j] = (*this)[ startRow+i ][ startCol+j ];
	}
	return result;
}

template<class T>
void Array2d<T>::EntrywiseOpImport(Array2d<T> & src, T op(T,T), size_t startRow, size_t startCol)
{	/// Combines contents of 'src' and a subset of the 'this' array via the function 'op(T,T)'.
	/// 'src' is unchanged, result is stored in subset of 'this' array.
	size_t endRow = startRow + src.rowNum, endCol = startCol + src.colNum;

	// Repair index overflow
	if ( endRow > rowNum )
		endRow = rowNum;
	if ( endCol > colNum )
		endCol = colNum;

	// Copy in sub-array data
	for ( size_t i = startRow ; i < endRow ; i++ )
	{
		for ( size_t j = startCol ; j < endCol ; j++ )
		{
			Set(i,j, op( src.Get(i-startRow,j-startCol), Get(i,j) ) );
		}
	}
}

//// END TEMPLATE CLASS Array2d =========================================

//// BEGIN PROBLEM-SPECIFIC FUNCTIONS ===================================

Array2d<bool> processValues(Array2d<float> & input)
/* "This function takes in as input a 2D float array and provides as
 * output a 2D boolean array of the same size.
 * This function can only process strips of input data up to 128 rows
 * x 1024 columns at a time."
 */
{
	Array2d<bool> output(input.GetRowNum(), input.GetColNum(), 0 );

	// Do something with filters...
	for ( size_t i = 0 ; i < output.a.size() ; i++ )
		output.a[i] = bool(input.a[i]);

	return output;
}

bool blend(bool src, bool dest)
{	/// Logical OR function
	return src or dest;
}

bool copy(bool src, bool dest)
{	/// Direct copy function
	return src;
}

inline float myRandom( float in )
{	/// Weird throwaway function to randomize array values via SetAllVals
	return ( rand() % (4 + int(in) % 8) ) / 2;
}

/// "Runnable" data
typedef struct {
	Array2d<float> arrayIn;
	Array2d<bool> arrayOut;
	size_t rowOffset;
} TData;

/// "Runnable" process
void * computeLeaf( void * args )
{
	TData * leaf = (TData *)args;
	leaf->arrayOut = processValues(leaf->arrayIn);
	return 0;
}

//// MAIN IS HERE ======================================================

int main(int argc, char **argv)
{
	/// User input concerning the size of the test arrays
	size_t sideLen;
	std::string input = "";
	printf( "What SIDE_LEN should the (sqr) array have? (MAX_SIDE_LEN is %d)\n", int(MAX_SIDE_LEN) );
	getline(std::cin, input);
	std::stringstream myStream(input);
	if (myStream >> sideLen)
	{
		if (sideLen > MAX_SIDE_LEN)
			sideLen = MAX_SIDE_LEN;
		printf("Thank you for your input! Using ");
	}
	else
	{
		printf("Invalid number. Using DEFAULT_");
		sideLen = DEFAULT_SIDE_LEN;
	}
	printf("SIDE_LEN=%d.\n", int(sideLen) );
	
	/// Declare/allocate storage vars
	size_t chunkSize = sideLen / 8;
	size_t overlap = chunkSize / 2;
	size_t stepSize = chunkSize - overlap;
	Array2d<float> data(sideLen, sideLen );
	Array2d<bool> result(sideLen, sideLen );
	std::vector<TData> leaves;
	std::vector<std::thread> threads;

	/// Initialize input array with random values
	data.SetAllVals( myRandom );
	data[sideLen-1][sideLen-1] = 0;
	for (size_t i = 0 ; i < sideLen ; i++)
		data[i][0] = i;

	if (sideLen < MAX_VIEWABLE)
		data.Print("Input array");
	else
		printf("Input array exceeds easily readable size, hence not shown.\n");
	
	/// Last -1 is because we don't need extra overlapping leaf at end of array for data rows divisible by 16
	size_t leafNum = (sideLen + stepSize-1) / stepSize -1; 
	std::cout << "Using " << leafNum << " leaves." << std::endl;

	/// Pre-processing: Split data into manageable chunks
	for ( size_t i = 0 ; i + stepSize < sideLen ; i += stepSize ) 
	{  /* delete the '+ stepSize' if you need to use on array of an unusual size 
		* (i.e., if 'sideLen' not divisible by 16 */
		TData temp;
		temp.arrayIn = data.SubArray(i,0,i+chunkSize,sideLen);
		temp.arrayOut = Array2d<bool>();
		temp.rowOffset = i;
		leaves.push_back( temp );
	}

	/// Array Processing
	for ( TData & leaf : leaves )
	{
		if ( false ) /// SERIALIZED
			leaf.arrayOut = processValues(leaf.arrayIn);
		else /// PARALLELIZED
			threads.push_back( std::thread(computeLeaf, &leaf) );
	}

	/// Wait until all threads are finished
	for ( size_t i = 0 ; i < threads.size() ; i++ )
		threads[i].join();

	/// Post-processing: Reassemble data chunks
	// (Try it homogeneously before optimizing)
	std::cout << "Now we put 'Humpty' together again." << std::endl;
	for ( TData & leaf : leaves )
		result.EntrywiseOpImport( leaf.arrayOut, blend, leaf.rowOffset, 0 );

	// Potential optimization 
	/*
	// Even-numbered chunks can be copied without danger
	for ( size_t i = 0 ; i < leaves.size() ; i+=2 )
		result.EntrywiseOpImport( leaves[i].arrayOut, copy, leaves[i].rowOffset, 0 );
	// Odd-numbered chunks must be blended (logically OR'ed) onto their (previously copied) even-numbered neighbors
	for ( size_t i = 1 ; i < leaves.size() ; i+=2 )
		result.EntrywiseOpImport( leaves[i].arrayOut, blend, leaves[i].rowOffset, 0 );
	*/

	if (sideLen < MAX_VIEWABLE)
		result.Print("Processed array");
	else
		printf("Output array exceeds easily readable size, hence not shown.\n");
	std::cout << "Process used " << leaves.size() << " threads." << std::endl \
			  << "chunkSize was " << chunkSize << ", stepSize was " << stepSize \
			  << ", and overlap was " << overlap << std::endl;
	std::cout << "Each chunk was of size " \
			  << leaves[0].arrayIn.GetRowNum() << " x " << leaves[0].arrayIn.GetColNum() \
			  << std::endl;
	
	return 0;
}