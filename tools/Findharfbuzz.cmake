# Distributed under the OSI-approved BSD 3-Clause License.
# Original author: Ebrahim Byagowi
# Original file and context: https://gitlab.kitware.com/cmake/cmake/-/merge_requests/1776
#
# The original file was modified by Boris Dalstein for VGC, in order to use
# harfbuzz::harfbuzz (all-lowercase) instead of Harfbuzz::Harfbuzz, to match the
# officially exported target by HarfBuzz (which vcpkg uses), see:
#
# https://github.com/harfbuzz/harfbuzz/pull/819
# https://github.com/harfbuzz/harfbuzz/pull/822
#
# Note: for FreeType, we are currently doing the opposite: we are using
# Freetype::Freetype (as provided by the CMake builtin FindFreetype module)
# instead of the officially exported target "freetype". We may want to change
# this in the future (use "freetype" instead of Freetype::Freetype), especially
# since the "freetype" exported target was created on 2015-11-27:
#
# https://git.savannah.gnu.org/cgit/freetype/freetype2.git/commit/?id=c80620c7fe1c11a4a6080cdec10c09ccd6a4eddaor
#
# While the Freetype::Freetype target from FindFreetype only exists since CMake
# 3.10 (released 2017-11-20). But since there are a lot of people already using
# Freetype::FreeType and Harfbuzz::Harfbuzz (e.g., WebKit, Allegro), maybe the
# best would be to have a discussion between FreeType and HarbBuzz maintainers
# and see if it wouldn't be better to change the officially exported target to
# these CamelCase variants, with namespace.
#

set(HARFBUZZ_FIND_ARGS
  HINTS
    ENV HARFBUZZ_DIR
)

find_path(
  HARFBUZZ_INCLUDE_DIRS
  hb.h
  ${HARFBUZZ_FIND_ARGS}
  PATH_SUFFIXES
    src
    harfbuzz
)

if(NOT HARFBUZZ_LIBRARY)
  find_library(HARFBUZZ_LIBRARY
    NAMES
      harfbuzz
      libharfbuzz
    ${HARFBUZZ_FIND_ARGS}
    PATH_SUFFIXES
      lib
  )
else()
  # on Windows, ensure paths are in canonical format (forward slahes):
  file(TO_CMAKE_PATH "${HARFBUZZ_LIBRARY}" HARFBUZZ_LIBRARY)
endif()

unset(HARFBUZZ_FIND_ARGS)

# set the user variables
set(HARFBUZZ_LIBRARIES "${HARFBUZZ_LIBRARY}")

include(FindPackageHandleStandardArgs)

# Notes:
#
# - In CMake 3.2 and before, find_package_handle_standard_args() uses the
#   uppercase HARFBUZZ_FOUND by default. There is an optional FOUND_VAR argument
#   that can be given to use the original case harfbuzz_FOUND instead.
#
# - Starting CMake 3.3, FOUND_VAR is made obsolete, and
#   find_package_handle_standard_args() sets both harfbuzz_FOUND and
#   HARFBUZZ_FOUND by default.
#
# Since for now, we target CMake 3.1+, we manually define harfbuzz_FOUND in case
# only HARFBUZZ_FOUND is defined.
#
find_package_handle_standard_args(
  harfbuzz
  REQUIRED_VARS
    HARFBUZZ_LIBRARY
    HARFBUZZ_INCLUDE_DIRS
  VERSION_VAR
  HARFBUZZ_VERSION_STRING
)
set(HARFBUZZ_FOUND ${harfbuzz_FOUND})

if(harfbuzz_FOUND)
  if(NOT TARGET harfbuzz::harfbuzz)
    add_library(harfbuzz::harfbuzz UNKNOWN IMPORTED)
    set_target_properties(harfbuzz::harfbuzz PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${HARFBUZZ_INCLUDE_DIRS}"
      IMPORTED_LINK_INTERFACE_LANGUAGES "C"
      IMPORTED_LOCATION "${HARFBUZZ_LIBRARY}")
  endif()
endif()
