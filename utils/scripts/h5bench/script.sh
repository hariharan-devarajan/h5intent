#!/bin/bash
CONF_DIR=$1
NODES=$2
DARSHAN=$3
MACHINE=`hostname`
TIME="0:30"
H5INTENT_DIR=/usr/WS2/iopp/software/h5intent
SCRIPTS_DIR=${H5INTENT_DIR}/scripts/h5bench
CASE=`basename $CONF_DIR`
LOGS=${H5INTENT_DIR}/runtime-logs/${MACHINE}/$CASE/

mkdir -p $LOGS
for conf in "$CONF_DIR"/*.json; 
do   
filename=`basename $conf .json`
LOG_FILE=${LOGS}/${filename}.log
echo "bsub -J ${filename} -nnodes $NODES -W $TIME -o ${LOG_FILE} -e ${LOG_FILE} -core_isolation 0 -G asccasc -q pbatch $SCRIPTS_DIR/main.sh $conf $CASE $DARSHAN"
bsub -J ${filename} -nnodes $NODES -W $TIME -o ${LOG_FILE} -e ${LOG_FILE} -core_isolation 0 -G asccasc -q pbatch $SCRIPTS_DIR/main.sh $conf $CASE $DARSHAN
done
