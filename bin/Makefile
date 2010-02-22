include ../Makefile.rules

all: merge theta

merge: merge.o
	@$(LINK_EXE) -o $@ $<

theta: theta.o
	@$(LINK_EXE) -o $@ $<

clean-subdir:
	@rm -f merge theta

