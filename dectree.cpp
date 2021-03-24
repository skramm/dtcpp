/**
\file
\brief Command-line app to train a Decision Tree with a dataset, given as argument.
See doc on https://github.com/skramm/dtcpp
\author S. Kramm - 2021
*/

#include "dtcpp.h"
#include "argh.h" //  https://github.com/adishavit/argh

using namespace dtcpp;


int main( int argc, const char** argv )
{
	std::cout << argv[0] << ": build on " << __DATE__ << " with boost " << BOOST_VERSION << '\n';
	Fparams fparams;
	Params  params;
	argh::parser cmdl; //({ "-sep"});
	cmdl.parse(argc, argv, argh::parser::PREFER_PARAM_FOR_UNREG_OPTION );

	if( cmdl.size() < 2 )
	{
		std::cerr << "Error, no data file name given !\n";
		std::exit(1);
	}
	std::string fname = cmdl[1];

// optional arg: -sep X => X used as datafile field separator
	auto str_sepcl = cmdl("sep").str();
	if( !str_sepcl.empty() )
		fparams.sep = str_sepcl[0];
	std::cout << " - using '" << fparams.sep << "' as datafile field separator\n";

	if( cmdl["cf"] )
		fparams.classIsfirst = true;
	if( cmdl["cl"] )
		fparams.classIsfirst = false;
	std::cout << " - using " << (fparams.classIsfirst ? "first": "last") << " element as class value\n";

	if( cmdl["fl"] )                     // at present, we just ignore the first line if true
		fparams.firstLineLabels = true;

	auto str_loglevel = cmdl("ll").str();       // Log Level
	if( !str_loglevel.empty() )
	{
		g_params.verboseLevel = std::stoi( str_loglevel );
		g_params.verbose = true;
		std::cout << " - enabling logging with level " << g_params.verboseLevel << '\n';
	}

	auto str_maxDepth = cmdl("md").str();
	if( !str_maxDepth.empty() )
		params.maxTreeDepth = std::stoi( str_maxDepth );
	std::cout << " - max depth for tree=" << params.maxTreeDepth << '\n';

// optional boolean arg: -cs => the class value in the datafile is given as a string value
	if( cmdl["cs"] )
		fparams.classAsString = true;

	uint nbBins = 15;
	auto str_histoBins = cmdl("nbh").str();
	if( !str_histoBins.empty() )
	{
		nbBins = std::stoi( str_histoBins );
	}
	std::cout << " - histograms built on " << nbBins << " bins\n";

// optional boolean arg: -i => only prints info about the data set and exit
	bool noTraining = false;
	if( cmdl["i"] )
		noTraining = true;

// optional arg: -nf x => evaluate performance on training dataset by doing 'x' folds on data
	int nbFolds = 0;
	auto str_folds = cmdl("nf").str();
	if( !str_folds.empty() )
	{
		nbFolds = std::stoi( str_folds );
		assert( nbFolds>0 );
		std::cout << " - training with " << nbFolds << " on dataset\n";
	}

	bool doRemoveOutliers = false;
	if( cmdl["ro"] )
		doRemoveOutliers = true;
	std::cout << " - removal of outliers: " << std::boolalpha << doRemoveOutliers << '\n';

	auto str_sortData = cmdl("sd").str();
	if( !str_sortData.empty() )
		params.useSortToFindThresholds = true;
	std::cout << " - threshold finding technique =" << (params.useSortToFindThresholds?"sort point":"histogram binning") << '\n';



	DataSet dataset;
	if( !dataset.load( fname, fparams ) )
	{
		std::cerr << "Error, unable to load data file: " << fname << '\n';
		std::exit(1);
	}

	dataset.printInfo( std::cout );
	auto stats = dataset.computeStats<float>( nbBins );
	std::cout << stats;
	dataset.generateAttribPlot( "dataA", stats );
	dataset.generateClassDistrib( "class_distribution" );

	if( doRemoveOutliers )
	{
		dataset.tagOutliers( stats );
		std::cout << "* outlier tagging: " << dataset.nbOutliers() << '\n';
		dataset.printInfo( std::cout );
		auto stats2 = dataset.computeStats<float>( nbBins );
		std::cout << stats2;
		dataset.generateAttribPlot( "dataB", stats2 );
	}

	if( noTraining )
	{
		std::cout << "No training required, exiting\n";
		std::exit(2);
	}

	if( nbFolds == 0 )
	{
		TrainingTree tt( dataset.getClassIndexMap() );
		tt.train( dataset, params );
//		auto cm = tt.classify( dataset );
//		std::cout << cm << "\n";
		tt.printInfo( std::cout, "Before Pruning" );
		tt.printDot( "dectree" );

		tt.pruning();
		tt.printInfo( std::cout, "After pruning" );
		tt.printDot( "dectree_p" );
		auto cm = tt.classify( dataset );
		std::cout << cm << "\n";
		if( dataset.nbClasses() == 2 )
			cm.printAllScores( std::cout );
		else
			cm.printAverageScores( std::cout );

	}
	else
	{
		dataset.shuffle();

		for( uint i=0; i<(uint)nbFolds; i++ )
		{
			TrainingTree tt( dataset.getClassIndexMap() );
			auto p_data_subsets = dataset.getFolds( i, nbFolds );
			auto data_train = p_data_subsets.first;
			auto data_test  = p_data_subsets.second;

			data_train.generateClassDistrib( "histo_tr_" + std::to_string(i) );
			data_test.generateClassDistrib(  "histo_te_" + std::to_string(i) );

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
