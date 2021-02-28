# dtcpp
Naive attempt of a Decision Tree implementation for continuous variables (WIP !!!)

* author: S. Kramm
* licence: GPL v3
* home page: https://github.com/skramm/dtcpp
* language: C++14


This software can be used
* either as a classification program
* either as a library, by including the file dtcpp.h in your user code

## Features
* input dataset format: csv style
 * class values: string or numerical (integer values)
 * attribute values: only numerical at present
 * class value position: either first or last element of the line
 * field separator character adjustable

## Recommended tools

While not mandatory, this software will greatly benefit from some additional standard tools,
that are freely available on any OS/architecture:
* Gnuplot
* Graphviz

For those who intent to dive into the code, you will also need (besides a C++14 compiler...):
* GnuMake
* Doxygen

## Command-line

`$ dtcpp <switches> input_datafile`

### Swiches

* `-cs` : means class value is a string
* `-sep "x"` : use 'x' as field separator in the input file
* `-cf` : means class value is the **First** value of line
* `-cl` : means class value is the **Last** value of line (default)
* `-ll X` : sets log level to 'X'. Available levels: 0,1,2,3
* `-i` : load the datafile and prints Info on its contents, then exit (no processing)
* `-f` : do Folding on the provided data
* `-nb xx` : number of bins for building the histogram analysis (useful only for app `datanalylis`)
* `-ro` : remove outliers before training

## Build information

### Dependencies:

* for the library, the only dependency is Boost. Two libs are used:
 * [Boost::Graph](https://www.boost.org/doc/libs/1_75_0/libs/graph/doc/index.html)<br>
 * [Boost::histogram](https://www.boost.org/doc/libs/1_75_0/libs/histogram)

If you don't have Boost on your system, the easiest will be => `sudo apt install libboost-all-dev`

* For the demo classifier program:
 * `argh`: command-line parser, from https://github.com/adishavit/argh<br>
(version 1.3.1 included for conveniency)

* For the test build:
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

