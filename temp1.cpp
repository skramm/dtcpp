
//#include <boost/histogram.hpp>
#define DEBUG
#include "dtcpp.h"
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

//using namespace boost::histogram;
using PairAtvalClass = std::pair<float,ClassVal>;

struct MyHistogram;
//---------------------------------------------------------------------
/// A histogram bin for MyHistogram, holds an occurrence counter and a class counter
struct MyBin
{
	friend struct MyHistogram;

	std::map<ClassVal,size_t> _mClassCounter;
	float                     _startValue;
	std::vector<size_t>       _vIdxPt;      ///< indexes of the points in original dataset

/// A bin can be split if more then 1 classes and more than 2 points
	bool isSplittable() const
	{
		if( _vIdxPt.size() > 2 && _mClassCounter.size() > 1 )
			return true;
		return false;
	}
	size_t size()      const { return _vIdxPt.size(); }
	size_t nbClasses() const { return _mClassCounter.size(); }
};


//---------------------------------------------------------------------
/// Variable bin-size histogram
struct MyHistogram
{
	private:
		const std::vector<PairAtvalClass>* p_src = 0;  ///< pointer on source data
		std::list<MyBin> _lBins;
		double _step;
		double _vmin;

	public:
		MyHistogram( const std::vector<PairAtvalClass>& src, size_t nbBins, float vmin, float vmax )
			: p_src( &src )
			, _vmin(vmin)
		{
			_step = (vmax - vmin) / nbBins;
//			COUT << "STEP=" << _step << '\n';
			_lBins.resize( nbBins );

			int i = 0;
			for( auto& b: _lBins )
				b._startValue = vmin + i++ * _step;
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
	bool match = true;
	auto it_prev = _lBins.begin();
	COUT << "searching bin for val=" << pac.first << '\n';
	for( auto it=_lBins.begin(); it!=_lBins.end() && match; it++ )    // iterate the bins, except the last one
	{
		match = false;
		auto& bin = *it;
//		COUT << "trying thres "<< bin._startValue << '\n';
		if( pac.first >= bin._startValue )
		{
			match = true;
//			std::cout << "higher !\n";
		}

		if( match == false )
		{
//			COUT << "FOUND!\n";
			auto& bin_prev = *it_prev;
			bin_prev._vIdxPt.push_back( idx );
			bin_prev._mClassCounter[pac.second]++;
		}
		else
		{
			if( std::next(it)==_lBins.end() )  // for values going in last bin
			{
//				COUT << "FOUND LAST!\n";
				bin._vIdxPt.push_back( idx );
				bin._mClassCounter[pac.second]++;
			}
		}
		it_prev = it;
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
	bool retval = false;
	auto& bin = *it;                // current bin
	auto it_next = std::next(it);  // next one (will insert before this one)
	cout << "-start split bin at " << bin._startValue << '\n';
	if( bin.isSplittable() )
	{
		cout << " -splittable, bin has " << bin.size() << " pts\n";
		auto v1 = bin._startValue;
		auto v2 = v1 + _step;
		auto start2 = (v1+v2) / 2.;
		cout << "old thres: " << v1 << " - " << v2 << '\n';
		cout << "new thres: " << v1 << " - " << start2 << " - " << v2 << '\n';

		MyBin newBin;
		newBin._startValue = start2;

		std::vector<size_t> newvec;  // new vector of indexes for the current bin
		newvec.reserve( bin.size() );
		bin._mClassCounter.clear();     // reset class counter

		for( const auto idx: bin._vIdxPt )  // parse the points
		{
			COUT << "checling point " << idx << '\n';
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
		_lBins.insert( it_next, newBin );

		auto b1 = splitBin( it );
		auto b2 = splitBin( std::next(it) );

		retval = true;
	}
	else
		cout << " -NOT splittable!\n";

	return retval;
}

//---------------------------------------------------------------------
MyHistogram
buildHistogram(
	const std::vector<PairAtvalClass>& v_pac,   ///< input vector of pairs (attribute value,class)
	size_t                             nbBins   ///< initial nb of bins
)
{
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

	cout << "min=" << val_min.first << "-" << val_min.second << '\n';
	cout << "max=" << val_max.first << "-" << val_max.second << '\n';

	MyHistogram histo( v_pac, nbBins, val_min.first, val_max.first );

	for( size_t i=0; i<v_pac.size(); i++ )
		histo.assignToBin( v_pac[i], i );

//	for( const auto& p: v_pac )
//		histo.assignToBin( p );

	return histo;
}

//---------------------------------------------------------------------
/// Input:  a vector or pairs, size=nb of points in data set
/// first value: the attribute value, second: the class value
std::vector<float>
getThresholds( const std::vector<PairAtvalClass>& v_pac, int nbBins )
{
	auto histo = buildHistogram( v_pac, nbBins );

	histo.print( std::cout );
	cout << "Start splitting, nb bins=" << histo.nbBins();

	for( auto it=histo.begin(); it!=histo.end(); it++ )

	bool splitOccured = false;
	int iter1 = 0;

	do
	{
		COUT << "iter1 " << iter1++ << '\n';
		int iter2 = 0;
		auto it = histo.begin();
		do
		{
			COUT << "iter2 " << iter2++ << '\n';
			splitOccured = histo.splitBin( it );
			it = std::next(it);
		}
		while( !splitOccured && it != std::end(it); );
	}
	while( splitOccured );

	std::vector<float> ret;
	return ret;
}

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
/*	vpac.push_back( std::make_pair( 0., ClassVal(1) ) );
	vpac.push_back( std::make_pair( 0.6, ClassVal(1) ) );
	vpac.push_back( std::make_pair( 0.7, ClassVal(1) ) );
	vpac.push_back( std::make_pair(   3, ClassVal(2) ) );*/
	auto vThres = getThresholds( vpac, 3 );
	std::cout << "Done !\n";
}

