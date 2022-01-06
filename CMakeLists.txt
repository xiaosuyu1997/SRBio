cmake_minimum_required(VERSION 3.12)

project(SRBio VERSION 0.1.0)

# disable in-source builds
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

# set C99 standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

# set module path
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

file(GLOB_RECURSE C_SOURCE ${PROJECT_SOURCE_DIR}/src/*.c)
list(FILTER C_SOURCE EXCLUDE REGEX "${PROJECT_SOURCE_DIR}/src/main\.c")
list(FILTER C_SOURCE EXCLUDE REGEX "${PROJECT_SOURCE_DIR}/src/mex_srbio_.*\.c")
set(SOURCE ${C_SOURCE})

# set RPATH
set(CMAKE_SKIL_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# the RPATH to be used when installing, but only if it's not a system directory
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
if("${isSystemDir}" STREQUAL "-1")
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif("${isSystemDir}" STREQUAL "-1")


# define options
option(BUILD_SHARED_LIBS "Enable shared library. Default: ON" ON)
option(ENABLE_IF_MATLAB
    "Enable the build of matlab interface. Default: OFF" OFF)

# set compile options
if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
    message(STATUS "CMAKE_BUILD_TYPE is unset, defaulting to Debug")
    set(CMAKE_BUILD_TYPE "Debug")
endif()

if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
    add_compile_options(-Wall -Wextra -Wno-format-truncation -m64 -march=native)
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        add_compile_options(-Wpedantic)
    else()
        add_compile_options(-Wno-unused-parameter)
    endif()
elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "Intel")
    # note: not working for current version because C++17 support is poor
    add_compile_options(-Wall -inline-forceinline)
elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options(-Wall -Wextra -march=native)
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        add_compile_options(-Wpedantic)
    else()
        add_compile_options(-Wno-unused-parameter)
    endif()
else()
    message(WARNING "Unsupport C compiler ${CMAKE_C_COMPILER_ID}")
endif()

include_directories("${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/include")

# define targets
add_executable(rbio src/main.c)
add_library(SRBio_lp64_double ${SOURCE})
add_library(SRBio_ilp64_double ${SOURCE})
add_library(SRBio_lp64_single ${SOURCE})
add_library(SRBio_ilp64_single ${SOURCE})
target_link_libraries(rbio SRBio_lp64_double)
target_compile_definitions(SRBio_lp64_double PRIVATE SRBIO_DOUBLE_PRECISION)
target_compile_definitions(SRBio_ilp64_double PRIVATE SRBIO_ILP64 SRBIO_DOUBLE_PRECISION)
target_compile_definitions(SRBio_lp64_single PRIVATE SRBIO_SINGLE_PRECISION)
target_compile_definitions(SRBio_ilp64_single PRIVATE SRBIO_ILP64 SRBIO_SINGLE_PRECISION)



# zlib support
find_package(ZLIB)
if (ZLIB_FOUND)
    message(STATUS "Enable zlib support for SRBio")
    set(SRBIO_USE_ZLIB ON)
    target_link_libraries(SRBio_lp64_double ZLIB::ZLIB)
    target_link_libraries(SRBio_lp64_single ZLIB::ZLIB)
    target_link_libraries(SRBio_ilp64_double ZLIB::ZLIB)
    target_link_libraries(SRBio_ilp64_single ZLIB::ZLIB)
endif()

# bzip2 support
find_package(BZip2)
if (BZIP2_FOUND)
    message(STATUS "Enable bzip2 support for SRBio")
    set(SRBIO_USE_BZIP2 ON)
    target_link_libraries(SRBio_lp64_double BZip2::BZip2)
    target_link_libraries(SRBio_lp64_single BZip2::BZip2)
    target_link_libraries(SRBio_ilp64_double BZip2::BZip2)
    target_link_libraries(SRBio_ilp64_single BZip2::BZip2)
endif()

# config file
configure_file(include/SRBio_config.h.in SRBio_config.h)

# matlab support
if (ENABLE_IF_MATLAB)
    find_package(Matlab COMPONENTS MEX_COMPILER)

    if (Matlab_FOUND)
        message(STATUS "Matlab found at ${Matlab_ROOT_DIR}")

        # add matlab interface library
        matlab_add_mex(
            NAME mex_srbio_read SHARED
            SRC ${PROJECT_SOURCE_DIR}/src/mex_srbio_read.c
            LINK_TO SRBio_ilp64_double
        )
        matlab_add_mex(
            NAME mex_srbio_write SHARED
            SRC ${PROJECT_SOURCE_DIR}/src/mex_srbio_write.c
            LINK_TO SRBio_ilp64_double
        )

        # installing
        install(TARGETS mex_srbio_read LIBRARY DESTINATION lib)
        install(TARGETS mex_srbio_write LIBRARY DESTINATION lib)
        install(DIRECTORY src/matlab DESTINATION src)

    else()
        message(WARNING "-DENABLE_IF_MATLAB=ON is set but Matlab not found. "
            "Try passing -DMatlab_ROOT_DIR to specify Matlab installation location")
    endif()
endif()

# installing
install(TARGETS rbio SRBio_lp64_double SRBio_lp64_single SRBio_ilp64_double SRBio_ilp64_single
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )
install(DIRECTORY include/
    DESTINATION include/SRBio
    FILES_MATCHING PATTERN "*.h"
    )
install(FILES ${PROJECT_BINARY_DIR}/SRBio_config.h
    DESTINATION include/SRBio)

