/**
\file
\brief Naive implementation attempt of a classifier using a Decision Tree
for continuous data values (aka real numbers)
\author S. Kramm - 2021

- home: https://github.com/skramm/dtcpp
- multiclass
- Limited to binary classification (a tree node has only two childs)
- input datasets:
    - csv style
    - field separator can be defined, see Fparams
    - class field MUST be the last one
    - number of attributes set automatically
    - classes may be integer values or string values, see Fparams
- Does not handle missing values
- using boost::graph to model the tree
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <random>

#include <boost/graph/adjacency_list.hpp>
#include "boost/graph/graphviz.hpp"
#include <boost/graph/graph_utility.hpp> // needed only for print_graph();

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
	bool operator == ( const NamedType& nt2 ) const
	{
		return get() == nt2.get();
	}
	bool operator < ( const NamedType& nt2 ) const
	{
		return get() < nt2.get();
	}
	bool operator != ( const NamedType& nt2 ) const
	{
		return !( *this == nt2 );
	}

	friend std::ostream& operator << ( std::ostream& f, const NamedType& nt )
	{
		f << nt.value_;
		return f;
	}
private:
    T value_;
};

using ThresholdVal = NamedType<float,struct ThresholdValTag>;
using ClassVal     = NamedType<int,  struct ClassValTag>;

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
/// Run-time parameters for training
struct Params
{
	float minGiniCoeffForSplitting = 0.05f;
	uint  minNbPoints = 3;                   ///< minumum nb of points to create a node
	float removalCoeff = 0.05f;  ///< used to remove close attribute values when searching the best threshold. See removeDuplicates()
	bool  verbose = true;        ///< to allow logging of some run-time details
	bool  doFolding = false;
	uint  maxTreeDepth = 10;
//	uint  initialVertexId = 1u;
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
		std::vector<float> _attrValue;   ///< attributes
		ClassVal _class = ClassVal(-1);  ///< Class of the datapoint, -1 for undefined

	public:
#ifdef TESTMODE
/// Constructor used in tests
		DataPoint( const std::vector<float>& vec, int c ) :
			_attrValue(vec), _class(ClassVal(c))
		{}
#endif
/// Constructor from a vector of strings (used by file reader)
		DataPoint( const std::vector<std::string>& v_string, ClassVal c )
		{
			assert( v_string.size() > 0 );              // at least one attribute and a class value

			for( size_t i=0; i<v_string.size(); i++ )
				_attrValue.push_back( std::stof( v_string[i] ) );
			_class = c;
		}

		size_t nbAttribs() const
		{
			return _attrValue.size();
		}
		ClassVal classVal() const { return _class; }
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

		friend std::ostream& operator << ( std::ostream& f, const DataPoint& pt )
		{
			f << "Datapoint: ";
			for( const auto& v: pt._attrValue )
				f << v << '-';
			f << "C=" << pt._class.get() << ' ';
			return f;
		}
};

//---------------------------------------------------------------------
/// Parameters for data files
struct Fparams
{
	char sep = ' ';              ///< input field separator
	bool classAsString = false;  ///< class values are given as strings
};

//---------------------------------------------------------------------
/// Stats for a single attribute
template<typename T>
struct AttribStats
{
	T _minVal;
	T _maxVal;
	T _meanVal;
	T _stddevVal;
	T _medianVal;

	friend std::ostream& operator << ( std::ostream& f, const AttribStats& st )
	{
		f << "min="      << st._minVal
			<< " max="    << st._maxVal
			<< " range="  << st._maxVal - st._minVal
			<< " mean="   << st._meanVal
			<< " stddev=" << st._stddevVal
			<< " median=" << st._medianVal
			<< ' ';
		return f;
	}
};
//---------------------------------------------------------------------
/// Holds attribute stats, see DataSet::computeStats()
template<typename T>
struct DatasetStats
{
	std::vector<AttribStats<T>> v_stats;
	friend std::ostream& operator << ( std::ostream& f, const DatasetStats& st )
	{
		f << "DatasetStats: " << st.v_stats.size() << " attributes:"; // << st.nbClasses() << '\n';
		for( uint i=0; i<st.v_stats.size(); i++ )
			f << "\n -attribute " << i << ": " << st.v_stats[i];
		f << '\n';
		return f;
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

		std::vector<DataPoint>::const_iterator
		begin() const
		{
			return _data.begin();
		}
		std::vector<DataPoint>::const_iterator
		end() const
		{
			return _data.end();
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
		bool load( std::string fname, const Fparams=Fparams() );
		void print( std::ostream& ) const;
		void print( std::ostream&, const std::vector<uint>& ) const;
		void printInfo( std::ostream& ) const;

		void clear() { _data.clear(); }
		std::pair<DataSet,DataSet> getFolds( uint i, uint nbFolds ) const;

/// Shuffle the data (taken from https://stackoverflow.com/a/6926473/193789)
		void shuffle()
		{
			std::shuffle(std::begin(_data), std::end(_data), std::random_device() );
		}
		template<typename T>
		DatasetStats<T> computeStats() const;
		uint nbClasses() const;

	private:
		size_t _nbAttribs = 0;
		std::vector<DataPoint> _data;
//		uint _nbClasses;
//		std::vector<DataPoint<T>> _dataPoint;
};
//using DataSetf = DataSet<float>;
//using DataSetd = DataSet<double>;

//---------------------------------------------------------------------
uint
DataSet::nbClasses() const
{
	std::set<ClassVal> clset;
	for( const auto& point: _data )
		clset.insert( point.classVal() );

	return clset.size();
}

//---------------------------------------------------------------------
/// Compute statistics of the dataset, attributes by attribute.
/**
Done by storing for a given attribute all the values in a vector, then computing stats on that vector

- median: https://stackoverflow.com/a/42791986/193789
- stddev: https://stackoverflow.com/a/7616783/193789

*/
template<typename T>
DatasetStats<T>
DataSet::computeStats() const
{
	DatasetStats<T> dstats;
	for( uint i=0; i<nbAttribs(); i++ )
	{
		std::vector<float> vat;
		vat.reserve( size() );               // guarantees we won't have any reallocating
		for( const auto& point: _data )
			vat.push_back( point.attribVal(i) );

		auto it_mm = std::minmax_element( vat.begin(), vat.end() );
		AttribStats<T> pt_stat { *it_mm.first, *it_mm.second };

		auto sum = std::accumulate( vat.begin(), vat.end(), 0. );
		auto mean = sum / size();
		pt_stat._meanVal = mean;

		std::vector<double> diff( size() );
		std::transform( vat.begin(), vat.end(), diff.begin(), [mean](double x) { return x - mean; });

		auto sq_sum = std::inner_product( diff.begin(), diff.end(), diff.begin(), 0. );

		pt_stat._stddevVal = std::sqrt( sq_sum / size() );

		if( size() % 2 == 0)  // if even
		{
			const auto median_it1 = vat.begin() + vat.size() / 2 - 1;
			const auto median_it2 = vat.begin() + vat.size() / 2;

			std::nth_element( vat.begin(), median_it1 , vat.end() );
			const auto e1 = *median_it1;

			std::nth_element( vat.begin(), median_it2 , vat.end() );
			const auto e2 = *median_it2;

			pt_stat._medianVal = (e1 + e2) / 2;

		}
		else                // if odd
		{
			const auto median_it = vat.begin() + vat.size() / 2;
			std::nth_element( vat.begin(), median_it , vat.end() );
			pt_stat._medianVal = *median_it;
		}
		dstats.v_stats.push_back( pt_stat );
	}
	return dstats;
}
//---------------------------------------------------------------------
/// Returns sub-datasets
/**
If 100 pts and nbFolds=5, this will return 20 pts in \c ds_test and 80 pts in \c ds_train
*/
std::pair<DataSet,DataSet>
DataSet::getFolds( uint index, uint nbFolds ) const
{
 	DataSet ds_train( nbAttribs() );
 	DataSet ds_test(  nbAttribs() );
	uint nb = size() / nbFolds;
	for( uint i=0; i<size(); i++ )
	{
		if( i / nb == index )
		{
//			COUT << "adding pt " << i << " to test\n";
			ds_test.addDataPoint( getDataPoint(i) );
		}
		else
		{
//			COUT << "adding pt " << i << " to train\n";
			ds_train.addDataPoint( getDataPoint(i) );
		}
	}

	COUT << "ds_test #=" << ds_test.size()
		<< " ds_train #=" << ds_train.size() << "\n";

	return std::make_pair( ds_train, ds_test );
}

