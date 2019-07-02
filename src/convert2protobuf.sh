#!/bin/bash

declare -a images
i=0
while read line
do
    images[ $i ]="$line"    
    (( i++ ))
done < <(ls $1)

for e in "${images[@]}"; 
do
    printf "\n\n$e" 
    printf "\n=====================================\n"
    printf "$ png2protobuf $e \n"
    timeout 5m ./png2protobuf $1/$e $2/$e.protobuf
done

