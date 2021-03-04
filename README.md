# dtcpp
Naive attempt of a Decision Tree implementation for continuous variables (WIP !!!)

* author: S. Kramm
* licence: GPL v3
* home page: https://github.com/skramm/dtcpp
* language: C++14


## WORK IN PROGRESS, NO RELEASES YET !


## Features
* input dataset format: csv style
 * class values: string or numerical (integer values)
 * attribute values: only numerical at present
 * class value position: either first or last element of the line
 * field separator character adjustable
 * decimal character for floating-point values can be either '`.`'' or '`,`', does not matter.

## Recommended tools

While not mandatory, this software will greatly benefit from some additional standard tools,
that are freely available on any OS/architecture:
* Gnuplot
* Graphviz


## Command-line

`$ dtcpp <switches> input_datafile`

### Switches

* `-cs` : means class value is a string
* `-sep "x"` : use 'x' as field separator in the input file
* `-cf` : means class value is the **First** value of line
* `-cl` : means class value is the **Last** value of line (default)
* `-ll X` : sets log level to 'X'. Available levels: 0,1,2,3
* `-i` : load the datafile and prints info and stats on its contents, then exit (no training)
* `-nf x` : do training on 'x' folds of data
* `-nbh xx` : number of bins for building the histograms
* `-ro` : remove outliers before training
* `-md xx` : max depth for tree
* `-fl` : First line of input data file holds labels,ignore it

## Build information

### Needed tools

* C++14 compiler
* GnuMake
* Doxygen

### Dependencies:

* Boost. If you don't have Boost on your system, the easiest will be => `sudo apt install libboost-all-dev`<br>
The following libs are used:
 * [Boost::Graph](https://www.boost.org/doc/libs/1_75_0/libs/graph/doc/index.html)<br>
 * [Boost::histogram](https://www.boost.org/doc/libs/1_75_0/libs/histogram)
 * [Boost::bimap](https://www.boost.org/doc/libs/1_75_0/libs/bimap/)


 * `argh`: command-line parser, from https://github.com/adishavit/argh<br>
(version 1.3.1 included for conveniency)

For the test build:
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

