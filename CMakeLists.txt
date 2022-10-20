cmake_minimum_required(VERSION 3.21)
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/dependency/.spack-env/view ${CMAKE_BINARY_DIR}/lib/cmake)
set(H5INTENT_CMAKE_DIR ${CMAKE_SOURCE_DIR}/CMake)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${H5INTENT_CMAKE_DIR})
project(h5intent)
set(H5INTENT_VERSION_MAJOR "0")
set(H5INTENT_VERSION_MINOR "0")
set(H5INTENT_VERSION_PATCH "1")
set(H5INTENT_PACKAGE ${PROJECT_NAME})
set(H5INTENT_PACKAGE_NAME ${PROJECT_NAME})
set(H5INTENT_PACKAGE_VERSION "${H5INTENT_VERSION_MAJOR}.${H5INTENT_VERSION_MINOR}.${H5INTENT_VERSION_PATCH}")
set(H5INTENT_PACKAGE_VERSION_MAJOR "${H5INTENT_VERSION_MAJOR}.${H5INTENT_VERSION_MINOR}")
set(H5INTENT_PACKAGE_VERSION_MINOR "${H5INTENT_VERSION_PATCH}")
set(H5INTENT_PACKAGE_STRING "${H5INTENT_PACKAGE_NAME} ${H5INTENT_PACKAGE_VERSION}")
set(H5INTENT_PACKAGE_TARNAME "${H5INTENT_PACKAGE}")


add_subdirectory(external/cpp-logger)
find_package(cpp-logger REQUIRED)
if (${CPP_LOGGER_FOUND})
    include_directories(${CPP_LOGGER_INCLUDE_DIRS})
    set(DEPENDENCY_LIB ${DEPENDENCY_LIB} ${CPP_LOGGER_LIBRARIES})
    message(STATUS "[H5Intent] found cpp-logger at ${CPP_LOGGER_INCLUDE_DIRS}")
else ()
    message(FATAL_ERROR "-- [H5Intent] cpp-logger is needed for ${PROJECT_NAME} build")
endif ()
enable_testing()
include(CTest)
add_subdirectory(vol)
#------------------------------------------------------------------------------
# Setup install and output Directories
#------------------------------------------------------------------------------
if (NOT H5INTENT_INSTALL_BIN_DIR)
    set(H5INTENT_INSTALL_BIN_DIR ${CMAKE_INSTALL_PREFIX}/bin)
    message(STATUS ${H5INTENT_INSTALL_BIN_DIR})
endif ()
if (NOT H5INTENT_INSTALL_LIB_DIR)
    set(H5INTENT_INSTALL_LIB_DIR ${CMAKE_INSTALL_PREFIX}/lib)
endif ()
if (NOT H5INTENT_INSTALL_INCLUDE_DIR)
    set(H5INTENT_INSTALL_INCLUDE_DIR ${CMAKE_INSTALL_PREFIX}/include)
endif ()
if (NOT H5INTENT_INSTALL_DATA_DIR)
    set(H5INTENT_INSTALL_DATA_DIR ${CMAKE_INSTALL_PREFIX}/share)
endif ()

if (NOT CMAKE_INSTALL_RPATH)
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif ()

#------------------------------------------------------------------------------

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
        ${CMAKE_BINARY_DIR}/bin CACHE PATH "Single Directory for all Executables."
        )

set(CMAKE_INCLUDE_OUTPUT_DIRECTORY
        ${CMAKE_BINARY_DIR}/include CACHE PATH "Store the headers."
        )
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
        ${CMAKE_BINARY_DIR}/lib CACHE PATH "Single Directory for all Libraries"
        )
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
        ${CMAKE_BINARY_DIR}/lib CACHE PATH "Single Directory for all static libraries."
        )

set_target_properties(${PROJECT_NAME} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
#------------------------------------------------------------------------------
# Disallow in-source build
#------------------------------------------------------------------------------
if ("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR
            "${PROJECT_NAME} requires an out of source Build. "
            "Please create a separate binary directory and run CMake there.")
endif ()

#------------------------------------------------------------------------------
# Set a default build type if none was specified
#------------------------------------------------------------------------------
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
            "MinSizeRel" "RelWithDebInfo")
endif ()

#-----------------------------------------------------------------------------
# Targets built within this project are exported at Install time for use
# by other projects.
#-----------------------------------------------------------------------------
if (NOT H5INTENT_EXPORTED_TARGETS)
    set(H5INTENT_EXPORTED_TARGETS "${PROJECT_NAME}-targets")
endif ()

set(H5INTENT_LIBTYPE SHARED)
set_target_properties(${libtarget}
        PROPERTIES
        DEBUG_OUTPUT_NAME ${LIB_DEBUG_NAME}
        RELEASE_OUTPUT_NAME ${LIB_RELEASE_NAME}
        MINSIZEREL_OUTPUT_NAME ${LIB_RELEASE_NAME}
        RELWITHDEBINFO_OUTPUT_NAME ${LIB_RELEASE_NAME}
        VERSION ${LIB_VERSION}
        SOVERSION ${LIB_VERSION}
        )

include(GNUInstallDirs)

#-----------------------------------------------------------------------------
# Configure the config.cmake file for the build directory
#-----------------------------------------------------------------------------
configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/CMake/${PROJECT_NAME}-config.cmake.build.in
        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake/${PROJECT_NAME}/${PROJECT_NAME}-config.cmake @ONLY
)
install(
        FILES
        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake/${PROJECT_NAME}/${PROJECT_NAME}-config.cmake
        DESTINATION
        ${H5INTENT_INSTALL_LIB_DIR}/cmake/${PROJECT_NAME}
)

install(TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
#-----------------------------------------------------------------------------
# Configure the ${PROJECT_NAME}-config-version .cmake file for the install directory
#-----------------------------------------------------------------------------
configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/CMake/${PROJECT_NAME}-config-version.cmake.in
        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake/${PROJECT_NAME}/${PROJECT_NAME}-config-version.cmake @ONLY
)

install(
        FILES
        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake/${PROJECT_NAME}/${PROJECT_NAME}-config-version.cmake
        DESTINATION
        ${H5INTENT_INSTALL_LIB_DIR}/cmake/${PROJECT_NAME}
)
install(
        TARGETS
        ${PROJECT_NAME}
        EXPORT
        ${H5INTENT_EXPORTED_TARGETS}
        LIBRARY DESTINATION ${H5INTENT_INSTALL_LIB_DIR}
        ARCHIVE DESTINATION ${H5INTENT_INSTALL_LIB_DIR}
        RUNTIME DESTINATION ${H5INTENT_INSTALL_BIN_DIR}
)
export(EXPORT ${H5INTENT_EXPORTED_TARGETS}
        FILE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake/${PROJECT_NAME}/${PROJECT_NAME}Targets.cmake")
install(EXPORT
        ${H5INTENT_EXPORTED_TARGETS}
        DESTINATION
        ${H5INTENT_INSTALL_LIB_DIR}/cmake/${PROJECT_NAME}
        FILE
        ${H5INTENT_EXPORTED_TARGETS}.cmake
        )