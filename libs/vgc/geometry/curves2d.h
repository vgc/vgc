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

#ifndef VGC_GEOMETRY_CURVES2D_H
#define VGC_GEOMETRY_CURVES2D_H

#include <vgc/core/array.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/curvecommand.h>
#include <vgc/geometry/vec2d.h>

namespace vgc::geometry {

class Curves2d;

/// \class vgc::geometry::Curves2dCommandRef
/// \brief Proxy class representing a reference to a Curves2d command.
///
/// Curves2dCommandRef is a proxy class representing a reference to a Curves2d
/// command. It provides access to a Curves2d command type and its parameters.
///
/// ```cpp
/// std::string s;
/// vgc::core::StringWriter out(s);
/// vgc::geometry::Curves2d curves;
/// for (vgc::geometry::Curves2dCommandRef c : curves.commands()) {
///     switch (c.type()) {
///     case vgc::geometry::CurveCommandType::Close:
///         vgc::core::write(out, " Z");
///         break;
///     case vgc::geometry::CurveCommandType::MoveTo:
///         vgc::core::write(out, " M ", c.p());
///         break;
///     case vgc::geometry::CurveCommandType::LineTo:
///         vgc::core::write(out, " L ", c.p());
///         break;
///     case vgc::geometry::CurveCommandType::QuadraticBezierTo:
///         vgc::core::write(out, " Q ", c.p1(), ' ', c.p2());
///         break;
///     case vgc::geometry::CurveCommandType::CubicBezierTo:
///         vgc::core::write(out, " C ", c.p1(), ' ', c.p2(), ' ', c.p3());
///         break;
///     }
/// }
/// ```
///
/// Internally, a Curves2dCommandRef stores internal pointers and indices into
/// a Curves2d, which are invalidated whenever the Curves2d is modified or
/// destructed. Using an invalidated Curves2dCommandRef is undefined behavior,
/// and may result in a segmentation fault or corrupted memory. Therefore, like
/// an iterator, you typically shouldn't store a Curves2dCommandRef in a member
/// variable, and instead only use it temporarily in a loop.
///
class VGC_GEOMETRY_API Curves2dCommandRef {
public:
    // Copy and move constructors
    Curves2dCommandRef(const Curves2dCommandRef&) = default;
    Curves2dCommandRef(Curves2dCommandRef&&) = default;

    /// Returns the type of the command.
    ///
    CurveCommandType type() const;

    /// Returns the Vec2d parameter of the MoveTo or LineTo command.
    ///
    Vec2d p() const;

    /// Returns the first Vec2d parameter of the QuadraticBezier or CubicBezier
    /// command.
    ///
    Vec2d p1() const;

    /// Returns the second Vec2d parameter of the QuadraticBezier or
    /// CubicBezier command.
    ///
    Vec2d p2() const;

    /// Returns the third Vec2d parameter of the CubicBezier command.
    ///
    Vec2d p3() const;

    /// Returns whether the two Curves2dCommandRef are equal, that is, whether
    /// they reference the same command of the same Curve2d, similar to
    /// pointer-equality.
    ///
    friend bool operator==(const Curves2dCommandRef& c1, const Curves2dCommandRef& c2) {
        return c1.curves_ == c2.curves_                //
               && c1.commandIndex_ == c2.commandIndex_ //
               && c1.paramIndex_ == c2.paramIndex_;
    }

    /// Returns whether the two iterators Curves2dCommandRef are different,
    /// that is, whether they reference different command of the same Curve2d,
    /// or commands of different Curve2d. This is similar to pointer-equality.
    ///
    friend bool operator!=(const Curves2dCommandRef& c1, const Curves2dCommandRef& c2) {
        return !(c1 == c2);
    }

private:
    friend class Curves2dCommandIterator;

    const Curves2d* curves_;
    Int commandIndex_;
    Int paramIndex_;

    // Constructor. Reserved for Curves2dCommandIterator.
    Curves2dCommandRef(const Curves2d* curves, Int commandIndex, Int paramIndex)
        : curves_(curves)
        , commandIndex_(commandIndex)
        , paramIndex_(paramIndex) {
    }

    // Copy and move assignment. Reserved for Curves2dCommandIterator.
    // Alternatively, we could define public assignment operators which modify
    // the referenced data, not the reference itself. However, this would
    // require to implement our own Curves2dCommandIterator's assignment
    // operators: indeed, copying the iterator should copy the reference
    // itself, not the referenced data.
    //
    // Related:
    // https://en.cppreference.com/w/cpp/container/vector_bool
    // https://en.cppreference.com/w/cpp/container/vector_bool/reference
    //
    Curves2dCommandRef& operator=(const Curves2dCommandRef&) = default;
    Curves2dCommandRef& operator=(Curves2dCommandRef&&) = default;

