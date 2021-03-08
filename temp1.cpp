
//#include <boost/histogram.hpp>
#define DEBUG
#define DEBUG_START
//#define BIN_PRINT_POINTS

#include "histac.hpp"
#include "dtcpp.h"
#include <algorithm>

#include <iostream>
#include <map>
#include <list>
#include <cmath>

/**

  /\
  |
 4+           +--------+
  |           |        |
 3+  +--------+        |
  |  | c[1]=0 |        |
 2+  | c[2]=2 |        +--
  |  | c[3]=1 |        | (other bins)
 1+  |        |        |
  |  |        |        |
--+--+--------+--------+--------->
	.15      .25      .35


h( make_pair( .21, 2 );
h( make_pair( .22, 2 );
h( make_pair( .23, 3 );

*/
using namespace std;
using namespace dtcpp;

using PairAtvalClass = std::pair<float,ClassVal>;


//---------------------------------------------------------------------
/// Input:  a vector or pairs, size=nb of points in data set
/// first value: the attribute value, second: the class value
std::vector<float>
getThresholds( const std::vector<PairAtvalClass>& v_pac, int nbBins )
{
	START;

// Step 1 - build initial histogram, evenly spaced
	histac::VBS_Histogram<float,ClassVal> histo( v_pac, nbBins );

	histo.print( std::cout, "AFTER BUILD" );

// Step 2 - split bins that need to be splitted
	histo.splitSearch();
	histo.print( std::cout, "AFTER SPLIT" );

// Step 3 - merge adjacent bins holding same class
	histo.mergeSearch();
	histo.print( std::cout, "AFTER MERGE" );

// Step 4 - build thresholds from bins
	std::vector<float> vThres( histo.nbBins()-1 );

	size_t i=0;
	for( auto it=histo.begin(); it != histo.end(); it++ )
	{
		if( std::next(it) !=  histo.end() )
			vThres[i++] = it->getBorders().second;
	}

	return vThres;
}

//---------------------------------------------------------------------
int main()
{
	auto c1 = ClassVal(1);
	auto c2 = ClassVal(2);
//	auto c3 = ClassVal(3);
	std::vector<PairAtvalClass> vpac{
		{ 0., c1 },
		{ 0.5, c1 },
		{ 0.6, c1 },
		{ 0.7, c2 },
		{ 0.8, c2 },
		{ 3,   c2 },
	};
	auto vThres = getThresholds( vpac, 3 );
	priv::printVector( std::cout, vThres, "threshold values" );

	std::cout << "Done !\n";
}
//---------------------------------------------------------------------
