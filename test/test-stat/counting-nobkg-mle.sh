#!/bin/bash


. ../lib.sh

#A. use 5.0 as signal mean:
sed 's/__THETA__/5.0/g' counting-nobkg.cfg.tpl > counting-nobkg.cfg
exec_theta counting-nobkg-mle.cfg
#the estimated and produced number of events have to agree up to 10^-4, so select all that don't fulfill this:
num_fail=$(sqlite3 counting-nobkg-mle.db "SELECT COUNT(*) FROM products WHERE abs(mle__Theta - writer__n_events_o) > 0.0005;")
[ "$?" -ne 0 ] && fail "sqlite3 execution returned error"
[ "$num_fail" -ne 0 ] && fail "maximum likelihood estimate off too much in $num_fail cases"
rm -f counting-nobkg-mle.db
rm -f counting-nobkg-mle.cfg

pass $0 "Theta = 5.0"

sed 's/__THETA__/10000.0/g' counting-nobkg.cfg.tpl > counting-nobkg.cfg
exec_theta counting-nobkg-mle.cfg
num_fail=$(sqlite3 counting-nobkg-mle.db "SELECT COUNT(*) FROM products WHERE abs(mle__Theta - writer__n_events_o) > 1.0;")
[ "$?" -ne 0 ] && fail "sqlite3 execution returned error"
[ "$num_fail" -ne 0 ] && fail "maximum likelihood estimate off too much in $num_fail cases"
#here, one more test for the error estimate of the mle method:
num_fail=$(sqlite3 counting-nobkg-mle.db "SELECT COUNT(*) FROM products WHERE abs(mle__Theta_error * mle__Theta_error - mle__Theta) > 1.0;")

rm -f counting-nobkg-mle.db
rm -f counting-nobkg-mle.cfg

pass $0 "Theta = 1000.0"
