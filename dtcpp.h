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
#include <fstream>
#include <vector>
#include <numeric>

#include <boost/graph/adjacency_list.hpp>
#include "boost/graph/graphviz.hpp"
#include <boost/graph/graph_utility.hpp> // needed only for print_graph();


#define DEBUG

#ifdef DEBUG
	#define COUT if(1) std::cout << __FUNCTION__ << "(): "
#else
	#define COUT if(0) std::cout
#endif // DEBUG

#ifdef DEBUG_START
	#define START if(1) std::cout << "* Start: " << __FUNCTION__ << "()\n"
#else
	#define START ;
#endif // DEBUG

#define LOG \
	if( params.verbose ) \
		std::cout

//---------------------------------------------------------------------
/// A template to have strong types, taken from J. Boccara
/**
 * https://www.fluentcpp.com/2016/12/08/strong-types-for-strong-interfaces/
*/
template <typename T, typename Parameter>
class NamedType
{
public:
	NamedType() {}
    explicit NamedType(T const& value) : value_(value) {}
    explicit NamedType(T&& value) : value_(std::move(value)) {}
    T&       get()       { return value_; }
    T const& get() const { return value_; }
	friend std::ostream& operator << ( std::ostream& f, const NamedType& nt )
	{
		f << nt.value_;
		return f;
	}
private:
    T value_;
};
using ThresholdVal = NamedType<float,struct ThresholdValTag>;

//---------------------------------------------------------------------

// forward declaration
//template<typename U>
//class DataSet;

//---------------------------------------------------------------------

// % % % % % % % % % % % % % %
/// inner namespace
namespace priv {
// % % % % % % % % % % % % % %

//---------------------------------------------------------------------
/// General utility function
template<typename T>
void
printVector( std::ostream& f, const std::vector<T>& vec, const char* msg=0 )
{
	f << "Vector: ";
	if( msg )
		f << msg;
	f << " #=" << vec.size() << ":\n";
	for( const auto& elem : vec )
		f << elem << "-";
	f << "\n";
}

//-------------------------------------------------------------------
/// Remove multiple spaces AND TABS in string, allows only one, except if in first position
/**
Also replaces tabs with spaces
*/
std::string
trimSpaces( const std::string& input )
{
	assert( input.size() > 0 );
	bool HasOneAlready( false );
	bool FirstElem( true );
	std::string out;
	for( auto c: input )
	{
		if( c != ' ' && c != 9 )
		{
			out.push_back( c );
			HasOneAlready = false;
			FirstElem = false;
		}
		else {
			if( !HasOneAlready && !FirstElem )
			{
				out.push_back( ' ' ); // add a space character
				HasOneAlready = true;
			}
		}
	}
	if( out.back() == ' ' ) // if last element is a space, then remove it
		out.erase( out.end()-1 );

	return out;
}

//---------------------------------------------------------------------
/// General string tokenizer, taken from http://stackoverflow.com/a/236803/193789
/**
- see also this one: http://stackoverflow.com/a/53878/193789
*/
inline
std::vector<std::string>
splitString( const std::string &s, char delim )
{
	std::vector<std::string> velems;
    std::stringstream ss( trimSpaces(s) );
    std::string item;
    while( std::getline( ss, item, delim ) )
        velems.push_back(item);

    return velems;
}

// % % % % % % % % % % % % % %
} // namespace priv
// % % % % % % % % % % % % % %

//---------------------------------------------------------------------
/// Parameters
struct Params
{
	float minGiniCoeffForSplitting = 0.05f;
	uint  minNbPoints = 3;                   ///< minumum nb of points to create a node
	float removalCoeff = 0.05f; ///< used to remove close attribute values when searching ofr best threshold. See removeDuplicates()
	bool  verbose = true;        ///< to allow logging of some run-time details
	bool  doFolding = false;
};

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
/// Constructor used in tests
		DataPoint( const std::vector<float>& vec, int c ) :
			_attrValue(vec), _class(c)
		{}
