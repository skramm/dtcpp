/**
\file
\brief Naive implementation attempt of a classifier using a Decision Tree
for continuous data values (aka real numbers)
\author S. Kramm - 2021

- Limited to binary classification (a tree node has only two childs) but goal is to expand this to multiclass
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

#define START std::cout << __FUNCTION__ << "()\n";


//---------------------------------------------------------------------
/// A datapoint, holds a set of attributes value and a corresponding (binary) class
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
		int classVal() const { return _class; }
		void setSize( size_t n ) { _attrValue.resize(n); }
		float attribVal( size_t idx ) const
		{
			assert( idx<_attrValue.size() );
			return _attrValue[idx];
		}
};

//---------------------------------------------------------------------
/// A dataset, holds a set of \ref DataPoint
class DataSet
{
	public:
		DataSet() : _nbAttribs(0)
		{
		}
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
		bool load( std::string fname );
	private:
		size_t _nbAttribs = 0;
		std::vector<DataPoint> _dataPoint;
};

//---------------------------------------------------------------------
bool
DataSet::load( std::string fname )
{

	return true;
}
//---------------------------------------------------------------------
enum NodeType
{
	NT_Root, NT_Decision, NT_Final
};

//---------------------------------------------------------------------
/// A node of the tree
/// \todo separate class into two, because decision node and terminal nodes do not hold the same things
struct NodeC
{
	NodeType _type;
	size_t   _attrIndex = 0;     ///< Attribute Index that this nodes classifies
	float    _threshold = 0.f;   ///< Threshold on the attribute value (only for decision nodes)
	int      _class = -1;        ///< (only for terminal nodes)
	uint     depth = 0;         ///< depth of the node in the tree
};


//---------------------------------------------------------------------
struct NodeT
{
	NodeType _type;
	int      _class = -1;        ///< (only for terminal nodes)
	size_t   _attrIndex = 0;     ///< Attribute Index that this nodes classifies
	float    _threshold = 0.f;   ///< Threshold on the attribute value (only for decision nodes)
	uint     depth = 0;         ///< depth of the node in the tree

	std::vector<uint> v_Idx; ///< data point indexes
};

//---------------------------------------------------------------------
/// Edge of the tree. Single parameter is true/false of the above decision, depending on threshold
struct EdgeC
{
	bool side;
};
//---------------------------------------------------------------------
/// Edge of the tree. Single parameter is true/false of the above decision, depending on threshold
struct EdgeT
{
	bool side;
};

//---------------------------------------------------------------------
/// used for classifying
using GraphC = boost::adjacency_list<
		boost::vecS,
		boost::vecS,
		boost::directedS,
		NodeC,
		EdgeC
	>;

/// used for training
using GraphT = boost::adjacency_list<
		boost::vecS,
		boost::vecS,
		boost::directedS,
		NodeT,
		EdgeT
	>;

using vertexT_t = boost::graph_traits<GraphT>::vertex_descriptor;
using vertexC_t = boost::graph_traits<GraphC>::vertex_descriptor;

//---------------------------------------------------------------------
/// This one holds edges that each have a vector holding the index of datapoints.
/// This is memory costly, but useless for classifying, so once it is trained, we can use the \ref DecisionTree class
/// \todo unify by templating the type of edges
class TrainingTree
{
	private:
		GraphT _graph;
		size_t _maxDepth = 1;  ///< defined by training
	public:
		bool Train( const DataSet& );
		void printDot( std::ostream& ) const;
};

//---------------------------------------------------------------------
class DecisionTree
{
	private:
		GraphC _graph;
		size_t _maxDepth = 1;  ///< defined by training

	public:
		DecisionTree();
		int Classify( const DataPoint& ) const;
		void addDecision();
		size_t maxDepth() const { return _maxDepth; }
};

//---------------------------------------------------------------------
/// Create a DT having a single decision point (root node) and 3 nodes
DecisionTree::DecisionTree()
{
/*	auto r  = boost::add_vertex(_graph);
	auto v1 = boost::add_vertex(_graph);
	auto v2 = boost::add_vertex(_graph);
	_graph[r]._type  = NT_Root;
	_graph[v1]._type = NT_Final;
	_graph[v2]._type = NT_Final;

	auto et = boost::add_edge( r, v1, _graph );
	auto ef = boost::add_edge( r, v2, _graph );
	_graph[et.first]._res = true;
	_graph[ef.first]._res = false;*/
}

