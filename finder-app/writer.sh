#!/bin/bash

if [ -z "$1" -o -z "$2" ]
then
	echo "Please specify a filepath and a string in the cmdline args."
	exit 1
fi

writefile="$1"
writestr="$2"


mkdir -p "$(dirname $writefile)"
echo $writestr > $writefile
if [ $? -ne 0 ]
then
	echo "File $writefile could not be created."
	exit 1
fi


