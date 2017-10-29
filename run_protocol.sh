#!/usr/bin/env bash

for i in `seq $1 1 $2`;
do
	./SemiHonestBMR -partyID ${i} -circuitFile ${3} -inputsFile ${4} -partiesFile ${5} -key ${6} -version ${7} &
	echo "Running $i..."
done