//---------------------------------------------------------------------
/// Print a DOT file of the tree
void
TrainingTree::printDot( std::ostream& f ) const
{
// TODO
}
//---------------------------------------------------------------------
/// Compute entropy of attribute \c atIdx of the subset of data given by \c v_dpidx
float
computeEntropy(
	uint                     atIdx,   ///< attribute index
	const std::vector<uint>& v_dpidx, ///< datapoint indexes to consider
	const DataSet&           data     ///< dataset
)
{
	START;
	float v;
// TODO
	return v;
}
//---------------------------------------------------------------------
/// Finds the best attributes to use, considering the data points of the current node
/// and compute threshold on that attribute so that the two classes are separated at best.
/// Returns the index of this attribute
std::pair<uint,float>
findBestAttribute(
	const std::vector<uint>& vIdx,  ///< indexes of data points we need to consider
	const DataSet&           data,  ///< whole dataset
	std::map<uint,bool>&     aMap   ///< map of the attributes that "may" be left
)
{
	START;
	using pfi_t = std::pair<float,int>;

// step 1 - fetch the indexes of the attributes we need to explore
	std::vector<uint> v_attrIdx;
	for( auto elem: aMap )
		if( elem.second == false )
			v_attrIdx.push_back(elem.first);

// step 2 - compute IG for each of these attributes, only for the considered points
	std::vector<float> v_entropy;
	for( auto atIdx: v_attrIdx )
		v_entropy.push_back( computeEntropy( atIdx, vIdx, data ) );

// step 3 - get the one with min value and compute the best threshold
	auto it_mval = std::min_element( std::begin(v_entropy), std::end(v_entropy) );

	std::pair<uint,float> retval;
	retval.first = *it_mval;

// Store the attribute values and the output class into a container, so it can be sorted
	std::vector<pfi_t> v_pairs( vIdx.size() );
	uint i=0;
	for( auto dpIdx : vIdx )
	{
		const auto& dataPoint = data.getDataPoint( dpIdx );
		v_pairs.at(i).first  = dataPoint.attribVal( retval.first );
		v_pairs.at(i).second = dataPoint.classVal();
		i++;
	}

// sort it
	std::sort(
		std::begin(v_pairs),
		std::end(v_pairs),
		[]                         // lambda
		( pfi_t p1, pfi_t p2 )
		{
			return p1.first < p2.first;
		}
	);


	return retval	;
}
//---------------------------------------------------------------------
/// Recursive helper function, used by TrainingTree::Train()
/**
Computes the threshold, splits the dataset and assigns the split to 2 sub nodes (that get created)
*/
bool
splitNode(
	vertexT_t            v,         ///< current node
	std::map<uint,bool>& aMap,      ///< attribute map, holds true for all the attributes already used
	GraphT&              graph,     ///< graph
	const DataSet&       data       ///< dataset
)
{
	START;
	auto vIdx = graph[v].v_Idx; // vector holding the indexes of the datapoints for this node


// step 1.1 - check if there are different output classes in the given data points
// if not, then we are done
	size_t c1 = 0;
	for( auto idx: vIdx )
		if( data.getDataPoint( idx ).classVal() )
			c1++;
	size_t c0 = vIdx.size() - c1;
	COUT << "c0=" << c0 << " c1=" << c1 << "\n";
	if( c0 == 0 || c1 == 0 )
	{
		COUT << "single class in current dataset split holding " << vIdx.size() << " pts\n";
		graph[v]._type = NT_Final;
		graph[v]._class = (c0 ? 0 : 1);
		return true;
	}

// step 1.2 - check if there are some remaining attributes to use
// if not, then we are done
	bool foundAnother = false;
	for( const auto& elem: aMap )
		if( elem.second == false )
			foundAnother = true;
	if( !foundAnother )
	{
		COUT << "no more attributes, done\n";
		graph[v]._type = NT_Final;
		return true;
	}

// step 2 - find the best attribute to use to split the data, considering the data points of the current node
	auto pair_id_th = findBestAttribute( vIdx, data, aMap );
	auto attribIdx  = pair_id_th.first;
	auto threshold  = pair_id_th.second;
	COUT << "best attrib=" << attribIdx << " thres value=" << threshold << "\n";

	aMap[attribIdx] = true;  // so we will not use it again
	graph[v]._attrIndex = attribIdx;
	graph[v]._threshold = threshold;
	graph[v]._type = NT_Decision;

// step 3 - different classes here: we create two child nodes and split the dataset
	auto v1 = boost::add_vertex(graph);
	auto v2 = boost::add_vertex(graph);

	graph[v1].depth = graph[v].depth+1;
	graph[v2].depth = graph[v].depth+1;


	auto et = boost::add_edge( v, v1, graph );
	auto ef = boost::add_edge( v, v2, graph );
	graph[et.first].side = true;
	graph[ef.first].side = false;

	COUT << "two nodes added, total nb=" << boost::num_vertices(graph) << "\n";

	for( auto idx: vIdx )           // separate the data points into two sets
	{
		auto point = data.getDataPoint( idx );
		auto attrVal = point.attribVal( attribIdx );
		if( attrVal < threshold )
			graph[v1].v_Idx.push_back( idx );
		else
			graph[v2].v_Idx.push_back( idx );
		auto b1 = splitNode( v1, aMap, graph, data );
		auto b2 = splitNode( v2, aMap, graph, data );
		if( b1 && b2 )
		{
			COUT << "both nodes are done, return true\n";
			return true;
		}
	}
	return false;
}
//---------------------------------------------------------------------
/// Train tree using data.
/// Return false if failure
bool
TrainingTree::Train( const DataSet& data )
{
	START;
	auto nbAttribs = data.nbAttribs();
	if( !nbAttribs )
		return false;

// step 1 - compute entropy of dataset


// step 2 - compute entropy of each attribute and find the one that maximizes information

int attribIdx = 0;
/*
	for( auto i=0; i<nbAttribs; i++ )
	{
		for( auto j=0; j<data.size(); j++ )
		{
			auto datapoint = data.getDataPoint( j );
			auto attrVal = datapoint.attribVal( i );
		}
	}*/

// step 3 - Create initial node, and assign to it the whole dataset

	auto n0  = boost::add_vertex(_graph);
	std::vector<uint> v_idx( data.size() );
	uint i=0;
	for( auto& id: v_idx )
		id = i++;
	_graph[n0].v_Idx = v_idx;
	_graph[n0]._type = NT_Root;

	std::map<uint,bool> attribMap;
	for( uint i=0; i<nbAttribs; i++ )
		attribMap[i] = false;

// Call the "split" function (recursive)
	splitNode( n0, attribMap, _graph, data );


//order attribute by entropy, so we start with the attribute that has the highest entropy


//	std::vector<size_t> sortedAttributeIndex(nbAttribs);

// step 4 - split iteratively dataset and build tree
/*	bool done = true;
	for( uint i=0; i<nbAttribs; i++ )
	{
		for( uint dpidx=0; dpidx<data.size(); dpidx++ )
		{
			auto point = data.getDataPoint( dpidx );
//			if( point.class() )

		}

	}*/

	return true;
}

//---------------------------------------------------------------------
/// Returns class of data point as classified by tree
int
DecisionTree::Classify( const DataPoint& point ) const
{
	int retval = -1;
	vertexC_t v = 0;   ///< initialize for first node
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
			if( _graph[et].side )
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

