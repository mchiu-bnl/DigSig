#!/bin/sh
#
# condor executes runana.cmd 0, runana.cmd 1, etc.
#

if [[ $# -lt 1 ]]
then
  echo "usage: dt5742_toroot.cmd <n>"
  exit -1
fi

echo PWD=${PWD}
echo LD_LIBRARY_PATH=${LD_LIBRARY_PATH}
echo HOST=`hostname`

ulimit -c 0     # no core files

# get directory
let line=$1+1
dirname=`sed -n "${line}p" scan.list`

TOPDIR=${PWD}

cd ${dirname}
if [[ $? -ne 0 ]]
then
  echo "error in cd to directory " $dir
  exit -1
fi

ln -sf ${TOPDIR}/dt5742.h .
ln -sf ${TOPDIR}/dt5742.C .
ln -sf ${TOPDIR}/dt5742_toroot.C .

root.exe -b -q dt5742_toroot.C

