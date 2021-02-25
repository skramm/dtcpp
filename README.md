# dtcpp
Naive attempt of a Decision Tree implementation for continuous variables (WIP !!!)

* author: S. Kramm
* licence: GPL v3
* home page: https://github.com/skramm/dtcpp
* language: C++14 (but might be ok with C++11, untested)


This software can be used
* either as a classification program
* either as a library, by including the file dtcpp.h in your user code

## Features
* input dataset format: csv style
 * class values: string or numerical (integer values)
 * class value position: either first or last element of the line
 * field separator character adjustable


## Build information

### Dependencies:
* for the library, the only dependency is [Boost Graph](https://www.boost.org/doc/libs/1_75_0/libs/graph/doc/index.html)<br>
Isn't provided, if not installed => `sudo apt install libboost-all-dev`

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

