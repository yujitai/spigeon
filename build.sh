#!/bin/bash
set -ex

buildmake=$HOME/tools/buildmake/buildmake

noclean="0"
if [[ $# -gt 0 && "$1" == "noclean" ]]; then
    noclean="1"
fi

function build {
    if [[ ! -z ${SCMPF_MODULE_VERSION} ]]; then
        # build on scm
        # since scm doesn't support comake2, what we can do 
        # is only try to make, no matter Makefile exists or not
        make || exit 1
    elif [[ ! -f Makefile ]]; then
        # no Makefile, maybe first checkout from git
        echo "build Makefile"
        eval $(buildmake) && make clean && make || exit 1
        # FIXME: comake2 failed because dependency not found
    else
        # check md5 of COMAKE
        a=$(cat Makefile | grep '^BUILDMAKE_MD5=' | sed 's/BUILDMAKE_MD5=//')
        b=$(md5sum BUILDMAKE)
        if [[ "$a" != "$b" ]]; then
            echo "rebuild Makefile"
            eval $(buildmake) || exit 1
        fi

        [[ "$noclean" == "0" ]] && make clean
        make || exit 1
    fi
}

#build deps
[[ "$noclean" == "0" ]] && make -C deps clean
make -C deps || exit 1

#build protobuf
LD_LIBRARY_PATH=./deps/protobuf/src/.libs ./deps/protobuf/src/.libs/protoc src/command/*.proto --cpp_out=. || exit 1

rm -rf output

#build store-framework
build

# merge all .a into a single one
rm -rf output/lib/libstoreframework-partial.a
rm -rf libstoreframework.a
./merge-lib.sh libstoreframework.a \
             libstoreframework-partial.a \
             deps/libev-4.11/.libs/libev.a \
             deps/new-config/libconfig.a \
             deps/evhttpclient/libevhttpclient.a \
             deps/protobuf/src/.libs/libprotobuf.a

# copy lib
mkdir output/lib -p
mv libstoreframework.a output/lib
rm libstoreframework-partial.a

# 新版buildmake会拷贝头文件到output/include目录
# 暂时先兼容新版buildmake
rm output/include/*.h

# copy headers
mkdir -p output/include output/include/server output/include/util output/include/db output/include/inc output/include/client output/include/engine
cp src/server/*.h output/include/server/
cp src/util/*.h output/include/util/
cp src/inc/*.h output/include/inc/
cp src/engine/*.h output/include/engine/
cp src/client/*.h output/include/client/
cp src/db/db_define.h output/include/db/
cp src/*.h output/include/


