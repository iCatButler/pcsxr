message(STATUS "* Configuring gdbstub")

set(SRCS gdbstub/gdbstub.c gdbstub_sys.c)
include_directories(gdbstub .)
add_library(gdbstub STATIC ${SRCS})
