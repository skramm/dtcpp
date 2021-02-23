# dtcpp
Naive attempt of a Decision Tree implementation for continuous variables

* author: S. Kramm
* licence: GPL v3
* language: C++14 (but might be ok with C++11, untested)


This software can be used
* either as a classification program
* either as a library, by including the file dtcpp.h in your user code


## Build information

### dependencies:
* for the library, the only dependency is boost graph<br>
=> `sudo apt install libboost-all-dev`

* For the demo classifier program:
`argh`: command-line parser, get it on https://github.com/adishavit/argh
<br>
tested with v1.3.1:
https://github.com/adishavit/argh/releases/tag/v1.3.1

* For the test project:
catch2: https://github.com/catchorg/Catch2/
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

