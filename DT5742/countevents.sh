#!/bin/bash

bcnt=`wc -c wave_0.dat | cut -f 1 -d ' ' `
#echo | awk '{ print '$bcnt'/4096 }'

echo | awk '{ print '$bcnt'/4120 }'

