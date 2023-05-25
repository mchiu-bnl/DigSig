#!/usr/bin/env bash
#
# prdf2root <fname>
# convert prdf files to root
# <fname> can be a run and setting text file,
# or it can be a prdf

# Check that user specifies a file
if [[ $# -lt 1 ]]
then
    echo "Usage: prdf2root.sh <fname>"
    exit -1
fi

prdf=$1
nevt=0
if [[ $# -gt 1 ]]
then
  nevt=$2
  echo Processing $2 events
fi

if echo $1 | grep 'prdf$'
then

  echo root.exe -b -q prdf2root.C\(\"${prdf}\",${nevt}\)
  root.exe -b -q prdf2root.C\(\"${prdf}\",${nevt}\)
  run=${prdf%%.prdf}
  run=${run#*-}

else

  echo "Processing $1"
  cat $1 | while read run setting
  do
    prdf=`printf "junk/junk-%08d-0000.prdf" $run`
    echo $run $setting $prdf
    root.exe -b -q prdf2root.C\(\"${prdf}\",${nevt}\)
  done

fi

