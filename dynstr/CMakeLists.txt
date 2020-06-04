message(STATUS "* Configuring dynstr")

set(SRCS dynstr/dynstr.c)
include_directories(dynstr/include)
add_library(dynstr STATIC ${SRCS})
