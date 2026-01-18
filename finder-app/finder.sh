#!/bin/sh

if [ -z "$1" -o -z "$2" ]
then
	echo "Please specify a filepath and a string to search for."
	exit 1
fi

filesdir="$1"
searchstr="$2"

if [ -d $filesdir ]
then
	files=$( echo find $filesdir 2>/dev/null )
	X=$( $files | wc -l )
	Y=$( grep -a -s -R $searchstr $files | wc -l )
	echo "The number of files are $((X - 1)) and the number of matching lines are "$Y""
else
	echo "First argument is not a valid path."
	exit 1
fi

exit 0
