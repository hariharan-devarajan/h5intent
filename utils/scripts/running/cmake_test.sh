#!/bin/bash

source /usr/workspace/iopp/install_scripts/bin/iopp-init

NUM_NODES=$1
TEST_NAME=$2
PROJECT_DIR=/usr/workspace/iopp/software/h5intent

pushd $PROJECT_DIR

env=$PROJECT_DIR/dependency
build=build
SUB='darshan'
if [[ "$TEST_NAME" == *"$SUB"* ]]; then
   env=$PROJECT_DIR/dependency-profile
   build=build-profile
   echo enabled_darshan
fi

echo "loading $env"
spack env activate -p $env
export CC=/usr/tce/packages/gcc/gcc-8.3.1/bin/gcc
export CXX=/usr/tce/packages/gcc/gcc-8.3.1/bin/g++
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$env/.spack-env/view/lib:$env/.spack-env/view/lib64
pushd $build
make -j
ctest -R ${TEST_NAME}$ -VV
OIFS=$IFS
IFS='_'
split_values=${TEST_NAME}
for x in $split_values; do     a=$x; done
rm -f /p/gpfs1/iopp/temp/h5bench/${a}_${NUM_NODES}_40/*.h5
