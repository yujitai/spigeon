#!/bin/bash
#author@yujitai
#date@20190820
#function@build zframework

set -ex
buildmake=$HOME/tools/buildmake/buildmake

# Build zframework and deps
echo "buildmake: Create Makefile"
eval $buildmake 
make -C deps || exit 1
rm -rf output && rm -f libzframework.a
make clean && make -j8 || exit 1

# Merge libzframework.a and it's dep libs
./libmerger.sh libzframework.a \
             libzframework.a \
             deps/new-config/libconfig.a \
             deps/libev-4.11/.libs/libev.a \
             deps/evhttpclient/libevhttpclient.a 

# Copy headers
rm output/include/zframework/*.h
mkdir -p output/include/zframework/server \
         output/include/zframework/client \
         output/include/zframework/util 
cp src/server/*.h output/include/zframework/server
cp src/client/*.h output/include/zframework/client
cp src/util/*.h output/include/zframework/util
cp src/*.h output/include/zframework

