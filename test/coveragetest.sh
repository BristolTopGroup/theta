#!/bin/bash

#run this from the theta main directory!

[ -d test ] || { echo "not in theta root directory!"; exit 1; }

. test/lib.sh

#rm -rf build-coverage
#mkdir build-coverage
cd build-coverage

execute_checked cmake .. -Dcoverage:BOOL=ON
execute_checked make
cd ..
execute_checked test/test
