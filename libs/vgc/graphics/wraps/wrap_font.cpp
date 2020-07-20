// Copyright 2020 The VGC Developers
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

#include <vgc/core/wraps/common.h>
#include <vgc/graphics/font.h>

namespace vgc {
namespace graphics {

namespace {

void wrapFontLibrary(py::module& m)
{
    using This = FontLibrary;
    using Holder = FontLibraryPtr;
    using Parent = core::Object;
    py::class_<This, Holder, Parent>(m, "FontLibrary")
        .def(py::init([]() { return This::create(); } ))
        .def("addFace", &This::addFace)
    ;
}

void wrapFontFace(py::module& m)
{
    using This = FontFace;
    using Holder = FontFacePtr;
    using Parent = core::Object;
    py::class_<This, Holder, Parent>(m, "FontFace")
        .def("getGlyphFromCodePoint", &This::getGlyphFromCodePoint)
        .def("getGlyphFromIndex", &This::getGlyphFromIndex)
        .def("getGlyphIndexFromCodePoint", &This::getGlyphIndexFromCodePoint)
    ;
}

void wrapFontGlyph(py::module& m)
{
    using This = FontGlyph;
    using Holder = FontGlyphPtr;
    using Parent = core::Object;
    py::class_<This, Holder, Parent>(m, "FontGlyph")
        .def_property_readonly("name", &This::name)
        .def_property_readonly("index", &This::index)
    ;
}

} // namespace

void wrap_font(py::module& m)
{
    // Necessary to define inheritance across modules. See:
    // http://pybind11.readthedocs.io/en/stable/advanced/misc.html#partitioning-code-over-multiple-extension-modules
    py::module::import("vgc.core");

    wrapFontLibrary(m);
    wrapFontFace(m);
    wrapFontGlyph(m);
}

} // namespace graphics
} // namespace vgc
