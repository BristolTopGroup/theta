include ../Makefile.rules

ifeq ($(shell which root-config || echo -n A),A)
all:
		@echo -e \\nNOTE: no root-config found. Not building root-dependent stuff.\\n

else

EXTRA_COMPILE := $(shell root-config --cflags)
EXTRA_LINK_SO := $(shell root-config --libs) -lMinuit2 $(shell root-config --ldflags)
EXTRA_LINK_EXE := $(EXTRA_LINK_SO)

sources := $(wildcard *.cpp)
objects := $(patsubst %.cpp,%.o,$(sources))

all: ../lib/root.so histos

histos: histos.o
	@$(LINK_EXE) -o $@ $<
	
db2root: db2root.o
	@$(LINK_EXE) -o $@ $<

../lib/root.so: $(objects)
	@$(LINK_SO) -o $@ $+

clean-subdir:
	@rm -f histos db2root ../lib/root.so

endif
