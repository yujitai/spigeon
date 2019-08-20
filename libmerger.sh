#!/bin/sh
#author@yujitai
#date@20190820
#function@merge libs

set -eu

if [ $# -lt 3 ]; then
    echo 'usage:sh libmerger.sh libtarget.a libsource1.a [libsource2.a ...]'
    exit -1;
fi

libtarget=$1
shift

tmpdir=./tmp_libmerger.$$
mkdir -p $tmpdir

cp $* $tmpdir && cd $tmpdir
for i in *.a
do
    ar x $i
done
ar crs $libtarget *.o && cp $libtarget .. && cd -

rm -rf $tmpdir

