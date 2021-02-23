

BIN_DIR=build/bin
OBJ_DIR=build/obj

SRC_FILES = $(wildcard *.cpp)
DOT_FILES = $(wildcard *.dot)

PNG_FILES = $(patsubst %.dot,%.png,$(DOT_FILES))

OBJ_FILES = $(patsubst %.cpp,$(OBJ_DIR)/%,$(SRC_FILES))

all: $(BIN_DIR)/main
	@echo "done"

# default run
run: all
	$(BIN_DIR)/main -f

# iris dataset
run2: all
	$(BIN_DIR)/main ~/data/iris.data -sep , -cs -f


show:
	@echo "OBJ_FILES=$(OBJ_FILES)"
	@echo "EXEC_FILES=$(EXEC_FILES)"
	@echo "DOT_FILES=$(DOT_FILES)"
	@echo "PNG_FILES=$(PNG_FILES)"

$(OBJ_DIR)/%.o: %.cpp dtcpp.h $(SRC_FILES)
	$(CXX) -Wall -std=gnu++14 -fexceptions -O2 -c $< -o $@

$(BIN_DIR)/%:$(OBJ_DIR)/%.o
	$(CXX) -o $@ $< -s

doc:
	@doxygen Doxyfile 1>build/doxygen_stdout 2>build/doxygen_stderr
	@xdg-open html/index.html

test: build/bin/test_catch
	$(BIN_DIR)/test_catch

#$(BIN_DIR)/test_catch: build/obj/test_catch.o
#	$(CXX) -o $(BIN_DIR)/test_catch build/obj/test_catch.o -s

#build/obj/test_catch.o: test_catch.cpp dtcpp.h
#	$(CXX) -Wall -std=gnu++14 -fexceptions -O2 -c test_catch.cpp -o build/obj/test_catch.o

dot: $(PNG_FILES)

%.png:%.dot $(DOT_FILES)
	dot -Tpng $< >$@

clean:
	@-rm html/*
	@-rm $(OBJ_DIR)/*

cleanall: clean
	@-rm $(BIN_DIR)/*
