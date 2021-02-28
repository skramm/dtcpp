
/**
\file
\brief Command-line app to train a Decision Tree with a dataset, given as argument.
See doc on https://github.com/skramm/dtcpp
\author S. Kramm - 2021
*/

//#define DEBUG
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

// optional boolean arg: -cs => the class value in the datafile is given as a string value
	if( cmdl["cs"] )
		fparams.classAsString = true;

// optional boolean arg: -i => only prints info about the data set and exit
	bool noProcess = false;
	if( cmdl["i"] )
		noProcess = true;

// optional arg: -f => evaluate performance on training dataset by doing folds on data
	bool doFolding = false;
	if( cmdl["f"] )
		doFolding = true;

	bool doRemoveOutliers = false;
	if( cmdl["ro"] )
		doRemoveOutliers = true;
	std::cout << " - removal of outliers: " << std::boolalpha << doRemoveOutliers << '\n';

	DataSet dataset;
	std::string fname = "sample_data/tds_1.csv";
	if( cmdl.size() > 1 )
		fname = cmdl[1];

	if( !dataset.load( fname, fparams ) )
	{
		std::cerr << "Error, unable to load data file: " << fname << '\n';
		std::exit(1);
	}
	dataset.printInfo( std::cout );
	dataset.printClassHisto( "histo" );

	if( noProcess )
	{
		std::cout << "No processing required, exiting\n";
		std::exit(2);
	}

	auto stats = dataset.computeStats<float>();
	std::cout << stats;

	if( doRemoveOutliers )
		dataset.tagOutliers( stats );

	if( !doFolding )
	{
		TrainingTree tt( dataset.getClassIndexMap() );
		tt.train( dataset, params );
		auto cm = tt.classify( dataset );
		std::cout << cm << "\n";
		tt.printInfo( std::cout );
		tt.printDot( "dectree" );
		cm.printAllScores( std::cout );
	}
	else
	{
		dataset.shuffle();

		uint nbFolds = 5;
		for( uint i=0; i<nbFolds; i++ )
		{
			TrainingTree tt( dataset.getClassIndexMap() );
			auto p_data_subsets = dataset.getFolds( i, nbFolds );
			auto data_train = p_data_subsets.first;
			auto data_test  = p_data_subsets.second;

			data_train.printClassHisto( "histo_tr_" + std::to_string(i) );
			data_test.printClassHisto(  "histo_te_" + std::to_string(i) );

			data_train.printInfo( std::cout, "train" );
			data_test.printInfo(  std::cout, "test" );
			tt.train( data_train, params );
			tt.printDot( "dectree_" + std::to_string(i) );

			auto cm_train = tt.classify( data_train );
			auto cm_test  = tt.classify( data_test );
			std::cout << "\n* Fold " << i+1 << "/" << nbFolds << '\n';
			std::cout << "Confusion Matrix (train):" << cm_train << "\n";
			cm_train.printAllScores( std::cout, "train" );
			std::cout << "Confusion Matrix (test):"  << cm_test << "\n";
			cm_test.printAllScores( std::cout, "test" );
		}
	}
}