//---------------------------------------------------------------------
//template<typename T>
bool
DataSet::load( std::string fname, const Fparams params )
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
				_data.push_back( DataPoint( v_tok, ClassVal(classIndex) ) );
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
			<< "\n  - nb classes 2=" << nbClasses()
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
void
DataSet::printInfo( std::ostream& f ) const
{
	f << "Dataset:\n # points=" << size()
		<< "\n # attributes="   << nbAttribs()
		<< "\n # classes="      << nbClasses()
		<< '\n';
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
		f << pt;
/*		for( const auto& val: pt._attrValue )
			f << val << ";";

		f << pt.classVal() << "\n";
*/
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
	ClassVal _class = ClassVal(-1);        ///< (only for terminal nodes)
	uint     depth = 0;         ///< depth of the node in the tree
};


//---------------------------------------------------------------------
/// A node of the training tree
struct NodeT
{
	static uint s_Counter;
	uint     _nodeId = 0;            ///< Id of the node. Needed to print the dot file. \todo could be removed if graph switches to \c VecS
	NodeType _type = NT_undef;       ///< Type of the node (Root, leaf, or decision)
	ClassVal _class = ClassVal(-1);  ///< Class (only for terminal nodes)
	size_t   _attrIndex = 0;         ///< Attribute Index that this nodes classifies
	float    _threshold = 0.f;       ///< Threshold on the attribute value (only for decision nodes)
	uint     depth = 0;              ///< Depth of the node in the tree
	float    giniImpurity = 0.f;
	std::vector<uint> v_Idx;         ///< Data point indexes

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
	static void resetNodeId()
	{
		s_Counter = 0;
	}
	NodeT() { _nodeId = s_Counter++; }
	NodeT( const NodeT& n ) {  _nodeId = s_Counter++; }
};

 /// Instanciation of static
 uint NodeT::s_Counter = 0;

