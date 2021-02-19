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

#define DEBUG

#ifdef DEBUG
	#define COUT if(1) std::cout
#else
	#define COUT if(0) std::cout
#endif // DEBUG

#define START std::cout << __FUNCTION__ << "()\n";

// forward declaration
//template<typename U>
//class DataSet;

//---------------------------------------------------------------------
/// General utility function
template<typename T>
void
printVector( std::ostream& f, const std::vector<T>& vec )
{
	f << "Vector: #=" << vec.size() << ":\n";
	for( const auto& elem : vec )
		f << elem << "-";
	f << "\n";
}

//---------------------------------------------------------------------
/// A datapoint, holds a set of attributes value and a corresponding (binary) class
//template<typename T>
class DataPoint
{
//	template<typename U>
	friend class DataSet;

	private:
//		std::vector<T> _attrValue;
		std::vector<float> _attrValue;
		int _class = -1;  ///< Class of the datapoint, -1 for undefined

	public:
		DataPoint( const std::vector<float>& vec, int c ) :
			_attrValue(vec), _class(c)
		{}

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

//		template<typename U>
		void setAttribVector( const std::vector<float>& vec )
		{
			assert( vec.size() == nbAttribs() );
			_attrValue = vec;
		}
		void setClass( int c )
		{
			assert( c >=0 );  // -1 is illegal
			_class = c;
		}
};

//---------------------------------------------------------------------
/// A dataset, holds a set of \ref DataPoint
//template<typename T>
class DataSet
{
	public:
		DataSet() : _nbAttribs(0)
		{
		}
		DataSet( size_t n ) : _nbAttribs(n)
		{ assert( n ); }
		size_t size() const
		{ return _dataPoint.size(); }

		size_t nbAttribs() const
		{ return _nbAttribs; }

		void setNbAttribs( uint n )
		{
			assert( n>1 );
			if( size() )
				throw std::runtime_error( "cannot set size if data set not empty" );
			_nbAttribs = n;
		}

//		template<typename U>
		void addDataPoint( const DataPoint& dp )
		{
			assert( dp.nbAttribs() == _nbAttribs );
			_dataPoint.push_back( dp );
		}
//		template<typename U>
		DataPoint& getDataPoint( uint idx )
		{
			assert( idx < _dataPoint.size() );
			return _dataPoint[idx];
		}
//		template<typename U>
		const DataPoint& getDataPoint( uint idx ) const
		{
			assert( idx < _dataPoint.size() );
			return _dataPoint[idx];
		}
		bool load( std::string fname );
		void print( std::ostream& ) const;

	private:
		size_t _nbAttribs = 0;
		std::vector<DataPoint> _dataPoint;
//		std::vector<DataPoint<T>> _dataPoint;
};
//using DataSetf = DataSet<float>;
//using DataSetd = DataSet<double>;

