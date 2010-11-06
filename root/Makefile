ifeq ($(shell which root-config || echo -n A),A)
all:
	@echo -e \\nNOTE: no root-config found. Not building root-dependent stuff.\\n

include ../Makefile.rules

else

sources := $(wildcard *.cpp)
objects := $(patsubst %.cpp,.bin/%.o,$(sources))
deps := $(patsubst %.cpp,.bin/%.d,$(sources)) .bin/histos.d .bin/db2root.d

include ../Makefile.rules

EXTRA_COMPILE := $(shell root-config --cflags)
EXTRA_LINK_SO := $(shell root-config --libs) -lMinuit2 $(shell root-config --ldflags)
EXTRA_LINK_EXE := $(EXTRA_LINK_SO)

all: ../lib/root.so histos

histos: .bin/histos.o
	@$(LINK_EXE) -o $@ $<
	
db2root: .bin/db2root.o
	@$(LINK_EXE) -o $@ $<

../lib/root.so: $(objects)
	@$(LINK_SO) -o $@ $+

endif

#define clean target in any case:
clean-subdir:
	@rm -f histos db2root ../lib/root.so
