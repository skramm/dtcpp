
/**
Implementation of a Decision Tree using boost::graph
for continuous data values (aka real numbers)

- Limited to binary tree (a node has only two childs)
- Does not handle missing values
*/

#include "dtcpp.h"

using namespace std;

int main()
{
    cout << "Hello world!" << endl;

    DataSet dataset;
    dataset.load( "demodata1.csv" );

    TrainingTree tt;
    tt.Train( dataset );
    tt.printDot( std::cout );
}
