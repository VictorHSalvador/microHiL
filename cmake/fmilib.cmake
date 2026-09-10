set(MICROHIL_FMILIB_VERSION "3.0.4")
set(MICROHIL_FMILIB_REVISION "4a4b21ec10a632b2768a604c2330c54204919644")

set(FMILIB_ROOT "" CACHE PATH "FMILibrary installation prefix")
set(FMILIB_INCLUDE_DIR "" CACHE PATH "Directory containing fmilib.h")
set(FMILIB_LIBRARY "" CACHE FILEPATH "Path to the FMILibrary binary")

function(microhil_resolve_fmilib result_target)
    if(NOT FMILIB_INCLUDE_DIR)
        find_path(FMILIB_INCLUDE_DIR
            NAMES fmilib.h
            HINTS
                "${FMILIB_ROOT}/include"
                "$ENV{FMILIB_ROOT}/include"
        )
    endif()

    if(NOT FMILIB_LIBRARY)
        find_library(FMILIB_LIBRARY
            NAMES fmilib fmilib_shared
            HINTS
                "${FMILIB_ROOT}/lib"
                "${FMILIB_ROOT}/lib64"
                "$ENV{FMILIB_ROOT}/lib"
                "$ENV{FMILIB_ROOT}/lib64"
        )
    endif()

    if(FMILIB_INCLUDE_DIR AND FMILIB_LIBRARY)
        add_library(microhil_fmilib_external UNKNOWN IMPORTED)
        set_target_properties(microhil_fmilib_external PROPERTIES
            IMPORTED_LOCATION "${FMILIB_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${FMILIB_INCLUDE_DIR}"
        )
        message(STATUS
            "Using external FMILibrary at ${FMILIB_LIBRARY}; the integrator must prove its version and compatibility."
        )
        set(${result_target} microhil_fmilib_external PARENT_SCOPE)
        return()
    endif()

    if(MICROHIL_FETCH_FMILIB)
        include(FetchContent)

        set(FMILIB_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(FMILIB_GENERATE_DOXYGEN_DOC OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(microhil_fmilib
            GIT_REPOSITORY https://github.com/modelon-community/fmi-library.git
            GIT_TAG ${MICROHIL_FMILIB_REVISION}
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(microhil_fmilib)

        # FMILibrary configures bundled dependencies as external projects and expects a cache file in its own binary directory.
        file(TOUCH "${microhil_fmilib_BINARY_DIR}/CMakeCache.txt")

        if(NOT TARGET fmilib_shared)
            message(FATAL_ERROR
                "Pinned FMILibrary ${MICROHIL_FMILIB_VERSION} (${MICROHIL_FMILIB_REVISION}) did not provide fmilib_shared."
            )
        endif()

        message(STATUS
            "Using fetched FMILibrary ${MICROHIL_FMILIB_VERSION} at immutable revision ${MICROHIL_FMILIB_REVISION}."
        )
        set(${result_target} fmilib_shared PARENT_SCOPE)
        return()
    endif()

    message(FATAL_ERROR
        "FMILibrary is required because MICROHIL_BUILD_RUNNER=ON. Provide an external installation with "
        "-DFMILIB_ROOT=<prefix> or -DFMILIB_INCLUDE_DIR=<include-dir> -DFMILIB_LIBRARY=<library>, "
        "or fetch the pinned official ${MICROHIL_FMILIB_VERSION} revision with -DMICROHIL_FETCH_FMILIB=ON."
    )
endfunction()
