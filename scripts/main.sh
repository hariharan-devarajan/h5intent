#!/bin/bash
SCRIPT_NAME=$1
NODE=$2
TEST_NAME=$3
LOG_NAME=${SCRIPT_NAME}_${NODE}_${TEST_NAME}.out
PROJECT_DIR=/usr/workspace/iopp/software/h5intent
echo	bsub -J ${SCRIPT_NAME}_${NODE}_${TEST_NAME} -nnodes ${NODE} -W $((30)) -env NNODES=$NODE -core_isolation 0 -G asccasc -q pbatch -cwd ${PROJECT_DIR}/scripts -o ${PROJECT_DIR}/scripts/${LOG_NAME} -e ${PROJECT_DIR}/scripts/${LOG_NAME} ${PROJECT_DIR}/scripts/$SCRIPT_NAME $NODE $TEST_NAME
bsub -J ${SCRIPT_NAME}_${NODE}_${TEST_NAME} -nnodes ${NODE} -W $((30)) -env NNODES=$NODE -core_isolation 0 -G asccasc -q pbatch -cwd ${PROJECT_DIR}/scripts -o ${PROJECT_DIR}/scripts/${LOG_NAME} -e ${PROJECT_DIR}/scripts/${LOG_NAME} ${PROJECT_DIR}/scripts/$SCRIPT_NAME $NODE $TEST_NAME
