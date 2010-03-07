#!/bin/bash

#TODO: complete ...

fail()
{
   echo "FAILED: $*";
   exit 1;
}

pass()
{
    echo "PASSED $*";
}

exec_theta()
{
    ../bin/theta -q $*
    [ "$?" -gt 0 ] && fail "theta returned error"
}

#for Theta in 5.0 10000.0; do
for Theta in 10000.0; do
    sed "s/__THETA__/${Theta}/g" counting-nobkg.cfg.tpl > counting-nobkg.cfg
    exec_theta counting-nobkg-deltanll_intervals.cfg
done