//---------------------------------------------------------------------
/// Edge of the tree. Value is true/false of the above decision, depending on threshold
struct EdgeData
{
	bool edgeSide;
};

//---------------------------------------------------------------------
#if 0
/// used for classifying
using GraphC = boost::adjacency_list<
		boost::vecS,
		boost::vecS,
		boost::directedS,
		NodeC,
		EdgeData
	>;
#endif
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
		EdgeData
	>;

using vertexT_t = boost::graph_traits<GraphT>::vertex_descriptor;
//using vertexC_t = boost::graph_traits<GraphC>::vertex_descriptor;

//---------------------------------------------------------------------
/// Classification performance (to be expanded)
struct Perf
{
	Perf( float err ): errorRate(err) {}

	float errorRate = 0.f;
	friend std::ostream& operator << ( std::ostream& f, const Perf& p )
	{
		f << "Perf: errorRate=" << p.errorRate
			<< "\n";
		return f;
	}
};

//---------------------------------------------------------------------
class ConfusionMatrix;
// % % % % % % % % % % % % % %
namespace priv {
// % % % % % % % % % % % % % %
/// Private class, used to hold the counters extracted from the ConfusionMatrix,
/// see ConfusionMatrix::p_score()
class CM_Counters
{
	friend class ConfusionMatrix;

	CM_Counters( double TP, double FP, double TN, double FN )
		: tp(TP),fp(FP),tn(TN), fn(FN)
	{
//		nbValues = tp + fp + tn + fn;
//		assert( nbValues > 0 );
		assert( tp + fp + tn + fn > 0 );
	}
	double tp,fp,tn,fn;
//	size_t nbValues;
};
// % % % % % % % % % % % % % %
} // namespace priv
// % % % % % % % % % % % % % %

//---------------------------------------------------------------------
/// Performance score of classification, see ConfusionMatrix for definitions
enum CM_Score
{
    CM_TPR      ///< True Positive Rate
    ,CM_TNR     ///< True Negative Rate
    ,CM_ACC     ///< Accuracy
    ,CM_BACC    ///< Balanced Accuracy
};
//---------------------------------------------------------------------
/// Confusion Matrix,
/// handles both 2 class and multiclass problems, but usage will be different
/**
Layout:
- columns: true class
- lignes: predicted (classified) class

Definitions:
- For 2- class problems, follows definitions from https://en.wikipedia.org/wiki/Confusion_matrix
- For multiclass, see definitions here: https://stats.stackexchange.com/a/338240/23990

Usage:

For 2-class problem, you get (for example) the "True Positive Rate" metric like this:
\code
    auto score = cmat.getScore( CM_TPR );
\endcode

For multiclass situations, you need to add for what class you are requesting this.
\code
    auto score = cmat.getScore( CM_TPR, ClassValue );
\endcode
*/
struct ConfusionMatrix
{
	ConfusionMatrix( size_t nbClasses )
	{
		assert( nbClasses>1 );
		_mat.resize( nbClasses );
		for( auto& li: _mat )
		{
			li.resize( nbClasses );
			std::fill( li.begin(), li.end(), 0u );
		}
	}

#ifdef TESTMODE
	ConfusionMatrix( const std::vector<std::vector<uint>>& m )
	{
		_mat = m;
	}
#endif //  TESTMODE

