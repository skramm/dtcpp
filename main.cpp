
/**
Implementation of a Decision Tree using boost::graph
for continuous data values (aka real numbers)

- Limited to binary tree (a node has only two childs, i.e. a single threshold value)
- Does not handle missing values
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
    argh::parser cmdl; //({ "-sep"});
    cmdl.parse(argc, argv, argh::parser::PREFER_PARAM_FOR_UNREG_OPTION );

// optional arg: -sep X => X used as datafile field separator
    auto sepcl = cmdl("sep").str();
    if( !sepcl.empty() )
        fparams.sep = sepcl[0];
	std::cout << " - using '" << fparams.sep << "' as datafile field separator\n";


// optional arg: -cs => the class value in the datafile is given as a string value
    if( cmdl["-cs"] )
        fparams.classAsString = true;

// optional arg: -i => only prints info about the data set and exit
	bool noProcess = false;
    if( cmdl["-i"] )
        noProcess = true;

// optional arg: -f => evaluate performance on training dataset by doing folds on data
    bool doFolding = false;
    if( cmdl["-f"] )
        doFolding = true;

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


    if( noProcess )
    {
    	std::cout << "No processing required, exiting\n";
        std::exit(2);
    }

	auto stats = dataset.computeStats<float>();
	std::cout << stats;

	auto nbClasses = dataset.nbClasses();

    if( !doFolding )
    {
		TrainingTree tt( nbClasses );
        tt.train( dataset );
        auto cm = tt.classify( dataset );
        std::cout << "Confusion Matrix:" << cm << "\n";
        tt.printInfo( std::cout );
        tt.printDot( "demo.dot" );
        cm.printAllScores( std::cout );
    }
    else
    {
		dataset.shuffle();

        uint nbFolds = 5;
        for( uint i=0; i<nbFolds; i++ )
        {
			TrainingTree tt( nbClasses );
            auto p_data_subsets = dataset.getFolds( i, nbFolds );
            auto data_train = p_data_subsets.first;
            auto data_test  = p_data_subsets.second;
			data_train.printInfo( std::cout );
			data_test.printInfo( std::cout );
            tt.train( data_train );
            std::ostringstream oss;
            oss << "demo_" << i << ".dot";
            tt.printDot( oss.str() );
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