/// Constructor from a vector of strings (used by file reader)
		DataPoint( const std::vector<std::string>& v_string, int c )
		{
			assert( v_string.size() > 0 );              // at least one attribute and a class value

			for( size_t i=0; i<v_string.size(); i++ )
				_attrValue.push_back( std::stof( v_string[i] ) );
//			_class = std::stoi( v_string.back() );          // last value of the vector is the class
			_class = c;
		}

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
/// Parameters for data file
struct Fparams
{
	char sep = ' ';  ///< field separator
	bool classAsString = false;  ///< class values are given as strings
	bool noProcess = false;      ///< no processing if true
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
		{ return _data.size(); }

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
			_data.push_back( dp );
		}
//		template<typename U>
		DataPoint& getDataPoint( uint idx )
		{
			assert( idx < _data.size() );
			return _data[idx];
		}
//		template<typename U>
		const DataPoint& getDataPoint( uint idx ) const
		{
			assert( idx < _data.size() );
			return _data[idx];
		}
		bool load( std::string fname, const Fparams& );
		void print( std::ostream& ) const;
		void print( std::ostream&, const std::vector<uint>& ) const;

		void clear() { _data.clear(); }
		std::pair<DataSet,DataSet> getFolds( uint i, uint nbFolds ) const;

	private:
		size_t _nbAttribs = 0;
		std::vector<DataPoint> _data;
//		std::vector<DataPoint<T>> _dataPoint;
};
//using DataSetf = DataSet<float>;
//using DataSetd = DataSet<double>;

//---------------------------------------------------------------------
std::pair<DataSet,DataSet>
DataSet::getFolds( uint i, uint nbFolds ) const
{
 	DataSet ds_train, ds_test;
// TODO
	return std::make_pair( ds_train, ds_test );
}

//---------------------------------------------------------------------
//template<typename T>
bool
DataSet::load( std::string fname, const Fparams& params )
{
	std::ifstream f( fname );
	if( !f.is_open() )
	{
		std::cerr << "Unable to open file " << fname << "\n";
		return false;
	}
	clear();

	std::map<std::string,uint> classMap;  // used only if classes are given as strings
	uint classIndexCounter = 0;

	std::map<uint,uint> classValues;
	size_t nb_lines     = 0;
	size_t nb_empty     = 0;
	size_t nb_comment   = 0;
	do
	{
		std::string temp;
		std::getline( f, temp );
		nb_lines++;

		if( temp.empty() )          // if empty
			nb_empty++;
		else                        // if NOT empty
		{
			if( temp.at(0) == '#' )  // if comment
				nb_comment++;
			else                     // if NOT comment
			{
				auto v_tok = priv::splitString( temp, params.sep );
				if( v_tok.size() < 2 )
				{
					std::cerr << "-Error: only one value on line " << nb_lines
						<< "\n-Line=" << temp << " \n-length=" << temp.size() << '\n';
					return false;
				}

				if( size() == 0 )                     // if this is the first datapoint, then set the nb of attributes
					setNbAttribs( v_tok.size()-1 );

				int classIndex = 0;
				auto cla = v_tok.back();
				if( !params.classAsString )
					classIndex = std::stoi( cla );
				else
				{
					auto it = classMap.find( cla );
					if( it == classMap.end() )  // not there
						classMap[ cla ] = classIndexCounter++; // new class
					else
						classIndex = classMap[ cla ];
				}
				classValues[classIndex]++;
//				v_tok.resize( v_tok.size()-1 );
				v_tok.erase( v_tok.end()-1 );   // remove last element (class)
				_data.push_back( DataPoint( v_tok, classIndex ) );
			}
		}
	}
	while( !f.eof() );

	#if 1
		std::cout << " - Read " << size() << " points in file " << fname;
		std::cout << "\n - file info:"
			<< "\n  - nb lines=" << nb_lines
			<< "\n  - nb empty=" << nb_empty
			<< "\n  - nb comment=" << nb_comment
			<< "\n  - nb classes=" << classValues.size()
			<< "\nClasses frequency:\n";
		for( const auto& cval: classValues )
			std::cout << cval.first << ": "
				<< cval.second
				<< " (" << 1. * cval.second/size()
				<< " %)\n";
	#endif


	return true;
}
//---------------------------------------------------------------------
//template<typename T>
void
DataSet::print( std::ostream& f ) const
{
	f << "# -------------------------------------------\n";
	f << "# Dataset, nb pts=" << size() << " nb attributes=" << nbAttribs() << "\n";
	for( size_t i=0; i<nbAttribs(); i++ )
		f << i << "; ";
	f << " class\n";
	for( const auto& pt: _data )
	{
		for( const auto& val: pt._attrValue )
			f << val << ";";

		f << pt.classVal() << "\n";
	}
	f << "# -------------------------------------------\n";
}
//---------------------------------------------------------------------
//template<typename T>
void
DataSet::print( std::ostream& f, const std::vector<uint>& vIdx ) const
{
	f << "# -------------------------------------------\n";
	f << "# Dataset, total nb pts=" << size()
		<< " requested=" << vIdx.size()
		<< " nb attributes=" << nbAttribs() << "\n";
	for( size_t i=0; i<nbAttribs(); i++ )
		f << i << "; ";
	f << " class\n";
	for( const auto& id: vIdx )
	{
		const auto& pt = getDataPoint( id );
		f << id << " ";
		for( const auto& val: pt._attrValue )
			f << val << ";";

		f << pt.classVal() << "\n";
	}
	f << "# -------------------------------------------\n";
}

