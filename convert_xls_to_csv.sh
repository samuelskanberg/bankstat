#!/bin/bash

if [ $# -ne 2 ]; then
	echo usage: $0 input.xls output.csv
	exit 1
fi

input=$1
tmp=$1.txt
out=$2

ssconvert -O 'eol=unix separator=;' $input $tmp

# For some reason the output format for dates are YYYY/MM/DD, it should be YYYY-MM-DD
# And for some reason the "Behållning" columns is withinh quotes
awk -F';' 'BEGIN {OFS=";"} {gsub("/","-",$1); gsub("/","-", $2); gsub("\"", "", $5); print}' $tmp > $out
rm $tmp
