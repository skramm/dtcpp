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

#ifdef DEBUG
	#define COUT if(1) std::cout << __FUNCTION__ << "(), l." << __LINE__ << ": "
#else
	#define COUT if(0) std::cout
#endif // DEBUG

#ifdef DEBUG_START
	#define START if(1) std::cout << "* Start: " << __FUNCTION__ << "()\n"
#else
	#define START
#endif // DEBUG

#define LOG( level, msg ) \
	{ \
		if( g_params.verbose && level<=g_params.verboseLevel ) \
		{ \
			std::cout << std::setfill('0') << std::setw(4) << g_params.timer.getDuration(level); \
			::priv::spaceLog( level ); \
			std::cout << " E" << std::setfill('0') << std::setw(4) << ::priv::logCount(level)++ << '-' << __FUNCTION__ << "(): " << msg << '\n'; \
		} \
	}

// forward declaration
namespace dtcpp {
class DataSet;
}

// % % % % % % % % % % % % % %
/// private namespace; not part of API
namespace priv {
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
		std::cout << "  ";
}

//---------------------------------------------------------------------
/// Holds timing
/// \todo add level to have a timing PER log level
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



//namespace dtcpp {

// % % % % % % % % % % % % % %
/// private namespace; not part of API
// namespace priv {
// % % % % % % % % % % % % % %

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
priv::Gparams g_params;


#endif // PRIVATE_HG
