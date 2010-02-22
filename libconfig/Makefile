include ../Makefile.rules

sources := $(wildcard *.c)
objects := $(patsubst %.c,%.o,$(sources)) libconfigcpp.o

all: ../lib/libconfig.so

../lib/libconfig.so: $(objects)
	$(LINK_SO) -o $@ $+

