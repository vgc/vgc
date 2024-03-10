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

#include <vgc/geometry/mat.h>
#include <vgc/geometry/mat2d.h>
#include <vgc/geometry/mat2f.h>
#include <vgc/geometry/mat3d.h>
#include <vgc/geometry/mat3f.h>
#include <vgc/geometry/mat4d.h>
#include <vgc/geometry/mat4f.h>
#include <vgc/geometry/vec2d.h>
#include <vgc/geometry/vec2f.h>
#include <vgc/geometry/vec3d.h>
#include <vgc/geometry/vec3f.h>
#include <vgc/geometry/vec4d.h>
#include <vgc/geometry/vec4f.h>

#include <vgc/core/wraps/class.h>
#include <vgc/core/wraps/common.h>
#include <vgc/geometry/wraps/vec.h>

using vgc::geometry::Mat;
using vgc::geometry::Vec;

namespace {

template<int dimension, typename T>
struct PointParam_;

template<typename T>
struct PointParam_<2, T> {
    using type = T;
};

template<typename T>
struct PointParam_<3, T> {
    using type = const Vec<2, T>&;
};

template<typename T>
struct PointParam_<4, T> {
    using type = const Vec<3, T>&;
};

template<int dimension, typename T>
using PointParam = typename PointParam_<dimension, T>::type;

// Allows `mat[i][j]` Python syntax
template<int dimension, typename T>
class MatRowView {
private:
    using TMat = vgc::geometry::Mat<dimension, T>;

public:
    MatRowView(TMat& mat, vgc::Int i)
        : mat_(&mat)
        , i_(i) {
    }

    T& operator[](vgc::Int j) {
        return (*mat_)(i_, j);
    }

