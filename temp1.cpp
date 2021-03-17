
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
int main()
{
#if 0
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
#else
	int nbpts = 200;
	std::vector<PairAtvalClass> vpac;
	for( auto i=0; i<nbpts; i++ )
	{
		vpac.push_back(
			std::make_pair(
				(float)100. * std::rand()/RAND_MAX,
				ClassVal( 4. * std::rand()/RAND_MAX )
			)
		);
	}

#endif
	auto vThres = getThresholds<float,ClassVal>( vpac, 10 );
	priv::printVector( std::cout, vThres, "threshold values" );

	std::cout << "Done !\n";
}
//---------------------------------------------------------------------
