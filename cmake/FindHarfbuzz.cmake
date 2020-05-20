find_package(PkgConfig)

set(Harfbuzz_DEPS)

if(PKG_CONFIG_FOUND)
  pkg_search_module(Harfbuzz_PKG harfbuzz)
endif()

find_library(Harfbuzz_LIBRARY harfbuzz HINTS ${Harfbuzz_PKG_LIBRARY_DIRS})
set(Harfbuzz harfbuzz)

if(Harfbuzz_LIBRARY)
  add_library(${Harfbuzz} SHARED IMPORTED)
  set_property(TARGET ${Harfbuzz} PROPERTY IMPORTED_LOCATION "${Harfbuzz_LIBRARY}")
  set_property(TARGET ${Harfbuzz} PROPERTY INTERFACE_COMPILE_OPTIONS "${Harfbuzz_PKG_CFLAGS_OTHER}")

  set(Harfbuzz_INCLUDE_DIRS)

  find_path(Harfbuzz_INCLUDE_DIR "hb.h"
    HINTS ${Harfbuzz_PKG_INCLUDE_DIRS})

  if(Harfbuzz_INCLUDE_DIR)
    file(STRINGS "${Harfbuzz_INCLUDE_DIR}/hb-version.h" HB_VERSION_MAJOR REGEX "^#define HB_VERSION_MAJOR +\\(?([0-9]+)\\)?$")
    string(REGEX REPLACE "^#define HB_VERSION_MAJOR \\(?([0-9]+)\\)?$" "\\1" HB_VERSION_MAJOR "${HB_VERSION_MAJOR}")
    file(STRINGS "${Harfbuzz_INCLUDE_DIR}/hb-version.h" HB_VERSION_MINOR REGEX "^#define HB_VERSION_MINOR +\\(?([0-9]+)\\)?$")
    string(REGEX REPLACE "^#define HB_VERSION_MINOR \\(?([0-9]+)\\)?$" "\\1" HB_VERSION_MINOR "${HB_VERSION_MINOR}")
    file(STRINGS "${Harfbuzz_INCLUDE_DIR}/hb-version.h" HB_VERSION_MICRO REGEX "^#define HB_VERSION_MICRO +\\(?([0-9]+)\\)?$")
    string(REGEX REPLACE "^#define HB_VERSION_MICRO \\(?([0-9]+)\\)?$" "\\1" HB_VERSION_MICRO "${HB_VERSION_MICRO}")
    set(Harfbuzz_VERSION "${HB_VERSION_MAJOR}.${HB_VERSION_MINOR}.${HB_VERSION_MICRO}")
    unset(HB_VERSION_MAJOR)
    unset(HB_VERSION_MINOR)
    unset(HB_VERSION_MICRO)

    list(APPEND Harfbuzz_INCLUDE_DIRS ${Harfbuzz_INCLUDE_DIR})
    set_property(TARGET ${Harfbuzz} PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${Harfbuzz_INCLUDE_DIR}")
  endif()
endif()

set(Harfbuzz_DEPS_FOUND_VARS)
foreach(harfbuzz_dep ${Harfbuzz_DEPS})
  find_package(${harfbuzz_dep})

  list(APPEND Harfbuzz_DEPS_FOUND_VARS "${harfbuzz_dep}_FOUND")
  list(APPEND Harfbuzz_INCLUDE_DIRS ${${harfbuzz_dep}_INCLUDE_DIRS})

  set_property (TARGET ${Harfbuzz} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${${harfbuzz_dep}}")
endforeach(harfbuzz_dep)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Harfbuzz
    REQUIRED_VARS
      Harfbuzz_LIBRARY
      Harfbuzz_INCLUDE_DIRS
      ${Harfbuzz_DEPS_FOUND_VARS}
    VERSION_VAR
      Harfbuzz_VERSION)

unset(Harfbuzz_DEPS_FOUND_VARS)