	void clear()
	{
		for( auto& li: _mat )
			std::fill( li.begin(), li.end(), 0u );
	}
	size_t nbClasses() const
	{
        return _mat.size();
	}
    double getScore( CM_Score, ClassVal ) const;
    double getScore( CM_Score ) const;
	size_t nbValues() const
	{
        size_t sum = 0u;
		for( const auto& li: _mat )
            sum += std::accumulate( li.begin(), li.end(), 0u );
        return sum;
	}
	void add( ClassVal trueVal, ClassVal predictedVal )
	{
		assert( trueVal.get() >= 0 );
		assert( predictedVal.get() >=0 );

		auto col = static_cast<size_t>( trueVal.get() );
		auto li  = static_cast<size_t>( predictedVal.get() );

		assert( li < _mat.size() && col < _mat.size() );
		_mat[li][col]++;
	}
	friend std::ostream& operator << ( std::ostream& f, const ConfusionMatrix& cm )
	{
		f << "ConfusionMatrix:\n    ";
		for( size_t i=0; i<cm._mat.size(); i++ )
			f << i+1 << "  ";
		f << "\n";

		for( size_t i=0; i<cm._mat.size(); i++ )
		{
			f << i+1 << " | ";
			for( const auto& elem: cm._mat[i] )
				f << elem << " - ";
			f << "|\n";
		}
		return f;
	}
	private:
		double p_score( CM_Score scoreId, priv::CM_Counters ) const;
	private:
		std::vector<std::vector<uint>> _mat;
};

//---------------------------------------------------------------------
/// Private, compute scores for both 2-class and multi-class confusion matrices
double
ConfusionMatrix::p_score( CM_Score scoreId, priv::CM_Counters cmc ) const
{
	auto TPR = cmc.tp / ( cmc.tp + cmc.fn );
	auto TNR = cmc.tn / ( cmc.tn + cmc.fp );
    switch( scoreId )
    {
        case CM_TPR:  scoreVal =  TPR; break;
        case CM__TNR: scoreVal =  TNR; break;
        case CM_ACC:  scoreVal = (cmc.tp + cmc.tn)/nbValues(); break;
        case CM_BACC: scoreVal = (TPR + TNR ) / 2.; break;
        default: assert(0);
    }
    return scoreVal;
}
//---------------------------------------------------------------------
/// Used for 2-class situations
double
ConfusionMatrix::getScore( CM_Score scoreId ) const
{
    assert( nbValues() > 2 );
    assert( nbClasses() == 2 );

    const auto& TP = _mat[0][0];
    const auto& FP = _mat[0][1];
    const auto& FN = _mat[1][0];
    const auto& TN = _mat[1][1];

    return p_score( scoreId, priv::CM_Counters(TP,FP,TN,FN) );
}
//---------------------------------------------------------------------
/// Used for multi-class situations
double
ConfusionMatrix::getScore( CM_Score scoreId, ClassVal cval ) const
{
    assert( nbValues() > 2 );
    assert( nbClasses() > 2 );

	assert( cval.get() >= 0 );
	assert( cval.get() < nbClasses() );
	size_t c = static_cast<size_t>( cval.get() );

    const auto& TP = _mat[c][c];

    const auto FP = std::accumulate( std::begin(_mat[c]), std::end(_mat[c]), 0. ) - TP;

    auto FN = 0u;
    for( size_t li=0; li<_mat.size(); li++ )
		if( li != c )
			FN += _mat[li][c];

    const auto TN = nbValues() - TP - FN - FP;

    return p_score( scoreId, priv::CM_Counters(TP,FP,TN,FN) );
}
//---------------------------------------------------------------------
/// This one holds edges that each have a vector holding the index of datapoints.
/// This is memory costly, but useless for classifying, so once it is trained, we can use the \ref DecisionTree class
/// \todo unify by templating the type of edges
//template<typename T>
class TrainingTree
{
	private:
		GraphT    _graph;
		vertexT_t _initialVertex;
		size_t    _maxDepth = 1;  ///< defined by training
	public:
		void clear()
		{
			_graph.clear();
		}

