#!/bin/bash
#author@yujitai
#date@20190820
#function@build zframework

set -ex
buildmake=$HOME/tools/buildmake/buildmake

# Build zframework and deps
echo "buildmake: Create Makefile"
eval $buildmake 
# make -C deps clean 
make -C deps || exit 1
rm -rf output && rm -f libzframework.a
make clean && make -j8 || exit 1

# Merge libzframework.a and it's dep libs
./libmerger.sh libzframework.a \
             libzframework.a \
             deps/new-config/libconfig.a \
             deps/libev-4.11/.libs/libev.a \
             deps/evhttpclient/libevhttpclient.a \
             deps/jemalloc_stable4/lib/libjemalloc.a
echo "done!"

