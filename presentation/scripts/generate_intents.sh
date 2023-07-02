node=$1
ppn=$2

H5INTENT_DIR=/usr/workspace/iopp/software/h5intent
PYTHON_EXE=/usr/WS2/iopp/software/venvs/h5bench-py/bin/python
export LD_LIBRARY_PATH=${H5INTENT_DIR}/dependency/.spack-env/view/lib:$LD_LIBRARY_PATH
base_path="/usr/workspace/iopp/software/h5intent/presentation"
data_dirs="/p/gpfs"
darshan_logs="logs/darshan/sync_libdarshan_none_${node}_${ppn}"
property_json="logs/property-json/sync_libdarshan_none_${node}_${ppn}"
LOG=${H5INTENT_DIR}/presentation/logs/runtime-logs/sync_libdarshan_none_${node}_${ppn}_intent.log
echo "Running Intent Generator Tool"
$PYTHON_EXE $H5INTENT_DIR/h5intent/intent_generator/intent_generator.py \
--base-path $base_path \
--darshan-logs $darshan_logs \
--property-json $property_json \
--data-dirs $data_dirs > $LOG
intent_file=`ls ${H5INTENT_DIR}/presentation/${property_json}/*`
echo "Generated intents file $intent_file"