    // TODO: we may also want to define a class vgc::geometry::Curves2dCommand,
    // which actually stores a copy of the data by value, with automatic
    // conversion from Curves2dCommandRef to Curves2dCommand.
};

/// \class vgc::geometry::Curves2dCommandIterator
/// \brief Proxy iterator for iterating over Curves2d commands.
///
/// This class is a proxy iterator for iterating over all commands of a
/// Curves2d. See Curves2dCommandRef for example usage.
///
/// Note that unlike most iterator, dereferencing this iterator returns a
/// Curves2dCommandRef by value, instead of an actual C++ reference to some
/// object. This is why it is called a "proxy iterator": it doesn't exactly
/// satisfy the requirements of STL iterators, and generic algorithms working
/// on iterators may not work with Curves2dCommandIterators. For more
/// information on proxy iterators, see:
///
/// - http://ericniebler.com/2015/01/28/to-be-or-not-to-be-an-iterator/
/// - https://en.cppreference.com/w/cpp/container/vector_bool
///
class VGC_GEOMETRY_API Curves2dCommandIterator {
public:
    using difference_type = ptrdiff_t;
    using value_type = Curves2dCommandRef; // anything better?
    using reference = Curves2dCommandRef;
    using pointer = const Curves2dCommandRef*;
    using iterator_category = std::bidirectional_iterator_tag;

    /// Returns a Curves2dCommandRef by value, which allows you to access the
    /// command referenced by this iterator.
    ///
    Curves2dCommandRef operator*() const {
        return c_;
    }

    /// Returns a const pointer to the underlying Curves2dCommandRef stored in
    /// this iterator. This allows you to call a method of the
    /// Curves2dCommandRef.
    ///
    const Curves2dCommandRef* operator->() const {
        return &c_;
        // Note: in the future, we may want to use an "arrow proxy" instead:
        // https://quuxplusone.github.io/blog/2019/02/06/arrow-proxy/
    }

    /// Prefix-increments this iterator.
    ///
    Curves2dCommandIterator& operator++();

    /// Postfix-increments this iterator.
    ///
    Curves2dCommandIterator operator++(int) {
        Curves2dCommandIterator res(*this);
        ++(*this);
        return res;
    }

    /// Prefix-decrements this iterator.
    ///
    Curves2dCommandIterator& operator--();

    /// Postfix-decrements this iterator.
    ///
    Curves2dCommandIterator operator--(int) {
        Curves2dCommandIterator res(*this);
        --(*this);
        return res;
    }

    /// Returns whether the two iterators are equal.
    ///
    friend bool
    operator==(const Curves2dCommandIterator& i1, const Curves2dCommandIterator& i2) {
        return i1.c_ == i2.c_;
    }

    /// Returns whether the two iterators are different.
    ///
    friend bool
    operator!=(const Curves2dCommandIterator& i1, const Curves2dCommandIterator& i2) {
        return !(i1 == i2);
    }

private:
    friend class Curves2d;
    Curves2dCommandRef c_;

    Curves2dCommandIterator(const Curves2d* curves, Int commandIndex, Int paramIndex)
        : c_(curves, commandIndex, paramIndex) {
    }
};

/// \class vgc::geometry::Curves2dCommandRange
/// \brief A pair of Curves2dCommandIterators.
///
/// This class stores a pair of Curves2dCommandIterators, to be used in
/// range-based for loops, or algorithms taking a range as input.
///
class VGC_GEOMETRY_API Curves2dCommandRange {
public:
    /// Creates a Curves2dCommandRange with the two given iterators.
    ///
    Curves2dCommandRange(Curves2dCommandIterator begin, Curves2dCommandIterator end)
        : begin_(begin)
        , end_(end) {
    }

    /// Returns the beginning of the range.
    ///
    Curves2dCommandIterator begin() const {
        return begin_;
    }

    /// Returns the end of the range.
    ///
    Curves2dCommandIterator end() const {
        return end_;
    }

private:
    Curves2dCommandIterator begin_;
    Curves2dCommandIterator end_;
};

/// \class vgc::core::Curves2dSampleParams
/// \brief Parameters for Curves2d::sample()
///
// TODO:
//   minSamplesPerSegment?
//   maxDistance?
//   preserveSegmentEndPoints?
//
class VGC_GEOMETRY_API Curves2dSampleParams {
protected:
    Curves2dSampleParams(double minDistance, double maxAngle, Int maxSamplesPerSegment)
        : minDistance_(minDistance)
        , maxAngle_(maxAngle)
        , maxSamplesPerSegment_(maxSamplesPerSegment) {
    }

public:
    /// Creates a Curves2dSampleParams to be used for adaptive sampling.
    /// This sets `maxDistance()` to `0.0`.
    ///
    static Curves2dSampleParams
    adaptive(double maxAngle = 0.05, Int maxSamplesPerSegment = 64) {
        return Curves2dSampleParams(0.0, maxAngle, maxSamplesPerSegment);
    }

