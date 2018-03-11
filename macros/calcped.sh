#!/bin/bash

#ln -sf ~/spindisk/DigSig/macros/digsig_calcped.C .
#mkdir -p PED

for i in bbcmag????.root
do
  #cd $i
  echo root.exe -b -q digsig_calcped.C\(\"${i}\"\)
  root.exe -b -q digsig_calcped.C\(\"${i}\"\)
  #cd ..
done
