#!/bin/bash

rm -rf input2
mkdir -p input2

count=30
min=50
max=200
colors=(blue yellow green red tomato steelblue black white grey)
ncolors=${#colors[@]}

for ((i = 0; i < count; i++))
do
	w=$(( RANDOM % ( max - min ) + min ))
	h=$(( RANDOM % ( max - min ) + min ))
	a=$(( RANDOM % ncolors ))
	b=$(( RANDOM % ncolors ))
	color=${colors[a]}-${colors[b]}
	
	convert -size ${w}x${h} xc: -channel G \
			-fx '(1-(2*i/w-1)^4)*(1-(2*j/h-1)^4)' \
			-separate +channel plasma:${color} \
			-compose multiply \
			-composite \
			input2/image-${i}.png
done
