#!/bin/sh
set -eu

if [ $# -lt 2 ]; then 
    echo 'usage: ./merge-lib.sh target.a source1.a [source2.a ...]'
    exit -1;
fi

tmpdir=./tmp.merge-lib.$$
mkdir -p $tmpdir

target=$1
shift

function decompress_all {
    for i in *.a; do
        ar x $i;
    done
}

cp $@ $tmpdir && cd $tmpdir && decompress_all && ar crs $target *.o && cp $target .. && cd -

rm -rf $tmpdir
