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

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <vgc/scene/scene.h>

namespace py = pybind11;
using vgc::scene::Scene;
using vgc::scene::SceneSharedPtr;

void wrap_scene(py::module& m)
{
    py::class_<Scene, SceneSharedPtr>(m, "Scene")
        .def(py::init([](){ return Scene::create(); }))
        .def("clear", &Scene::clear, "Clears the scene.")
        .def("addCurve", &Scene::addCurve, "Adds a curve to the scene.")
        .def("__repr__", [](const Scene& s) {
                return "<Scene containing "
                       + std::to_string(s.curves().size())
                       + " curves>"; })
    ;
}
