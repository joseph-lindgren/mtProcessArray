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

#include <iostream>
#include <vector>
#include <pthread.h>
#include "mt/ThreadGroup.h"
#include <import/sys.h>
#include <import/mt.h>


#define	SIDE_LEN	1024


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

	inline size_t GetRowNum() { return rowNum; }
	inline size_t GetColNum() { return colNum; }
	inline T Get(size_t row, size_t col) { return a[colNum*row+col]; }
	inline void Set(size_t row, size_t col, T val) { a[colNum*row+col] = val; }
	inline T * operator[](size_t row) { return &a[colNum*row]; }

	void Print(std::string name = "Array");
	Array2d<T> SubArray( size_t startRow, size_t startCol, size_t endRow, size_t endCol );
	void EntrywiseOpImport(Array2d<T> & src, T op(T,T), size_t startRow, size_t startCol);//, size_t endRow, size_t endCol )

	// Big Three are default:
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
				//~ cout << (*this)[i][j] << ",";
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

	// Allocate storage and set dimensions
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

	// Copies in sub-array data
	for ( size_t i = startRow ; i < endRow ; i++ )
	{
		for ( size_t j = startCol ; j < endCol ; j++ )
		{
			//~ (*this)[i][j] = op( src[i-startRow][j-startCol], (*this)[i][j] );
			Set(i,j, op( src.Get(i-startRow,j-startCol), Get(i,j) ) );
		}
	}
}

////////////////////////////////////////////////////////////////////////

Array2d<bool> processValues(Array2d<float> & input)
/* This function takes in as input a 2D float array and provides as
 * output a 2D boolean array of the same size.
 * This function can only process strips of input data up to 128 rows
 * x 1024 columns at a time.
 */
{
	Array2d<bool> output(input.GetRowNum(), input.GetColNum(), 0 );

	//Do something...
	for ( size_t i = 0 ; i < output.a.size() ; i++ )
		output.a[i] = bool(input.a[i]);

	return output;
}

bool blend(bool src, bool dest)
{
	return src or dest;
}

bool copy(bool src, bool dest)
{
	return src;
}

typedef struct {
	Array2d<float> arrayIn;
	Array2d<bool> arrayOut;
	size_t rowOffset;
} TData;

void * computeLeaf( void * args )
{
	TData * leaf = (TData *)args;
	leaf->arrayOut = processValues(leaf->arrayIn);
	pthread_exit(NULL);
	return 0;
}

class myTask : public sys::Runnable
{
private:
	Array2d<float> arrayIn;
	Array2d<bool> arrayOut;
	size_t rowOffset;
public:
	myTask( TData taskdata ) : Runnable() 
	{
		arrayIn = taskdata.arrayIn;
		arrayOut = taskdata.arrayOut;
		rowOffset = taskdata.rowOffset;
	}
	void run() 
	{
		arrayOut = processValues(arrayIn);
	}
};

////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
	size_t chunkSize = SIDE_LEN / 8;
	size_t overlap = chunkSize / 2;
	size_t stepSize = chunkSize - overlap;
	Array2d<float> data(SIDE_LEN, SIDE_LEN, 2 );
	Array2d<bool> result(SIDE_LEN, SIDE_LEN, 0 );
	std::vector<TData> leaves;

	data[SIDE_LEN-1][SIDE_LEN-1] = 0;
	for (size_t i = 0 ; i < SIDE_LEN ; i++)
		data[i][0] = i;
	
	/// Last -1 is because we don't need extra overlapping leaf at end of array
	size_t leafNum = (SIDE_LEN + stepSize-1) / stepSize -1; 
	std::cout << "Using " << leafNum << " leaves." << std::endl;

	/// Pre-processing: Split data into manageable chunks
	for ( size_t i = 0 ; i + stepSize < SIDE_LEN ; i += stepSize )
	{
		TData temp;
		temp.arrayIn = data.SubArray(i,0,i+chunkSize,SIDE_LEN);
		temp.arrayOut = Array2d<bool>();
		temp.rowOffset = i;
		leaves.push_back( temp );
	}

	
	{
		using namespace sys;
		using namespace mt;
		using namespace std;

		ThreadGroup * mythreads = new ThreadGroup(); /// <- source of the current compiler error
	}
	int rc = 0;
	int n = 0;
	std::vector<pthread_t> threads(leaves.size());
	for ( TData & leaf : leaves )
	{
		if ( false )
		{ /// SERIAL
			//~ leaf.arrayIn.Print();
			leaf.arrayOut = processValues(leaf.arrayIn);
			//~ leaf.arrayOut.Print();
		}
		else 
		{ /// PARALLEL: modelled on https://computing.llnl.gov/tutorials/pthreads/#CreatingThreads
			rc = pthread_create( &(threads[n]), NULL, computeLeaf, &leaf );
			if (rc)
			{
				printf("ERROR; return code from pthread_create() is %d\n", rc);
				exit(-1);
			}
		}
		n++;
	}

	/// Wait until all threads are finished
	for ( pthread_t thread : threads )
		pthread_join( thread, NULL );

	/// Post-processing: Reassemble data chunks
	// (Try it homogeneously before optimizing)
	std::cout << "Now we put 'Humpty' together again." << std::endl;
	for ( TData & leaf : leaves )
		result.EntrywiseOpImport( leaf.arrayOut, blend, leaf.rowOffset, 0 );

	// Potential optimization
	//~ // Even-numbered chunks can be copied without danger
	//~ for ( size_t i = 0 ; i < leaves.size() ; i+=2 )
		//~ result.EntrywiseOpImport( leaves[i].arrayOut, copy, leaves[i].rowOffset, 0 );
	//~ // Odd-numbered chunks must be blended (logically OR'ed) onto their (previously copied) even-numbered neighbors
	//~ for ( size_t i = 1 ; i < leaves.size() ; i+=2 )
		//~ result.EntrywiseOpImport( leaves[i].arrayOut, blend, leaves[i].rowOffset, 0 );

	//~ result.Print();
	std::cout << "Process used " << leaves.size() << " threads." << std::endl;

	pthread_exit(NULL);
	return 0;
}
