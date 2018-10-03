#! /bin/sh

[ -z "$1" ] && exit

#caen_client SaveCorrectionTables_CAEN $1 1
#caen_client SaveCorrectionTables "$1.dat" 1

caen_client SaveCorrectionTables_CAEN $1 2
caen_client SaveCorrectionTables "$1.dat" 2