    const T& operator[](vgc::Int j) const {
        return (*mat_)(i_, j);
    }

private:
    TMat* mat_;
    vgc::Int i_;
};

template<typename TMat>
TMat matFromSequence(py::sequence s) {
    using T = typename TMat::ScalarType;
    constexpr vgc::Int dimension = TMat::dimension;
    using TVec = typename vgc::geometry::Vec<dimension, T>;

    bool isFlatList = (s.size() == dimension * dimension);
    if (!isFlatList && s.size() != dimension) {
        throw py::value_error("Sequence length does not match dimension of Mat type.");
    }

    // clang-format off
    if constexpr (dimension == 2) {
        if (isFlatList) {
            return TMat(s[0].cast<T>(), s[1].cast<T>(),
                        s[2].cast<T>(), s[3].cast<T>());
        }
        else {
            // Note: Here, we thought it would make sense to write:
            //
            //   TVec v0 = s[0].cast<TVec>()
            //
            // But unfortunately it doesn't work. For example, when s[0] is a
            // `tuple` it fails with:
            //
            //   RuntimeError: Unable to cast Python instance of type <class
            //   'tuple'> to C++ type 'vgc::geometry::Vec2d'
            //
            // despite the conversion from tuple to Vec2d working when doing it
            // in Python. We would have to investigate why calling
            // `py::cast<Vec2d>(obj)` in C++ is not equivalent to calling
            // `Vec2d(obj)` in Python, but as a workaround, for now we simply
            // manually convert from a py::sequence to a Vec, which covers
            // most use cases.
            //
            // XXX What about if s[0] is a string, e.g., Mat2d(['(1, 2)', '(3, 4)'])
            //
            py::sequence s0 = s[0].cast<py::sequence>();
            py::sequence s1 = s[1].cast<py::sequence>();
            TVec v0 = vgc::geometry::wraps::vecFromSequence<TVec>(s0);
            TVec v1 = vgc::geometry::wraps::vecFromSequence<TVec>(s1);
            return TMat(v0[0], v0[1],
                        v1[0], v1[1]);
        }
    }
    else if constexpr (dimension == 3) {
        if (isFlatList) {
            return TMat(s[0].cast<T>(), s[1].cast<T>(), s[2].cast<T>(),
                        s[3].cast<T>(), s[4].cast<T>(), s[5].cast<T>(),
                        s[6].cast<T>(), s[7].cast<T>(), s[8].cast<T>());
        }
        else {
            py::sequence s0 = s[0].cast<py::sequence>();
            py::sequence s1 = s[1].cast<py::sequence>();
            py::sequence s2 = s[2].cast<py::sequence>();
            TVec v0 = vgc::geometry::wraps::vecFromSequence<TVec>(s0);
            TVec v1 = vgc::geometry::wraps::vecFromSequence<TVec>(s1);
            TVec v2 = vgc::geometry::wraps::vecFromSequence<TVec>(s2);
            return TMat(v0[0], v0[1], v0[2],
                        v1[0], v1[1], v1[2],
                        v2[0], v2[1], v2[2]);
        }
    }
    else if constexpr (dimension == 4) {
        if (isFlatList) {
            return TMat(s[0].cast<T>(), s[1].cast<T>(), s[2].cast<T>(), s[3].cast<T>(),
                        s[4].cast<T>(), s[5].cast<T>(), s[6].cast<T>(), s[7].cast<T>(),
                        s[8].cast<T>(), s[9].cast<T>(), s[10].cast<T>(), s[11].cast<T>(),
                        s[12].cast<T>(), s[13].cast<T>(), s[14].cast<T>(), s[15].cast<T>());
        }
        else {
            py::sequence s0 = s[0].cast<py::sequence>();
            py::sequence s1 = s[1].cast<py::sequence>();
            py::sequence s2 = s[2].cast<py::sequence>();
            py::sequence s3 = s[3].cast<py::sequence>();
            TVec v0 = vgc::geometry::wraps::vecFromSequence<TVec>(s0);
            TVec v1 = vgc::geometry::wraps::vecFromSequence<TVec>(s1);
            TVec v2 = vgc::geometry::wraps::vecFromSequence<TVec>(s2);
            TVec v3 = vgc::geometry::wraps::vecFromSequence<TVec>(s3);
            return TMat(v0[0], v0[1], v0[2], v0[3],
                        v1[0], v1[1], v1[2], v1[3],
                        v2[0], v2[1], v2[2], v2[3],
                        v3[0], v3[1], v3[2], v3[3]);
        }
    }
    // clang-format on
    else {
        static_assert("Mat type not supported");
    }
}

template<int dimension, typename T>
void wrap_mat(py::module& m, const std::string& name) {

    using TPointParam = PointParam<dimension, T>;
    using TVec2 = vgc::geometry::Vec<2, T>;
    using TVec = vgc::geometry::Vec<dimension, T>;
    using TMat = vgc::geometry::Mat<dimension, T>;
    using TRow = MatRowView<dimension, T>;

    // Wrap RowView class, implementing the Sequence protocol
    vgc::core::wraps::Class<TRow>(m, (name + "RowView").c_str())
        .def(
            "__getitem__",
            [](const TRow& row, vgc::Int j) {
                if (j < 0 || j >= dimension) {
                    throw py::index_error();
                }
                return row[j];
            })
        .def(
            "__setitem__", //
            [](TRow& row, vgc::Int j, T x) {
                if (j < 0 || j >= dimension) {
                    throw py::index_error();
                }
                row[j] = x;
            })
        .def("__len__", [](TRow&) { return dimension; });

    vgc::core::wraps::Class<TMat> cmat(m, name.c_str());

    // Default constructor, copy constructor, and diagonal matrix constructor.
    cmat.def(py::init<>())     //
        .def(py::init<TMat>()) //
        .def(py::init<T>());

    // Constructor with explicit initialization of all elements, e.g.:
    //
    // m = Mat3d(1, 2, 3,
    //           4, 5, 6,
    //           7, 8, 9)
    //
    // Note that in numpy the typical syntax is:
    //
    // A = np.array([[1, 2, 3],
    //               [4, 5, 6],
    //               [7, 8, 9]])
    //
    // The "list of list" is required so that numpy can infer the numRows and
    // numColumns, since numpy doesn't have per-size specific matrix types. We
    // don't have this constraint, so there is no good reason to do the same.
    //
    // TODO: Despite the above, it might still be a good idea to also support
    // constructing from "tuple of tuple" and/or "array of array", to enable
    // easier copy-pasting from debug output to Python Console.
    //
    // clang-format off
    if constexpr (dimension == 2) {
        cmat.def(py::init<T, T,
                          T, T>());
    }
    if constexpr (dimension == 3) {
        cmat.def(py::init<T, T, T,
                          T, T, T,
                          T, T, T>());
    }
    else if constexpr (dimension == 4) {
        cmat.def(py::init<T, T, T, T,
                          T, T, T, T,
                          T, T, T, T,
                          T, T, T, T>());
    }
    // clang-format on

    // Constructor from string (parse)
    cmat.def(py::init([](const std::string& s) { return vgc::core::parse<TMat>(s); }));

    // Constructor from Sequence.
    //
    // This allows both a Sequence of Sequence (outer size N and inner size N),
    // as well as a flat Sequence (size N*N), so that all of these are allowed:
    //
    // ```python
    // m = Mat2d((1, 2, 3, 4))
    // m = Mat2d([1, 2, 3, 4])
    // m = Mat2d(((1, 2), (3, 4)))
    // m = Mat2d([[1, 2], [3, 4]])
    // m = Mat2d([(1, 2), (3, 4)])
    // m = Mat2d(([1, 2], [3, 4]))
    // m = Mat4d([i+j for i, j in Mat4d.indices])
    // ```
    //
    // Important: this must be defined after the string overload, otherwise it
    // would take precendence since a string implements the Sequence protocol.
    //
    // TODO: Also allow `m = Mat2d((1, 2), (3, 4))`. For consistency, this
    // requires to also add the constructor `Mat2d(const Vec2d&, const Vec2d&)`
    // in C++.
    //
    cmat.def(py::init([](py::sequence s) { return matFromSequence<TMat>(s); }));

    // Get/Set individual element via `m[i][j] = 42`
    cmat.def(
        "__getitem__",
        [](TMat& mat, vgc::Int i) {
            if (i < 0 || i >= dimension) {
                throw py::index_error();
            }
            return TRow(mat, i);
        },
        py::keep_alive<0, 1>()); // we want the Row to keep the Mat alive
    // Note: py::return_value_policy::reference_internal wouldn't work
    // here because it expects a T* or T&, and here we return a T.

    // Implements the Sequence protocol. See wrap_vec.cpp.
    cmat.def("__len__", [](const TMat&) { return dimension; });

    // Convenient way to iterate over all valid indices in the matrix type
    cmat.def_property_readonly_static("indices", [](py::object) {
        py::list res(dimension * dimension);
        for (int i = 0; i < dimension; ++i) {
            for (int j = 0; j < dimension; ++j) {
                py::tuple t(2);
                t[0] = i;
                t[1] = j;
                res[j + i * dimension] = t;
            }
        }
        return res;
    });

    // Overload of arithmetic operators
    auto self2 = py::self; // Fix https://github.com/pybind/pybind11/issues/1893
    cmat.def(py::self += py::self)
        .def(py::self + py::self)
        .def(+py::self)
        .def(self2 -= py::self)
        .def(py::self - py::self)
        .def(-py::self)
        .def(py::self *= py::self)
        .def(py::self * py::self)
        .def(py::self *= T())
        .def(T() * py::self)
        .def(py::self * T())
        .def(py::self /= T())
        .def(py::self / T())
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def(py::self * TVec());

    // Transform
    cmat.def(
        "transformPoint",
        py::overload_cast<TPointParam>(&TMat::transformPoint, py::const_));
    cmat.def(
        "transformPointAffine",
        py::overload_cast<TPointParam>(&TMat::transformPointAffine, py::const_));
    if constexpr (dimension == 4) {
        cmat.def(
            "transformPoint",
            py::overload_cast<const TVec2&>(&TMat::transformPoint, py::const_));
        cmat.def(
            "transformPointAffine",
            py::overload_cast<const TVec2&>(&TMat::transformPointAffine, py::const_));
    }

    // Identity
    cmat.def_property_readonly_static(
        "identity", [](py::object) -> TMat { return TMat::identity; });

    // Other methods
    cmat.def("setElements", &TMat::setElements)
        .def("setToDiagonal", &TMat::setToDiagonal)
        .def("setToZero", &TMat::setToZero)
        .def("setToIdentity", &TMat::setToIdentity)
        .def("inverted", [](const TMat& m) {
            bool isInvertible;
            TMat res = m.inverted(&isInvertible);
            if (!isInvertible) {
                throw py::value_error("The matrix is not invertible.");
            }
            return res;
        });

    if constexpr (dimension == 3) {
        cmat.def(
                "translate",
                py::overload_cast<T, T>(&TMat::translate),
                "vx"_a,
                "vy"_a = 0)
            .def("translate", py::overload_cast<TPointParam>(&TMat::translate))
            .def("rotate", &TMat::rotate, "t"_a, "orthosnap"_a = true)
            .def("scale", py::overload_cast<T>(&TMat::scale))
            .def("scale", py::overload_cast<T, T>(&TMat::scale))
            .def("scale", py::overload_cast<const TVec2&>(&TMat::scale));
    }
    else if constexpr (dimension == 4) {
        cmat.def(
                "translate",
                py::overload_cast<T, T, T>(&TMat::translate),
                "vx"_a,
                "vy"_a = 0,
                "vz"_a = 0)
            .def("translate", py::overload_cast<TPointParam>(&TMat::translate))
            .def("rotate", &TMat::rotate, "t"_a, "orthosnap"_a = true)
            .def("scale", py::overload_cast<T>(&TMat::scale))
            .def(
                "scale",
                py::overload_cast<T, T, T>(&TMat::scale),
                "sx"_a,
                "sy"_a,
                "sz"_a = 0)
            .def("scale", py::overload_cast<TPointParam>(&TMat::scale));
    }

    // Conversion to string
    cmat.def("__repr__", [](const TMat& m) { return vgc::core::toString(m); });
}

} // namespace

void wrap_mat(py::module& m) {
    wrap_mat<2, float>(m, "Mat2f");
    wrap_mat<2, double>(m, "Mat2d");
    wrap_mat<3, float>(m, "Mat3f");
    wrap_mat<3, double>(m, "Mat3d");
    wrap_mat<4, float>(m, "Mat4f");
    wrap_mat<4, double>(m, "Mat4d");
}