//---------------------------------------------------------------------
/// Holds the node type
enum NodeType
{
	 NT_undef, NT_Root, NT_Decision, NT_Final
};

inline
std::string
getString( NodeType nt )
{
	const char* s = nullptr;
	switch( nt )
	{
		case NT_undef:    s="UNDEF";    break;
		case NT_Root:     s="Root";     break;
		case NT_Decision: s="Decision"; break;
		case NT_Final:    s="Final";    break;
		default: assert(0);
	}
	return std::string(s);
}
//---------------------------------------------------------------------
/// A node of the tree
/// \todo do some inheritance here.
struct NodeC
{
	NodeType _type = NT_undef;
	size_t   _attrIndex = 0;     ///< Attribute Index that this nodes classifies
	float    _threshold = 0.f;   ///< Threshold on the attribute value (only for decision nodes)
	int      _class = -1;        ///< (only for terminal nodes)
	uint     depth = 0;         ///< depth of the node in the tree
};


//---------------------------------------------------------------------
/// A node of the training tree
struct NodeT
{
	static uint s_Counter;
	uint TEMP_IDX = 0;
	NodeType _type = NT_undef;
	int      _class = -1;        ///< (only for terminal nodes)
	size_t   _attrIndex = 0;     ///< Attribute Index that this nodes classifies
	float    _threshold = 0.f;   ///< Threshold on the attribute value (only for decision nodes)
	uint     depth = 0;         ///< depth of the node in the tree
	float    giniImpurity = 0.f;

	std::vector<uint> v_Idx; ///< data point indexes
	friend std::ostream& operator << ( std::ostream& f, const NodeT& n )
	{
		f << "C=" << n._class
			<< "\nattr=" << n._attrIndex
			<< "\nthres=" << n._threshold
			<< "\ndepth=" << n.depth
			<< "\n#v=" << n.v_Idx.size()
			;
		return f;
	}
	NodeT() { TEMP_IDX = s_Counter; s_Counter++; }
	NodeT( const NodeT& n ) {  TEMP_IDX = s_Counter; s_Counter++; }
};

 /// Instanciation of static
 uint NodeT::s_Counter = 0;

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

/// Used for training
/**
\note IMPORTANT: we use list because when splitting the vector of indexes of points of a node,
we use the current node's vector as a reference (to avoid copying the whole vector).
BUT: if we where using \c vectS, adding new nodes may invalidate the current vertex descriptor.
Thus, we use \c listS
*/
using GraphT = boost::adjacency_list<
		boost::listS,  // boost::vecS,
		boost::listS,
		boost::directedS,
		NodeT,
		EdgeT
	>;

using vertexT_t = boost::graph_traits<GraphT>::vertex_descriptor;
using vertexC_t = boost::graph_traits<GraphC>::vertex_descriptor;

