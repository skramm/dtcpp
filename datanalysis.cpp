/**
\file datanalysis.cpp
\brief Command-line tool that produces from an inpu data file some stats and histogram, along with associated Gnuplot scripts
*/


#define DEBUG
//#define DEBUG_START

#include "dtcpp.h"
#include "argh.h" //  https://github.com/adishavit/argh

//using namespace std;
using namespace dtcpp;

int main( int argc, const char** argv )
{
	Fparams fparams;
	Params  params;
	argh::parser cmdl; //({ "-sep"});
	cmdl.parse(argc, argv, argh::parser::PREFER_PARAM_FOR_UNREG_OPTION );

	// optional arg: -sep X => X used as datafile field separator
	auto sepcl = cmdl("sep").str();
	if( !sepcl.empty() )
		fparams.sep = sepcl[0];
	std::cout << " - using '" << fparams.sep << "' as datafile field separator\n";

	if( cmdl["cf"] )
		fparams.classIsfirst = true;
	if( cmdl["cl"] )
		fparams.classIsfirst = false;
	std::cout << " - using " << (fparams.classIsfirst ? "first": "last") << " element as class value\n";

	auto loglevel = cmdl("ll").str();       // Log Level
	if( !loglevel.empty() )
	{
		params.verboseLevel = std::stoi( loglevel );
		params.verbose = true;
		std::cout << " - enabling logging with level " << params.verboseLevel << '\n';
	}

	uint nbBins = 15;
	auto histoBins = cmdl("nb").str();
	if( !histoBins.empty() )
	{
		nbBins = std::stoi( histoBins );
	}
	std::cout << " - histograms built on " << nbBins << " bins\n";


// optional boolean arg: -cs => the class value in the datafile is given as a string value
	if( cmdl["cs"] )
		fparams.classAsString = true;

	DataSet dataset;
	std::string fname = "sample_data/tds_1.csv";
	if( cmdl.size() > 1 )
		fname = cmdl[1];

	if( !dataset.load( fname, fparams ) )
	{
		std::cerr << "Error, unable to load data file: " << fname << '\n';
		std::exit(1);
	}

//	dataset.printClassHisto( "histo" );

	dataset.printInfo( std::cout );
	auto stats = dataset.computeStats<float>( nbBins );
	std::cout << stats;
	dataset.generateAttribPlot( "data", stats );

	auto nbOutliers = dataset.tagOutliers( stats );
	std::cout << "nb outliers=" << nbOutliers << " (" << 100. * nbOutliers / dataset.size() << " %)\n";

	dataset.printInfo( std::cout );
	auto stats2 = dataset.computeStats<float>( nbBins );
	std::cout << stats2;
	dataset.generateAttribPlot( "data2", stats2 );

}
