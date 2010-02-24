include ../Makefile.rules

sources = $(wildcard *.cpp)
objects = $(patsubst %.cpp,%.o,$(sources))

plg_sources = $(wildcard *.cxx)
plg_objects = $(patsubst %.cxx,%.o,$(plg_sources))

EXTRA_LINK_EXE = -lboost_unit_test_framework$(BS)

all: test test-ex.so

test-ex.so: $(plg_objects)
	$(LINK_SO) -o $@ $+

test: $(objects)
	@$(LINK_EXE) -o $@ $+

clean-subdir:
	@rm -f test-ex.so test

