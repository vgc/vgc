// Copyright 2023 The VGC Developers
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

#ifndef VGC_TOOLS_SKETCHPASS_H
#define VGC_TOOLS_SKETCHPASS_H

#include <vgc/core/span.h>
#include <vgc/tools/api.h>
#include <vgc/tools/sketchpoint.h>

namespace vgc::tools {

/// \class vgc::tools::SketchPointBuffer
/// \brief An array of SketchPoint together with information on which part is stable.
///
/// When sketching, the raw input points go through some processing passes (see
/// `SketchPass`) in order to perform dequantization, curve fitting, smoothing,
/// etc. These passes are performed continuously while sketching, that is, they
/// are done each time a new input point is given.
///
/// For performance reasons, but also for increased predictability for users,
/// it is typically a good idea for each pass to only modify the last few
/// output points when a new input point is given. That is, we want to have a
/// long "stable" part at the beginning of the stroke (whose points are
/// guaranteed to never change anymore), and a short "unstable" part at the end
/// of the stroke (whose points may be updated when a new input point is
/// given).
///
/// This class is a convenient helper to store and edit an array of SketchPoint
/// together with information on which part of the array is stable and which
/// part is unstable. Attempting to edit a point in the stable part throws an
/// exception, making it easier to catch bugs in the implementation of passes.
///
class VGC_TOOLS_API SketchPointBuffer {
public:
    /// Creates an empty `SketchPointBuffer`.
    ///
    SketchPointBuffer() noexcept = default;

    /// Removes all the points in this `SketchPointBuffer`.
    ///
    /// Throws an exception if `numStablePoints() > 0`.
    ///
    /// \sa `reset()`.
    ///
    void clear() {
        resize(0);
    }

    /// Resets this `SketchPointBuffer` to its initial state with
    /// `numStablePoints() == 0` and `length() == 0`.
    ///
    /// This is the only method modifying the stable part of this
    /// `SketchPointBuffer`. You must not call it when implementing a
    /// `SketchPass`, as it would break the invariant that stable points are
    /// not modified or removed during a pass update.
    ///
    /// This function preserves the capacity of the underlying array.
    ///
    /// \sa `clear()`.
    ///
    void reset() {
        points_.clear();
        numStablePoints_ = 0;
    }

    /// Returns the `SketchPoint` at index `i` as an immutable reference.
    ///
    const SketchPoint& operator[](Int i) const {
        return points_[i];
    }

    /// Returns the `SketchPoint` at index `i` as a mutable reference.
    ///
    /// Throws an exception if `i` refers to a stable point.
    ///
    SketchPoint& at(Int i) {
        if (i < numStablePoints_) {
            throw core::LogicError(
                "at(): cannot get a non-const reference to a stable point.");
        }
        return points_[i];
    }

    /// Returns the unstable subset of this `SketchPointBuffer`.
    ///
    core::Span<SketchPoint> unstablePoints() {
        return core::Span<SketchPoint>(points_.begin() + numStablePoints_, points_.end());
    }

    /// Returns the underlying `SketchPointArray` stored in this `SketchPointBuffer`.
    ///
    const SketchPointArray& data() const {
        return points_;
    }

    /// Allows iteration over all points in this `SketchPointBuffer`.
    ///
    SketchPointArray::const_iterator begin() const {
        return points_.begin();
    }

    /// Allows iteration over all points in this `SketchPointBuffer`.
    ///
    SketchPointArray::const_iterator end() const {
        return points_.end();
    }

    /// Returns an immutable reference to the first point.
    ///
    /// Throws `IndexError` if there are no points in this `SketchPointBuffer`.
    ///
    const SketchPoint& first() const {
        return points_.first();
    }

    /// Returns an immutable reference to the last point.
    ///
    /// Throws `IndexError` if there are no points in this `SketchPointBuffer`.
    ///
    const SketchPoint& last() const {
        return points_.last();
    }

    /// Returns the number of points in this `SketchPointBuffer`.
    ///
    Int length() const {
        return points_.length();
    }

    /// Increases the capacity of `data()`.
    ///
    void reserve(Int numPoints) {
        return points_.reserve(numPoints);
    }

