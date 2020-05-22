message(STATUS "* Configuring Core")

set(DYNAREC "auto" CACHE STRING "Build dynarec for arch.")
set_property(CACHE DYNAREC PROPERTY STRINGS auto x86_64 x86 ppc no)

option(ENABLE_CCDDA "Enables compressed CDDA support." OFF)
option(USE_LIBARCHIVE "Enables compressed data-tracks support." OFF)

include_directories(../dynstr/dynstr/include)

if (ENABLE_CCDDA)
  find_package(FFMPEG REQUIRED)
  include_directories(${FFMPEG_INCLUDE_DIRS})
  add_definitions(-DENABLE_CCDDA)
endif()

if (USE_LIBARCHIVE)
  find_package(LibArchive REQUIRED)
  include_directories(${LibArchive_INCLUDE_DIRS})
  add_definitions(-DHAVE_LIBARCHIVE)
endif()

# Architecture detection and arch specific settings
message(STATUS "Building PCSXr on arch " ${CMAKE_SYSTEM_PROCESSOR})
include(TargetArch)
target_architecture(PCSXR_TARGET_ARCH)
message(STATUS "Targeting arch " ${PCSXR_TARGET_ARCH})
if (${PCSXR_TARGET_ARCH} MATCHES "ppc")
    set(_ARCH_PPC 1)
elseif(${PCSXR_TARGET_ARCH} MATCHES "i386")
    set(_ARCH_32 1)
elseif(${PCSXR_TARGET_ARCH} MATCHES "x86_64")
    set(_ARCH_64 1)
else()
    message(STATUS "Unsupported arch. Will not build dynarec")
    add_definitions(-DNOPSXREC)
endif()
set(CMAKE_POSITION_INDEPENDENT_CODE OFF) #for x86

if (${DYNAREC} STREQUAL "auto")
    if (_ARCH_PPC)
#if anyone ever fixes ppc dynarec
#        set(DYNAREC_PPC 1)
#        message(STATUS "Autodetected PPC dynarec.")
    message(STATUS "Autodetected PPC dynarec is broken, sorry.")
    add_definitions(-DNOPSXREC)
    elseif(_ARCH_64)
        set(DYNAREC_64 1)
        message(STATUS "Autodetected x86_64 dynarec.")
    elseif(_ARCH_32)
        set(DYNAREC_32 1)
        message(STATUS "Autodetected x86 dynarec.")
    endif()
elseif (${DYNAREC} STREQUAL "ppc")
#if anyone ever fixes ppc dynarec
#    set(DYNAREC_PPC 1)
#    message(STATUS "User selected PPC dynarec")
    message(STATUS "User selected PPC dynarec is broken, sorry.")
    add_definitions(-DNOPSXREC)
elseif (${DYNAREC} STREQUAL "x86_64")
    set(DYNAREC_64 1)
    message(STATUS "User selected x86_64 dynarec.")
elseif (${DYNAREC} STREQUAL "x86")
    set(DYNAREC_32 1)
    message(STATUS "User selected x86 dynarec.")
elseif (${DYNAREC} STREQUAL "no")
    message(STATUS "User selected to not build dynarec.")
    add_definitions(-DNOPSXREC)
endif()


set(SRCS  psxbios.c
          cdrom.c
          psxcounters.c
          psxdma.c
          disr3000a.c
          gpu.c
          spu.c
          sio.c
          psxhw.c
          mdec.c
          psxmem.c
          misc.c
          plugins.c
          decode_xa.c
          r3000a.c
          psxinterpreter.c
          gte.c
          psxhle.c
          debug.c
          psxcommon.c
          cdriso.c
          cheat.c
          socket.c
          ppf.c
          pgxp_cpu.c
          pgxp_debug.c
          pgxp_gte.c
          pgxp_mem.c
          pgxp_value.c
          gdb_server.c
)

set(LIBS "-lm")

if(DYNAREC_64)
    file(GLOB_RECURSE DYNAREC_SRC  ix86_64/*.c)
elseif(DYNAREC_32)
    file(GLOB_RECURSE DYNAREC_SRC  ix86/*.c)
elseif(DYNAREC_PPC)
    enable_language(ASM-ATT)
    SET(CMAKE_ASM-ATT_SOURCE_FILE_EXTENSIONS nasm;nas;asm;s)
    file(GLOB_RECURSE DYNAREC_SRC  ppc/*.c)
    set(DYNAREC_SRC ${DYNAREC_SRC} ppc/pasm.s)
endif()

set(SRCS ${SRCS} ${DYNAREC_SRC})

add_library(pcsxcore STATIC ${SRCS})
target_link_libraries(pcsxcore dynstr ${FFMPEG_LIBRARIES} ${LibArchive_LIBRARIES} ${LIBS})
