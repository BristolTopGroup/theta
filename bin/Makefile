include ../Makefile.options

deps := .bin/merge.d .bin/theta.d

include ../Makefile.rules

all: merge theta

ifeq ($(OPTIONS_BUILD_STATIC),yes)
   all: theta-static
endif


merge: .bin/merge.o
	@$(LINK_EXE) -o $@ $<

theta: .bin/theta.o
	@$(LINK_EXE) -o $@ $<

theta-static: .bin/theta.o
	$(LINK_EXE_STATIC) -o $@ $<

clean-subdir:
	@rm -f merge theta

