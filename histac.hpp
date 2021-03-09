/**
\file histac.hpp
\brief holds class VBS_Histogram
*/
#ifndef HISTAC_HG
#define HISTAC_HG

#include <iostream>
#include <map>
#include <list>
#include <vector>
#include <cassert>
#include <algorithm>

#ifndef COUT
#define COUT std::cout
#endif

#define DEBUG

//#ifdef DEBUG
#include "private.hpp"
//#endif

namespace histac {

//---------------------------------------------------------------------
/// Variable bin-size histogram. 1st argument type is the floating-point type (\c float or \c double)
/// 2nd type is the key used for the mapping
template<typename U,typename KEY>
struct VBS_Histogram
{
//---------------------------------------------------------------------
/// Inner class, a histogram bin for VBS_Histogram, holds an occurrence counter and a class counter
	template<typename T>
	struct HBin
	{
		friend struct VBS_Histogram;

		private:
			std::map<KEY,size_t> _mClassCounter;  ///< number of pts per class
			T                    _startValue;     ///< bin left border
			T                    _endValue;       ///< bin right border
			std::vector<size_t>  _vIdxPt;         ///< indexes of the points in original dataset

		public:
			HBin( T v1, T v2 )
				: _startValue(v1), _endValue(v2)
			{
				assert( v1 < v2 );
			}
			HBin()
			{}

		/// A bin can be split if more then 1 classes and more than 2 points
			bool isSplittable() const
			{
				if( _vIdxPt.size() < 2 )  // not enough points
					return false;

				if( _mClassCounter.size() < 2 )  // single class, no need to split
					return false;
				return true;
			}
			size_t size()      const { return _vIdxPt.size(); }
			size_t nbClasses() const { return _mClassCounter.size(); }
			std::pair<T,T> getBorders() const
			{
				return std::make_pair( _startValue, _endValue );
			}
			friend std::ostream& operator << ( std::ostream& f, const HBin& b )
			{
				f << "Bin: " << b.size() << " pts, " << b.nbClasses() << " classes, range=" << b._startValue << "-" << b._endValue << ' ';
#ifdef BIN_PRINT_POINTS
				f << " points: ";
					priv::printVector( f, b._vIdxPt );
#endif // BIN_PRINT_POINTS
				return f;
			}
	};

	private:
		const std::vector<std::pair<U,KEY>>* p_src = 0;  ///< pointer on source data
		std::list<HBin<U>> _lBins;                       ///< list of bins
		size_t             _maxDepth = 12;

	public:
		VBS_Histogram( const std::vector<std::pair<U,KEY>>& src, size_t nbBins );

		const auto begin() const { return _lBins.begin(); }
		const auto end()   const { return _lBins.end();   }

		size_t nbBins() const { return _lBins.size(); }
		void mergeSearch();
		void splitSearch();
		void print( std::ostream&, const char* msg=0 ) const;

