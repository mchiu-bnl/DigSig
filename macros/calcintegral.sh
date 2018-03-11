#!/bin/bash

cat scan.list | while read dirname
do
  cd ${dirname}
  ln -sf ~/spindisk/DigSig/macros/digsig_calcintegral.C .
  echo root.exe -b -q digsig_calcintegral.C\(\"dt5742.root\"\)
  root.exe -b -q digsig_calcintegral.C\(\"dt5742.root\"\)
  cd ..
done
