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

#ifndef VGC_CORE_OBJECTARRAY_H
#define VGC_CORE_OBJECTARRAY_H

#include <functional> // reference_wrapper

#include <vgc/core/array.h>
#include <vgc/core/object.h>

namespace vgc::core {

/// \class vgc::core::ObjPtrArrayIterator
/// \brief Iterates over an ObjPtrArrayView
///
template<typename T>
class ObjPtrArrayIterator {
public:
    typedef T value_type;
    typedef Int difference_type;
    typedef value_type& reference;
    typedef value_type* pointer;
    typedef std::forward_iterator_tag iterator_category;

    using ObjPtrType = ObjPtr<T>;
    using ArrayType = Array<ObjPtrType>;
    using ArrayConstIteratorType = typename ArrayType::const_iterator;

    /// Constructs an invalid `ObjPtrArrayIterator`.
    ///
    ObjPtrArrayIterator() {
    }

    /// Constructs an `ObjPtrArrayIterator` from the given `Array<ObjPtr<T>>`'s iterator.
    ///
    explicit ObjPtrArrayIterator(ArrayConstIteratorType it)
        : it_(it) {
    }

    /// Prefix-increments this iterator.
    ///
    ObjPtrArrayIterator& operator++() {
        ++it_;
        return *this;
    }

    /// Postfix-increments this iterator.
    ///
    ObjPtrArrayIterator operator++(int) {
        ObjPtrArrayIterator res(*this);
        operator++();
        return res;
    }

    /// Dereferences this iterator with the star operator.
    ///
    value_type& operator*() const {
        const ObjPtrType& objPtr = *it_;
        return *objPtr.get();
    }

    /// Dereferences this iterator with the arrow operator.
    ///
    value_type* operator->() const {
        const ObjPtrType& objPtr = *it_;
        return objPtr.get();
    }

    /// Returns whether the two iterators are equal.
    ///
    bool operator==(const ObjPtrArrayIterator& other) const {
        return it_ == other.it_;
    }

    /// Returns whether the two iterators are different.
    ///
    bool operator!=(const ObjPtrArrayIterator& other) const {
        return it_ != other.it_;
    }

private:
    typename ArrayType::const_iterator it_ = {};
};

/// \class vgc::core::ObjPtrArrayView
/// \brief A mutable view into an Array of ObjPtr<T>.
///
/// Note that using this view, the objects inside the array are mutable,
/// but the array itself is non-mutable.
///
template<typename T>
class ObjPtrArrayView {
public:
    using ArrayType = Array<ObjPtr<T>>;

    /// Creates an `ObjPtrArrayView<T>` from the given `Array<ObjPtr<T>>`.
    ///
    /// The `array` must outlive this iterator.
    ///
    ObjPtrArrayView(const ArrayType& array)
        : array_(array) {
    }

    /// Returns the begin of the range.
    ///
    ObjPtrArrayIterator<T> begin() const {
        return ObjPtrArrayIterator<T>(array_.get().begin());
    }

    /// Returns the end of the range.
    ///
    ObjPtrArrayIterator<T> end() const {
        return ObjPtrArrayIterator<T>(array_.get().end());
    }

    /// Returns the number of objects in the range.
    ///
    /// Note that this function is slow (linear complexity), because it has to
    /// iterate over all objects in the range.
    ///
    Int length() const {
        return array_.get().length();
    }

private:
    const std::reference_wrapper<const ArrayType> array_;
};

} // namespace vgc::core

#endif // VGC_CORE_OBJECTARRAY_H
