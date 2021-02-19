/**
\file
\brief test program, needs Catch2: https://github.com/catchorg/Catch2
*/


#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"
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
		auto maj = getMajorityClass( std::vector<uint>{0,1,2,3,4}, dataset );
		CHECK( maj.first  == 0 );
		CHECK( maj.second == 0.6f );
	}
	{
		auto maj = getMajorityClass( std::vector<uint>{0,3,4}, dataset );
		CHECK( maj.first  == 1 );
		CHECK( maj.second == 0.66666666f );
	}
}

//-------------------------------------------------------------------------------------------
TEST_CASE( "compute IG", "[cig]" )
{
	DataSet dataset;
	dataset.load( "sample_data/tds_1.csv" );
	REQUIRE( dataset.size() == 10 );
	std::vector<uint> v_dpidx{0,1,2,3,4};   // all the points
	auto ig0 = computeIG( 0, v_dpidx, dataset );

	auto ig1 = computeIG( 0, v_dpidx, dataset );
}
//-------------------------------------------------------------------------------------------
