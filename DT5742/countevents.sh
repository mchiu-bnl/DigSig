#!/bin/bash
#
# usage: countEvents.sh <dir>
#
# if <dir> is not specified, then look for most recent directory in $CAENDATA_DIR
# 
# To count events in current data directory, use 
#    countEvents.sh .
#

CAENDATA_DIR=/home/lappd/Desktop/wavedump-3.7.4/FNAL-Mar2019

if [[ $# -eq 0 ]]
then

  # find most recent dir in CAENDATA_DIR
  cd $CAENDATA_DIR
  dir=`ls -lt | grep '^d' | awk 'NR==1 { print $NF}'`
  CAENDATA_DIR=${CAENDATA_DIR}/$dir

else

  CAENDATA_DIR=$1

fi

echo Number of events in ${CAENDATA_DIR##*/}

bcnt=`wc -c ${CAENDATA_DIR}/TR_0_0.dat | cut -f 1 -d ' ' `
#python -c "print $bcnt / 4096."
python -c "print $bcnt / 4120"    # for data with event headers

