// Copyright 2021 The VGC Developers
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

#include <pybind11/operators.h>
#include <vgc/core/wraps/common.h>
#include <vgc/graphics/font.h>

namespace vgc::graphics {

namespace {

void wrapFontHinting(py::module& m) {
    py::enum_<FontHinting>(m, "FontHinting")
        .value("None", FontHinting::None)
        .value("Native", FontHinting::Native)
        .value("AutoLight", FontHinting::AutoLight)
        .value("AutoNormal", FontHinting::AutoNormal);
}

void wrapSizedFontParams(py::module& m) {
    using This = SizedFontParams;
    py::class_<This>(m, "SizedFontParams")
        .def(py::init<Int, Int, FontHinting>())
        .def(py::init<Int, FontHinting>())
        .def(
            py::init([](Int pointSize, Int dpi, FontHinting hinting) {
                return This::fromPoints(pointSize, dpi, hinting);
            }),
            "pointSize"_a,
            "dpi"_a,
            "hinting"_a)
        .def(
            py::init([](Int pointSize, Int hdpi, Int vdpi, FontHinting hinting) {
                return This::fromPoints(pointSize, hdpi, vdpi, hinting);
            }),
            "pointSize"_a,
            "hdpi"_a,
            "vdpi"_a,
            "hinting"_a)
        .def_property_readonly("ppemWidth", &This::ppemWidth)
        .def_property_readonly("ppemHeight", &This::ppemHeight)
        .def_property_readonly("hinting", &This::hinting)
        .def(py::self == py::self)
        .def(py::self != py::self);
}

void wrapFontLibrary(py::module& m) {
    using This = FontLibrary;
    using Holder = FontLibraryPtr;
    using Parent = core::Object;
    py::class_<This, Holder, Parent>(m, "FontLibrary")
        .def(py::init([]() { return This::create(); }))
        .def("addFont", &This::addFont, "filename"_a, "index"_a = 0)
        .def_property("defaultFont", &This::defaultFont, &This::setDefaultFont);
}

void wrapFont(py::module& m) {
    using This = Font;
    using Holder = FontPtr;
    using Parent = core::Object;
    py::class_<This, Holder, Parent>(m, "Font")
        .def_property_readonly("library", &This::library)
        .def_property_readonly("index", &This::index)
        .def("getSizedFont", &This::getSizedFont)
        .def("getGlyphFromCodePoint", &This::getGlyphFromCodePoint)
        .def("getGlyphFromIndex", &This::getGlyphFromIndex)
        .def("getGlyphIndexFromCodePoint", &This::getGlyphIndexFromCodePoint);
}

void wrapGlyph(py::module& m) {
    using This = Glyph;
    using Holder = GlyphPtr;
    using Parent = core::Object;
    py::class_<This, Holder, Parent>(m, "Glyph")
        .def_property_readonly("font", &This::font)
        .def_property_readonly("index", &This::index)
        .def_property_readonly("name", &This::name);
}

void wrapSizedFont(py::module& m) {
    using This = SizedFont;
    using Holder = SizedFontPtr;
    using Parent = core::Object;
    py::class_<This, Holder, Parent>(m, "SizedFont")
        .def_property_readonly("font", &This::font)
        .def_property_readonly("params", &This::params)
        .def_property_readonly("ascent", &This::ascent)
        .def_property_readonly("descent", &This::descent)
        .def_property_readonly("height", &This::height)
        .def("getSizedGlyphFromCodePoint", &This::getSizedGlyphFromCodePoint)
        .def("getSizedGlyphFromIndex", &This::getSizedGlyphFromIndex)
        .def("getGlyphIndexFromCodePoint", &This::getGlyphIndexFromCodePoint);
}

void wrapSizedGlyph(py::module& m) {
    using core::FloatArray;
    using geometry::Mat3f;
    using geometry::Vec2f;

    using This = SizedGlyph;
    using Holder = SizedGlyphPtr;
    using Parent = core::Object;
    py::class_<This, Holder, Parent>(m, "SizedGlyph")
        .def_property_readonly("sizedFont", &This::sizedFont)
        .def_property_readonly("glyph", &This::glyph)
        .def_property_readonly("index", &This::index)
        .def_property_readonly("name", &This::name)
        .def_property_readonly("outline", &This::outline)
        .def_property_readonly("boundingBox", &This::boundingBox)
        .def_property_readonly(
            "fill", py::overload_cast<FloatArray&, const Mat3f&>(&This::fill, py::const_))
        .def_property_readonly(
            "fill", py::overload_cast<FloatArray&, const Vec2f&>(&This::fill, py::const_))
        .def_property_readonly("fillYMirrored", &This::fillYMirrored);
}

} // namespace

void wrap_font(py::module& m) {
    // Necessary to define inheritance across modules. See:
    // http://pybind11.readthedocs.io/en/stable/advanced/misc.html#partitioning-code-over-multiple-extension-modules
    py::module::import("vgc.core");

    wrapFontHinting(m);
    wrapSizedFontParams(m);
    wrapFontLibrary(m);
    wrapFont(m);
    wrapGlyph(m);
    wrapSizedFont(m);
    wrapSizedGlyph(m);
}

} // namespace vgc::graphics
