// Copyright 2018 The VGC Developers
// See the COPYRIGHT file at the top-level directory of this distribution
// and at https://github.com/vgc/vgc/blob/master/COPYRIGHT
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <vgc/core/vec2darray.h>
#include <vgc/core/stringutil.h>

namespace vgc {
namespace core {

std::string toString(const Vec2dArray& a)
{
    return toString(a.stdVector());
}

} // namespace core
} // namespace vgc

// Design notes:
//
// Instead of having a custom Vec2dArray, we could simply:
//
// using Vec2dArray = std::vector<Vec2d>;
//
// Why not doing that?
//
// The main reason stems from two goals:
//
// Goal #1: Provide the best possible user experience for end users writing
//          Python scripts. This is the most important.
//
// Goal #2: Have an API as similar as possible between Python code and C++ code.
//          This is important but less important than the first point.
//
// Having a nice idiomatic C++ code that expert C++ developers would favor is
// less important than the two goals above: expert C++ developers should have
// enough technical knowledge and willingness to adapt to new APIs.
//
// Regarding sequences of Vec2d elements, on Python side, we basically have
// three options:
//
// 1. Have users manipulates built-in Python lists of Vec2d
// 2. Have users manipulates numpy's ndarrays
// 3. Have users manipulate a custom type defined by the VGC core library.
//
// The first option is unacceptable for performance reasons. We may provide
// convenient conversion functions for converting from/to built-in Python
// list, but there should definitly be also an existing efficient type, which
// should be the default to avoid early pessimization.
//
// The second option is pretty good: ndarrays are performant and pythonic, some
// Python users might already know numpy, and using numpy allows to leverage
// existing library processing nparrays. Unfortunately, it is a bit awkward for
// users who just start writing Python scripts for the first time through VGC:
// there are inconsistencies between how VGC types look and how sequences of
// Vec2d elements look.
//
// For example, for accessing the i-th Vec2d in a ndarray, you would do:
//
// a = positionsAttribute.value() # 'a' is a ndarray of dimension 2
// p = a[3]                       # 'p' is a ndarray of dimension 1
//
// In order to get back a Vec2d to use with the rest of VGC API, you'd need to
// explicitly "cast":
//
// p = Vec2d(a[3])
//
// Of course, one solution would be to not have Vec2d as well! However, this
// provides less type safety (an ndarray can have any dimension), and ndarrays
// don't have the convenient low-dimension geometry features that you expect
// from a "2D vector" class, such as length(), normalize(), etc. As a case in
// point, to get the length, you'd have to do:
//
// np.linalg.norm(p) # equivalent to our p.length()
//
// Also, printing a ndarray uses "[1, 2]" syntax, while we desire "(1, 2)"
// syntax. Finally, one issue with ndarrays as replacement for Vec2d is
// that the number type of the array is deduced from the type of the elements
// in the Python sequences. For example:
//
// p = np.array([1, 2])
//
// Creates a ndarray using int64! One has to explicitly do:
//
// p = np.array([1, 2], dtype=np.float64)
//
// in order to get the correct type. Compare this our simple and readable:
//
// p = Vec2d(1, 2)
//
// In conclusion, we'd never trade Vec2d for ndarrays. And since indexing a
// ndarray gives a ndarray and not a Vec2d, we'd better have our own custom
// Vec2dArray type, which returns directly elements of type Vec2d.
//
// Back to C++: we still have the choice to use either std::vector<Vec2d> or
// our own custom type. It is possible to convert a C++ std::vector<Vec2d> to a
// Python's Vec2dArray in the bindings. However, this does create some
// inconsistency between Python and C++. I believe it would be frustrating for
// the occasional C++ programmer used to writing VGC Python script to lose the
// convenient API of Python's Vec2dArray. Also, for documentation purposes, it
// is easier to have a C++ class corresponding to the Python class.
//
// Of course, all of this is a bit subjective, and it's hard to tell exactly
// which solution is "better". But anyway, this is the rationale why we are
// using Vec2dArray instead of std::vector. It was a hard decision.
//

// XXX Actually, I just realize now that in the specific case of basic
// containers, it may not really a good idea to make the interface similar
// between python and C++, since Python have better syntax for sequences
// (slicing, __len__ function, etc.) with no C++ equivalent. Therefore, if the
// API for Vec2dArray diverges between Python and C++ anyway, maybe it is
// better to stick with std::vector in C++. In any case, it is still probably a
// good idea to keep using a typedef (using Vec2dArray = std::vector<Vec2d>) so
// that in the documentation, the signature of functions take Vec2dArray, which
// is consistent between C++ and Python. Or alternatively, like in NumPy, we
// may want to support uninitialized arrays. In this case, we would have to
// implement from scratch a vector type and not rely on std::vector at all.
//
