#!/usr/bin/env bash
#
# root2mbd <fname>
# convert digsig root files to MBD info
# <fname> can be a run and setting text file,
# or it can be a prdf

# Check that user specifies a file
if [[ $# -lt 1 ]]
then
  echo "Usage: root2mbd.sh <fname>"
  exit -1
fi

env

fname=$1    # DigSig ROOT filename

nevt=0
if [[ $# -gt 1 ]]
then
  nevt=$2
  echo Processing $2 events
fi

if echo $1 | grep 'root$'
then

  rootf=$fname
  echo root.exe -b -q digsig_calc_mbd.C\(\"${rootf}\",${nevt}\)
  echo PATH=$PATH
  echo LD_LIBRARY_PATH=$LD_LIBRARY_PATH

  # first process up to 250K uncalibrated events
  uncalib_events=250000
  if [[ $nevt -ne 0 ]] && [[ $nevt -lt 250000 ]]
  then
    uncalib_events=${nevt}
  fi
  root.exe -b -q digsig_calc_mbd.C\(\"${rootf}\",${uncalib_events},0\)

  mbd_rootf=${rootf%.root}_mbd.root

  # now run calibrations
  tcalib_events=100000
  if [[ $nevt -ne 0 ]] && [[ $nevt -lt 100000 ]]
  then
    tcalib_events=${nevt}
  fi
  root.exe -b -q cal_bbc_mip.C\(\"${mbd_rootf}\",0,${tcalib_events}\)  # time calibrations
  root.exe -b -q cal_bbc_mip.C\(\"${mbd_rootf}\",1\)         # charge calibrations

  # first process 250K uncalibrated events
  root.exe -b -q digsig_calc_mbd.C\(\"${rootf}\",${nevt}\)

else

  echo "Processing $fname"
  cat $fname | while read run setting
  do
    rootf=`printf "calib_mbd-%d.root" $run`
    echo $run $setting $rootf
    echo root.exe -b -q digsig_calc_mbd.C\(\"${rootf}\",${nevt}\)
    root.exe -b -q digsig_calc_mbd.C\(\"${rootf}\",${nevt}\)
    #mv prdf.root prdf_${run}.root
  done

fi

