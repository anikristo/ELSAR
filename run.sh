#!/bin/bash

# Constants
NUM_CPUS=`nproc`
SRC_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Read the runtime argument
INPUT="$1"
OUTPUT="$2"
TMPFS_ROOT="${3:-"."}"
NUM_SORTING_TH="${4:-${NUM_CPUS}}"

# DROP CACHES
echo ""
echo "$(tput bold)Dropping caches...$(tput sgr0)"
echo "-------------------------------------"
sudo sh -c "sync; echo 1 > /proc/sys/vm/drop_caches"
sleep 5s
echo "DONE"

# RUN SORT & TIME
echo ""
echo "$(tput bold)Sorting...$(tput sgr0)"
echo "-------------------------------------"

echo "Performing ELSAR";
time ${SRC_DIR}/.build/bin/ELSAR ${INPUT} ${OUTPUT} ${TMPFS_ROOT} ${NUM_SORTING_TH};

# VERIFY
echo ""
echo "$(tput bold)Veryifying the output with valsort...$(tput sgr0)"
echo "-------------------------------------"
${SRC_DIR}/third_party/valsort -t${NUM_CPUS} ${OUTPUT}

# CLEAN-UP
echo ""
echo "$(tput bold)Cleaning up...$(tput sgr0)"
echo "-------------------------------------"
rm ${OUTPUT}
echo "DONE"
