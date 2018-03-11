#!/bin/sh

for i in *80
do
  cd $i
  pwd
  for j in wave_[1-9].txt wave_1?.txt
  do
    cp /dev/null $j
  done
  cd ..
done
