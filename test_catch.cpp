/**
\file
\brief test program, needs Catch2: https://github.com/catchorg/Catch2
\copyright GPLv3
*/


#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

#define TESTMODE
#include "dtcpp.h"

#define DEBUG
using namespace dtcpp;


//-------------------------------------------------------------------------------------------
TEST_CASE( "dataset", "[dataset]" )
{
	DataSet dataset(3); // 3 attributes

// cannot add a datapoint holding 2 values
	CHECK_THROWS( dataset.addPoint( DataPoint( std::vector<float>{ 1., 2. } ) ) );
// this is ok
	CHECK_NOTHROW( dataset.addPoint( DataPoint( std::vector<float>{ 1., 2., 3. } ) ) );
	CHECK( dataset.nbClasses() == 0 );

	CHECK_NOTHROW( dataset.addPoint( DataPoint( std::vector<float>{ 4., 2., 3. }, ClassVal(1) ) ) );
	CHECK( dataset.nbClasses() == 1 );

	CHECK( dataset.size() == 2 );

	CHECK( dataset.getClassCount( ClassVal(1) ) == 1 );
	CHECK( dataset.getClassCount( ClassVal(-1) ) == 1 );  // 1 point with no class

	for( int i=0; i<11; i++ )
		dataset.addPoint( DataPoint( std::vector<float>{ 4., 2., 3. }, ClassVal(4) ) );

	CHECK( dataset.size() == 13 );
	CHECK( dataset.getClassCount( ClassVal(4)  ) == 11 );
	CHECK( dataset.getClassCount( ClassVal(1)  ) ==  1 );
	CHECK( dataset.getClassCount( ClassVal(-1) ) ==  1 );  // still 1 point with no class

	{
		auto subsets= dataset.getFolds( 0, 2 );
		const auto& ds_train = subsets.first;
		const auto& ds_test  = subsets.second;

		CHECK( ds_test.size() == 6 );  // because 13 pts / 2 folds = 6
		CHECK( ds_train.size() == 7 );  // the remaining points
		CHECK( ds_test.nbClasses() == 2 ); // hold 2 classes: 1 and 4
		CHECK( ds_train.nbClasses() == 1 ); // holds only class 4

		CHECK( ds_test.getClassCount(  ClassVal(-1) ) == 1 );
		CHECK( ds_test.getClassCount(  ClassVal( 1) ) == 1 );
		CHECK( ds_test.getClassCount(  ClassVal( 4) ) == 4 );

		CHECK( ds_train.getClassCount( ClassVal( -1) ) == 0 );
		CHECK( ds_train.getClassCount( ClassVal(  1) ) == 0 );
		CHECK( ds_train.getClassCount( ClassVal(  4) ) == 7 );
	}
	{
		auto subsets= dataset.getFolds( 0, 3 );
		const auto& ds_train = subsets.first;
		const auto& ds_test  = subsets.second;

		CHECK( ds_test.size() == 4 );  // because 13 pts / 3 folds = 4
		CHECK( ds_train.size() == 9 );  // the remaining points
		CHECK( ds_test.nbClasses() == 2 ); // hold 2 classes: 1 and 4
		CHECK( ds_train.nbClasses() == 1 ); // holds only class 4
	}
	{
		auto subsets= dataset.getFolds( 0, 4 );
		const auto& ds_train = subsets.first;
		const auto& ds_test  = subsets.second;

		CHECK( ds_test.size() == 3 );  // because 13 pts / 4 folds = 3
		CHECK( ds_train.size() == 10 );  // the remaining points
		CHECK( ds_test.nbClasses() == 2 ); // hold 2 classes: 1 and 4
		CHECK( ds_train.nbClasses() == 1 ); // holds only class 4
	}

	dataset.clear();
	CHECK( dataset.size() == 0 );

}
//-------------------------------------------------------------------------------------------
TEST_CASE( "confusion matrix", "[cmat]" )
{
// a sample confusion matrix (column: real class, lines, predicted class)
	std::vector<std::vector<uint>> v2{
		{ 1 /* TP */, 2 /* FP */ },
		{ 3 /* FN */, 4 /* TN */ }
	};
	ConfusionMatrix m2( v2 );
	CHECK( m2.getScore( CM_TPR ) == 0.25 );
	CHECK( m2.getScore( CM_TNR ) == 2./3 );
	CHECK( m2.getScore( CM_ACC ) == 0.5 );

// a sample confusion matrix (column: real class, lines, predicted class)
	std::vector<std::vector<uint>> v4{
		{ 1, 2, 3, 4 },
		{ 0, 1, 2, 3 },
		{ 0, 1, 5, 1 },
		{ 8, 1, 2, 1 },
	};
	ConfusionMatrix m4( v4 );
	CHECK( m4.getScore( CM_TPR, ClassVal(0) ) == 0. );
}
//-------------------------------------------------------------------------------------------
TEST_CASE( "maj vote", "[majv]" )
{
	std::cout << "Running tests with catch " << CATCH_VERSION_MAJOR << '.' << CATCH_VERSION_MINOR << '.' << CATCH_VERSION_PATCH << '\n';

	DataSet dataset(4); //  4 attributes

	dataset.addPoint( DataPoint( std::vector<float>{ 1,4,9,2 }, 0 ) );
	dataset.addPoint( DataPoint( std::vector<float>{ 7,8,9,1 }, 0 ) );
	dataset.addPoint( DataPoint( std::vector<float>{ 4,6,5,1 }, 0 ) );
	dataset.addPoint( DataPoint( std::vector<float>{ 8,8,5,2 }, 1 ) );
	dataset.addPoint( DataPoint( std::vector<float>{ 7,8,5,9 }, 1 ) );

	{
		auto nc = getNodeContent( std::vector<uint>{0,1,2,3,4}, dataset );
		CHECK( nc.dominantClass.get() == 0 );
		CHECK( nc.nbPtsOtherClasses   == 2 );
	}
	{
		auto nc = getNodeContent( std::vector<uint>{0,3,4}, dataset );
		CHECK( nc.dominantClass.get() == 1 );
		CHECK( nc.nbPtsOtherClasses   == 1 );
	}
}

