# makefile for tdcpp

SHELL=bash

.PHONY: fdoc clean cleanall all show doc dot test

BIN_DIR=build/bin
OBJ_DIR=build/obj


SRC_FILES = $(wildcard *.cpp)

DOT_FILES = $(wildcard out/*.dot)

PNG_FILES = $(patsubst %.dot,%.png,$(DOT_FILES))
OBJ_FILES = $(patsubst %.cpp,$(OBJ_DIR)/%,$(SRC_FILES))
EXE_FILES = $(patsubst %.cpp,$(BIN_DIR)/%,$(SRC_FILES))
#----------------------------------------------
ifeq "$(NDEBUG)" ""
	NDEBUG=N
endif

ifeq ($(NDEBUG),Y)
	CFLAGS += -DNDEBUG
endif



all: $(BIN_DIR)/dectree
	@echo "done"

run: all
	$(BIN_DIR)/dectree sample_data/iris.data -sep "," -cs

# iris dataset
run2: all
	$(BIN_DIR)/dectree sample_data/iris.data -sep "," -cs -nf 5

run3: all
	$(BIN_DIR)/dectree sample_data/winequality-white.csv -sep ";" -cs -ll 2


show:
	@echo "OBJ_FILES=$(OBJ_FILES)"
	@echo "EXE_FILES=$(EXE_FILES)"
	@echo "DOT_FILES=$(DOT_FILES)"
	@echo "PNG_FILES=$(PNG_FILES)"

$(OBJ_DIR)/%.o: %.cpp dtcpp.h
	$(CXX) -Wall -std=gnu++14 $(CFLAGS) -fexceptions -O2 -Iother/ -c $< -o $@

$(BIN_DIR)/%:$(OBJ_DIR)/%.o
	$(CXX) -o $@ $< -s

doc:
	@- build/html/*
	@doxygen misc/Doxyfile 1>build/doxygen_stdout 2>build/doxygen_stderr
	@xdg-open build/html/index.html

test: build/bin/test_catch
	$(BIN_DIR)/test_catch

#$(BIN_DIR)/test_catch: build/obj/test_catch.o
#	$(CXX) -o $(BIN_DIR)/test_catch build/obj/test_catch.o -s

#build/obj/test_catch.o: test_catch.cpp dtcpp.h
#	$(CXX) -Wall -std=gnu++14 -fexceptions -O2 -c test_catch.cpp -o build/obj/test_catch.o

dot: $(PNG_FILES)

out/%.png:out/%.dot $(DOT_FILES)
	dot -Tpng $< >$@

cleandoc:
	@-rm build/html/search/*
	@-rm build/html/*

cleanout:
	@-rm out/*

clean: cleandoc
	@-rm $(OBJ_DIR)/*
	@-rm build/*
	@-rm *.dat
	@-rm *.dot
	@-rm *.png
	@-rm *.plt

cleanall: clean
	@-rm $(BIN_DIR)/*
