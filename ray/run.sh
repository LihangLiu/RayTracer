#!/bin/bash

mkdir output
rm output/*.bmp
dir=$1
for filename in $dir/*.ray; do
	fname=`basename $filename`
	ray -r 8  $dir/$fname output/$fname.bmp
done

