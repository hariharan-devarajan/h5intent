#!/bin/bash

H5INTENT_DIR=/usr/workspace/iopp/software/h5intent
CONF=/usr/WS2/iopp/software/h5intent/presentation/logs/h5bench_confs/lassen/$2/$1
CASE=$2
DARSHAN=$3

source /usr/workspace/iopp/install_scripts/bin/iopp-init
source /usr/workspace/iopp/install_scripts/bin/spack-init
if [ $DARSHAN -eq 1 ]; then
echo "Setting up darshan environment"
  environ=dependency-profile
  BUILD_DIR=$H5INTENT_DIR/build-profile
else
echo "Setting up environment"
  environ=dependency
  BUILD_DIR=$H5INTENT_DIR/build
fi
spack env activate -p ${H5INTENT_DIR}/$environ
export LD_LIBRARY_PATH=${H5INTENT_DIR}/$environ/.spack-env/view/lib:$LD_LIBRARY_PATH
export PATH=$BUILD_DIR:$BUILD_DIR/bin:$BUILD_DIR/external/h5bench/:$PATH

filename=`basename $CONF .json`
if [ $DARSHAN -eq 1 ]; then
#echo "Setting up darshan"
#echo enabled_darshan
export DARSHAN_LOG_DIR_PATH=${H5INTENT_DIR}/presentation/logs/darshan/${CASE}/${filename}
rm -rf ${DARSHAN_LOG_DIR_PATH}
mkdir -p "${DARSHAN_LOG_DIR_PATH}"
#echo "DARSHAN_LOG_DIR_PATH=${H5INTENT_DIR}/presentation/logs/darshan/${CASE}/${filename}"
fi
#echo $CONF
echo "Running up h5bench"
LOG_DIR=${H5INTENT_DIR}/presentation/logs/runtime-logs/${CASE}
mkdir -p $LOG_DIR
#echo $BUILD_DIR/h5bench --debug $CONF
rm $LOG_DIR/${filename}.log
$BUILD_DIR/h5bench --debug $CONF > $LOG_DIR/${filename}.log 2>&1

H5BENCH_LOG_DIR=`cat $LOG_DIR/${filename}.log | grep "DIR:" | awk '{print $11}'`
if [ $DARSHAN -eq 1 ]; then
darshan_file=`ls $H5INTENT_DIR/presentation/logs/darshan/${CASE}/${filename}`
echo "Generated darshan file ${darshan_file}"
else
#echo "Written log in logs/runtime-logs/${CASE}/${filename}.log"

LOG_NAME=$H5BENCH_LOG_DIR/stdout
echo "========= Performance Results ========="
tail -n 12 $LOG_NAME | head -n -1
fi
