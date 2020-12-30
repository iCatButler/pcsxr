cmake_minimum_required(VERSION 3.4.0)
cmake_policy(SET CMP0065 NEW)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/macros)

set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build")
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)

project(pcsxr)

set(PCSXR_VERSION_MAJOR "1")
set(PCSXR_VERSION_MINOR "9")
set(PCSXR_VERSION_PATCH "94")
add_definitions(-DPACKAGE_VERSION="${PCSXR_VERSION_MAJOR}.${PCSXR_VERSION_MINOR}.${PCSXR_VERSION_PATCH}")
add_definitions(-DPACKAGE_NAME="PCSXr")
add_definitions(-DPACKAGE_STRING="PCSXr ${PCSXR_VERSION_MAJOR}.${PCSXR_VERSION_MINOR}.${PCSXR_VERSION_PATCH}")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
#uncomment for gdb server debugging
#add_definitions(-DDEBUG=1)

include(CheckCCompilerFlag)
include(GNUInstallDirs)

include_directories(${CMAKE_SOURCE_DIR})

add_definitions(-DENABLE_NLS -DGETTEXT_PACKAGE="pcsxr")

#WARNING! this is required for dynarec to work!
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fno-pie")
list(APPEND COMPILER_IDS "GNU" "Clang")
if(CMAKE_C_COMPILER_ID IN_LIST COMPILER_IDS)
    CHECK_C_COMPILER_FLAG("-no-pie" NO_PIE_UPSTREAM)
    if(NO_PIE_UPSTREAM)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -no-pie")
    endif()
    CHECK_C_COMPILER_FLAG("-nopie" NO_PIE_PATCHED)
    if(NO_PIE_PATCHED)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nopie")
    endif()
endif()

option(BUILD_SIO1 "Build SIO1 plugin." ON)
option(BUILD_OPENGL "Build OpenGL plugin." ON)

if (BUILD_SIO1)
  add_definitions(-DENABLE_SIO1API)
endif()

#components
add_subdirectory(libpcsxcore)
add_subdirectory(gui)
add_subdirectory(plugins)
add_subdirectory(doc)
add_subdirectory(dynstr)
add_subdirectory(gdbstub)
