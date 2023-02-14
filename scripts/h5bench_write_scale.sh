#!/bin/bash

source /usr/workspace/iopp/install_scripts/bin/iopp-init

NUM_NODES=$1
TEST_NAME=$2
PROJECT_DIR=/usr/workspace/iopp/software/h5intent

pushd $PROJECT_DIR
spack env activate -p ./dependency
export CC=/usr/tce/packages/gcc/gcc-8.3.1/bin/gcc
export CXX=/usr/tce/packages/gcc/gcc-8.3.1/bin/g++

pushd build
make -j
ctest -R $TEST_NAME -VV
