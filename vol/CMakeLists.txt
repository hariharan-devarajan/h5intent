include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
set(VOL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/h5intent/h5intent_vol.c)
set(VOL_PRIVATE_HEADER)
set(VOL_PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/include/h5intent/h5intent_vol.h)
add_library(h5intent_vol SHARED ${VOL_SRC} ${VOL_PUBLIC_HEADER})

#target_sources(h5intent PRIVATE ${VOL_SRC} ${VOL_PUBLIC_HEADER})
#target_include_directories(${PROJECT_NAME}
#        PRIVATE
#        # where the library itself will look for its internal headers
#        ${CMAKE_CURRENT_SOURCE_DIR}/src
#        PUBLIC
#        # where top-level project will look for the library's public headers
#        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
#        # where external projects will look for the library's public headers
#        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
#        )
set(DEPENDENCY_LIB "")
find_package(HDF5 COMPONENTS C REQUIRED)
if (${HDF5_FOUND})
    message(STATUS "[H5Intent] found hdf5 at ${HDF5_INCLUDE_DIRS}")
    include_directories(${HDF5_INCLUDE_DIRS})
    set(DEPENDENCY_LIB ${DEPENDENCY_LIB} ${HDF5_LIBRARIES})
else ()
    message(FATAL_ERROR "-- [H5Intent] hdf5 is needed for ${PROJECT_NAME} build")
endif ()

find_package(MPI COMPONENTS CXX REQUIRED)
if (MPI_FOUND)
    message(STATUS "[H5Intent] found mpi at ${MPI_CXX_INCLUDE_DIRS}")
    include_directories(${MPI_CXX_INCLUDE_DIRS})
    set(DEPENDENCY_LIB ${DEPENDENCY_LIB} ${MPI_CXX_LIBRARIES})
else ()
    message(FATAL_ERROR "-- [H5Intent] mpi is needed for ${PROJECT_NAME} build")
endif ()


target_compile_definitions(h5intent_vol PRIVATE ENABLE_INTENT_LOGGING)
target_link_libraries(h5intent_vol ${DEPENDENCY_LIB})

# the variant with PUBLIC_HEADER property unfortunately does not preserve the folder structure
#set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${public_headers}")
# so instead we iterate through public headers and install them "manually"
foreach (header ${VOL_PUBLIC_HEADER})
    file(RELATIVE_PATH header_file_path "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}" "${header}")
    get_filename_component(header_directory_path "${header_file_path}" DIRECTORY)
    install(
            FILES ${header}
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${header_directory_path}"
    )
    file(COPY ${header}
            DESTINATION "${CMAKE_INCLUDE_OUTPUT_DIRECTORY}/${header_directory_path}")
endforeach ()


add_subdirectory(test)