	private:
		void assignToBin( const std::pair<U,KEY>& pac, size_t idx );
		bool splitBin( decltype( _lBins.begin() ), char side );
};

//---------------------------------------------------------------------
/// Constructor, creates bins evenly spaced
template<typename T,typename KEY>
VBS_Histogram<T,KEY>::VBS_Histogram( const std::vector<std::pair<T,KEY>>& v_pac, size_t nbBins )
	: p_src( &v_pac )
{
	assert( v_pac.size() );
	auto it_mm = std::minmax_element(
		v_pac.begin(),
		v_pac.end(),
		[]                                                          // lambda
		( const auto& p1, const auto& p2 )
		{
			return p1.first < p2.first;
		}
	);

	auto itmin = it_mm.first;
	auto itmax = it_mm.second;
	auto val_min = *itmin;
	auto val_max = *itmax;    // sets min and max values

	auto vmin = val_min.first;
	auto vmax = val_max.first;

	auto step = (vmax - vmin) / nbBins;
	_lBins.resize( nbBins );

	int i = 0;
	for( auto& bin: _lBins )
	{
		bin._startValue = vmin + i     * step;
		bin._endValue   = vmin + (i+1) * step;
		i++;
	}

	for( size_t i=0; i<v_pac.size(); i++ )
		assignToBin( v_pac[i], i );
}
//---------------------------------------------------------------------
template<typename T,typename KEY>
void
VBS_Histogram<T,KEY>::assignToBin( const std::pair<T,KEY>& pac, size_t idx )
{
	bool keepOn = true;

//	COUT << "searching bin for val=" << pac.first << ", class=" << pac.second << '\n';
	for( auto it=_lBins.begin(); it!=_lBins.end() && keepOn; it++ )    // iterate on the bins
	{
		keepOn = true;
		auto& bin = *it;
		if( pac.first >= bin._startValue && pac.first < bin._endValue )
		{
			bin._vIdxPt.push_back( idx );
			bin._mClassCounter[pac.second]++;
			keepOn = false;
		}
	}

	if( keepOn )  // needed for the value used to compute the histogram range:
	{             // if point did not fit in any of the other bins, then we put it in the last bin
		auto& bin = _lBins.back();
		bin._vIdxPt.push_back( idx );
		bin._mClassCounter[pac.second]++;
	}
}

//---------------------------------------------------------------------
template<typename T,typename KEY>
void
VBS_Histogram<T,KEY>::print( std::ostream& f, const char* msg ) const
{
	f << "HISTOGRAM - ";
	if( msg )
		f << msg;
	f << ", nb bins=" << nbBins() << '\n';
	size_t i=0;
	for( const auto& bin: _lBins )
		f << "bin " << i++ << ": " << bin << '\n';
}
//---------------------------------------------------------------------
/// Attempt to split a bin, returns true if a split occurred
/**
This will also remove points if at a given max depth, we still can"t split the bin
*/
template<typename T,typename KEY>
bool
VBS_Histogram<T,KEY>::splitBin( decltype( _lBins.begin() ) it, char side )
{
	assert( p_src );
	static size_t depth;

	bool retval = false;
	auto& bin = *it;                // current bin
	auto it_next = std::next(it);  // next one (will insert before this one)
	COUT << side << ": depth=" << depth++ << " start split " << bin << '\n';

	if( depth >= _maxDepth )
	{
/*		priv::printVector( std::cout, bin._vIdxPt, "BEFORE points" );
		for( const auto idx: bin._vIdxPt )
			std::cout << idx << ": " <<p_src->at(idx).first << "-" << p_src->at(idx).second << "\n";
*/
		auto it = std::max_element(                      // find dominant class
			std::begin(bin._mClassCounter),
			std::end(bin._mClassCounter),
			[]                                   // lambda
			( const auto& p1, const auto& p2 )
			{
				return p1.second < p2.second;
			}
		);
		auto domClass = it->first;
		COUT << "domClass=" << domClass << "\n";

		std::vector<size_t> vec1;
		vec1.reserve( bin.size() );
		for( const auto idx: bin._vIdxPt )  // parse the points
		{
			const auto& pt = p_src->at(idx);
			if( pt.second == domClass )
				vec1.push_back( idx );
		}
		bin._vIdxPt = std::move(vec1);

/*		priv::printVector( std::cout, bin._vIdxPt, "AFTER points" );
		for( const auto idx: bin._vIdxPt )
			std::cout << idx << ": " <<p_src->at(idx).first << "-" << p_src->at(idx).second << "\n";
*/
		return retval;
	}

	if( bin.isSplittable() )
	{
		std::cout << "  => splittable\n";
		auto midValue = ( bin._startValue + bin._endValue ) / 2.;
		std::cout << "  -old thres: " << bin._startValue << " - " << bin._endValue << '\n';
		std::cout << "  -new thres: " << bin._startValue << " - " << midValue << " - " << bin._endValue << '\n';

		std::vector<size_t> vec1;  // new vector of indexes for the current bin
		std::vector<size_t> vec2;  // new vector of indexes for the new bin
		vec1.reserve( bin.size() );
		vec2.reserve( bin.size() );

		std::map<KEY,size_t> mapCount1;
		std::map<KEY,size_t> mapCount2;

		for( const auto idx: bin._vIdxPt )  // parse the points
		{
			auto pt = p_src->at(idx);        // and distribute them in
			if( pt.first >= midValue )         // the two bins
			{
				vec2.push_back( idx );
				mapCount2[ pt.second ]++;
			}
			else
			{
				vec1.push_back( idx );
				mapCount1[ pt.second ]++;
			}
		}

		VBS_Histogram::HBin<T> newBin( midValue, bin._endValue );
		bin._endValue = midValue;

		bin._vIdxPt           = std::move(vec1);
		bin._mClassCounter    = std::move(mapCount1);
		newBin._vIdxPt        = std::move(vec2);
		newBin._mClassCounter = std::move(mapCount2);
		COUT << "bin1: " << bin << '\n';
		COUT << "bin2: " << newBin << '\n';

		_lBins.insert( it_next, newBin );
		if( bin.size() )
			splitBin( it, 'A' );
		if( newBin.size() )
			splitBin( std::next(it), 'B' );

		retval = true;
	}
//	else
//		std::cout << " => NOT splittable!\n";

	depth--;
	return retval;
}

//---------------------------------------------------------------------
/// Searches for any splits
template<typename T,typename KEY>
void
VBS_Histogram<T,KEY>::splitSearch()
{
	COUT << "\n* Start splitting, nb bins=" << nbBins() << '\n';

	size_t iter1 = 0;
	bool splitOccured = false;
	do
	{
		splitOccured = false;
		size_t iter2 = 0;
		auto it = _lBins.begin();
		do
		{
			COUT << "iter1 " << iter1 << " iter2 " << iter2++ << '\n';
			splitOccured = splitBin( it, '0' );
			it = std::next(it);
		}
		while( !splitOccured && it != std::end(_lBins) );
		iter1++;
	}
	while( splitOccured );
}
//---------------------------------------------------------------------
/// Searches for any potential merges (adjacent bins holding same class)
template<typename T,typename KEY>
void
VBS_Histogram<T,KEY>::mergeSearch()
{
	COUT << "\n* Start merge search, nb bins=" << nbBins() << '\n';
	size_t iter1 = 0;
	bool mergeOccurred = false;
	do
	{
		auto it = _lBins.begin();
		mergeOccurred = false;
		size_t iter2 = 0;
		do
		{
			COUT << "-Iter1 " << iter1 << " iter2 " << iter2++ << '\n';
			if( std::next(it) != std::end(_lBins) ) // if there is a next bin
			{
				auto& b1 = *it;
				auto& b2 = *std::next(it);
//				std::cout << " - b1: " << b1 << '\n';
//				std::cout << " - b2: " << b2 << '\n';

				if( b1.nbClasses() == 1 && b2.nbClasses() == 1 )  // if the 2 bins only hold 1 class
				{
					auto itc1 = b1._mClassCounter.begin();
					auto itc2 = b2._mClassCounter.begin();
					if( itc1->first  == itc2->first  )             // if they hold the same class
					{
						COUT << "same class (" << itc1->first  << "), merging bins\n";
						b1._vIdxPt.insert(                        // then copy the points from b2 into b1
							b1._vIdxPt.end(),
							b2._vIdxPt.begin(),
							b2._vIdxPt.end()
						);
						b1._endValue = b2._endValue;
						mergeOccurred = true;
						_lBins.erase( std::next(it) );
					}
				}
				if( b2.size() == 0 )  // if next bin is empty, then merge it
				{
					COUT << "bin2 is empty, removing\n";
					mergeOccurred = true;
					b1._endValue = b2._endValue;
					_lBins.erase( std::next(it) );
				}
			}
			if( !mergeOccurred )
				it = std::next(it);
		}
		while( !mergeOccurred && it != std::end(_lBins) );
		iter1++;
	}
	while( mergeOccurred );

}
//---------------------------------------------------------------------
} //namespace histac

//---------------------------------------------------------------------
/// Input:  a vector or pairs, size=nb of points in data set
/// first value: the attribute value, second: the class value
template<typename T,typename KEY>
std::vector<float>
getThresholds( const std::vector<std::pair<T,KEY>>& v_pac, int nbBins )
{
// Step 1 - build initial histogram, evenly spaced
	histac::VBS_Histogram<T,KEY> histo( v_pac, nbBins );

	histo.print( std::cout, "AFTER BUILD" );

// Step 2 - split bins that need to be splitted
	histo.splitSearch();
	histo.print( std::cout, "AFTER SPLIT" );

// Step 3 - merge adjacent bins holding same class
	histo.mergeSearch();
	histo.print( std::cout, "AFTER MERGE" );

// Step 4 - build thresholds from bins
	std::vector<float> vThres( histo.nbBins()-1 );

	size_t i=0;
	for( auto it=histo.begin(); it != histo.end(); it++ )
	{
		if( std::next(it) !=  histo.end() )
			vThres[i++] = it->getBorders().second;
	}

	return vThres;
}
//---------------------------------------------------------------------

#endif // HISTAC_HG
