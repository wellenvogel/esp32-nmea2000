#! /bin/sh
i=0
[ "$2" != "" ] && i=$2
if [ "$1" = "" ] ; then
  echo "usage: $0 num [start]"
  exit 1
fi
num=$1

while [ $num -gt 0 ]
do
echo '{"name": "XDR'$i'","label": "XDR'$i'","type": "xdr","default": "", "check": "checkXDR","category":"xdr'$i'"},'
num=`expr $num - 1`
i=`expr $i + 1`
done