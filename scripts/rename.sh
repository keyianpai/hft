#!/bin/bash

if [ $# != 2 ] ; then echo "two parameters needed: for example ./rename.sh dev self_def" ; exit 1 ; fi

src=$1
dest=$2

if [ -e $dest ]; then echo "file existed for "$dest ; exit 1 ; fi

cp -rf $src $dest
cd $dest
file_set=`ls`

for file in $file_set
do
  new_file=${file/$src/$dest}
  echo $new_file
  mv $file $new_file
done
