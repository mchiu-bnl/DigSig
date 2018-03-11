#!/bin/bash

for fname in $*
do
  root.exe -b -q digsig_calcintegral.C\(\"${fname}\"\)
done
