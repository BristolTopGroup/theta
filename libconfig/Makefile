include ../Makefile.options

sources := $(wildcard *.c)
objects := $(patsubst %.c,.bin/%.o,$(sources)) .bin/libconfigcpp.o
deps := $(patsubst %.o,%.d,$(objects))

include ../Makefile.rules

all: ../lib/libconfig.so

ifeq ($(OPTIONS_BUILD_STATIC),yes)
all: ../lib/libconfig.a
endif

../lib/libconfig.so: $(objects)
	@$(LINK_SO) -o $@ $+

../lib/libconfig.a: $(objects)
	@$(CREATE_A) $@ $+

clean-subdir:
	@rm -f ../lib/libconfig.so
