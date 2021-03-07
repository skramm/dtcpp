
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

struct MyHistogram;
//---------------------------------------------------------------------
/// A histogram bin for MyHistogram, holds an occurrence counter and a class counter
template<typename T>
struct MyBin
{
	friend struct MyHistogram;

	private:
		std::map<ClassVal,size_t> _mClassCounter;
		float                     _startValue;
		float                     _endValue;
		std::vector<size_t>       _vIdxPt;      ///< indexes of the points in original dataset

	public:
		MyBin( float v1, float v2 )
			: _startValue(v1), _endValue(v2)
		{
			assert( v1 < v2 );
		}
		MyBin()
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
};


//---------------------------------------------------------------------
/// Variable bin-size histogram
struct MyHistogram
{
	private:
		const std::vector<PairAtvalClass>* p_src = 0;  ///< pointer on source data
		std::list<MyBin<float>> _lBins;
		double _step;
		double _vmin;

	public:
		MyHistogram( const std::vector<PairAtvalClass>& src, size_t nbBins, float vmin, float vmax )
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

		void print( std::ostream& ) const;
		PairAtvalClass getPoint(size_t idx) const
		{
			assert( p_src );
			return p_src->at(idx);
		}
		bool splitBin( decltype( _lBins.begin() ) );
		void assignToBin( const PairAtvalClass& pac, size_t idx );
};

//---------------------------------------------------------------------
void
MyHistogram::assignToBin( const PairAtvalClass& pac, size_t idx )
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
void
MyHistogram::print( std::ostream& f ) const
{
	f << "HISTOGRAM, nb bins=" << nbBins()
		<< " v0=" << _vmin << " step=" << _step
		<< '\n';
	size_t i=0;
	for( const auto& bin: _lBins )
	{
		f << "bin " << i++ << " nb pts=" << bin.size() << " nb classes=" << bin.nbClasses() << '\n';
	}
}
//---------------------------------------------------------------------
bool
MyHistogram::splitBin( decltype( _lBins.begin() ) it )
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
//		auto v1 = bin._startValue;
//		auto v2 = v1 + _step;
		auto start2 = ( bin._startValue + bin._endValue ) / 2.;
		cout << "old thres: " << bin._startValue << " - " << bin._endValue << '\n';
		cout << "new thres: " << bin._startValue << " - " << start2 << " - " << bin._endValue << '\n';

		MyBin<float> newBin( start2, bin._endValue );

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
		auto b1 = splitBin( it );
		auto b2 = splitBin( std::next(it) );

		retval = true;
	}
	else
		cout << " -NOT splittable!\n";

	depth--;
	return retval;
}

//---------------------------------------------------------------------
MyHistogram
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

	MyHistogram histo( v_pac, nbBins, val_min.first, val_max.first );

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
	auto histo = buildHistogram( v_pac, nbBins );
	histo.print( std::cout );

// Step 2 - split bins (that need to be split)
	COUT << "Start splitting, nb bins=" << histo.nbBins() << '\n';

	int iter1 = 0;
	bool splitOccured = false;
	do
	{
		COUT << "iter1 " << iter1++ << '\n';
		splitOccured = false;
		int iter2 = 0;
		auto it = histo.begin();
		do
		{
			COUT << "iter2 " << iter2++ << '\n';
			splitOccured = histo.splitBin( it );
			it = std::next(it);
		}
		while( !splitOccured && it != std::end(histo) );
	}
	while( splitOccured );

// Step 3 - build thresholds from bins
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
	auto c3 = ClassVal(3);
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