//---------------------------------------------------------------------
/// Classification performance (to be expanded)
struct Perf
{
	float errorRate = 0.f;
	friend std::ostream& operator << ( std::ostream& f, const Perf& p )
	{
		f << "Perf: errorRate=" << p.errorRate
			<< "\n";
		return f;
	}

};
//---------------------------------------------------------------------
/// This one holds edges that each have a vector holding the index of datapoints.
/// This is memory costly, but useless for classifying, so once it is trained, we can use the \ref DecisionTree class
/// \todo unify by templating the type of edges
//template<typename T>
class TrainingTree
{
	private:
		GraphT _graph;
		vertexT_t _initialVertex;
		size_t _maxDepth = 1;  ///< defined by training
	public:
		Perf train( const DataSet&, Params params=Params() );
		Perf classify( const DataSet& ) const;
		void printDot( std::ostream& ) const;
		void printDot( std::string fname ) const;
		void printInfo( std::ostream& ) const;
		size_t maxDepth() const { return _maxDepth; }
		size_t nbLeaves() const;
};

inline
size_t
TrainingTree::nbLeaves() const
{
	size_t c = 0;
	for(
		auto pit = boost::vertices( _graph );
		pit.first != pit.second;
		pit.first++
	)
		if( _graph[*pit.first]._type == NT_Final )
			c++;
	return c;
}

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
		size_t maxDepth()  const { return _maxDepth; }
};

//---------------------------------------------------------------------
/// Create a DT having a single decision point (root node) and 3 nodes
//template<typename T>
DecisionTree::DecisionTree()
{
}

// % % % % % % % % % % % % % %
namespace priv {
// % % % % % % % % % % % % % %

//---------------------------------------------------------------------
/// Recursive function, prints the current node
inline
void
printNodeChilds( std::ostream& f, vertexT_t v, const GraphT& graph )
{
//	START;
//	COUT << "nb out edges=" << boost::out_degree( v, graph ) << "\n";
	for( auto pit=boost::out_edges(v, graph); pit.first != pit.second; pit.first++ )
	{
		auto target = boost::target( *pit.first, graph );
		assert( graph[target]._type != NT_undef );

		f << graph[target].TEMP_IDX
			<< " [label=\"";
		if( graph[target]._type == NT_Decision )
			f << "attr=" << graph[target]._attrIndex
				<< " thres=" << graph[target]._threshold;
		else
			f << "class=" << graph[target]._class
				<< " GI=" << graph[target].giniImpurity;

		f << "\\ndepth=" << graph[target].depth
			<< " #=" << graph[target].v_Idx.size()
			<< "\"";
		switch( graph[target]._type )
		{
			case NT_Final:    f << ",color=red"; break;
			case NT_Decision: f << ",color=green"; break;
			default: assert(0);
		}
		f << "];\n";
		f << graph[v].TEMP_IDX << "->" << graph[target].TEMP_IDX  << ";\n";
		printNodeChilds( f, target, graph );
	}
}
// % % % % % % % % % % % % % %
} // namespace priv
// % % % % % % % % % % % % % %

//---------------------------------------------------------------------
/// Print a DOT file of the tree by calling the recursive function \ref printNodeChilds()
//template<typename T>
inline
void
TrainingTree::printDot( std::ostream& f ) const
{
	START;
	f << "digraph g {\nnode [shape=\"box\"];\n";
	f << _graph[_initialVertex].TEMP_IDX
		<< " [label=\""
		<< "attr="   << _graph[_initialVertex]._attrIndex
		<< " thres=" << _graph[_initialVertex]._threshold
		<< "\n#="    << _graph[_initialVertex].v_Idx.size()
		<< "\",color = blue];\n";
	priv::printNodeChilds( f, _initialVertex, _graph );
	f << "}\n";
}

inline
void
TrainingTree::printDot( std::string fname ) const
{
	std::ofstream f(fname);
	if( !f.is_open() )
		throw std::runtime_error( "unable to open file " + fname );
	printDot( f );
}

//---------------------------------------------------------------------
/// Print a DOT file of the tree
//template<typename T>
void
TrainingTree::printInfo( std::ostream& f ) const
{
	f << "Training tree info:"
		<< "\n -nb nodes=" << boost::num_vertices( _graph )
		<< "\n -nb edges=" << boost::num_edges( _graph )
		<< "\n -max depth=" << maxDepth()
		<< "\n -nb of leaves=" << nbLeaves()
		<< '\n';
//	f << "Boost printing:\n";
//	boost::print_graph( _graph );
}

