
//#include <boost/histogram.hpp>
#define DEBUG
#define DEBUG_START

#include "dtcpp.h"
#include <algorithm>
#include <iostream>
#include <map>
#include <list>
#include <cmath>

/**
https://www.boost.org/doc/libs/1_70_0/libs/histogram/doc/html/histogram/guide.html#histogram.guide.expert

  /\
  |
 4+           +--------+
  |           |        |
 3+  +--------+        |
  |  | c[1]=0 |        |
 2+  | c[2]=2 |        +--
  |  | c[3]=1 |        | (other bins)
 1+  |        |        |
  |  |        |        |
--+--+--------+--------+--------->
	.15      .25      .35


h( make_pair( .21, 2 );
h( make_pair( .22, 2 );
h( make_pair( .23, 3 );



*/
using namespace std;
using namespace dtcpp;

using PairAtvalClass = std::pair<float,ClassVal>;


//---------------------------------------------------------------------
/// Variable bin-size histogram
template<typename U>
struct VBS_Histogram
{
//---------------------------------------------------------------------
	/// Inner class, a histogram bin for VBS_Histogram, holds an occurrence counter and a class counter
	template<typename T>
	struct HBin
	{
		friend struct VBS_Histogram;

		private:
			std::map<ClassVal,size_t> _mClassCounter;
			T                         _startValue;
			T                         _endValue;
			std::vector<size_t>       _vIdxPt;      ///< indexes of the points in original dataset

		public:
			HBin( float v1, float v2 )
				: _startValue(v1), _endValue(v2)
			{
				assert( v1 < v2 );
			}
			HBin()
			{}

		/// A bin can be split if more then 1 classes and more than 2 points
			bool isSplittable() const
			{
				if( _vIdxPt.size() > 2 && _mClassCounter.size() > 1 )
					return true;
				return false;
			}
			size_t size()      const { return _vIdxPt.size(); }
			size_t nbClasses() const { return _mClassCounter.size(); }
			std::pair<T,T> getBorders() const
			{
				return std::make_pair( _startValue, _endValue );
			}
			friend std::ostream& operator << ( std::ostream& f, const HBin& b )
			{
				f << "Bin: " << b.size() << " pts, " << b.nbClasses() << " classes, range=" << b._startValue << "-" << b._endValue << '\n';
				return f;
			}
	};

	private:
		const std::vector<PairAtvalClass>* p_src = 0;  ///< pointer on source data
		std::list<HBin<float>> _lBins;
		double _step;
		double _vmin;

	public:
		VBS_Histogram( const std::vector<PairAtvalClass>& src, size_t nbBins, float vmin, float vmax )
			: p_src( &src )
			, _vmin(vmin)
		{
			auto step = (vmax - vmin) / nbBins;
			_lBins.resize( nbBins );

			int i = 0;
			for( auto& b: _lBins )
			{
				b._startValue = vmin + i     * step;
				b._endValue   = vmin + (i+1) * step;
				i++;
			}
		}

		const auto begin() const { return _lBins.begin(); }
		const auto end()   const { return _lBins.end();   }
		auto begin() { return _lBins.begin(); }
		auto end()   { return _lBins.end();   }

		size_t nbBins() const { return _lBins.size(); }
		void mergeSearch();
		void splitSearch();
		void print( std::ostream&, const char* msg=0 ) const;
		PairAtvalClass getPoint(size_t idx) const
		{
			assert( p_src );
			return p_src->at(idx);
		}
		bool splitBin( decltype( _lBins.begin() ) );
		void assignToBin( const PairAtvalClass& pac, size_t idx );
};

//---------------------------------------------------------------------
template<typename T>
void
VBS_Histogram<T>::assignToBin( const PairAtvalClass& pac, size_t idx )
{
	bool keepOn = true;

	COUT << "searching bin for val=" << pac.first << '\n';
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
}

//---------------------------------------------------------------------
template<typename T>
void
VBS_Histogram<T>::print( std::ostream& f, const char* msg ) const
{
	f << "HISTOGRAM - ";
	if( msg )
		f << msg;
	f << ", nb bins=" << nbBins() << " v0=" << _vmin << '\n';
	size_t i=0;
	for( const auto& bin: _lBins )
		f << "bin " << i++ << ": " << bin;
}
//---------------------------------------------------------------------
/// Attempt to split a bin, returns true if a split occured
template<typename T>
bool
VBS_Histogram<T>::splitBin( decltype( _lBins.begin() ) it )
{
	START;

	static int depth;

	bool retval = false;
	auto& bin = *it;                // current bin
	auto it_next = std::next(it);  // next one (will insert before this one)
	COUT<< "depth=" << depth++ << " start split bin with thres=" << bin._startValue << '\n';
	if( bin.isSplittable() )
	{
		cout << " -splittable, bin has " << bin.size() << " pts and " << bin.nbClasses() << " classes\n";
		auto start2 = ( bin._startValue + bin._endValue ) / 2.;
		cout << "old thres: " << bin._startValue << " - " << bin._endValue << '\n';
		cout << "new thres: " << bin._startValue << " - " << start2 << " - " << bin._endValue << '\n';

		VBS_Histogram::HBin<float> newBin( start2, bin._endValue );

		std::vector<size_t> newvec;  // new vector of indexes for the current bin
		newvec.reserve( bin.size() );
		bin._mClassCounter.clear();     // reset class counter

		for( const auto idx: bin._vIdxPt )  // parse the points
		{
			COUT << "checking point " << idx << '\n';
			auto pt = getPoint(idx);        // and distribute them in
			if( pt.first >= start2 )           // the two bins
			{
				newBin._mClassCounter[ pt.second ]++;
				newBin._vIdxPt.push_back( idx );
			}
			else
			{
				newvec.push_back( idx );
				bin._mClassCounter[ pt.second ]++;
			}
		}
		bin._vIdxPt = newvec;        // copy new vector of indexes for current bin
		bin._endValue = start2;
		_lBins.insert( it_next, newBin );

		COUT << "current bin: " << bin.size() << " pts, new bin:" << newBin.size() << " pts\n";
		splitBin( it );
		splitBin( std::next(it) );

		retval = true;
	}
	else
		cout << " -NOT splittable!\n";

	depth--;
	return retval;
}

