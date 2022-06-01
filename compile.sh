#!/bin/bash

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

NUM_CPUS=`nproc`

cd ${DIR}
if [ ! -d "${DIR}/.build" ] 
then 
mkdir .build 
fi
cd .build
cmake ..
make -j${NUM_CORES}
cd ..
