/**
\file private.hpp
\brief various stuff, only useful for debugging
*/


#ifndef PRIVATE_HG
#define PRIVATE_HG

#include <iostream>
//#include <map>
//#include <list>
//#include <vector>

#define DTCPP_PLOT_MAX_WIDTH 1500

#ifdef DEBUG_START
	#define START if(1) std::cout << "* Start: " << __FUNCTION__ << "()\n"
	#define DEBUG
#else
	#define START
#endif

#ifdef DEBUG
	#define COUT if(1) std::cout << __FUNCTION__ << "(), l." << __LINE__ << ": "
#else
	#define COUT if(0) std::cout
#endif

#define LOG( level, msg ) \
	{ \
		if( g_params.verbose && level<=g_params.verboseLevel ) \
		{ \
			std::cout << std::setfill('0') << std::setw(4) << g_params.timer.getDuration(level); \
			priv1::spaceLog( level ); \
			std::cout << " E" << std::setfill('0') << std::setw(4) << priv1::logCount(level)++ << '-' << __FUNCTION__ << "(): " << msg << '\n'; \
		} \
	}

// forward declaration
namespace dtcpp {
class DataSet;
}

// % % % % % % % % % % % % % %
/// private namespace; not part of API
namespace priv1 {
// % % % % % % % % % % % % % %
constexpr int nbLogLevels = 5;

uint& logCount(uint level)
{
	static std::array<uint,nbLogLevels> s_logCount;
	assert( level<nbLogLevels );
	return s_logCount[level];
}

//---------------------------------------------------------------------
/// Used in logging macro, see \ref LOG
void spaceLog( int n )
{
	std::cout << ':';
	for( int i=0; i<n; i++ )
		std::cout << " |";
}

//---------------------------------------------------------------------
/// Holds timing
struct Timer
{
	std::string getDuration(int level)
	{
		assert( level<nbLogLevels );
		auto t2 = std::chrono::high_resolution_clock::now();
		auto tdiff = std::chrono::duration_cast<std::chrono::milliseconds>( t2 - ck[level] ).count();
		ck[level] = t2;
		return std::to_string( tdiff );
	}
	void start()
	{
		for( auto& tp: ck )
			tp = std::chrono::high_resolution_clock::now();
	}
	std::array<std::chrono::high_resolution_clock::time_point,nbLogLevels> ck;
};

//---------------------------------------------------------------------
/// Some global runtime parameters
struct Gparams
{
#ifdef DEBUG
	bool  verbose = true;
	int   verboseLevel = 5;
#else
	bool  verbose = false;
	int   verboseLevel = 1;
#endif
	Timer timer; ///< Used for logging, to measure duration.
	Gparams()
	{
		timer.start();
	}
	const dtcpp::DataSet* p_dataset = nullptr; ///< this will always point on the loaded dataset
};


//---------------------------------------------------------------------
/// Type returned by findDominantClass()
template<typename C>
struct DominantClassInfo
{
	C      dominantClass;
	size_t dcCount;
	float  ambig;
};

/// Finds in a class-counting map the max value (first) and ambiguity of that max value (second)
template<typename C>
//std::pair<T,float>
DominantClassInfo<C>
findDominantClass( const std::map<C,size_t>& mcount )
{
	assert( mcount.size() > 1 );

	size_t vmax  = 0u;
	size_t vmax2 = vmax;
	C cmax  = C(-1);
	C cmax2 = cmax;

	for( const auto& p: mcount )
	{
		if( p.second > vmax )
		{
			vmax2 = vmax;
			cmax2 = cmax;
			vmax = p.second;
			cmax = p.first;
		}
	}
	assert( vmax>0 );
	return DominantClassInfo<C>{ cmax, vmax, 1.f * vmax2/vmax };
}

//---------------------------------------------------------------------
/// General utility function
template<typename T>
void
printVector( std::ostream& f, const std::vector<T>& vec, const char* msg=0, bool lineBreak=false )
{
	f << "Vector: ";
	if( msg )
		f << msg;
	f << " #=" << vec.size() << ":\n";
	for( const auto& elem : vec )
	{
		f << elem;
		if( lineBreak )
			f << '\n';
		else
			f << "-";
	}
	if( !vec.empty() )
		f << '\n';
}
//---------------------------------------------------------------------
/// General utility function
template<typename K, typename V>
void
printMap( std::ostream& f, const std::map<K,V>& m, const char* msg=0 )
{
	f << "Map: ";
	if( msg )
		f << msg;
	f << " #=" << m.size() << ":\n";
	for( const auto& elem : m )
		f << " -" << elem.first << "-" << elem.second << '\n';
	f << "\n";
}
//---------------------------------------------------------------------

// % % % % % % % % % % % % % %
} // namespace priv
//} // namespace dtcpp
// % % % % % % % % % % % % % %

/// Global parameters
priv1::Gparams g_params;


#endif // PRIVATE_HG
