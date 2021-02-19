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
TEST_CASE( "computeIG", "[cig]" )
{
	DataSet dataset;
	dataset.load( "sample_data/tds_2.csv" );
	REQUIRE( dataset.size() == 8 );

	auto v_dpidx = priv::setAllDataPoints( dataset );    // all the points

	auto giniCoeff = getGlobalGiniCoeff( v_dpidx, dataset );

	auto ig0 = computeIG( 0, v_dpidx, dataset, giniCoeff );
	auto ig1 = computeIG( 1, v_dpidx, dataset, giniCoeff );
}
//-------------------------------------------------------------------------------------------
TEST_CASE( "removeDuplicates", "[RD]" )
{
	std::vector<float> v0{1., 2., 3., 4., 2., 2.1 };
	{
		auto values = v0;
		removeDuplicates( values, 0.1 );
		CHECK( values.size() == 4 );
		CHECK( values == std::vector<float>{1., 2., 3., 4. } );
	}
	{
		auto values = v0;
		removeDuplicates( values, 0.01 );
		CHECK( values.size() == 5 );
		CHECK( values == std::vector<float>{1., 2., 2.1, 3., 4. } );
	}

}
//-------------------------------------------------------------------------------------------
