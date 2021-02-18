/**
\file
\brief Naive implementation attempt of a Decision Tree using boost::graph
for continuous data values (aka real numbers)
\author S. Kramm - 2021

- Limited to binary tree (a node has only two childs)
- Does not handle missing values
*/

#include <iostream>
#include <vector>
#include <boost/graph/adjacency_list.hpp>

#define DEBUG std::cout

//---------------------------------------------------------------------
class DataPoint
{
	private:
		std::vector<float> _attrValue;
		int _class = -1;  ///< Class of the datapoint, -1 for undefined
	public:
		void setSize( size_t n ) { _attrValue.resize(n); }
		float getValue( size_t idx ) const
		{
			assert( idx< _attrValue.size() );
			return _attrValue[idx];
		}
};

//---------------------------------------------------------------------
class DataSet
{
	std::vector<DataPoint> _dataPoint;
};

//---------------------------------------------------------------------
enum NodeType
{
	NT_Root, NT_Decision, NT_Final
};

//---------------------------------------------------------------------
/// A node of the tree
/// \todo separate class into two, because decision node and terminal nodes do not hold the same things
struct Node
{
	NodeType _type;
	size_t   _attrIndex = 0;     ///< Attribute Index that this nodes classifies
	float    _threshold = 0.f;   ///< Threshold on the attribute value (only for decision nodes)
	int      _class = -1;        ///< (only for terminal nodes)
};

//---------------------------------------------------------------------
/// Edge of the tree. Single parameter is true/false of the above decision, depending on threshold
struct Edge
{
	bool _res;
};

using Graph = boost::adjacency_list<
		boost::vecS,
		boost::vecS,
		boost::directedS,
		Node,
		Edge
	>;

using vertex_t = boost::graph_traits<Graph>::vertex_descriptor;

//---------------------------------------------------------------------
class DecisionTree
{
	private:
		Graph _graph;
	public:


	DecisionTree();
	void Train( const DataSet& );
	int Classify( const DataPoint& ) const;
	void addDecision();

};

//---------------------------------------------------------------------
/// Create a DT having a single decision point (root node) and 3 nodes
DecisionTree::DecisionTree()
{
	auto r  = boost::add_vertex(_graph);
	auto v1 = boost::add_vertex(_graph);
	auto v2 = boost::add_vertex(_graph);
	_graph[r]._type  = NT_Root;
	_graph[v1]._type = NT_Final;
	_graph[v2]._type = NT_Final;

	auto et = boost::add_edge( r, v1, _graph );
	auto ef = boost::add_edge( r, v2, _graph );
	_graph[et.first]._res = true;
	_graph[ef.first]._res = false;
}

//---------------------------------------------------------------------
/// Train tree using data
void
DecisionTree::Train( const DataSet& data )
{

}

//---------------------------------------------------------------------
/// Returns class of data point as classified by tree
int
DecisionTree::Classify( const DataPoint& point ) const
{
	int retval = -1;
	vertex_t v = 0;   ///< initialize for first node
	bool done = true;
	do
	{
		if( _graph[v]._type == NT_Final ) // then, we are done !
		{
			done = true;
			retval = _graph[v]._class;
			DEBUG << "Final node " << v << ", class = " << retval << "\n";
		}
		else
		{
			auto attrIndex = _graph[v]._attrIndex;
			auto pt_val = point.getValue( attrIndex );

			auto edges = boost::out_edges( v, _graph );    // get the two output edges of the node
			auto et = *edges.first;
			auto ef = *edges.second;
			if( _graph[et]._res )
				std::swap( et, ef );

			if( pt_val < _graph[v]._threshold )  // depending on threshold, define the next node
				v = boost::target( et, _graph );
			else
				v = boost::target( ef, _graph );
			DEBUG << "switching to node " << v << "\n";
		}
	}
	while( !done );
	return retval;
}

//---------------------------------------------------------------------
/// Add a decision node => transform given terminal node into decision node
void
DecisionTree::addDecision()
{

}

