#! /bin/sh
pdir=`dirname $0`
cd $pdir || exit 1
rm -rf flashtool/.idea
python3 -m zipapp flashtool -m flashtool:main

