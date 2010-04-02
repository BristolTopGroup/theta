#!/bin/bash

#run this from the theta main directory!

[ -d test ] || { echo "not in theta root directory!"; exit 1; }

. test/lib.sh

execute_checked test/test

for i in test/test-stat/*.py; do
   i=`basename $i`
   echo "START `date +%s`;     `date -R`     $i"
   (cd test/test-stat; ./$i) 2>&1 | awk "{print \"[$i]\", \$_}"
   echo "END `date +%s`;     `date -R`     $i"
done

