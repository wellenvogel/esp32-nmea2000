#! /bin/sh
if [ "$1" = "" ] ; then
  echo "usage: $0 targetDir"
  exit 1
fi
while true
do 
  inotifywait -e modify -e create -e delete -r `dirname $0`
  echo sync
  rsync -rav --exclude=\*.swp --exclude=\*~ `dirname $0`/ $1

done
