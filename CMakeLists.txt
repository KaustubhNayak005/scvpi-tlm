cmake_minimum_required(VERSION 3.15)

# SCVPI-TLM: Unified SystemC TLM-2.0 & SystemVerilog RTL Co-Verification Framework
project(SCVPI-TLM VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Reuse the SystemC find module shipped with the unified example so that
# standalone builds and example builds resolve SystemC the same way.
list(APPEND CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/examples/tinyalu_reg/tools/cmake"
)

find_package(SystemC 3.0.2 REQUIRED)
find_package(Threads REQUIRED)

# ------------------------------------------------------------------------------
# SCVPI bridge library
# ------------------------------------------------------------------------------
add_library(scvpi SHARED
    src/scvpi.cpp
    src/scvpi_tlm_bridge.cpp
)

target_include_directories(scvpi PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_definitions(scvpi PRIVATE
    SC_INCLUDE_DYNAMIC_PROCESSES
    SC_INCLUDE_FX
)

target_link_libraries(scvpi PRIVATE
    SystemC::SystemC
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

set_target_properties(scvpi PROPERTIES
    OUTPUT_NAME "scvpi"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# ------------------------------------------------------------------------------
# Optional CPython extension used by the TLM testbench
# ------------------------------------------------------------------------------
option(SCVPI_BUILD_PYTHON_EXT
    "Build the scvpi_tlm_ext CPython extension module"
    ON
)

if(SCVPI_BUILD_PYTHON_EXT)
    find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

    add_library(scvpi_tlm_ext MODULE
        src/scvpi_tlm_ext.cpp
    )

    target_include_directories(scvpi_tlm_ext PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )

    target_compile_definitions(scvpi_tlm_ext PRIVATE
        SC_INCLUDE_DYNAMIC_PROCESSES
    )

    target_link_libraries(scvpi_tlm_ext PRIVATE
        scvpi
        Python3::Python
        SystemC::SystemC
    )

    set_target_properties(scvpi_tlm_ext PROPERTIES
        PREFIX ""
        OUTPUT_NAME "scvpi_tlm_ext"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
endif()

# ------------------------------------------------------------------------------
# Install rules
# ------------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS scvpi
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

if(TARGET scvpi_tlm_ext)
    install(TARGETS scvpi_tlm_ext
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
endif()

install(DIRECTORY src/
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/scvpi"
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)
