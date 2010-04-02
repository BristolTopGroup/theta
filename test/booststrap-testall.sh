#!/bin/bash

#tmpdir=$(mktemp -d)
#logfile=$tmpdir/log.txt
#if [ $? -gt 0 ] || [ ! -d "$tmpdir" ]; then
#   echo "creating tmpdir failed!";
#   exit 1;
#fi
#cd $tmpdir
#svn --non-interactive --trust-server-cert co https://ekptrac.physik.uni-karlsruhe.de/svn/theta/trunk theta
#cd theta

rm -rf build-coverage
mkdir build-coverage
cd build-coverage

execute_checked cmake .. -Dcoverage:BOOL=ON

logfile=log.txt
test/testall.sh > $logfile 2>&1

#TODO: copy back logfile
valgrind --leak-check=full --show-reachable=yes test/test >> $logfile

lcov -c --directory build-coverage --output-file theta.info
genhtml theta.info --no-function-coverage -o doc/coverage

