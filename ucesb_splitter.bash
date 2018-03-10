#!/bin/bash

MAX_EVENTS=2500
PREFIX=run

if [ 1 -ne $# ]
then
	echo "Usage: $0 run-number" 1>&2
	exit 1
fi

seq=1
while true
do
	filename=$PREFIX$(printf '%04u_%04u' $1 $seq).lmd
	if [ -e $filename ]
	then
		echo "$filename already exists, something's wrong so I kill myself." 1>&2
		exit 1
	fi
	~/local/ucesb/unpacker/empty/empty --trans=rio4-1 --max-events=$MAX_EVENTS --output=$filename &
	pid=$!
	wait $pid
	seq=$((seq+1))
	sleep 1
done
