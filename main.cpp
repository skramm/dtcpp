
/**
Implementation of a Decision Tree using boost::graph
for continuous data values (aka real numbers)

- Limited to binary tree (a node has only two childs)
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


    auto sepcl = cmdl("sep").str();
    if( !sepcl.empty() )
    {
        std::cout << " - using '" << sepcl << " as datafile field separator\n";
        fparams.sep = sepcl[0];
    }


/*cout << "Positional args:\n";
for (auto& pos_arg : cmdl)
  cout << '\t' << pos_arg << '\n';
  cout << "cmdl size=" << cmdl.size() << '\n';
*/

    DataSet dataset;
    std::string fname = "sample_data/tds_1.csv";
    if( cmdl.size() > 1 )
        fname = cmdl[1];

    if( !dataset.load( fname, fparams ) )
    {
        std::cerr << "Error, unable to load data file: " << fname << '\n';
        std::exit(1);
    }

	dataset.print( std::cout );

    TrainingTree tt;
    tt.Train( dataset );
    tt.printInfo( std::cout );
    tt.printDot( "demo.dot" );
}
