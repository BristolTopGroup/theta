include ../Makefile.rules

EXTRA_COMPILE := $(shell root-config --cflags)
EXTRA_LINK_SO := $(shell root-config --libs) -lMinuit2 $(shell root-config --ldflags)
EXTRA_LINK_EXE := $(EXTRA_LINK_SO)

sources := $(wildcard *.cpp)
objects := $(patsubst %.cpp,%.o,$(sources))

all: ../lib/root.so histos

histos: histos.o
	$(LINK_EXE) -o $@ $<

../lib/root.so: $(objects)
	$(LINK_SO) -o $@ $+

