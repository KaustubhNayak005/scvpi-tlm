#[=======================================================================[.rst:
FindSystemC
-----------

Finds the SystemC static library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``SystemC::SystemC``
  The SystemC library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``SystemC_FOUND``
  True if the system has the SystemC library.
``SystemC_VERSION``
  The version of the SystemC library which was found.
``SystemC_INCLUDE_DIRS``
  Include directories needed to use SystemC.
``SystemC_LIBRARIES``
  Libraries needed to link to SystemC.

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``SystemC_INCLUDE_DIR``
  The directory containing ``systemc.h``.
``SystemC_LIBRARY``
  The path to the SystemC static library.

#]=======================================================================]


set(SystemC_PATHS
    # Prefer paths given as command line arguments, cache entries, or the
    # environment exported by the documented setup instructions.
    ${SystemC_ROOT}
    ${SystemC_ROOT_DIR}
    ${SYSTEMC_HOME}
    ${SYSTEMC_ROOT}
    $ENV{SYSTEMC_HOME}
    $ENV{SYSTEMC}

    # Hardcoded UNIX standard paths
    "/opt/systemc"
    "/usr/local/systemc"
)

foreach(PATH IN LISTS SystemC_PATHS)
    file(GLOB VERSIONED_PATH ${PATH}-[0-9].[0-9].[0-9])
    list(APPEND SystemC_PATHS ${VERSIONED_PATH})
endforeach()


# Try to find the SystemC include directory by
# searching for the "system.h" header file.
find_path(SystemC_INCLUDE_DIR
    NAMES
        systemc.h
    PATHS
        ${SystemC_PATHS}
    PATH_SUFFIXES
        include 
)

# Try to find the SystemC library path by
# searching for the SystemC library archive file.
find_library(SystemC_LIBRARY
    NAMES
        systemc libsystemc.a systemc.lib
    PATHS
        ${SystemC_PATHS}
    PATH_SUFFIXES
        lib       lib64
        lib-linux lib-linux64
        lib-macos lib-macos64
)


# Try to find the SystemC version file "sc_vers.h".
find_file(SystemC_VERSION_FILE
    NAMES
        sc_ver.h
    PATHS
        ${SystemC_PATHS}
    PATH_SUFFIXES
        include/sysc/kernel
)


if(SystemC_VERSION_FILE)
    # In case we found a SystemC version file, we parse it and
    # extract the SystemC version number.
    file(READ ${SystemC_VERSION_FILE} SystemC_VERSION_FILE_CONTENTS)

    string(
	REGEX MATCH "#define SC_VERSION_MAJOR[ \t]*[0-9]+"
        SystemC_VERSION_MAJOR "${SystemC_VERSION_FILE_CONTENTS}"
    )

    string(
	REGEX REPLACE "#define SC_VERSION_MAJOR[ \t ]*([0-9]+)" "\\1"
        SystemC_VERSION_MAJOR "${SystemC_VERSION_MAJOR}"
    )

    string(
	REGEX MATCH "#define SC_VERSION_MINOR[ \t]*[0-9]+"
        SystemC_VERSION_MINOR "${SystemC_VERSION_FILE_CONTENTS}"
    )

    string(
	REGEX REPLACE "#define SC_VERSION_MINOR[ \t]*([0-9]+)" "\\1"
        SystemC_VERSION_MINOR "${SystemC_VERSION_MINOR}"
    ) 

    string(
	REGEX MATCH "#define SC_VERSION_PATCH[ \t]*[0-9]+"
        SystemC_VERSION_PATCH "${SystemC_VERSION_FILE_CONTENTS}"
    )
    
    string(
	REGEX REPLACE "#define SC_VERSION_PATCH[ \t]*([0-9]+)" "\\1"
        SystemC_VERSION_PATCH "${SystemC_VERSION_PATCH}"
    )
    
    # Now set the required variable to the extracted version number.
    set(SystemC_VERSION
        "${SystemC_VERSION_MAJOR}.${SystemC_VERSION_MINOR}.${SystemC_VERSION_PATCH}"
    )
    
    mark_as_advanced(
	    SystemC_VERSION_FILE
    )

endif()


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    SystemC
    REQUIRED_VARS
        SystemC_LIBRARY
	    SystemC_INCLUDE_DIR
    VERSION_VAR
	    SystemC_VERSION
)

function(build_systemc)
    set(SystemC_PREFIX ${SYSTEMC_HOME})
    
    include(GNUInstallDirs)
    if(UNIX)
        set(SystemC_LIBDIR ${SystemC_PREFIX}/${CMAKE_INSTALL_LIBDIR})
    else()
        set(SystemC_LIBDIR ${SystemC_PREFIX}/lib)
    endif()

    set(SystemC_INCLUDE_DIR ${SystemC_PREFIX}/include)
    set(SystemC_LIBRARY
        ${SystemC_LIBDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}systemc${CMAKE_STATIC_LIBRARY_SUFFIX}
    )

    message(STATUS "Creating custom SystemC build in: ${SystemC_PREFIX}")

    include(ExternalProject)
    ExternalProject_Add(SystemC
        SOURCE_DIR ${SYSTEMC_HOME}
        PREFIX ${SystemC_PREFIX}
        CMAKE_ARGS
            "-DCMAKE_CXX_STANDARD=17"
            "-DCMAKE_CXX_STANDARD_REQUIRED=ON"
            "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
            "-DBUILD_SHARED_LIBS=OFF"
            "-DCMAKE_INSTALL_PREFIX=${SystemC_PREFIX}"
            "-DDISABLE_COPYRIGHT_MESSAGE=TRUE"
        BUILD_COMMAND
            cmake --build . --config Release
        INSTALL_COMMAND
            cmake --build . --config Release --target install
        BUILD_BYPRODUCTS
            ${SystemC_PREFIX}
            ${SystemC_LIBRARY}
    )

    add_library(SystemC::SystemC STATIC IMPORTED)

    add_dependencies(SystemC::SystemC SystemC)
    file(MAKE_DIRECTORY ${SystemC_INCLUDE_DIR})
    set_target_properties(
        SystemC::SystemC
        PROPERTIES
            IMPORTED_LOCATION ${SystemC_LIBRARY}
            INTERFACE_INCLUDE_DIRECTORIES ${SystemC_INCLUDE_DIR}
            INTERFACE_COMPILE_OPTIONS $<$<CXX_COMPILER_ID:MSVC>:-EHsc>
            INTERFACE_LINK_LIBRARIES $<$<CXX_COMPILER_ID:GNU>:pthread>
        )

endfunction()

if(SystemC_FOUND)
    message(STATUS "Found SystemC: ${SystemC_LIBRARY} (${SystemC_VERSION})")
    if(NOT TARGET SystemC::SystemC)
        add_library(SystemC::SystemC UNKNOWN IMPORTED)

        set_target_properties(SystemC::SystemC PROPERTIES
            IMPORTED_LOCATION ${SystemC_LIBRARY}
            INTERFACE_INCLUDE_DIRECTORIES ${SystemC_INCLUDE_DIR}
            INTERFACE_COMPILE_OPTIONS $<$<CXX_COMPILER_ID:MSVC>:-EHsc>
            INTERFACE_LINK_LIBRARIES $<$<CXX_COMPILER_ID:GNU>:pthread>
        )

    endif()
    
    set(SystemC_INCLUDE_DIRS ${SystemC_INCLUDE_DIR})
    set(SystemC_LIBRARIES SystemC::SystemC)
else()
    build_systemc()
endif()