    /// Changes the number of points in this `SketchPointBuffer`.
    ///
    /// Throws an exception if `numPoints < numStablePoints()`.
    ///
    void resize(Int numPoints) {
        if (numPoints < numStablePoints_) {
            throw core::LogicError("resize(): cannot decrease number of stable points.");
        }
        return points_.resize(numPoints);
    }

    /// Returns the number of stable points in this `SketchPointBuffer`.
    ///
    Int numStablePoints() const {
        return numStablePoints_;
    }

    /// Sets the number of stable points in this `SketchPointBuffer`.
    ///
    /// Throws an exception if `numPoints < numStablePoints()`.
    ///
    /// Throws an exception if `numPoints > length()`.
    ///
    void setNumStablePoints(Int numPoints) {
        if (numPoints < numStablePoints_) {
            throw core::LogicError(
                "setNumStablePoints(): cannot decrease number of stable points.");
        }
        else if (numPoints > points_.length()) {
            throw core::LogicError("setNumStablePoints(): number of stable points cannot "
                                   "be greater than number of points.");
        }
        numStablePoints_ = numPoints;
    }

    /// Appends an (unstable) point.
    ///
    SketchPoint& append(const SketchPoint& point) {
        points_.append(point);
        return points_.last();
    }

    /// Appends an (unstable) point with the given values.
    ///
    SketchPoint& emplaceLast(
        const geometry::Vec2d& position,
        double pressure,
        double timestamp,
        double width,
        double s = 0) {

        return points_.emplaceLast(position, pressure, timestamp, width, s);
    }

    /// Appends a sequence of (unstable) points.
    ///
    template<typename InputIt, VGC_REQUIRES(core::isInputIterator<InputIt>)>
    void extend(InputIt first, InputIt last) {
        points_.extend(first, last);
    }

    // Updates `p.s()` of all unstable points by computing their values
    // as cumulative chord lengths.
    //
    void updateChordLengths();

private:
    SketchPointArray points_;
    Int numStablePoints_ = 0;
};

/// \class vgc::tools::SketchPass
/// \brief Transforms an input SketchPointBuffer into another.
///
/// This is a base abstract class for implementing a processing step that
/// transforms an input SketchPointBuffer into another.
///
/// Subclasses should reimplement the virtual method `doUpdateFrom()`, and also
/// possibly `doReset()` if they store additional state that needs to be
/// reinitialized when starting processing a new SketchPointBuffer from
/// scratch.
///
class VGC_TOOLS_API SketchPass {
protected:
    SketchPass() noexcept = default;

public:
    virtual ~SketchPass() = default;

    /// Resets this pass, clearing the `buffer` in preparation of processing a
    /// new input from scratch.
    ///
    /// This calls `doReset()` which subclass should reimplement if they store
    /// additional state that needs to be reinitialized.
    ///
    void reset();

    /// Updates the `output()` buffer of this pass based on the given `input`
    /// buffer.
    ///
    /// This calls `doUpdateFrom()` which subclass should reimplement.
    ///
    void updateFrom(const SketchPointBuffer& input);

    /// Updates the `output()` buffer of this pass based on the `output()`
    /// buffer of the given `previousPass`.
    ///
    /// This is equivalent to `updateFrom(previousPass.output())`.
    ///
    void updateFrom(const SketchPass& previousPass) {
        updateFrom(previousPass.output());
    }

    /// Returns the output buffer that this pass computes during its
    /// `doUpdateFrom()` implementation.
    ///
    const SketchPointBuffer& output() const {
        return output_;
    }

protected:
    /// This is the main function that subclasses should implement. It should update
    /// the `output` buffer based on the new `input`.
    ///
    virtual void
    doUpdateFrom(const SketchPointBuffer& input, SketchPointBuffer& output) = 0;

    /// This method should be reimplemented by subclasses if they store
    /// additional state that needs to be reinitialized before processing a new
    /// input from scratch.
    ///
    /// The default implementation does nothing.
    ///
    virtual void doReset();

private:
    SketchPointBuffer output_;
};

} // namespace vgc::tools

#endif // VGC_TOOLS_SKETCHPASS_H
