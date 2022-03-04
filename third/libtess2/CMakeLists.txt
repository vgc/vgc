# SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
# Copyright (C) [dates of first publication] Silicon Graphics, Inc.
# All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice including the dates of first publication and either this
# permission notice or a reference to http://oss.sgi.com/projects/FreeB/ shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
# PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL SILICON GRAPHICS, INC.
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
# OR OTHER DEALINGS IN THE SOFTWARE.
#
# Except as contained in this notice, the name of Silicon Graphics, Inc. shall not
# be used in advertising or otherwise to promote the sale, use or other dealings in
# this Software without prior written authorization from Silicon Graphics, Inc.

# Author: Boris Dalstein, 2020

# Used features:
# - target_include_directories (2.8.11+)
cmake_minimum_required(VERSION 2.8.11...3.23)

project(libtess2)

# Get version from README file
file(READ ${CMAKE_CURRENT_LIST_DIR}/README.md README)
string(REGEX MATCH "Version ([0-9]+).([0-9]+).([0-9]+)" VERSION_MATCH "${README}")
set(libtess2_VERSION_STRING "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
message(STATUS "Version: ${libtess2_VERSION_STRING}")

add_library(libtess2
    ${CMAKE_CURRENT_LIST_DIR}/Source/bucketalloc.c
    ${CMAKE_CURRENT_LIST_DIR}/Source/dict.c
    ${CMAKE_CURRENT_LIST_DIR}/Source/geom.c
    ${CMAKE_CURRENT_LIST_DIR}/Source/mesh.c
    ${CMAKE_CURRENT_LIST_DIR}/Source/priorityq.c
    ${CMAKE_CURRENT_LIST_DIR}/Source/sweep.c
    ${CMAKE_CURRENT_LIST_DIR}/Source/tess.c
)

target_include_directories(libtess2
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/Include
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/Source
)
