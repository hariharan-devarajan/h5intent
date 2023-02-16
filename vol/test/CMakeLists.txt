set(examples h5_write)
set(ranks_per_nodes_local 1 2)
set(ranks_per_nodes_lassen 1 2 32)
set(machines "local" "lassen")
set(profiles "none" "darshan")
foreach (example ${examples})
    add_executable(${example} ${example}.cpp)
    target_link_libraries(${example} ${HDF5_LIBRARIES})
    target_link_libraries(${example} ${MPI_CXX_LIBRARIES})

    foreach (machine ${machines})
        if (${machine} STREQUAL "local")
            set(ranks_per_nodes ${ranks_per_nodes_local})
        elseif (${machine} STREQUAL "lassen")
            set(ranks_per_nodes ${ranks_per_nodes_lassen})
        endif ()
        foreach (ranks_per_node ${ranks_per_nodes})
            foreach (profile ${profiles})
                if (${machine} STREQUAL "local")
                    set(mpi_exec mpirun -n ${ranks_per_node})
                    if (${profile} STREQUAL "darshan")
                        set(mpi_exec ${mpi_exec} -x LD_PRELOAD=${CMAKE_SOURCE_DIR}/dependency/.spack-env/view/lib/libdarshan.so)
                    endif()
                elseif (${machine} STREQUAL "lassen")
                    set(mpi_exec jsrun -r 1 -a ${ranks_per_node} -c ${ranks_per_node} -d packed)
                endif ()
                set(test_name ${example}_${machine}_${ranks_per_node}_${profile}_native)
                add_test(NAME ${test_name} COMMAND
                        ${mpi_exec}
                        ${CMAKE_BINARY_DIR}/bin/${example}
                        -f $ENV{pfs}/temp/h5intent)
                if (${profile} STREQUAL "darshan")
                    if (${machine} STREQUAL "local" AND ${ranks_per_node} STREQUAL "1")
                        set_property(TEST ${test_name} APPEND PROPERTY ENVIRONMENT LD_PRELOAD=${CMAKE_SOURCE_DIR}/dependency/.spack-env/view/lib/libdarshan.so)
                    endif()
                    set_property(TEST ${test_name} APPEND PROPERTY ENVIRONMENT DARSHAN_LOG_DIR_PATH=${CMAKE_SOURCE_DIR}/h5bench/darshan-logs/native/h5_write)
                endif ()
            endforeach()
        endforeach()
    endforeach()

    add_test(NAME ${example}_h5intent COMMAND ${CMAKE_BINARY_DIR}/bin/${example} -f $ENV{pfs}/temp/h5intent)
    #set_property(TEST ${example}_h5intent APPEND PROPERTY ENVIRONMENT "LD_PRELOAD=${CMAKE_BINARY_DIR}/lib/libh5intent.so")
    set_property(TEST ${example}_h5intent APPEND PROPERTY ENVIRONMENT "LD_LIBRARY_PATH=$ENV{LD_LIBRARY_PATH}:${CMAKE_BINARY_DIR}/lib")
    set_property(TEST ${example}_h5intent APPEND PROPERTY ENVIRONMENT "HDF5_PLUGIN_PATH=${CMAKE_BINARY_DIR}/lib")
    set_property(TEST ${example}_h5intent APPEND PROPERTY ENVIRONMENT "HDF5_VOL_CONNECTOR=intent under_vol=0:under_info={${CMAKE_SOURCE_DIR}/h5bench/property-json/native/h5_write/h5_write.json}")
endforeach ()

add_subdirectory(h5bench)
function(h5bench_test test_name exec)
    set(ranks_per_node 40)
    set(mpi_exec jsrun -r 1 -a ${ranks_per_node} -c ${ranks_per_node} -d packed)
    add_test(NAME h5bench_${test_name} COMMAND
            ${mpi_exec}
            ${CMAKE_BINARY_DIR}/bin/${exec}
            /p/gpfs1/iopp/temp/h5bench/${test_name}/9f8c17e6/h5bench.cfg
            /p/gpfs1/iopp/temp/h5bench/${test_name}/test.h5)

    add_test(NAME h5bench_${test_name}_h5intent COMMAND
            ${mpi_exec}
            ${CMAKE_BINARY_DIR}/bin/${exec}
            /p/gpfs1/iopp/temp/h5bench/${test_name}/9f8c17e6/h5bench.cfg
            /p/gpfs1/iopp/temp/h5bench/${test_name}/test.h5)
    set_property(TEST h5bench_${test_name}_h5intent APPEND PROPERTY ENVIRONMENT "LD_LIBRARY_PATH=$ENV{LD_LIBRARY_PATH}:${CMAKE_BINARY_DIR}/lib")
    set_property(TEST h5bench_${test_name}_h5intent APPEND PROPERTY ENVIRONMENT "HDF5_PLUGIN_PATH=${CMAKE_BINARY_DIR}/lib")
    set_property(TEST h5bench_${test_name}_h5intent APPEND PROPERTY ENVIRONMENT "HDF5_VOL_CONNECTOR=intent under_vol=0:under_info={${CMAKE_SOURCE_DIR}/h5bench/property-json/sync_1_4_libdarshan/${test_name}/${exec}.json}")
endfunction()
h5bench_test(sync-write-1d-contig-contig h5bench_write)

set(folders sync_none_intent_1_40 sync_none_intent_2_40 sync_none_intent_4_40 sync_none_intent_8_40
            sync_none_none_1_40 sync_none_none_2_40 sync_none_none_4_40 sync_none_none_8_40)
foreach (folder ${folders})
    string(REPLACE "_" ";" folder_list ${folder})
    message("-- Processing folder " ${folder})
    list(GET folder_list 0 SYNC)
    list(GET folder_list 1 profiler)
    list(GET folder_list 2 vol)
    list(GET folder_list 3 num_nodes)
    list(GET folder_list 4 procs_per_node)
    FILE(GLOB apps "${CMAKE_CURRENT_SOURCE_DIR}/data/${folder}/*.json")
    foreach(app ${apps})
        get_filename_component(app_name ${app} NAME_WE)
        set(test_name h5bench_${folder}_${app_name})
        add_test(NAME ${test_name} COMMAND ${CMAKE_BINARY_DIR}/h5bench ${app})
    endforeach()
endforeach ()

#add_test(NAME h5bench_sync-write-1d-contig-contig COMMAND
#        #mpirun -n 4
#        ${CMAKE_BINARY_DIR}/bin/h5bench_write
#        /p/gpfs1/iopp/temp/h5bench/sync-write-1d-contig-contig/9f8c17e6/h5bench.cfg
#        /p/gpfs1/iopp/temp/h5bench/sync-write-1d-contig-contig/test.h5)
#
#set_property(TEST h5bench_sync-write-1d-contig-contig APPEND PROPERTY ENVIRONMENT "LD_LIBRARY_PATH=$ENV{LD_LIBRARY_PATH}:${CMAKE_BINARY_DIR}/lib")
#set_property(TEST h5bench_sync-write-1d-contig-contig APPEND PROPERTY ENVIRONMENT "HDF5_PLUGIN_PATH=${CMAKE_BINARY_DIR}/lib")
#set_property(TEST h5bench_sync-write-1d-contig-contig APPEND PROPERTY ENVIRONMENT "HDF5_VOL_CONNECTOR=intent under_vol=0:under_info={${CMAKE_SOURCE_DIR}/h5bench/property-json/sync_1_4_libdarshan/sync-write-1d-contig-contig/h5bench_write.json}")
