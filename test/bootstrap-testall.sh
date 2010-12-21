#!/bin/bash

curr_dir=$PWD
tmpdir=$(mktemp -d)
logfile=$tmpdir/log.txt

fail()
{
 echo "FAIL: " $* | tee -a $logfile
 cp $logfile $curr_dir
 exit 1
}

if [ $? -gt 0 ] || [ ! -d "$tmpdir" ]; then
   fail creating tempdir
fi
echo Using directory $tmpdir ...
cd $tmpdir
svn --non-interactive --trust-server-cert co https://ekptrac.physik.uni-karlsruhe.de/svn/theta/trunk theta &> /dev/null
[ $? -eq 0 ] || { fail svn co }
cd theta

{
make &> /dev/null
[ $? -eq 0 ] || fail make
test/testall.sh || fail test
} &> $logfile

echo SUCCESS

#TODO: copy back logfile
#valgrind is not run because ROOT spoils it all ...
#valgrind --leak-check=full --show-reachable=yes test/test >> $logfile

#lcov -c --directory build-coverage --output-file theta.info
#genhtml theta.info --no-function-coverage -o doc/coverage

