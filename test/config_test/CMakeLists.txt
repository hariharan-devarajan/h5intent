set(examples config_tester)
foreach(example ${examples})
    add_executable(${example} ${example}.cpp ${TEST_SRC})
    target_link_libraries(${example} ${TEST_LIBS} h5intent)
    add_dependencies(${example} h5intent)

    file(GLOB_RECURSE JSON_FILES ${CMAKE_SOURCE_DIR}/h5bench/property-json/*/*.json)
    foreach(json_file ${JSON_FILES})

        get_filename_component(json_directory_path "${json_file}" DIRECTORY)
        file(RELATIVE_PATH filepath "${CMAKE_SOURCE_DIR}/h5bench/property-json" "${json_directory_path}")
        #get_filename_component(header_directory_path "${json_file}" NAME)
        string(REPLACE "/" "_" filepath_txt ${filepath})
        #message(INFO ${filepath_txt})
        set(test_name ${example}_${filepath_txt})
        add_test(${test_name} ${CMAKE_BINARY_DIR}/bin/config_tester "TestConfig" --json_file ${json_file})
    endforeach()
    #
endforeach()

