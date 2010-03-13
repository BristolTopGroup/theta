#!/bin/bash

#TODO: complete ...

. ../lib.sh

#for Theta in 5.0 10000.0; do
for Theta in 10000.0; do
    sed "s/__THETA__/${Theta}/g" counting-nobkg.cfg.tpl > counting-nobkg.cfg
    exec_theta counting-nobkg-deltanll_intervals.cfg
done
