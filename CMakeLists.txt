project (SIMULATOR)

set (CMAKE_VERBOSE_MAKEFILE true)

cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0015 NEW)
cmake_policy(SET CMP0016 NEW)

set( ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${CMAKE_SOURCE_DIR}/local/lib-linux/pkgconfig" )
set( ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${CMAKE_SOURCE_DIR}/local/lib-linux64/pkgconfig" )

find_package(PkgConfig)
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH} ${CMAKE_SOURCE_DIR}/local")
pkg_check_modules(SYSTEMC REQUIRED systemc>=2.3.1)

find_path(SYSTEMC_INCLUDE_DIR systemc.h HINTS ${SYSTEMC_INCLUDE_DIRS})
find_library(SYSTEMC_LIBRARY
    systemc
    PATHS ${SYSTEMC_LIBRARY_DIRS}
)

include_directories(${SYSTEMC_INCLUDE_DIR})

include_directories(${CMAKE_SOURCE_DIR}/bitstream)

include_directories(${CMAKE_SOURCE_DIR}/rapidjson/include/)

set(OPTIMIZE "-O3 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g ${OPTIMIZE}")
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -std=c++11 -g ${OPTIMIZE}" )

set(SOURCEDIR ${CMAKE_SOURCE_DIR}/SimulatorSystemC)

include_directories(${SOURCEDIR})

set(SRC
    ${SOURCEDIR}/modules/elements/buffers/BufferFill.cpp 
    ${SOURCEDIR}/modules/elements/buffers/BufferFiFo.cpp
    ${SOURCEDIR}/modules/elements/buffers/BufferPicture.cpp
    ${SOURCEDIR}/modules/elements/buffers/BufferDecoder.cpp
    ${SOURCEDIR}/framework/CsvTrace.cpp
    ${SOURCEDIR}/main.cpp
    )

add_executable(simulator ${SRC})
target_link_libraries(simulator ${SYSTEMC_LIBRARY})