//---------------------------------------------------------------------
//template<typename T>
bool
DataSet::load( std::string fname )
{

	return true;
}
//---------------------------------------------------------------------
//template<typename T>
void
DataSet::print( std::ostream& f ) const
{
	f << "# Dataset, nb pts=" << size() << " nb attributes=" << nbAttribs() << "\n";
	for( size_t i=0; i<nbAttribs(); i++ )
		f << i << "; ";
	f << " class\n";
	for( const auto& pt: _dataPoint )
		for( const auto& val: pt._attrValue )
			f << val << ";";
	f << "\n";
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
//template<typename T>
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
//template<typename T>
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
//template<typename T>
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
//template<typename T>
void
TrainingTree::printDot( std::ostream& f ) const
{
// TODO
}
//---------------------------------------------------------------------
/// Computes the nb of votes for each class, for the points defined in \c v_Idx
std::map<uint,uint>
computeClassVotes( const std::vector<uint>& v_Idx, const DataSet&  data )
{
	assert( v_Idx.size()>1 );

	std::map<uint,uint> classVotes; // key: class index, value: votes
	for( auto idx: v_Idx )
	{
		const auto& dp = data.getDataPoint( idx );
		classVotes[ dp.classVal() ]++;
	}
	return classVotes;
}
//---------------------------------------------------------------------
/// Compute best IG (Information Gain) of attribute \c atIdx of the subset of data given by \c v_dpidx.
/// Will return the IG value AND the threshold value for which is was produced
/**
Details:
- Uses the Gini coeff: https://en.wikipedia.org/wiki/Gini_coefficient
- for details, see
 - https://en.wikipedia.org/wiki/Information_gain_in_decision_trees
 - https://towardsdatascience.com/under-the-hood-decision-tree-454f8581684e
*/
//template<typename T>
std::pair<float,float>
computeIG(
	uint                     atIdx,   ///< attribute index
	const std::vector<uint>& v_dpidx, ///< datapoint indexes to consider
	const DataSet&           data     ///< dataset
)
{
	START;
	std::pair<float,float> v;

// step 0 - compute Gini coeff for all the points
	auto classVotes = computeClassVotes( v_dpidx, data );

	double giniCoeff = 1.;
	for( auto elem: classVotes )
	{
		auto v = 1. * elem.second / v_dpidx.size();
		giniCoeff -= v*v;
	}

// step 1 - compute all the potential threshold values (mean value between two consecutive attribute values)

	std::vector<float> v_attribVal( v_dpidx.size() ); // pre-allocate vector size (faster than push_back)
	for( size_t i=0; i<v_dpidx.size(); i++ )
		v_attribVal[i] = data.getDataPoint( v_dpidx[i] ).attribVal( atIdx );

	std::sort( v_attribVal.begin(), v_attribVal.end() );         // sort the attribute values

	std::vector<float> v_thresVal( v_dpidx.size()-1 ); // if 10 values, then only 9 thresholds
	for( uint i=0; i<v_dpidx.size()-1; i++ )
		v_thresVal[i] = 1. * ( v_attribVal.at(i) + v_attribVal.at(i+1) ) / 2.;

	printVector( std::cout, v_thresVal );

// step 2: compute IG for each threshold value

	std::vector<float> deltaGini( v_thresVal.size() );
	for( uint i=0; i<v_thresVal.size(); i++ )  // for each threshold value
	{
		std::map<uint,uint> m_LT, m_HT; //
		uint nb_LT = 0;
		uint nb_HT = 0;
		for( auto ptIdx: v_dpidx )    // for each data point
		{
			auto point = data.getDataPoint(ptIdx);;
			auto attribVal = point.attribVal( atIdx );
			if( attribVal<v_thresVal[i] )
			{
				m_LT[ point.classVal() ]++;
				nb_LT++;
			}
			else
			{
				m_HT[ point.classVal() ]++;
				nb_HT++;
			}
		}

		auto g_LT = 1.;
		for( auto p: m_LT )  // for the values that are Lower Than the threshold
		{
			auto val = 1. * p.second / nb_LT;
			g_LT -= val*val;
		}
		auto g_HT = 1.;
		for( auto p: m_LT )  // for the values that are Higher Than the threshold
		{
			auto val = 1. * p.second / nb_HT;
			g_HT -= val*val;
		}
		deltaGini[i] = giniCoeff - (g_LT + g_HT) / 2.;
	}

// step 3 - find max value of the delta Gini
	auto max_pos = std::max_element(
		std::begin( deltaGini ),
		std::end( deltaGini )
	);

	COUT << "max gini for thres idx=" << *max_pos << "\n";

	return v;
}
//---------------------------------------------------------------------
/// Finds the best attributes to use, considering the data points of the current node
/// and compute threshold on that attribute so that the two classes are separated at best.
/// Returns the index of this attribute
//template<typename T>
std::pair<uint,float>
findBestAttribute(
	const std::vector<uint>& vIdx,  ///< indexes of data points we need to consider
	const DataSet&           data,  ///< whole dataset
	std::map<uint,bool>&     aMap   ///< map of the attributes that "may" be left to explore
)
{
	START;
	using Pfi_t = std::pair<float,int>;

// step 1 - fetch the indexes of the attributes we need to explore
	std::vector<uint> v_attrIdx;
	for( auto elem: aMap )
		if( elem.second == false )
			v_attrIdx.push_back(elem.first);

	assert( v_attrIdx.size() );   // if not, well... shouldn't happen !

// step 2 - compute best IG/threshold for each of these attributes, only for the considered points
	std::vector<std::pair<float,float>> v_IG;
	for( auto atIdx: v_attrIdx )
		v_IG.push_back( computeIG( atIdx, vIdx, data ) );

// step 3 - get the one with max value and compute the best threshold
	auto it_mval = std::max_element( std::begin(v_IG), std::end(v_IG) ); // TODO: need a lambda here !

	std::pair<uint,float> retval;
//	retval.first = *it_mval;

// Store the attribute values and the output class into a container, so it can be sorted
	std::vector<Pfi_t> v_pairs( vIdx.size() );
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
		( Pfi_t p1, Pfi_t p2 )
		{
			return p1.first < p2.first;
		}
	);


	return retval	;
}
//---------------------------------------------------------------------
/// Returns the class that is in majority in the points defined in \c vIdx
/// second value is the percentage of that majority, in the range [0,1] (well, ]0.5,1] actually !)
//template<typename T>
std::pair<int,float>
getMajorityClass( const std::vector<uint>& vIdx, const DataSet& data )
{
	using Pair = std::pair<uint,uint>;

	auto classVotes = computeClassVotes( vIdx, data );

	auto it_max = std::max_element(
		std::begin( classVotes ),
		std::end( classVotes ),
		[]                                      // lambda
		(const Pair& a, const Pair& b)->bool
		{ return a.second < b.second; }
	);

	auto idx_maj = it_max->first;

	return std::make_pair(
		idx_maj,
		1. * classVotes[idx_maj] / vIdx.size()
	);

}
//---------------------------------------------------------------------
/// Recursive helper function, used by TrainingTree::Train()
/**
Computes the threshold, splits the dataset and assigns the split to 2 sub nodes (that get created)
*/
////template<typename T>
bool
splitNode(
	vertexT_t            v,         ///< current node id
	std::map<uint,bool>& aMap,      ///< attribute map, holds true for all the attributes already used
	GraphT&              graph,     ///< graph
	const DataSet&    data       ///< dataset
)
{
	START;
	const auto& vIdx = graph[v].v_Idx; // vector holding the indexes of the datapoints for this node


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
		auto majo = getMajorityClass( vIdx, data );
		COUT << "no more attributes, done\n";
		graph[v]._type = NT_Final;
		graph[v]._class = majo.first;
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
//template<typename T>
bool
TrainingTree::Train( const DataSet& data )
{
	START;
	auto nbAttribs = data.nbAttribs();
	if( !nbAttribs )
		return false;

// step 1 - compute entropy of dataset


// step 2 - compute entropy of each attribute and find the one that maximizes information

//int attribIdx = 0;
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
//template<typename T>
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
//template<typename T>
void
DecisionTree::addDecision()
{

}