//---------------------------------------------------------------------
/// Utility functions, returns a vector of indexes holding all the points
// % % % % % % % % % % % % % %
namespace priv {
// % % % % % % % % % % % % % %
std::vector<uint>
setAllDataPoints( const DataSet& dataset )
{
	std::vector<uint> v( dataset.size() );
	std::iota( v.begin(), v.end(), 0 );
	return v;
}
// % % % % % % % % % % % % % %
} // namespace priv
// % % % % % % % % % % % % % %

//---------------------------------------------------------------------
/// Computes the nb of votes for each class, for the points defined in \c v_Idx
std::map<uint,uint>
computeClassVotes( const std::vector<uint>& v_Idx, const DataSet&  data )
{
	START;
	assert( v_Idx.size()>0 );

	std::map<uint,uint> classVotes; // key: class index, value: votes
	for( auto idx: v_Idx )
	{
		const auto& dp = data.getDataPoint( idx );
		classVotes[ dp.classVal() ]++;
	}
	return classVotes;
}
//---------------------------------------------------------------------
/// Computes the Gini coefficient for points listed in \c vdpidx
double
getGlobalGiniCoeff(
	const std::vector<uint>& v_dpidx, ///< datapoint indexes to consider
	const DataSet&           data     ///< dataset
)
{
	START;
	auto classVotes = computeClassVotes( v_dpidx, data );

	double giniCoeff = 1.;
	for( auto elem: classVotes )
	{
		auto v = 1. * elem.second / v_dpidx.size();
		giniCoeff -= v*v;
	}
	COUT << "global Gini Coeff=" << giniCoeff << '\n';
	return giniCoeff;
}
//---------------------------------------------------------------------
/// Utility function, sort vector and removes values whose difference is small
/**
\todo Check if not faster to use a set ?

This compares all the values and removes those which are "too close".

Say we have the values: <code>4 - 5 - 6 - 6.1 - 7 - 8</code> <br>
We want to remove the value "6.1"

First we compute the range: highest - lowest.
Here, \f$ range = 8 - 4 = 4 \f$ <br>
We check the differences between two consecutive values: <br>
\f$ d = \left| v_i - v_{i+1} \right| \f$ <br>
If \f$ d < coeff * range \f$, then it will be considered as "too close".
*/
size_t
removeDuplicates( std::vector<float>& vec, const Params& params )
{
	auto mm = std::minmax_element( std::begin(vec), std::end(vec) )	;
	auto k = (*mm.second - *mm.first) * params.removalCoeff;

	COUT << "min=" << *mm.first
		<< " max=" << *mm.second
		<< " range=" << (*mm.second - *mm.first)
		<< " k=" << k << "\n";
	std::sort( vec.begin(), vec.end() );

// remove all values that are equal
	auto it_end = std::unique(
		std::begin(vec),
		std::end(vec),
		[k]                        // lambda
		( float v1, float v2 )
		{
			if( std::fabs(v1-v2) < k )
				return true;
			return false;
		}
	);
	size_t nb_removal = vec.end() - it_end;
	vec.erase( it_end, vec.end() );
	return nb_removal;
}

//---------------------------------------------------------------------
/// Simple wrapper around a map holding a bool for each attribute index.
/// Used to check if an attribute has been already used or not.
/**
 * Benefit: has automatic initialization
*/
struct AttribMap
{
	private:
		std::map<uint,bool> _attribMap;
	public:
		AttribMap( uint nbAttribs )
		{
			for( uint i=0; i<nbAttribs; i++ )
				_attribMap[i] = false;
		}
		std::vector<uint> getUnusedAttribs() const
		{
			std::vector<uint> vout;
			for( auto elem: _attribMap )
				if( elem.second == false )
					vout.push_back(elem.first);
			return vout;
		}

/// Set attribute \c idx as used, so we will not use it again
/// \todo maybe add some checking here...
		void setAsUsed( uint idx )
		{
			_attribMap[idx] = true;
		}

/// Returns number of unused attributes
		uint nbUnusedAttribs() const
		{
			uint c=0;
			for( const auto& elem: _attribMap )
				if( elem.second == false )
					c++;
			return c;
		}
};
//---------------------------------------------------------------------
/// Holds all the data relative to an attribute to be able to select it.
struct AttributeData
{
	uint         _atIndex = 0u;         ///< (absolute) attribute index
	float        _gain  = 0.f;          ///< information gain, will be used to select which attribute we use
	ThresholdVal _threshold;            ///< threshold value, will be used to classify
	uint         _nbPtsLessThan = 0u;   ///< Nb of points that are less than the threshold

