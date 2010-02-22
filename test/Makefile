include ../Makefile.rules

sources = $(wildcard *.cpp)
objects = $(patsubst %.cpp,%.o,$(sources))

EXTRA_LINK_EXE = -lboost_unit_test_framework

all: test test-ex.so

test-ex.so: test-ex.o
	@$(LINK_SO) -o $@ $<

test: $(objects)
	@$(LINK_EXE) -o $@ $+

clean-subdir:
	@rm -f test-ex.so test