std::vector<uint>
setAllDataPoints( const DataSet& dataset )
{
	std::vector<uint> v( dataset.size() );
	std::iota( v.begin(), v.end(), 0 );
	return v;
}

//-------------------------------------------------------------------------------------------
TEST_CASE( "computeBestThreshold", "[cbt]" )
{
	DataSet dataset;
	dataset.load( "sample_data/tds_2.csv" );
	REQUIRE( dataset.size() == 8 );

	auto v_dpidx = setAllDataPoints( dataset );    // all the points

	auto giniCoeff = getGiniImpurity( v_dpidx, dataset );

	Params params;
	auto ig0 = computeBestThreshold( 0, v_dpidx, dataset, giniCoeff.first, params );
	std::cout << "ig0: " << ig0 <<'\n';
	auto ig1 = computeBestThreshold( 1, v_dpidx, dataset, giniCoeff.first, params );
	std::cout << "ig1: " << ig1 <<'\n';

	auto ba = findBestAttribute( v_dpidx, dataset, params );
}
//-------------------------------------------------------------------------------------------
TEST_CASE( "removeDuplicates", "[RD]" )
{
	Params params;
	std::vector<float> v0{1., 2., 3., 4., 2., 2.1 };
	{
		auto values = v0;
		params.removalCoeff = 0.1;
		removeDuplicates( values, params );
		CHECK( values.size() == 4 );
		CHECK( values == std::vector<float>{1., 2., 3., 4. } );
	}
	{
		auto values = v0;
		params.removalCoeff = 0.01;
		removeDuplicates( values, params );
		CHECK( values.size() == 5 );
		CHECK( values == std::vector<float>{1., 2., 2.1, 3., 4. } );
	}
}
//-------------------------------------------------------------------------------------------
