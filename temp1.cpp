
//#include <boost/histogram.hpp>

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
//using PairAtvalClass = std::pair<float,ClassVal>;
using PairAtvalClass = std::pair<float,ClassVal>;

struct MyHistogram;
//---------------------------------------------------------------------
/// A histogram bin for MyHistogram, holds an occurrence counter and a class counter
struct MyBin
{
	friend struct MyHistogram;
//	static const MyHistogram* p_histo;

//	size_t                    _counter = 0u;
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
	size_t size() const { return _vIdxPt.size(); }
};


//---------------------------------------------------------------------
/// Variable bin-size histogram
struct MyHistogram
{
	const std::vector<PairAtvalClass>* p_src = 0;  ///< pointer on source data
	std::list<MyBin> _lBins;
	double _step;
	double _vmin;

	MyHistogram( const std::vector<PairAtvalClass>& src, size_t nbBins, float vmin, float vmax )
		: p_src( &src )
		, _vmin(vmin)
	{
		_step = (vmax - vmin) / nbBins;
		_lBins.resize( nbBins );

		int i = 0;
		for( auto& b: _lBins )
			b._startValue = vmin + i * _step;
	}

	const auto begin() const { return _lBins.begin(); }
	const auto end()   const { return _lBins.end();   }
	auto begin() { return _lBins.begin(); }
	auto end()   { return _lBins.end();   }

	PairAtvalClass getPoint(size_t idx) const
	{
		assert( p_src );
		return p_src->at(idx);
	}
	void splitBin( decltype( _lBins.begin() ) );

	void assignToBin( const PairAtvalClass& pac, size_t idx )
	{
		auto val = (pac.first - _vmin) / _step;
		cout <<"b=" << val << " bin=" << std::trunc( val ) << '\n';

		bool match = false;
		auto it_prev = _lBins.begin();

		for( auto it=_lBins.begin(); it!=_lBins.end(); it++ )    // iterate the bins, except the last one
		{
			match = false;
			auto& bin = *it;
			if( pac.first >= bin._startValue )
				match = true;

			if( match == false )
			{
				auto& bin_prev = *it_prev;
				bin_prev._vIdxPt.push_back( idx );
				bin_prev._mClassCounter[pac.second]++;
			}

			it_prev = it;
		}
	}
};


//---------------------------------------------------------------------
void
MyHistogram::splitBin( decltype( _lBins.begin() ) it )
{
	auto& bin = *it;
	auto& it_next = std::next(it);
	cout << "-start split bin at " << bin._startValue << '\n';
	if( bin.isSplittable() )
	{
		cout << " -splittable, bin has " << bin.size() << " pts\n";
		auto v1 = bin._startValue;
		auto v2 = v1 + _step;
		auto start2 = (v1+v2) / 2.;
		MyBin newBin;
		for( const auto id: bin._vIdxPt )
		{
			auto pt = getPoint(id);
			if( pt.first >= start2 )
		}

		_lBins.insert( newBin );
	}
	else
		cout << " -NOT splittable!\n";

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

	cout << "Start splitting\n";
	for( auto it=histo.start(); it!=histo.end(); it++ )
	{
		histo.splitBin( it );
	}
	std::vector<float> ret;
	return ret;
}

int main()
{
	std::cout << "Hi!\n";

	std::vector<PairAtvalClass> vpac;
	vpac.push_back( std::make_pair( 0.5, ClassVal(1) ) );
	vpac.push_back( std::make_pair( 0.6, ClassVal(1) ) );
	vpac.push_back( std::make_pair(   3, ClassVal(2) ) );
	auto vThres = getThresholds( vpac, 3 );
	std::cout << "Done !\n";
}



#if 0
/*	struct my_axis {
		axis::index_type index(const std::tuple<double, double>& point) const
		{
      const auto x = std::get<0>(point);
      const auto y = std::get<1>(point);
      return x * x + y * y <= 1.0;
    }
*/
	auto nbBins = 10;
	auto h = boost::histogram::make_histogram(
		boost::histogram::axis::regular<>(
			nbBins,
			0., 10.
		)
	);

	h( 5.3 );
	h( 5.3 );
	h( 6.3 );

	cout << "bins:"
		<< "\n0=" << h.at(0)
		<< "\n5=" << h.at(5)
		<< "\n6=" << h.at(6)
		<< '\n';
#endif

#if 0
	cout << "BOOST_VERSION=" << BOOST_VERSION << '\n';
/*	struct MyType: accumulators::count<>
	{
// what is missing here ?
		std::map<int,size_t> counter;
	};

	const auto axis = axis::regular<>(3, 0.0, 1.0);
	auto h = make_histogram_with(dense_storage<MyType>(), axis);

	h(0.15, sample(2));
	h(0.14, sample(2));
	h(0.16, sample(3));

	cout
		<< "bin 0: c2=" << h.at(0)->counter[2]   // should print 2
		<< "bin 0: c3=" << h.at(0)->counter[3]   // should print 1
		<< '\n';
		*/
#endif