    /// Creates a Curves2dSampleParams to be used for adaptive sampling, but
    /// never adds a new sample between two samples if the distance between
    /// these two samples is already smaller than minDistance.
    ///
    static Curves2dSampleParams semiAdaptive(
        double minDistance = 1.0,
        double maxAngle = 0.05,
        Int maxSamplesPerSegment = 64) {

        return Curves2dSampleParams(minDistance, maxAngle, maxSamplesPerSegment);
    }

    /// Returns the minimum distance between two samples required for a new
    /// sample to be added.
    ///
    double minDistance() const {
        return minDistance_;
    }

    /// Sets the minimum distance.
    ///
    void setMinDistance(double minDistance) {
        minDistance_ = minDistance;
    }

    /// Returns the maximum angle allowed between two segments.
    ///
    double maxAngle() const {
        return maxAngle_;
    }

    /// Sets the maximum angle.
    ///
    void setMaxAngle(double maxAngle) {
        maxAngle_ = maxAngle;
    }

    /// Returns the maximum number of samples allowed as part of one
    /// QuadraticTo/CubicTo/etc. segment.
    ///
    Int maxSamplesPerSegment() const {
        return maxSamplesPerSegment_;
    }

    /// Sets the maximum number of samples.
    ///
    void setMaxSamplesPerSegment(Int maxSamplesPerSegment) {
        maxSamplesPerSegment_ = maxSamplesPerSegment;
    }

private:
    double minDistance_;
    double maxAngle_;
    Int maxSamplesPerSegment_;
};

/// \class vgc::geometry::WindingRule
/// \brief Specifies which area of polygons must be considered filled in case of overlaps.
///
enum class WindingRule {
    Odd,
    NonZero,
    Positive,
    Negative,
};

/// \class vgc::geometry::Curves2d
/// \brief Sequence of double-precision 2D curves.
///
/// A sequence of double-precision 2D curves, stored as a sequence of commands
/// such as MoveTo, LineTo, etc.
///
class VGC_GEOMETRY_API Curves2d {
public:
    /// Construct an empty sequence of curves.
    ///
    Curves2d() {
    }

    /// Returns a Curves2dCommandRange to iterate over all commands in this Curves2d.
    ///
    /// ```cpp
    /// for (Curves2dCommandRef& c : curves.commands()) {
    ///     // ...
    /// }
    /// ```
    ///
    // TODO: currently, this const variant actually returns a non-const
    // iterator, which would allow to mutate the geometric data if we add
    // mutators to Curves2dCommandRef. We need instead to have both const and
    // non-const variants, with the corresponding Curves2dCommandConstRange,
    // Curves2dCommandConstIterator, and Curves2dCommandConstRef.
    //
    Curves2dCommandRange commands() const {
        return Curves2dCommandRange(
            Curves2dCommandIterator(this, 0, 0),
            Curves2dCommandIterator(this, commandData_.length(), data_.length()));
    }

    /// Returns a const reference to the underlying raw geometric data, that
    /// is, a DoubleArray containing all command parameters (but without the
    /// command types). This can be useful for Curves2d that you know have a
    /// uniform structure (example: cubic Bezier segments only), and want to
    /// perform some raw processing on the data.
    ///
    const core::DoubleArray& data() const {
        return data_;
    }

    /// Adds a Close command.
    ///
    void close();

    /// Adds a new MoveTo command.
    ///
    void moveTo(const Vec2d& p);

    /// Adds a new MoveTo command.
    ///
    void moveTo(double x, double y);

    /// Adds a new LineTo command.
    ///
    void lineTo(const Vec2d& p);

    /// Adds a new LineTo command.
    ///
    void lineTo(double x, double y);

    /// Adds a new QuadraticBezierTo command.
    ///
    void quadraticBezierTo(const Vec2d& p1, const Vec2d& p2);

    /// Adds a new QBezierTo command.
    ///
    void quadraticBezierTo(double x1, double y1, double x2, double y2);

    /// Adds a new CBezierTo command.
    ///
    void cubicBezierTo(const Vec2d& p1, const Vec2d& p2, const Vec2d& p3);

    /// Adds a new CBezierTo command.
    ///
    void cubicBezierTo(double x1, double y1, double x2, double y2, double x3, double y3);

