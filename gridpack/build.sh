#!/bin/bash

# keep same directory structure: lib for plugins or theta-built shared objects and bin for
# binaries.
# add extlib for external libraries

[ -e ../bin/theta ] || { echo "../bin/theta not found. Compile theta and execute this script from within its directory!"; exit 1; }

rm -rf gridpack-tmp
mkdir -p gridpack-tmp/lib
mkdir -p gridpack-tmp/bin
mkdir -p gridpack-tmp/extlib
mkdir -p gridpack-tmp/etc/plugins

cp scripts/theta gridpack-tmp/bin/
cp ../bin/theta gridpack-tmp/bin/theta.exe
cp ../lib/*.so gridpack-tmp/lib/
if [ ! -z "$ROOTSYS" ]; then
   cp -r $ROOTSYS/etc/plugins/TVirtualStreamerInfo gridpack-tmp/etc/plugins/
   cp $ROOTSYS/etc/system.rootrc gridpack-tmp/etc/
fi

# add the dependencies:
scripts/copydeps.py gridpack-tmp/bin/theta.exe gridpack-tmp/extlib gridpack-tmp/lib
for plugin in gridpack-tmp/lib/*.so; do
   scripts/copydeps.py $plugin gridpack-tmp/extlib gridpack-tmp/lib gridpack-tmp/extlib
done

rm -f gridpack.tgz
cd gridpack-tmp
tar zcf ../gridpack.tgz *

