#!/usr/bin/env bash
#
# prdf2root <fname>
# convert prdf files to root
# <fname> can be a run and setting text file,
# or it can be a prdf

# Check that user specifies a file
if [[ $# -lt 1 ]]
then
    echo "Usage: root2mbd.sh <fname>"
    exit -1
fi

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
	echo root.exe -b -q digsig_calc_mdb.C\(\"${rootf}\",${nevt}\)
	root.exe -b -q digsig_calc_mbd.C\(\"${rootf}\",${nevt}\)
else

    echo "Processing $fname"
	cat $fname | while read run setting
	do
	    rootf=`printf "prdf_%d.root" $run`
	    echo $run $setting $rootf
	    echo root.exe -b -q digsig_calc_mbd.C\(\"${rootf}\",${nevt}\)
	    root.exe -b -q digsig_calc_mbd.C\(\"${rootf}\",${nevt}\)
	    #mv prdf.root prdf_${run}.root
	done
fi

