/**
\file
\brief Naive implementation attempt of a classifier using a Decision Tree
for continuous data values (aka real numbers)
\author S. Kramm - 2021

- Limited to binary tree (a node has only two childs)
- Does not handle missing values
- using boost::graph to model the tree
*/

#include <iostream>
#include <vector>
#include <boost/graph/adjacency_list.hpp>

#ifdef DEBUG
	#define COUT if(1) std::cout
#else
	#define COUT if(0) std::cout
#endif // DEBUG

//---------------------------------------------------------------------
class DataPoint
{
	private:
		std::vector<float> _attrValue;
		int _class = -1;  ///< Class of the datapoint, -1 for undefined
	public:
		size_t nbAttribs() const
		{
			return _attrValue.size();
		}
		void setSize( size_t n ) { _attrValue.resize(n); }
		float attribVal( size_t idx ) const
		{
			assert( idx< _attrValue.size() );
			return _attrValue[idx];
		}
};

//---------------------------------------------------------------------
class DataSet
{
	public:
		DataSet( size_t n ) : _nbAttribs(n)
		{ assert( n ); }
		size_t size() const { return _dataPoint.size(); }
		size_t nbAttribs() const
		{ return _nbAttribs; }
		void addDataPoint( const DataPoint& dp )
		{
			assert( dp.nbAttribs() == _nbAttribs );
			_dataPoint.push_back( dp );
		}
		DataPoint& getDataPoint( size_t idx )
		{
			assert( idx < _dataPoint.size() );
			return _dataPoint[idx];
		}
		const DataPoint& getDataPoint( size_t idx ) const
		{
			assert( idx < _dataPoint.size() );
			return _dataPoint[idx];
		}
	private:
		size_t _nbAttribs = 0;
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
	size_t   _depth = 0;         ///< depth of the node in the tree
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
		size_t _maxDepth = 1;  ///< defined by training

	public:
		DecisionTree();
		bool Train( const DataSet& );
		int Classify( const DataPoint& ) const;
		void addDecision();
		size_t maxDepth() const { return _maxDepth; }
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
/// Train tree using data.
/// Return false if failure
bool
DecisionTree::Train( const DataSet& data )
{
	auto nbAttribs = data.nbAttribs();
	if( !nbAttribs )
		return false;

// step 1 - compute entropy of dataset


// step 2 - compute entropy of each attribute

// key: attribute index, val: entropy
//	std::vector<size_t,float> entropyVal( nbAttribs );


	for( auto i=0; i<nbAttribs; i++ )
	{
		for( auto j=0; j<data.size(); j++ )
		{
			auto datapoint = data.getDataPoint( j );
			auto attrVal = datapoint.attribVal( i );
		}
	}


// step 3 - order attribute by entropy, so we start with the attribute that has the highest entropy


	std::vector<size_t> sortedAttributeIndex;

// step 4 - split iteratively dataset and build tree
	bool done = true;
	for( uint i=0; i<nbAttribs; i++ )
	{
		;
	}

	return true;
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
			COUT << "Final node " << v << ", class = " << retval << "\n";
		}
		else
		{
			auto attrIndex = _graph[v]._attrIndex;  // get attrib index that this node handles
			auto pt_val = point.attribVal( attrIndex );  // get data point value for this attribute

			auto edges = boost::out_edges( v, _graph );    // get the two output edges of the node
			auto et = *edges.first;
			auto ef = *edges.second;
			if( _graph[et]._res )
				std::swap( et, ef );

			if( pt_val < _graph[v]._threshold )  // depending on threshold, define the next node
				v = boost::target( et, _graph );
			else
				v = boost::target( ef, _graph );
			COUT << "switching to node " << v << "\n";
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

