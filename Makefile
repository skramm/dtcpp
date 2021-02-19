
doc:
	@doxygen Doxyfile
	@xdg-open html/index.html

test: build/bin/test_catch
	build/bin/test_catch

build/bin/test_catch: build/obj/test_catch.o
	$(CXX) -o build/bin/test_catch build/obj/test_catch.o -s

build/obj/test_catch.o: test_catch.cpp dtcpp.h
	$(CXX) -Wall -std=gnu++14 -fexceptions -O2 -c test_catch.cpp -o build/obj/test_catch.o

clean:
	@-rm html/*

