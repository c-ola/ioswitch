#!/bin/bash

pid=$1
DEVICE_ID=11

en_dev () {
    xinput enable $DEVICE_ID
}

dis_dev () {
    xinput disable $DEVICE_ID
}

sleep  10 2>/dev/null &

spin[0]="-"
spin[1]="\\"
spin[2]="|"
spin[3]="/"

# echo -n "[sleeping] ${spin[0]}"

#while ps -p $pid >/dev/null 2>&1       # using ps
while kill -0 $pid >/dev/null 2>&1      # using kill
do
    for i in "${spin[@]}"
    do
        echo -ne "\b$i"
        sleep 0.5
    done
done

en_dev
