cmake_minimum_required(VERSION 3.15)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../tools/cmake")
set(SCVPI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

project(tinyalu_reg_sc CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(SystemC 3.0.2 REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

option(TLM_TARGET_CONSUMES_TIME
    "Target consumes time using wait() inside b_transport() instead of returning annotated delay"
    OFF
)

add_compile_options(
    -Wall
    -Wno-deprecated
    -Wno-unused-function
    -Wno-unused-parameter
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
    src/tinyalu_reg_tlm.cpp
    ${SCVPI_DIR}/src/scvpi.cpp
    ${SCVPI_DIR}/src/scvpi_tlm_bridge.cpp
)

set_target_properties(TINYALU_REG_SC PROPERTIES ENABLE_EXPORTS ON)

if(TLM_TARGET_CONSUMES_TIME)
    message(STATUS "Building TLM model with target-side time consumption enabled")
    target_compile_definitions(TINYALU_REG_SC PRIVATE TLM_TARGET_CONSUMES_TIME)
else()
    message(STATUS "Building TLM model with annotated-delay timing")
endif()

target_include_directories(TINYALU_REG_SC PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SCVPI_DIR}/src
    ${Python3_INCLUDE_DIRS}
    ${SystemC_INCLUDE_DIRS}
)

target_link_libraries(TINYALU_REG_SC PRIVATE
    SystemC::SystemC
    dl
    rt
    atomic
    util
)

add_library(scvpi_tlm_ext SHARED
    ${SCVPI_DIR}/src/scvpi_tlm_ext.cpp
)

target_include_directories(scvpi_tlm_ext PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SCVPI_DIR}/src
    ${Python3_INCLUDE_DIRS}
    ${SystemC_INCLUDE_DIRS}
)

target_link_libraries(scvpi_tlm_ext PRIVATE
    Python3::Python
    SystemC::SystemC
)

set_target_properties(scvpi_tlm_ext PROPERTIES
    OUTPUT_NAME "scvpi_tlm_ext"
    POSITION_INDEPENDENT_CODE ON
)