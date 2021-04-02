# dtcpp
Naive attempt of a Decision Tree implementation for continuous variables (WIP !!!)

* author: S. Kramm
* licence: GPL v3
* home page: https://github.com/skramm/dtcpp
* language: C++14


## WORK IN PROGRESS, NO RELEASES YET !

## current status 20210322:


WIP: histogram of classes vs trheshfold values

## Features

This sofware can train a decision tree using some input data.
The tree can then be used to classify some other data.
During the training step, it also analyses the input data and produces different output data files and plots.
 * a histogram of the classes found,
 * for each attribute, a histogram of the attribute values
 * for each attribute, a plot of the value vs. output class, also whowing the mean, meadian, and standard deviation value
 * a plot of the tree


* Input dataset format: csv style
 * class values: string or numerical (integer values), see [`-cs` option](#ss_clswitch)
 * attribute values: only numerical at present
 * class value position: either first or last element of the line
 * field separator character adjustable
 * decimal character for floating-point values can be either '`.`'' or '`,`', does not matter.
 * handles classless points: default behavior is to consider negative values as classless

### Training algorithm

The algorithm is more or less based on C4.5: https://en.wikipedia.org/wiki/C4.5_algorithm
At each step, it searches for the best attribute to use and best threshold on that attribute
so that a split will maximize the Gini Impurity coefficient:
https://en.wikipedia.org/wiki/Decision_tree_learning#Gini_impurity



### Performance scores

See https://en.wikipedia.org/wiki/Confusion_matrix for definitions.

* For two-class algorithms, the following scores are computed:
True Positive Rate (recall), True Negative Rate, Positive Predictive Value (precision),
Accuracy, Balanced Accuracy and F1-score<br>
see
* For multiclass tasks, only Macro recall and precision are computed at present

### Sources:

* https://www.kdnuggets.com/2020/01/decision-tree-algorithm-explained.html
* https://en.wikipedia.org/wiki/Decision_tree_learning
* https://en.wikipedia.org/wiki/C4.5_algorithm
* https://en.wikipedia.org/wiki/Information_gain_in_decision_trees


### Related software:

* https://rulequest.com/download.html : C5 algorithm

## Recommended tools

While not mandatory, this software will greatly benefit from some additional standard tools,
that are freely available on any OS/architecture:
* Gnuplot
* Graphviz


## Command-line usage

`$ dtcpp <switches> input_datafile`

<a name="ss_clswitch"></a>
### Switches

* `-cs` : class value is a string. Numerical indexes will be automatically associated to each string value.
* `-sep "x"` : use 'x' as field separator in the input file
* `-cf` : means class value is the **First** value of line
* `-cl` : means class value is the **Last** value of line (default)
* `-ll X` : sets log level to 'X'. Available levels: 0,1,2,3
* `-i` : load the datafile and prints info and stats on its contents, then exit (no training)
* `-nf x` : do training on 'x' folds of data
* `-nbh xx` : number of bins for building the histograms
* `-ro` : remove outliers before training
* `-md xx` : max depth for tree
* `-fl` : First line of input data file holds labels, ignore it
* `-sd` :  use sorting of points to find thresholds, to evaluate best split (default is histogram binning technique)

## Build information

This software is build from 2 files only:
 * dtcpp.h : header that hold all the useful code
 * dtcpp.cpp : basically only a command-line parser, then calls the code from the header


### Needed tools

* C++14 compiler
* GnuMake
* Doxygen

### Dependencies:

* Boost:
  * [Boost::graph](https://www.boost.org/doc/libs/1_75_0/libs/graph/doc/index.html)<br>
  * [Boost::histogram](https://www.boost.org/doc/libs/1_75_0/libs/histogram)
  * [Boost::bimap](https://www.boost.org/doc/libs/1_75_0/libs/bimap/)

 Tested successfully (march 2021) with releases 1.70 and 1.75

 **Warning**: 1.70 seems to be the minimal required version, and depending on you distribution,
 `sudo apt install libboost-all-dev` might install an older release.<br>
 Check  with: `cat /usr/include/boost/version.hpp`

* `argh`: command-line parser, from https://github.com/adishavit/argh<br>
(version 1.3.1 included for conveniency)

For the test build only:
* Catch2: https://github.com/catchorg/Catch2/
(tested with catch 2.13.4)


### Error handling

The default behavior on errors is to throw exceptions.
This enabling catching them.
The downside is that this might decrease performance.
If speed is an issue, you can define the symbol
`DTCPP_ERRORS_ASSERT`.
This will replace tests and `throw` instructions with classical assertions
(that will immediatly abort execution).
The main advantage is that you can remove all these tests by defining the symbol `NDEBUG`.