	AttributeData()
	{}
	AttributeData( uint atIdx, float ig, ThresholdVal tval, uint nbpLT ) :
		_atIndex(atIdx),
		_gain(ig),
		_threshold(tval),
		_nbPtsLessThan(nbpLT)
	{}

	friend std::ostream&operator << ( std::ostream& f, const AttributeData& ad )
	{
		f << "AttributeData: index=" << ad._atIndex
			<< " gain=" << ad._gain
			<< " thres=" << ad._threshold
			<< " nbPointsLessThan=" << ad._nbPtsLessThan
			<< "\n";
		return f;
	}
};

//---------------------------------------------------------------------
/// Compute "Gini impurity" (Information Gain) of attribute \c atIdx of the subset of data given by \c v_dpidx.
/**
\return Returns a pair holding the IG value AND the threshold value for which it was produced

Details:
- Uses the Gini impurity coeff: https://en.wikipedia.org/wiki/Decision_tree_learning#Gini_impurity
- for details, see
 - https://en.wikipedia.org/wiki/Information_gain_in_decision_trees
 - https://towardsdatascience.com/under-the-hood-decision-tree-454f8581684e

\todo Add the number of values less than threshold in return value
(so we can decide to split or not to split local dataset without recounting them).
*/
//template<typename T>
AttributeData
computeIG(
	uint                     atIdx,     ///< attribute index we want to process
	const std::vector<uint>& v_dpidx,   ///< datapoint indexes to consider
	const DataSet&           data,      ///< dataset
	double                   giniCoeff, ///< Global Gini coeff for all the points
	const Params&            params     ///< run-time parameters
)
{
	START;
	COUT << "atIdx=" << atIdx << " nb pts=" << v_dpidx.size() << '\n';

// step 1 - compute all the potential threshold values (mean value between two consecutive attribute values)

	std::vector<float> v_attribVal( v_dpidx.size() ); // pre-allocate vector size (faster than push_back)
	for( size_t i=0; i<v_dpidx.size(); i++ )
		v_attribVal[i] = data.getDataPoint( v_dpidx[i] ).attribVal( atIdx );

	auto nbRemoval = removeDuplicates( v_attribVal, params );
	std::cout << "Removal of " << nbRemoval << " attribute values\n";

	if( v_attribVal.size() < 2 )         // if only one value, is pointless
	{
		std::cout << "WARNING, unable to compute best threshold value for attribute " << atIdx
			<< ", maybe check value of 'removalCoeff'\n";
		return AttributeData();
	}

	std::vector<float> v_thresVal( v_attribVal.size()-1 ); // if 10 values, then only 9 thresholds
	for( uint i=0; i<v_thresVal.size(); i++ )
		v_thresVal[i] = ( v_attribVal.at(i) + v_attribVal.at(i+1) ) / 2.f; // threshold is mean value between the 2 attribute values

// step 2: compute IG for each threshold value

	std::vector<float> deltaGini( v_thresVal.size() );
	std::vector<uint> nb_LT( v_thresVal.size(), 0u );
	for( uint i=0; i<v_thresVal.size(); i++ )              // for each threshold value
	{
//		COUT << "thres " << i << "=" << v_thresVal[i] << '\n';
		std::map<uint,uint> m_LT, m_HT;

		uint nb_HT = 0;
		for( auto ptIdx: v_dpidx )                         // for each data point
		{
			const auto& point = data.getDataPoint(ptIdx);
			auto attribVal = point.attribVal( atIdx );
			if( attribVal < v_thresVal[i] )
			{
				m_LT[ point.classVal() ]++;
				nb_LT[i]++;
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
			auto val = 1. * p.second / nb_LT[i];
			g_LT -= val*val;
		}
		auto g_HT = 1.;

		for( auto p: m_HT )  // for the values that are Higher Than the threshold
		{
			auto val = 1. * p.second / nb_HT;
			g_HT -= val*val;
		}
		deltaGini[i] = giniCoeff - (g_LT + g_HT) / 2.;
	}

// step 3 - find max value of the delta Gini
	auto max_pos = std::max_element( std::begin( deltaGini ), std::end( deltaGini ) );

	COUT << "max gini for thres idx=" << std::distance( std::begin( deltaGini ), max_pos ) << " val=" << *max_pos
		<< " thresval=" << v_thresVal.at( std::distance( std::begin( deltaGini ), max_pos ) ) << "\n";

	auto best_thres_idx = std::distance( std::begin( deltaGini ), max_pos );

	return AttributeData(
		atIdx,
		*max_pos,
		ThresholdVal(v_thresVal.at( best_thres_idx ) ),
		nb_LT.at( best_thres_idx )
	);
}
//---------------------------------------------------------------------
/// Finds the best attributes to use, considering the data points of the current node
/// and compute threshold on that attribute so that the two classes are separated at best.
/**
\return A pair holding 1-the index of this attribute and 2-the corresponding threshold value
\return AttributeData
*/
//template<typename T>
AttributeData
findBestAttribute(
	const std::vector<uint>& vIdx,   ///< indexes of data points we need to consider
	const DataSet&           data,   ///< whole dataset
//	AttribMap&               aMap,   ///< map of the attributes that "may" be left to explore
	const Params&            params  ///< parameters
)
{
	START;

	auto giniCoeff = getGlobalGiniCoeff( vIdx, data );

//	COUT << "nb of attributes left=" << aMap.nbUnusedAttribs() << '\n';

// step 1 - fetch the indexes of the attributes we need to explore
#if 0
	std::vector<uint> v_attrIdx = aMap.getUnusedAttribs();
	assert( v_attrIdx.size() );   // if not, well... shouldn't happen !

	priv::printVector( std::cout, v_attrIdx, "attributes left" );
#endif

// step 2 - compute best IG/threshold for each of these attributes, only for the considered points

	std::vector<AttributeData> v_IG;
//	for( auto atIdx: v_attrIdx )
	for( uint atIdx=0; atIdx<data.nbAttribs(); atIdx++ )
		v_IG.push_back( computeIG( atIdx, vIdx, data, giniCoeff, params ) );

//	priv::printVector( std::cout, v_IG, "v_IG" );
/*	COUT << "vector v_IG, #=" << v_IG.size() << "\n";
	for( const auto& elem : v_IG )
		std::cout << elem.first << "-" << elem.second << "\n"; */

// step 3 - get the one with max gain value
	auto it_mval = std::max_element(
		std::begin(v_IG),
		std::end(v_IG),
		[]                         // lambda
		( const AttributeData& p1, const AttributeData& p2 )
		{
			return p1._gain < p2._gain;
		}
	);
	COUT << "highest Gain:" << *it_mval << "\n";

	return *it_mval;
}
//---------------------------------------------------------------------
/// Returns the class that is in majority in the points defined in \c vIdx.
/// The second value is the percentage of that majority, in the range \f$ [0,1]\f$  (well, \f$ ]0.5,1]\f$  actually !)
//template<typename T>
std::pair<int,float>
getMajorityClass( const std::vector<uint>& vIdx, const DataSet& data )
{
	START;
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
/// Recursive helper function, used by TrainingTree::train()
/**
Computes the threshold, splits the dataset and assigns the split to 2 sub nodes (that get created)
*/
////template<typename T>
void
splitNode(
	vertexT_t         v,         ///< current node id
//	AttribMap&        aMap,      ///< attribute map, holds true for all the attributes already used
	GraphT&           graph,     ///< graph
	const DataSet&    data,      ///< dataset
	const Params&     params     ///< parameters
)
{
	START;
	static int s_recDepth;
	s_recDepth++;
	if( s_recDepth>10){ COUT << "TOO MUCH DEPTH!!!\n"; std::exit(1);}

	COUT << "RECDEPTH="<< s_recDepth << "\n";
	const auto& vIdx = graph[v].v_Idx; // vector holding the indexes of the datapoints for this node
	COUT << "set holds " << vIdx.size() << " points\n";
//	data.print( std::cout, vIdx );

// step 1.1 - check if there are different output classes in the given data points
// if not, then we are done

	auto giniCoeff = getGlobalGiniCoeff( vIdx, data );
	auto majo = getMajorityClass( vIdx, data );

	graph[v]._class = majo.first;
	graph[v].giniImpurity = giniCoeff;
	graph[v]._type = NT_Final;

	if( giniCoeff < params.minGiniCoeffForSplitting )
	{
		LOG << "dataset is (almost or completely) pure, gini coeff=" << giniCoeff << ", STOP\n";
		s_recDepth--;
		return;
	}

#if 0
// step 1.2 - check if there are some remaining attributes to use
// if not, then we are done
	if( aMap.nbUnusedAttribs() == 0 )
	{
		LOG << "all attributes are used, STOP\n";
		s_recDepth--;
		return;
	}
#endif

// step 2 - find the best attribute to use to split the data, considering the data points of the current node
//	auto bestAttrib = findBestAttribute( vIdx, data, aMap, params );
	auto bestAttrib = findBestAttribute( vIdx, data, params );
	auto attribIdx  = bestAttrib._atIndex;
	auto threshold  = bestAttrib._threshold;
	COUT << "best attrib:" << bestAttrib << "\n";

//	aMap.setAsUsed(attribIdx); // so we will not use it again

// before splitting, make sure that one of the childs will not have an insufficient number of points
	auto n1= bestAttrib._nbPtsLessThan;
	auto n2 = vIdx.size() - n1;
	if( n1 < params.minNbPoints || n2 < params.minNbPoints )
	{
		LOG << "not enough points if splitting: n1=" << n1 << " n2=" << n2 << ", STOP\n";
		s_recDepth--;
		return;
	}
//
// !!! from here, a split will occur !!!
//
	graph[v]._attrIndex = attribIdx;
	graph[v]._threshold = threshold.get();
	graph[v].giniImpurity = -1.f;
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

	graph[v1].v_Idx.reserve( vIdx.size() );
	graph[v2].v_Idx.reserve( vIdx.size() );
	for( auto idx: vIdx )           // separate the data points into two sets
	{
		auto attrVal = data.getDataPoint( idx ).attribVal( attribIdx );
		if( attrVal < threshold.get() )
			graph[v1].v_Idx.push_back( idx );
		else
			graph[v2].v_Idx.push_back( idx );
	}
	COUT << "after split: v1:"<< graph[v1].v_Idx.size() << " points, v2:"<< graph[v2].v_Idx.size() << " points\n";

	if( graph[v1].v_Idx.size() )
		splitNode( v1, graph, data, params );

	if( graph[v2].v_Idx.size() )
		splitNode( v2, graph, data, params );

	COUT << "Return\n";
	s_recDepth--;
}
//---------------------------------------------------------------------
/// Train tree using data.
/**
\return false if failure

\todo proceed to measure classification error
*/
//template<typename T>
Perf
TrainingTree::train( const DataSet& data, const Params params )
{
	START;
	auto nbAttribs = data.nbAttribs();
	if( !nbAttribs )
		throw std::runtime_error( "no attributes!" );
	if( data.size()<2 )
		throw std::runtime_error( "no enough data points!" );

// step 3 - Create initial node, and assign to it the whole dataset

	auto n0  = boost::add_vertex(_graph);
	_initialVertex = n0;
	std::vector<uint> v_idx( data.size() );
	std::iota( v_idx.begin(), v_idx.end(), 0 );
//	priv::printVector( std::cout, v_idx, "initial set of pt indexes" );

	_graph[n0].v_Idx = v_idx;
	_graph[n0]._type = NT_Root;

//	AttribMap attribMap(nbAttribs);

// Call the "split" function (recursive)
//	splitNode( n0, attribMap, _graph, data, params );
	splitNode( n0, _graph, data, params );


	return Perf();
}

//---------------------------------------------------------------------
Perf
TrainingTree::classify( const DataSet& dataset ) const
{
	return Perf();
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

