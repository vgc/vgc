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

template<typename TMat, typename TVec, typename T>
TMat mat2FromSequence_(py::sequence s, bool isFlatList) {
    using vgc::geometry::wraps::detail::cast;
    if (isFlatList) {
        // clang-format off
        return TMat(cast<T>(s[0]), cast<T>(s[1]),
                    cast<T>(s[2]), cast<T>(s[3]));
        // clang-format on
    }
    else {
        TVec v0 = vgc::geometry::wraps::vecFromObject<TVec>(s[0]);
        TVec v1 = vgc::geometry::wraps::vecFromObject<TVec>(s[1]);
        return TMat(v0, v1);
    }
}

template<typename TMat, typename TVec, typename T>
TMat mat3FromSequence_(py::sequence s, bool isFlatList) {
    using vgc::geometry::wraps::detail::cast;
    if (isFlatList) {
        // clang-format off
        return TMat(cast<T>(s[0]), cast<T>(s[1]), cast<T>(s[2]),
                    cast<T>(s[3]), cast<T>(s[4]), cast<T>(s[5]),
                    cast<T>(s[6]), cast<T>(s[7]), cast<T>(s[8]));
        // clang-format on
    }
    else {
        TVec v0 = vgc::geometry::wraps::vecFromObject<TVec>(s[0]);
        TVec v1 = vgc::geometry::wraps::vecFromObject<TVec>(s[1]);
        TVec v2 = vgc::geometry::wraps::vecFromObject<TVec>(s[2]);
        return TMat(v0, v1, v2);
    }
}

template<typename TMat, typename TVec, typename T>
TMat mat4FromSequence_(py::sequence s, bool isFlatList) {
    using vgc::geometry::wraps::detail::cast;
    if (isFlatList) {
        // clang-format off
        return TMat(cast<T>(s[0]), cast<T>(s[1]), cast<T>(s[2]), cast<T>(s[3]),
                    cast<T>(s[4]), cast<T>(s[5]), cast<T>(s[6]), cast<T>(s[7]),
                    cast<T>(s[8]), cast<T>(s[9]), cast<T>(s[10]), cast<T>(s[11]),
                    cast<T>(s[12]), cast<T>(s[13]), cast<T>(s[14]), cast<T>(s[15]));
        // clang-format on
    }
    else {
        TVec v0 = vgc::geometry::wraps::vecFromObject<TVec>(s[0]);
        TVec v1 = vgc::geometry::wraps::vecFromObject<TVec>(s[1]);
        TVec v2 = vgc::geometry::wraps::vecFromObject<TVec>(s[2]);
        TVec v3 = vgc::geometry::wraps::vecFromObject<TVec>(s[3]);
        return TMat(v0, v1, v2, v3);
    }
}

template<typename TMat>
TMat matFromSequence(py::sequence s) {
    using T = typename TMat::ScalarType;
    constexpr vgc::Int dimension = TMat::dimension;
    using TVec = typename vgc::geometry::Vec<dimension, T>;

    bool isFlatList = (s.size() == dimension * dimension);
    if (!isFlatList && s.size() != dimension) {
        vgc::geometry::wraps::detail::throwUnableToConvertVecOrMat<TMat>(
            s, "Incompatible sizes.");
    }

    try {
        if constexpr (dimension == 2) {
            return mat2FromSequence_<TMat, TVec, T>(s, isFlatList);
        }
        else if constexpr (dimension == 3) {
            return mat3FromSequence_<TMat, TVec, T>(s, isFlatList);
        }
        else if constexpr (dimension == 4) {
            return mat4FromSequence_<TMat, TVec, T>(s, isFlatList);
        }
        else {
            static_assert("Mat type not supported");
        }
    }
    catch (const std::exception& error) {
        vgc::geometry::wraps::detail::throwUnableToConvertVecOrMat<TMat>(s, error.what());
    }
    catch (...) {
        vgc::geometry::wraps::detail::throwUnableToConvertVecOrMat<TMat>(s);
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

    // Constructor from row vectors, e.g.:
    //
    // m = Mat3d(Vec3d(1, 2, 3),
    //           Vec3d(4, 5, 6),
    //           Vec3d(7, 8, 9))
    //
    // or thanks to implicit conversion from tuple to Vec:
    //
    // m = Mat3d((1, 2, 3),
    //           (4, 5, 6),
    //           (7, 8, 9))
    //
    if constexpr (dimension == 2) {
        cmat.def(py::init<const TVec&, const TVec&>());
    }
    if constexpr (dimension == 3) {
        cmat.def(py::init<const TVec&, const TVec&, const TVec&>());
    }
    else if constexpr (dimension == 4) {
        cmat.def(py::init<const TVec&, const TVec&, const TVec&, const TVec&>());
    }

    // Constructor with explicit initialization of all elements, e.g.:
    //
    // m = Mat3d(1, 2, 3,
    //           4, 5, 6,
    //           7, 8, 9)
    //
    // Note that in numpy the typical syntax to create a matrix is:
    //
    // A = np.array([[1, 2, 3],
    //               [4, 5, 6],
    //               [7, 8, 9]])
    //
    // Such "list of list" syntax is necessary in numpy because otherwise it
    // couldn't know the number of rows and columns, as this info is not part
    // of the type itself. In our case, the dimension is part of the type,
    // therefore we do not have this constraint, and we can support both the
    // "concise" syntax defined here, but also the "list of list" syntax (see
    // "Constructor from Sequence" below), which is still useful for
    // compatibility and conversion to and from numpy types and other
    // third-party libs.
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