    /// Computes and returns an approximation of this Curves2d using line
    /// segments only (MoveTo, LineTo, and Close commands).
    ///
    /// Using the default parameters, the sampling is "adaptive". This means
    /// that the number of samples generated for each original curved segment
    /// depends on the curvature: the higher the curvature, the more samples
    /// are generated. More precisely, consecutive line segments within a given
    /// original curved segment never have an angle more than the given \p
    /// maxAngle (expressed in radian). This ensures that the curve looks
    /// "smooth" at any zoom level, except of course when the curve has an
    /// intentional corner.
    ///
    /// \verbatim
    ///                  o p2
    ///                .`
    ///              .` } maxAngle
    /// o----------o`- - - - -
    /// p0         p1
    /// \endverbatim
    ///
    /// The additional parameter \p maxSamplesPerSegment allows you to limit
    /// how many samples there can be for each original curved segment. This is
    /// necessary to break infinite loops in case the segment contains a cusp.
    ///
    Curves2d sample(const Curves2dSampleParams& params) const;

    /// Strokes this curve, that is, appends triangle data to the
    /// given DoubleArray. The appended data is of the form:
    ///
    /// ```
    /// [x1, y1,     // First vertex of first triangle
    ///  x2, y2,     // Second vertex of first triangle
    ///  x3, y3,     // Third vertex of first triangle
    ///
    ///  x4, y4,     // First vertex of second triangle
    ///  x5, y5,     // Second vertex of second triangle
    ///  x6, y6,     // Third vertex of second triangle
    ///
    ///  ...]
    /// ```
    ///
    // Note: in the future, we may want to add a "VertexFormat" options to
    // Specify an offset, stride, whether to add a z = 0 value and/or color
    // information, etc. Also, The "width" is constant for now. Later, it
    // could be some sort of CurveParamVec1d, while color could be CurveParamColor,
    // or something like that.
    //
    void stroke( //
        core::DoubleArray& data,
        double width,
        const Curves2dSampleParams& params) const;

    /// Fills this Curves2d, that is, triangulate the interior of the curves
    /// interpreted as contours of a polygon, using the non-zero winding rule.
    /// Subcurves which are not closed are ignored. The triangle data is
    /// appended to the given DoubleArray in this form:
    ///
    /// ```
    /// [x1, y1,     // First vertex of first triangle
    ///  x2, y2,     // Second vertex of first triangle
    ///  x3, y3,     // Third vertex of first triangle
    ///
    ///  x4, y4,     // First vertex of second triangle
    ///  x5, y5,     // Second vertex of second triangle
    ///  x6, y6,     // Third vertex of second triangle
    ///
    ///  ...]
    /// ```
    ///
    // TODO: add winding rule, anti-aliasing options, auto-closing open subcurves, etc.
    //
    void fill(
        core::DoubleArray& data,
        const Curves2dSampleParams& params,
        WindingRule windingRule = WindingRule::NonZero) const;

    /// \overload
    ///
    void fill(
        core::FloatArray& data,
        const Curves2dSampleParams& params,
        WindingRule windingRule = WindingRule::NonZero) const;

private:
    friend Curves2dCommandRef;
    friend Curves2dCommandIterator;
    detail::CurveCommandDataArray commandData_;
    core::DoubleArray data_;
};

inline CurveCommandType Curves2dCommandRef::type() const {
    return curves_->commandData_[commandIndex_].type;
}

inline Vec2d Curves2dCommandRef::p() const {
    // TODO: exception if Close?
    const double* d = &(curves_->data_[paramIndex_]);
    return Vec2d(*d, *(d + 1));
}

inline Vec2d Curves2dCommandRef::p1() const {
    // TODO: exception if Close?
    const double* d = &(curves_->data_[paramIndex_]);
    return Vec2d(*d, *(d + 1));
}

inline Vec2d Curves2dCommandRef::p2() const {
    // TODO: exception if Close, MoveTo, LineTo?
    const double* d = &(curves_->data_[paramIndex_ + 2]);
    return Vec2d(*d, *(d + 1));
}

inline Vec2d Curves2dCommandRef::p3() const {
    // TODO: exception if Close, MoveTo, LineTo, QuadraticBezierTo?
    const double* d = &(curves_->data_[paramIndex_ + 4]);
    return Vec2d(*d, *(d + 1));
}

inline Curves2dCommandIterator& Curves2dCommandIterator::operator++() {
    // TODO: support subcommands
    c_.paramIndex_ = c_.curves_->commandData_[c_.commandIndex_].endParamIndex;
    c_.commandIndex_ += 1;
    return *this;
}

inline Curves2dCommandIterator& Curves2dCommandIterator::operator--() {
    // TODO: support subcommands
    c_.commandIndex_ -= 1;
    if (c_.commandIndex_ == 0) {
        c_.paramIndex_ = 0;
    }
    c_.paramIndex_ = c_.curves_->commandData_[c_.commandIndex_].endParamIndex;
    return *this;
}

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_CURVES2D_H
