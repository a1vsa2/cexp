#!/bin/bash

source utils.sh
cdir=$(pwd)

exit 0
if [ ! -d ${cdir}/bin ]; then
    echo "current dir: $cdir"
    mkdir -p ${cdir}/bin
fi
echo xx$?

#make -C ./libs
if [ ! $? ]; then
    echo "error occured"
    pause
    exit 1
fi

make -C src
pause