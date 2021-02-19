
doc:
	@doxygen Doxyfile
	@xdg-open html/index.html

clean:
	@-rm html/*
