#!/usr/bin/env python3
#
# Copyright 2022 The VGC Developers
# See the COPYRIGHT file at the top-level directory of this distribution
# and at https://github.com/vgc/vgc/blob/master/COPYRIGHT
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import re
from pathlib import Path

autogenInfoIn = '''
// This file is used to generate all the variants of this class.
// You must manually run generate.py after any modification.
'''

autogenInfoOut = '''
// This file was automatically generated, please do not edit directly.
// Instead, edit tools/{} then run tools/generate.py.
'''


# Note: we must call replaceAutogenInfo() after replaceTypes(), otherwise our
# desired "vec2x.h" would be replaced to "vec2f.h". And since we call it after,
# note that we must search for "vec2f.py" in autogenInfoIn, not for "vec2x.py".
def replaceAutogenInfo(s, typ, n, x, suffix):
    return s.replace(autogenInfoIn,
                     autogenInfoOut.format(f"{typ}{n}x{suffix}"))


def replaceTypes(s, x):
    X = x.upper()
    for n in [1, 2, 3, 4]:
        for typ in ["mat", "range", "rect", "triangle", "vec"]:
            Typ = typ.title()
            TYP = typ.upper()
            s = s.replace(f"{typ}{n}x", f"{typ}{n}{x}")
            s = s.replace(f"{Typ}{n}x", f"{Typ}{n}{x}")
            s = s.replace(f"{TYP}{n}X", f"{TYP}{n}{X}")
    return s


def replaceIncludes(s, x):
    s = re.sub('#include "([0-9a-z]+)x.h"', f'#include <vgc/geometry/\\1{x}.h>', s)
    return s


def replaceFloat(s, xname):
    if xname == "float":
        s = s.replace("%scalar-type-description%", "single-precision floating points")
    elif xname == "double":
        s = s.replace("float", xname)
        s = s.replace("%scalar-type-description%", "double-precision floating points")
        s = s.replace("1e-5f", "1e-9")
        s = re.sub(r"([0-9]+\.?[0-9]*([eE][+-]?[0-9]+)?)f", r"\1", s)
        s = s.replace("isClose(double, double)", "isClose(float, float)")
    return s


def generateFile(typ, n, xname, suffix):
    x = xname[0]
    thisFile = Path(__file__).resolve()
    inFile = thisFile.parent / f"{typ}{n}x{suffix}"
    outFile = thisFile.parent.parent / f"{typ}{n}{x}{suffix}"
    s = inFile.read_text()
    s = replaceIncludes(s, x)
    s = replaceTypes(s, x)
    s = replaceFloat(s, xname)
    s = replaceAutogenInfo(s, typ, n, x, suffix)
    outFile.write_text(s)


def generateVariant(typ, n, xname):
    generateFile(typ, n, xname, ".h")
    generateFile(typ, n, xname, ".cpp")


if __name__ == "__main__":
    generateVariant("mat", "2", "float")
    generateVariant("mat", "2", "double")
    generateVariant("mat", "3", "float")
    generateVariant("mat", "3", "double")
    generateVariant("mat", "4", "float")
    generateVariant("mat", "4", "double")
    generateVariant("range", "1", "float")
    generateVariant("range", "1", "double")
    generateVariant("rect", "2", "float")
    generateVariant("rect", "2", "double")
    generateVariant("triangle", "2", "float")
    generateVariant("triangle", "2", "double")
    generateVariant("vec", "2", "float")
    generateVariant("vec", "2", "double")
    generateVariant("vec", "3", "float")
    generateVariant("vec", "3", "double")
    generateVariant("vec", "4", "float")
    generateVariant("vec", "4", "double")
