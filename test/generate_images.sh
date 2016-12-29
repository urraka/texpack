#!/bin/bash

# this script uses ImageMagick to create a bunch of images

mkdir -p input

for i in {1..20}
do
	size=$(( ( ( RANDOM % 6 ) + 1 ) * 50 ))
	convert -background transparent -fill yellow -font Arial -pointsize $size label:$i input/N$i.png
done
