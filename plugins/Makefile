include ../Makefile.rules

libsources = $(wildcard *.cpp)
libobjects = $(patsubst %.cpp,%.o,$(libsources))

all: ../lib/core-plugins.so

../lib/core-plugins.so: $(libobjects)
	$(LINK_SO) -o $@ $+
