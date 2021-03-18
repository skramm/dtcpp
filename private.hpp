/**
\file private.hpp
\brief various stuff, only useful for debugging
*/


#ifndef PRIVATE_HG
#define PRIVATE_HG

//#include <iostream>
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


//namespace dtcpp {

// % % % % % % % % % % % % % %
/// private namespace; not part of API
namespace priv {
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


#endif // PRIVATE_HG