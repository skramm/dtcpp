
/**
Implementation of a Decision Tree using boost::graph
for continuous data values (aka real numbers)

- Limited to binary tree (a node has only two childs, i.e. a single threshold value)
- Does not handle missing values
*/

#include "dtcpp.h"
#include "argh.h" //  https://github.com/adishavit/argh

using namespace std;

int main( int argc, const char** argv )
{
    Fparams fparams;
    argh::parser cmdl({ "-sep"});
    cmdl.parse(argc, argv, argh::parser::PREFER_PARAM_FOR_UNREG_OPTION );

// optional arg: -sep X => X used as datafile field separator
    auto sepcl = cmdl("sep").str();
    if( !sepcl.empty() )
    {
        std::cout << " - using '" << sepcl << "' as datafile field separator\n";
        fparams.sep = sepcl[0];
    }

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
    if( noProcess )
    {
        std::exit(2);
    }

	auto stats = dataset.computeStats<float>();
	std::cout << stats;

    TrainingTree tt;
    if( !doFolding )
    {
        tt.train( dataset );
        auto perf = tt.classify( dataset );
        std::cout << "Score=" << perf << "\n";
        tt.printInfo( std::cout );
        tt.printDot( "demo.dot" );
    }
    else
    {
		dataset.shuffle();

        uint nbFolds = 5;
        for( uint i=0; i<nbFolds; i++ )
        {
            auto p_data_subsets = dataset.getFolds( i, nbFolds );
            auto data_train = p_data_subsets.first;
            auto data_test  = p_data_subsets.second;

            tt.train( data_train );
            auto perf_t = tt.classify( data_train );
            auto perf_c = tt.classify( data_test );
            std::cout << "Fold " << i+1 << "/" << nbFolds
				<< "\n- Training score=" << perf_t
				<< "\n- Test score=" << perf_c
				<< "\n";
        }
    }
}
