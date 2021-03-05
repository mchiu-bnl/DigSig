#!/bin/sh
#
# condor executes runana.cmd 0, runana.cmd 1, etc.
#

if [[ $# -lt 1 ]]
then
  echo "usage: dt5742_toroot.cmd <linenum> <nevts>"
  exit -1
fi

echo PWD=${PWD}
echo LD_LIBRARY_PATH=${LD_LIBRARY_PATH}
echo HOST=`hostname`

ulimit -c 0     # no core files

# get directory
let line=$1+1
dirname=`sed -n "${line}p" goodruns.list`

TOPDIR=${PWD}

cd run0${dirname}
if [[ $? -ne 0 ]]
then
  echo "error in cd to directory " $dir
  exit -1
fi

nevt=0
if [[ $# -ge 2 ]]
then
  nevt=$2
  echo "Processing ${nevt} events"
else
  echo "dtreeProcessing all events"
fi

ln -sf ${TOPDIR}/dt5742.h .
ln -sf ${TOPDIR}/dt5742.C .
ln -sf ${TOPDIR}/dt5742_toroot.C .
ln -sf ${TOPDIR}/run_dt5742_toroot.C .

root.exe -b -q run_dt5742_toroot.C\(${nevt}\)

