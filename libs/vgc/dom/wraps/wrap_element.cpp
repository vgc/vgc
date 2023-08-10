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

#include <string>

#include <vgc/core/format.h>
#include <vgc/dom/document.h>
#include <vgc/dom/element.h>
#include <vgc/dom/strings.h>

#include <vgc/core/wraps/array.h>
#include <vgc/core/wraps/common.h>
#include <vgc/core/wraps/object.h>

using This = vgc::dom::Element;
using Parent = vgc::dom::Node;

using vgc::core::StringId;
using vgc::dom::Document;
using vgc::dom::Element;

namespace {

template<typename OutputIt, typename T>
OutputIt writeRepr(OutputIt out, T&& s) {
    py::object obj = py::cast(std::forward<T>(s));
    std::string r = py::cast<std::string>(obj.attr("__repr__")());
    return vgc::core::copyStringTo(out, r);
}

template<typename OutputIt>
OutputIt writeAttributes(OutputIt out, const This& self) {
    namespace ss = vgc::dom::strings;
    namespace core = vgc::core;

    vgc::dom::Value v = self.getAuthoredAttribute(ss::name);
    if (v.hasValue()) {
        core::copyStringTo(out, " name=");
        out = writeRepr(out, v.getStringId().string());
    }

    v = self.getAuthoredAttribute(ss::id);
    if (v.hasValue()) {
        core::copyStringTo(out, " id=");
        out = writeRepr(out, v.getStringId().string());
    }
    for (const vgc::dom::AuthoredAttribute& attr : self.authoredAttributes()) {
        const vgc::core::StringId name = attr.name();
        if (name != ss::name && name != ss::id) {
            *out++ = ' ';
            out = core::copyStringTo(out, name.string());
            *out++ = '=';
            fmt::memory_buffer mbuf;
            fmt::format_to(std::back_inserter(mbuf), "{}", attr.value());
            std::string sr(mbuf.begin(), mbuf.size());
            out = writeRepr(out, sr);
        }
    }

    return out;
}

template<typename OutputIt>
OutputIt writeAttributesRepr(OutputIt out, const This& self) {
    namespace ss = vgc::dom::strings;
    namespace core = vgc::core;

    vgc::dom::Value v = self.getAuthoredAttribute(ss::name);
    if (v.hasValue()) {
        core::copyStringTo(out, " name=");
        out = writeRepr(out, v.getStringId().string());
    }

    v = self.getAuthoredAttribute(ss::id);
    if (v.hasValue()) {
        core::copyStringTo(out, " id=");
        out = writeRepr(out, v.getStringId().string());
    }

    //for (const vgc::dom::AuthoredAttribute& attr : self.authoredAttributes()) {
    //    const vgc::core::StringId name = attr.name();
    //    if (name != ss::name && name != ss::id) {
    //        *out++ = ' ';
    //        out = core::copyStringTo(out, name.string());
    //        *out++ = '=';
    //        // XXX redundant with code in wrap_value
    //        out = attr.value().visit([&](auto&& arg) -> OutputIt {
    //            using T = std::decay_t<decltype(arg)>;
    //            if constexpr (std::is_same_v<T, vgc::dom::NoneValue>) {
    //                return core::copyStringTo(out, "vgc.dom.Value.none");
    //            }
    //            else if constexpr (std::is_same_v<T, vgc::dom::InvalidValue>) {
    //                return core::copyStringTo(out, "vgc.dom.Value.invalid");
    //            }
    //            else {
    //                py::object obj = py::cast(&arg);
    //                std::string r = py::cast<std::string>(obj.attr("__repr__")());
    //                return core::copyStringTo(out, r);
    //            }
    //        });
    //    }
    //}

    return out;
}

} // namespace

void wrap_element(py::module& m) {

    vgc::core::wraps::wrapObjectCommon<This>(m, "Element");
    vgc::core::wraps::wrap_array<This*, false>(m, "Element");

    vgc::core::wraps::ObjClass<This>(m, "Element")
        .def_create<This*, Document*, std::string_view>("parent"_a, "tagName"_a)
        .def_create<This*, Document*, StringId>("parent"_a, "tagName"_a)
        .def_create<This*, Element*, std::string_view, Element*>(
            "parent"_a, "tagName"_a, "nextSibling"_a = static_cast<Element*>(nullptr))
        .def_create<This*, Element*, StringId, Element*>(
            "parent"_a, "tagName"_a, "nextSibling"_a = static_cast<Element*>(nullptr))
        .def_property_readonly("tagName", &Element::tagName)
        // .name = "name" is simpler than .name = StringId("name") and often the name is new anyway so it has
        // to be registered.
        .def_property(
            "name",
            &Element::name,
            static_cast<void (This::*)(std::string_view)>(&Element::setName))
        .def_property_readonly("id", &Element::id)
        .def("getOrCreateId", &Element::getOrCreateId)
        .def("getAttribute", &Element::getAttribute)
        .def(
            "setAttribute",
            static_cast<void (This::*)(vgc::core::StringId, const vgc::dom::Value&)>(
                &Element::setAttribute))
        .def("clearAttribute", &Element::clearAttribute)
        .def(
            "__str__",
            [](const This& self) -> std::string {
                fmt::memory_buffer mbuf;
                mbuf.push_back('<');
                mbuf.append(self.tagName().string());
                writeAttributes(std::back_inserter(mbuf), self);
                mbuf.push_back('>');
                return std::string(mbuf.begin(), mbuf.size());
            })
        .def("__repr__", [](const This& self) -> std::string {
            fmt::memory_buffer mbuf;
            mbuf.append(std::string_view("<vgc.dom.Element at "));
            mbuf.append(vgc::core::format("{}", vgc::core::asAddress(&self)));
            mbuf.append(std::string_view(" tagName=\'"));
            mbuf.append(self.tagName().string());
            mbuf.push_back('\'');
            writeAttributesRepr(std::back_inserter(mbuf), self);
            mbuf.push_back('>');
            return std::string(mbuf.begin(), mbuf.size());
        });
}
