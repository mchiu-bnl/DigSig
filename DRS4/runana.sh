#!/usr/bin/env bash

if [[ $# -lt 1 ]]
then
  echo "Usage: runana.sh <datfiles...>"
  exit -1
fi

for datfile in $*
do
  echo root.exe -b -q run_ana_drs4.C\(\"$datfile\"\)
  root.exe -q run_ana_drs4.C\(\"$datfile\",0\)
done
