
/**
Implementation of a Decision Tree using boost::graph
for continuous data values (aka real numbers)

- Limited to binary tree (a node has only two childs)
- Does not handle missing values
*/

#include "dtcpp.h"

using namespace std;

int main( int argc, const char** argv )
{
    DataSet dataset;
    if( argc > 1 )
        dataset.load( argv[1] );
    else
        dataset.load( "sample_data/tds_1.csv" );

	dataset.print( std::cout );
    TrainingTree tt;
    tt.Train( dataset );
    tt.printInfo( std::cout );
    tt.printDot( "demo.dot" );
}
