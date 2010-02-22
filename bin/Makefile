include ../Makefile.rules

#sources = $(wildcard *.cpp)
#exes = $(patsubst %.cpp,%.exe,$(sources))

all: merge theta

merge: merge.o
	$(LINK_EXE) -o $@ $<

theta: theta.o
	$(LINK_EXE) -o $@ $<

#	$(LINK_EXE) -o $@ $+

