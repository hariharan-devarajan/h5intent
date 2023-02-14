#!/bin/bash
CONF=$1
CASE=$2
DARSHAN=$3

H5BENCH_DIR=/usr/workspace/iopp/applications/h5bench/build
IOPP_DIR=/usr/workspace/iopp/software/h5intent/h5bench/

export LD_LIBRARY_PATH=/usr/WS2/iopp/software/spack/var/spack/environments/h5bench/.spack-env/view/lib:$LD_LIBRARY_PATH

source /usr/workspace/iopp/install_scripts/bin/iopp-init
source /usr/workspace/iopp/install_scripts/bin/spack-init
spack env activate -p h5bench

export PATH=$H5BENCH_DIR:$PATH

filename=`basename $CONF .json`
if [ $DARSHAN -eq 1 ]; then
echo enabled_darshan
export DARSHAN_LOG_DIR_PATH=${IOPP_DIR}/darshan-logs/${CASE}/${filename}
mkdir -p "${DARSHAN_LOG_DIR_PATH}"
fi
echo "DARSHAN_LOG_DIR_PATH=${IOPP_DIR}/darshan-logs/${CASE}/${filename}"
echo $CONF
pushd $H5BENCH_DIR
echo ./h5bench --debug $CONF
./h5bench --debug $CONF
popd
