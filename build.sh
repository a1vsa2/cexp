#!/bin/bash

source utils.sh
cdir=$(pwd)

if [ ! -d cdir/bin ]; then
    echo "current dir: $cdir"
    mkdir -p cdir/bin
fi

#make -C ./libs
if [ ! $? ]; then
    echo "error occured"
    pause
    exit 1
fi

make -C src
pause