//---------------------------------------------------------------------
/// Searches for any splits
template<typename T>
void
VBS_Histogram<T>::splitSearch()
{
	COUT << "Start splitting, nb bins=" << nbBins() << '\n';

	int iter1 = 0;
	bool splitOccured = false;
	do
	{
		COUT << "iter1 " << iter1++ << '\n';
		splitOccured = false;
		int iter2 = 0;
		auto it = _lBins.begin();
		do
		{
			COUT << "iter2 " << iter2++ << '\n';
			splitOccured = splitBin( it );
			it = std::next(it);
		}
		while( !splitOccured && it != std::end(_lBins) );
	}
	while( splitOccured );
}
//---------------------------------------------------------------------
/// Searches for any potential merges (adjacent bins holding same class)
template<typename T>
void
VBS_Histogram<T>::mergeSearch()
{
	COUT << "Start merge search, nb bins=" << nbBins() << '\n';
	bool mergeOccurred = false;
	do
	{
		COUT << "LOOP1 start, nb bins=" << nbBins() << '\n';
		auto it = _lBins.begin();
		mergeOccurred = false;
		do
		{
			COUT << "LOOP2 start, nb bins=" << nbBins() << '\n';
			if( std::next(it) != std::end(_lBins) ) // if there is a next bin
			{
				auto& b1 = *it;
				auto& b2 = *std::next(it);
				std::cout << "b1: " << b1;
				std::cout << "b2: " << b2;

				if( b1.nbClasses() == 1 && b2.nbClasses() == 1 )
				{
					auto itc1 = b1._mClassCounter.begin();
					auto itc2 = b2._mClassCounter.begin();
					COUT << "classes: " << itc1->first << "-" << itc2->first  << '\n';
//					COUT << "classes: " << *itc1 << "-" << *itc2->first  << '\n';
					if( itc1->first  == itc2->first  )                        // if they hold the same class
					{
						COUT << "same class, merging bins\n";
						b1._vIdxPt.insert(                        // then copy the points from b2 into b1
							b1._vIdxPt.end(),
							b2._vIdxPt.begin(),
							b2._vIdxPt.end()
						);
						b1._endValue = b2._endValue;
						mergeOccurred = true;
						_lBins.erase( it );
					}
				}
			}
			if( !mergeOccurred )
				it = std::next(it);
		}
		while( !mergeOccurred && it != std::end(_lBins) );
	}
	while( mergeOccurred );

}
//---------------------------------------------------------------------
template<typename T>
VBS_Histogram<T>
buildHistogram(
	const std::vector<PairAtvalClass>& v_pac,   ///< input vector of pairs (attribute value,class)
	size_t                             nbBins   ///< initial nb of bins
)
{
	START;

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

	VBS_Histogram<T> histo( v_pac, nbBins, val_min.first, val_max.first );

	for( size_t i=0; i<v_pac.size(); i++ )
		histo.assignToBin( v_pac[i], i );

	return histo;
}

//---------------------------------------------------------------------
/// Input:  a vector or pairs, size=nb of points in data set
/// first value: the attribute value, second: the class value
std::vector<float>
getThresholds( const std::vector<PairAtvalClass>& v_pac, int nbBins )
{
	START;
// Step 1 - build histogram, even spaced
	auto histo = buildHistogram<float>( v_pac, nbBins );
	histo.print( std::cout, "AFTER BUILD" );

// Step 2 - split bins (that need to be split)
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
int main()
{
	auto c1 = ClassVal(1);
	auto c2 = ClassVal(2);
//	auto c3 = ClassVal(3);
	std::vector<PairAtvalClass> vpac{
		{ 0., c1 },
		{ 0.5, c1 },
		{ 0.6, c1 },
		{ 0.7, c2 },
		{ 0.8, c2 },
		{ 3,   c2 },
	};
	auto vThres = getThresholds( vpac, 3 );
	priv::printVector( std::cout, vThres );

	std::cout << "Done !\n";
}
//---------------------------------------------------------------------
