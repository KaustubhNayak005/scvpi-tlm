cmake_minimum_required(VERSION 3.15)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../tools/cmake")
set(SCVPI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

project(tinyalu_reg_sc CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

find_package(SystemC 3.0.2 REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

include_directories(
    include
    ${SCVPI_DIR}/src
)

add_compile_options(
    -Wno-deprecated
    -Wno-unused-parameter
    -Wall
    -Wno-unused-function
    -DSC_INCLUDE_FX
    -DSC_INCLUDE_DYNAMIC_PROCESSES
)

add_link_options(
    -fno-lto
    -Xlinker
    -export-dynamic
)

add_executable(TINYALU_REG_SC
    src/main.cpp
    ${SCVPI_DIR}/src/scvpi.cpp
    ${SCVPI_DIR}/src/scvpi_tlm_bridge.cpp
)

target_include_directories(TINYALU_REG_SC PRIVATE
    ${SystemC_INCLUDE_DIRS}
)

target_link_libraries(TINYALU_REG_SC PRIVATE
    SystemC::SystemC
    dl
    rt
    atomic
    util
)