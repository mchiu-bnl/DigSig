#!/usr/bin/env bash
#
# prdf2root <fname>
# convert prdf files to root
# <fname> can be a run and delay text file,
# or it can be a prdf

# Check that user specifies a file
if [[ $# -lt 1 ]]
then
    echo "Usage: prdf2root.sh <fname>"
    exit -1
fi

prdf=$1
# 
if echo $1 | grep 'prdf$'
then
	echo root.exe -b -q prdf2root.C\(\"${prdf}\"\)
	root.exe -b -q prdf2root.C\(\"${prdf}\"\)
else

    echo "Processing $1"
	cat $1 | while read run delay
	do
	    prdf=`printf "junk-%08d-0000.prdf" $run`
	    echo $run $delay $prdf
	    root.exe -b -q prdf2root.C\(\"${prdf}\"\)
	    mv prdf.root prdf_${run}.root
	done
fi

