include ../Makefile.rules

all: true_q

true_q: true_q.o
	@$(LINK_EXE_NOLIBS) -o $@ $<

clean-subdir:
	@rm -f theta_q
	@rm -f *.db counting-nobkg.cfg
