#!/bin/bash
CONF=$1
CASE=$2
DARSHAN=$3

H5INTENT_DIR=/usr/workspace/iopp/software/h5intent
H5BENCH_DIR=$H5INTENT_DIR/external/h5bench
BUILD_DIR=$H5INTENT_DIR/build-profile

source /usr/workspace/iopp/install_scripts/bin/iopp-init
source /usr/workspace/iopp/install_scripts/bin/spack-init
spack env activate -p ${H5INTENT_DIR}/dependency-profile

export LD_LIBRARY_PATH=${H5INTENT_DIR}/dependency-profile/.spack-env/lib:$LD_LIBRARY_PATH
export PATH=$BUILD_DIR:$BUILD_DIR/bin:$BUILD_DIR/external/h5bench/:$PATH

filename=`basename $CONF .json`
if [ $DARSHAN -eq 1 ]; then
echo enabled_darshan
export DARSHAN_LOG_DIR_PATH=${H5INTENT_DIR}/logs/darshan/${CASE}/${filename}
mkdir -p "${DARSHAN_LOG_DIR_PATH}"
echo "DARSHAN_LOG_DIR_PATH=${H5INTENT_DIR}/logs/darshan/${CASE}/${filename}"
fi
echo $CONF

echo $BUILD_DIR/h5bench --debug $CONF
$BUILD_DIR/h5bench --debug $CONF
