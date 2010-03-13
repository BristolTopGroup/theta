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

logfile=log.txt

cd ..

test/coveragetest.sh > $logfile 2>&1

for i in test/test-stat/*.sh; do
   i=`basename $i`
   echo "START `date +%s`;     `date -R`     $i" >> $logfile
   (cd test/test-stat; ./$i) 2>&1 | awk "{print \"[$i]\", \$_}" >> $logfile
   echo "END `date +%s`;     `date -R`     $i" >> $logfile
done

#TODO: copy back logfile
