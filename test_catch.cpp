/**
\file
\brief test program, needs Catch2: https://github.com/catchorg/Catch2
*/


#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

#define TESTMODE
#include "dtcpp.h"

//-------------------------------------------------------------------------------------------
TEST_CASE( "maj vote", "[majv]" )
{
	std::cout << "Running tests with catch " << CATCH_VERSION_MAJOR << '.' << CATCH_VERSION_MINOR << '.' << CATCH_VERSION_PATCH << '\n';

	DataSet dataset(4); //  4 attributes

	dataset.addDataPoint( DataPoint( std::vector<float>{ 1,4,9,2 }, 0 ) );
	dataset.addDataPoint( DataPoint( std::vector<float>{ 7,8,9,1 }, 0 ) );
	dataset.addDataPoint( DataPoint( std::vector<float>{ 4,6,5,1 }, 0 ) );
	dataset.addDataPoint( DataPoint( std::vector<float>{ 8,8,5,2 }, 1 ) );
	dataset.addDataPoint( DataPoint( std::vector<float>{ 7,8,5,9 }, 1 ) );

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
