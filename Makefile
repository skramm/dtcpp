# makefile for tdcpp

SHELL=bash

.PHONY: fdoc clean cleanall all show doc dot test check touch

BIN_DIR=build/bin
OBJ_DIR=build/obj

APPNAME=dectree

SRC_FILES = $(wildcard *.cpp)
HEADERS = $(wildcard *.h*)

DOT_FILES = $(wildcard out/*.dot)

PLOT_IN_FILES = $(wildcard out/*.plt)

PNG_DOT_FILES = $(patsubst %.dot,%.png,$(DOT_FILES))
OBJ_FILES = $(patsubst %.cpp,$(OBJ_DIR)/%,$(SRC_FILES))
EXE_FILES = $(patsubst %.cpp,$(BIN_DIR)/%,$(SRC_FILES))
PLOT_OUT_FILES= $(patsubst out/%.plt,out/%.png,$(PLOT_IN_FILES))

#----------------------------------------------
# MAKEFILE OPTIONS

#----------------------------------------------
ifeq "$(NDEBUG)" ""
	NDEBUG=N
endif

ifeq ($(NDEBUG),Y)
	CFLAGS += -DNDEBUG
endif

#----------------------------------------------
ifeq "$(DEBUG)" ""
	DEBUG=N
endif

ifeq ($(DEBUG),Y)
	CFLAGS += -DDEBUG
endif

#----------------------------------------------
ifeq "$(DEBUGS)" ""
	DEBUGS=N
endif

ifeq ($(DEBUGS),Y)
	CFLAGS += -DDEBUG_START -DDEBUG
endif

#----------------------------------------------
ifeq "$(HMV)" ""
	HMV=N
endif

ifeq ($(HMV),Y)
	CFLAGS += -DHANDLE_MISSING_VALUES
endif

#----------------------------------------------
ifeq "$(HO)" ""
	HO=N
endif

ifeq ($(HO),Y)
	CFLAGS += -DHANDLE_OUTLIERS
endif

#----------------------------------------------
# MAKEFILE TARGETS

all: $(BIN_DIR)/dectree
	@echo "done"

check:
	cppcheck . --enable=all 2>cppcheck.log
	xdg-open cppcheck.log

show:
	@echo "OBJ_FILES=$(OBJ_FILES)"
	@echo "EXE_FILES=$(EXE_FILES)"
	@echo "DOT_FILES=$(DOT_FILES)"
	@echo "PNG_FILES=$(PNG_DOT_FILES)"
	@echo "PLOT_OUT_FILES=$(PLOT_OUT_FILES)"
	@echo "PLOT_IN_FILES=$(PLOT_IN_FILES)"

touch:
	touch dtcpp.h histac.hpp

$(OBJ_DIR)/%.o: %.cpp $(HEADERS)
	$(CXX) -Wall -std=gnu++14 $(CFLAGS) -fexceptions -O2 -Iother/ -c $< -o $@

$(BIN_DIR)/%:$(OBJ_DIR)/%.o
	$(CXX) -o $@ $< -s

doc: cleandoc
	@echo "Doxygen version: $$(doxygen --version)" >build/doxygen_stdout
	@doxygen misc/Doxyfile 1>>build/doxygen_stdout 2>build/doxygen_stderr
	@xdg-open build/html/index.html

testb:
	@echo "Testing build for all possible configurations"
	@echo "App size for different build options" >build/app_size.txt
	@echo "build test" >build/make_stdout
	@echo "build test" >build/make_stderr
#
	@opt="HO=Y HMV=Y"; \
	make cleanbin; \
	echo -e "\n***** Build options: $$opt" >>build/make_stdout; \
	make $${opt} 1>>build/make_stdout 2>>build/make_stderr; \
	echo "$$opt: $$(stat --printf="%s" $(BIN_DIR)/$(APPNAME))" >> build/app_size.txt
#
	@opt="HO=Y HMV=N"; \
	make cleanbin; \
	echo -e "\n***** Build options: $$opt" >>build/make_stdout; \
	make $${opt} 1>>build/make_stdout 2>>build/make_stderr; \
	echo "$$opt: $$(stat --printf="%s" $(BIN_DIR)/$(APPNAME))" >> build/app_size.txt
#
	@opt="HO=N HMV=Y"; \
	make cleanbin; \
	echo -e "\n***** Build options: $$opt" >>build/make_stdout; \
	make $${opt} 1>>build/make_stdout 2>>build/make_stderr; \
	echo "$$opt: $$(stat --printf="%s" $(BIN_DIR)/$(APPNAME))" >> build/app_size.txt
#
	@opt="HO=N HMV=N"; \
	make cleanbin; \
	echo -e "\n***** Build options: $$opt" >>build/make_stdout; \
	make $${opt} 1>>build/make_stdout 2>>build/make_stderr; \
	echo "$$opt: $$(stat --printf="%s" $(BIN_DIR)/$(APPNAME))" >> build/app_size.txt

# add --success to see successful tests
test: build/bin/test_catch
	$(BIN_DIR)/test_catch

#$(BIN_DIR)/test_catch: build/obj/test_catch.o
#	$(CXX) -o $(BIN_DIR)/test_catch build/obj/test_catch.o -s

#build/obj/test_catch.o: test_catch.cpp dtcpp.h
#	$(CXX) -Wall -std=gnu++14 -fexceptions -O2 -c test_catch.cpp -o build/obj/test_catch.o

plt: $(PLOT_OUT_FILES)
	@cp misc/out_style.css out/

out/%.png:out/%.plt
	@chmod u+x out/*.plt
	@echo "-processing file $<"
	@cd out; ./$(notdir $<) 1>> gnuplot.stdout 2>>gnuplot.stderr

dot: $(PNG_DOT_FILES)

out/%.png:out/%.dot $(DOT_FILES)
	@dot -Tpng $< >$@

cleandoc:
	@-rm build/html/search/*
	@-rm build/html/*

cleanout:
	@-rm out/*


clean: cleandoc
	@-rm $(OBJ_DIR)/*
	@-rm build/*

#	@-rm *.dat
#	@-rm *.dot
#	@-rm *.png
#	@-rm *.plt

cleanall: clean cleanout cleanbin

cleanbin:
	@-rm $(BIN_DIR)/*
