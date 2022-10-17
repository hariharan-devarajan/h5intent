#!/bin/bash
CONF_DIR=$1
NODES=$2
DARSHAN=$3
TIME="0:30"
IOPP_DIR=/usr/workspace/iopp/software/iopp/apps/h5bench/
CASE=`basename $CONF_DIR`
LOGS=$IOPP_DIR/logs/$CASE

mkdir -p $LOGS
for conf in "$CONF_DIR"/*.json; 
do   
filename=`basename $conf .json`
LOG_FILE=${LOGS}/${filename}.log
echo "bsub -J ${filename} -nnodes $NODES -W $TIME -o ${LOG_FILE} -e ${LOG_FILE} -core_isolation 0 -G asccasc -q pbatch $IOPP_DIR/main.sh $conf $CASE $DARSHAN"
bsub -J ${filename} -nnodes $NODES -W $TIME -o ${LOG_FILE} -e ${LOG_FILE} -core_isolation 0 -G asccasc -q pbatch $IOPP_DIR/main.sh $conf $CASE $DARSHAN

done