		void train( const DataSet&, Params params=Params() );
		Perf classify( const DataSet& ) const;
		ClassVal classify( const DataPoint& ) const;
		void printDot( std::ostream& ) const;
		void printDot( std::string fname ) const;
		void printInfo( std::ostream& ) const;
		size_t maxDepth() const { return _maxDepth; }
		size_t nbLeaves() const;
};

//---------------------------------------------------------------------
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


// % % % % % % % % % % % % % %
namespace priv {
// % % % % % % % % % % % % % %

//---------------------------------------------------------------------
/// Recursive function used to print the Dot file, prints the current node
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

		f << graph[target]._nodeId
			<< " [label=\"" << graph[target]._nodeId << '-';
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
		f << graph[v]._nodeId << "->" << graph[target]._nodeId  << ";\n";
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
	f << _graph[_initialVertex]._nodeId
		<< " [label=\"" << _graph[_initialVertex]._nodeId
		<< "-attr="     << _graph[_initialVertex]._attrIndex
		<< " thres="    << _graph[_initialVertex]._threshold
		<< "\\n#="      << _graph[_initialVertex].v_Idx.size()
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
/// Computes the nb of votes for each class, for the points defined in \c v_Idx
std::map<ClassVal,uint>
computeClassVotes( const std::vector<uint>& v_Idx, const DataSet& data )
{
	START;
	assert( v_Idx.size()>0 );

	std::map<ClassVal,uint> classVotes; // key: class index, value: votes
	for( auto idx: v_Idx )
	{
		const auto& dp = data.getDataPoint( idx );
		classVotes[ dp.classVal() ]++;
	}
	return classVotes;
}

//---------------------------------------------------------------------
/// Computes the Gini coefficient for points listed in \c vdpidx
/**
Returns a pair holding as first:the Gini Impurity, second: the class votes
*/
std::pair<double,std::map<ClassVal,uint>>
getGiniImpurity(
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
//	COUT << "global Gini Coeff=" << giniCoeff << '\n';

	return std::make_pair(
		giniCoeff,
		classVotes
	);
}
//---------------------------------------------------------------------
#if 0
/// Returns the class that is in majority in the points defined in \c vIdx.
/// The second value is the percentage of that majority, in the range \f$ [0,1]\f$  (well, \f$ ]0.5,1]\f$  actually !)
//template<typename T>
std::pair<ClassVal,float>
getMajorityClass( const std::vector<uint>& vIdx, const DataSet& data )
{
	START;
	using Pair = std::pair<ClassVal,uint>;

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
#endif

//---------------------------------------------------------------------
/// Describes how a node holds different classes, see getNodeContent()
struct NodeContent
{
	double   GiniImpurity = 0.;
	ClassVal dominantClass;
	size_t   datasize = 0u;
	size_t   nbPtsOtherClasses = 0u;
	size_t   nbClasses = 0u;

	friend std::ostream& operator << ( std::ostream& f, const NodeContent& nc )
	{
		f << "NodeContent: "
		<< " GiniImpurity="      << nc.GiniImpurity
		<< " dominantClass="     << nc.dominantClass
		<< " datasize="          << nc.datasize
		<< " nbPtsOtherClasses=" << nc.nbPtsOtherClasses
		<< " nbClasses="         << nc.nbClasses
		<< '\n';
		return f;
	}
};

//---------------------------------------------------------------------
/// Returns some info on what a node holding the points defined by \c v_dpidx holds.
/**
\todo Here, we iterate twice on the set, that could probably be done with a single iteration.
*/
NodeContent
getNodeContent(
	const std::vector<uint>& v_dpidx, ///< datapoint indexes to consider
	const DataSet&           data     ///< dataset
)
{
	START;
	auto gImp = getGiniImpurity( v_dpidx, data );

	const auto& classVotes = gImp.second;

//	COUT << "global Gini Impurity=" << gImp.first << '\n';

	using Pair = std::pair<ClassVal,uint>;
	auto it_max = std::max_element(  // search max value based on nb of votes
		std::begin( classVotes ),
		std::end( classVotes ),
		[]                                      // lambda
		(const Pair& a, const Pair& b)->bool
		{ return a.second < b.second; }
	);

	auto idx_maj = it_max->first;

	return NodeContent{
		gImp.first,
		idx_maj,
		v_dpidx.size(),
		v_dpidx.size() - classVotes.at(idx_maj),
		classVotes.size()
	};
}


//---------------------------------------------------------------------
/// Utility function, sort vector and removes values whose difference is small
/**
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
/// Holds all the data relative to an attribute to be able to select it.
struct AttributeData
{
	uint         _atIndex = 0u;         ///< Absolute attribute index
	float        _gain  = 0.f;          ///< Information gain, will be used to select which attribute we use
	ThresholdVal _threshold;            ///< Threshold value, will be set by training and used to classify
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
			<< ' ';
		return f;
	}
};

//---------------------------------------------------------------------
/// Compute best threshold for attribute \c atIdx, using the Gini Impurity, for the subset of data given by \c v_dpidx.
/**
\return Returns an object of type AttributeData

Details:
- Uses the Gini impurity coeff: https://en.wikipedia.org/wiki/Decision_tree_learning#Gini_impurity
- for details, see
 - https://en.wikipedia.org/wiki/Information_gain_in_decision_trees
 - https://towardsdatascience.com/under-the-hood-decision-tree-454f8581684e
*/
//template<typename T>
AttributeData
computeBestThreshold(
	uint                     atIdx,     ///< attribute index we want to process
	const std::vector<uint>& v_dpidx,   ///< datapoint indexes to consider
	const DataSet&           data,      ///< dataset
	double                   giniCoeff, ///< Global Gini coeff for all the points
	const Params&            params     ///< run-time parameters
)
{
	START;
//	COUT << "atIdx=" << atIdx << " nb pts=" << v_dpidx.size() << '\n';

// step 1 - compute all the potential threshold values (mean value between two consecutive attribute values)

	std::vector<float> v_attribVal( v_dpidx.size() ); // pre-allocate vector size (faster than push_back)
	for( size_t i=0; i<v_dpidx.size(); i++ )
		v_attribVal[i] = data.getDataPoint( v_dpidx[i] ).attribVal( atIdx );

	auto nbRemoval = removeDuplicates( v_attribVal, params );
//	std::cout << "Removal of " << nbRemoval << " attribute values\n";

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
		std::map<ClassVal,uint> m_LT, m_HT;

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

//	COUT << "max gini for thres idx=" << std::distance( std::begin( deltaGini ), max_pos ) << " val=" << *max_pos
//		<< " thresval=" << v_thresVal.at( std::distance( std::begin( deltaGini ), max_pos ) ) << "\n";

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
	const Params&            params  ///< parameters
)
{
	START;
	auto giniCoeff = getGiniImpurity( vIdx, data );

// step 1 - compute best IG/threshold for each attribute, only for the considered points
	std::vector<AttributeData> v_IG;
	for( uint atIdx=0; atIdx<data.nbAttribs(); atIdx++ )
		v_IG.push_back( computeBestThreshold( atIdx, vIdx, data, giniCoeff.first, params ) );

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
/// Recursive helper function, used by TrainingTree::train()
/**
Computes the threshold, splits the dataset and assigns the split to 2 sub nodes (that get created)
*/
////template<typename T>
void
splitNode(
	vertexT_t         v,         ///< current node id
	GraphT&           graph,     ///< graph
	const DataSet&    data,      ///< dataset
	const Params&     params     ///< parameters
)
{
	START;
	static uint s_recDepth;
	s_recDepth++;

//	static uint s_nodeId = params.initialVertexId;   // starts at one because the initial node (id=0) is created elsewhere

	const auto& vIdx = graph[v].v_Idx; // vector holding the indexes of the datapoints for this node

// step 1.1 - check if there are different output classes in the given data points
// if not, then we are done

	auto nodeContent = getNodeContent( vIdx, data );
	COUT << nodeContent;

	auto giniImpurity = nodeContent.GiniImpurity;

	graph[v]._class = nodeContent.dominantClass;
	graph[v].giniImpurity = giniImpurity;
	graph[v]._type = NT_Final;

	if( s_recDepth>params.maxTreeDepth )
	{
		LOG << "tree reached max depth (=" << params.maxTreeDepth << "), STOP\n";
		s_recDepth--;
		return;
	}

	if( giniImpurity < params.minGiniCoeffForSplitting )
	{
		LOG << "dataset is (almost or completely) pure, gini coeff=" << giniImpurity << ", STOP\n";
		s_recDepth--;
		return;
	}

// step 2 - find the best attribute to use to split the data, considering the data points of the current node
	auto bestAttrib = findBestAttribute( vIdx, data, params );
	auto attribIdx  = bestAttrib._atIndex;
	auto threshold  = bestAttrib._threshold;
	COUT << "best attrib:" << bestAttrib << "\n";


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
	graph[et.first].edgeSide = true;
	graph[ef.first].edgeSide = false;

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

	s_recDepth--;
}
//---------------------------------------------------------------------
/// Train tree using data.
/**
\return false if failure

*/
//template<typename T>
void
TrainingTree::train( const DataSet& data, const Params params )
{
	START;
	LOG << "Start training\n";

	NodeT::resetNodeId();
	auto nbAttribs = data.nbAttribs();
	if( !nbAttribs )
		throw std::runtime_error( "no attributes!" );
	if( data.size()<2 )
		throw std::runtime_error( "no enough data points!" );

	_initialVertex = boost::add_vertex(_graph);  // create initial vertex

	std::vector<uint> v_idx( data.size() );  // create vector holding indexes of all the data points
	std::iota( v_idx.begin(), v_idx.end(), 0 );

	_graph[_initialVertex].v_Idx = v_idx;
	_graph[_initialVertex]._type = NT_Root;

	splitNode( _initialVertex, _graph, data, params ); // Call the "split" function (recursive)
	LOG << "Training done\n";
	printInfo( std::cout );
}

//---------------------------------------------------------------------
/// Returns class of data point as classified by tree
//template<typename T>
ClassVal
TrainingTree::classify( const DataPoint& point ) const
{
	ClassVal retval{-1};
	vertexT_t v = _initialVertex;   // initialize to first node
//	COUT << point << '\n';
	bool done = false;
//	uint iter = 0;
	do
	{
//		COUT << "\n*iter=" << iter++ << " NODE " << _graph[v]._nodeId << std::endl;
		if( _graph[v]._type == NT_Final ) // then, we are done !
		{
			done = true;
			retval = _graph[v]._class;
//			COUT << "Final node: class = " << retval << "\n";
		}
		else
		{
			auto attrIndex = _graph[v]._attrIndex;  // get attrib index that this node handles
			auto atValue   = point.attribVal( attrIndex );  // get data point value for this attribute
//			COUT << "Considering attribute " << attrIndex << " with value " << atValue << std::endl;
			assert( boost::out_degree( v, _graph ) == 2 );

			auto edges = boost::out_edges( v, _graph );    // get the two output edges of the node
			auto et = edges.first++;
			auto ef = edges.first;
			if( _graph[*ef].edgeSide )
				std::swap( et, ef );

//			COUT << "node thres=" << _graph[v]._threshold << std::endl;
			if( atValue < _graph[v]._threshold )  // depending on threshold, define the next node
				v = boost::target( *et, _graph );
			else
				v = boost::target( *ef, _graph );
		}
	}
	while( !done );
	return retval;
}

//---------------------------------------------------------------------
/// Classify \c dataset and returns performance score
Perf
TrainingTree::classify( const DataSet& dataset ) const
{
	size_t nbErrors = 0;
	COUT << "dataset.nbClasses()=" << dataset.nbClasses()  << "\n";
	auto nbClasses = dataset.nbClasses();
	if( nbClasses>1 )
	{
        ConfusionMatrix confmat( nbClasses );
        for( const auto& datapoint: dataset )
        {
            auto cla1 = datapoint.classVal();
            auto cla2 = classify( datapoint );
            if( cla1 != cla2 )
                nbErrors++;
            confmat.add( cla1, cla2 );
        }
        COUT << "Nb classification errors=" << nbErrors << " / " << dataset.size() << " datapoints\n";
        std::cout << confmat;
    }
       return Perf( 1. * nbErrors/ dataset.size() );
}

//---------------------------------------------------------------------

