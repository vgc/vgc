// Copyright 2022 The VGC Developers
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

#ifndef VGC_DOM_PATH_H
#define VGC_DOM_PATH_H

#include <string>
#include <string_view>
#include <unordered_map>

#include <vgc/core/algorithms.h>
#include <vgc/core/array.h>
#include <vgc/core/flags.h>
#include <vgc/core/format.h>
#include <vgc/core/id.h>
#include <vgc/core/object.h>
#include <vgc/core/parse.h>
#include <vgc/core/stringid.h>
#include <vgc/core/templateutil.h> // RequiresValid
#include <vgc/dom/api.h>
#include <vgc/dom/noneor.h>

namespace vgc::dom {

VGC_DECLARE_OBJECT(Node);
VGC_DECLARE_OBJECT(Document);
VGC_DECLARE_OBJECT(Element);

class Path;

/*
path examples:
/layer/rect.v[0] = "/layer/rect/v0" if real ?
    element: /layer/rect
    attribute: v
    arrayIndex: 0
/layer/curve.startVertex
    element: /layer/curve
    attribute: startVertex
*/

/// \enum vgc::dom::PathSegmentType
/// \brief Specifies the type of a path segment.
///
// clang-format off
enum class PathSegmentType : UInt8 {
    Root = 0,
    Id,
    //Dot,
    Element,
    Attribute,
};
// clang-format on

/// \enum vgc::dom::PathSegmentFlag
/// \brief Specifies special properties of a path segment.
///
// clang-format off
enum class PathSegmentFlag : UInt8 {
    None        = 0x00,
    Indexed     = 0x01, // only allowed for attributes atm
};
// clang-format on

VGC_DEFINE_FLAGS(PathSegmentFlags, PathSegmentFlag)

/// \class vgc::dom::PathSegment
/// \brief Represents a path segment.
///
/// It can be an `Element` name or an attribute name with an optional index.
///
class VGC_DOM_API PathSegment {
private:
    friend Path;

public:
    using ArrayIndex = Int;

    // Constructs a segment representing the root element.
    constexpr PathSegment() noexcept = default;

    explicit PathSegment(
        core::StringId id,
        PathSegmentType type = PathSegmentType::Element,
        PathSegmentFlags flags = PathSegmentFlag::None,
        Int arrayIndex = 0) noexcept;

    core::StringId nameOrId() const noexcept {
        return nameOrId_;
    }

    PathSegmentType type() const noexcept {
        return type_;
    }

    PathSegmentFlags flags() const noexcept {
        return flags_;
    }

    bool isIndexed() const noexcept {
        return flags_.has(PathSegmentFlag::Indexed);
    }

    Int arrayIndex() const noexcept {
        return arrayIndex_;
    }

    size_t hash() const noexcept {
        size_t res = 1347634503; // 'PSEG'
        core::hashCombine(res, nameOrId_, flags_);
        if (flags_.has(PathSegmentFlag::Indexed)) {
            core::hashCombine(res, arrayIndex_);
        }
        return res;
    }

    bool operator==(const PathSegment& other) const noexcept;

    bool operator!=(const PathSegment& other) const noexcept {
        return !operator==(other);
    }

    bool operator<(const PathSegment& other) const noexcept;

private:
    core::StringId nameOrId_;
    PathSegmentType type_ = PathSegmentType::Root;
    PathSegmentFlags flags_ = PathSegmentFlag::None;
    Int arrayIndex_ = 0;
};

using PathSegmentArray = core::Array<PathSegment>;

namespace detail {

struct PathUpdater;

} // namespace detail

/// \class vgc::dom::Path
/// \brief Represents a path to a node or attribute.
///
class VGC_DOM_API Path {
public:
    using SegmentIterator = PathSegmentArray::iterator;
    using ConstSegmentIterator = PathSegmentArray::const_iterator;

    /// Constructs a null path.
    Path() noexcept = default;

    Path(std::string_view path);
    //Path(Element* element, std::string_view relativePath);

    static Path fromId(core::StringId id) {
        Path p;
        p.segments_.emplaceLast(id, PathSegmentType::Id);
        return p;
    }

    size_t hash() const noexcept {
        size_t res = 1346458696; // 'PATH'
        for (const PathSegment& seg : segments_) {
            core::hashCombine(res, seg.hash());
        }
        return res;
    }

    std::string toString() const {
        fmt::memory_buffer b;
        write_(b);
        return std::string(b.begin(), b.size());
    }

    bool isAbsolute() const noexcept {
        if (!segments_.isEmpty()) {
            PathSegmentType type = segments_.begin()->type();
            return (type == PathSegmentType::Root) || (type == PathSegmentType::Id);
        }
        return false;
    }

    bool isRelative() const noexcept {
        return !isAbsolute();
    }

    bool isIdBased() const noexcept {
        return !segments_.isEmpty() && segments_.begin()->type() == PathSegmentType::Id;
    }

    core::StringId baseId() const noexcept {
        if (!segments_.isEmpty()) {
            const PathSegment& seg0 = segments_[0];
            if (seg0.type() == PathSegmentType::Id) {
                return seg0.nameOrId();
            }
        }
        return core::StringId();
    }

    bool isElementPath() const noexcept {
        return !isAttributePath();
    }

    bool isAttributePath() const noexcept {
        return !segments_.isEmpty()
               && segments_.rbegin()->type() == PathSegmentType::Attribute;
    }

    const core::Array<PathSegment>& segments() const {
        return segments_;
    }

    Path getElementPath() const;
    Path getElementRelativeAttributePath() const;

    void appendAttributePath(const Path& other);

    friend bool operator==(const Path& lhs, const Path& rhs) noexcept {
        return lhs.segments_.size() == rhs.segments_.size()
               && std::equal(
                   lhs.segments_.begin(), lhs.segments_.end(), rhs.segments_.begin());
    }

    friend bool operator!=(const Path& lhs, const Path& rhs) noexcept {
        return !(lhs == rhs);
    }

    friend bool operator<(const Path& lhs, const Path& rhs) noexcept {
        // XXX slow
        return lhs.toString() < rhs.toString();
    }

    template<typename OStream>
    friend void write(OStream& out, const Path& path) {
        if constexpr (std::is_same_v<OStream, fmt::memory_buffer>) {
            path.write_(out);
        }
        else {
            fmt::memory_buffer b;
            path.write_(b);
            std::string_view strv(b.begin(), static_cast<std::streamsize>(b.size()));
            write(out, strv);
        }
    }

private:
    core::Array<PathSegment> segments_;

    // Used for path updates.
    // Could use a wrapper InternalPath in Value/Schema
    // if the size of Path becomes an issue.
    mutable core::Id baseInternalId_ = {};
    mutable core::Id targetInternalId_ = {};
    friend detail::PathUpdater;

    Path(core::Array<PathSegment>&& segments)
        : segments_(std::move(segments)) {
    }

    void write_(fmt::memory_buffer& out) const;
};

namespace detail {

class PathUpdateData {
public:
    PathUpdateData() = default;

    const std::unordered_map<core::Id, core::Id>& copiedElements() const {
        return copiedElements_;
    }

    void addCopiedElement(core::Id oldInternalId, core::Id newInternalId) {
        copiedElements_[oldInternalId] = newInternalId;
    }

    const core::Array<core::Id>& absolutePathChangedElements() const {
        return absolutePathChangedElements_;
    }

    void addAbsolutePathChangedElement(core::Id internalId) {
        if (!absolutePathChangedElements_.contains(internalId)) {
            absolutePathChangedElements_.append(internalId);
        }
    }

private:
    std::unordered_map<core::Id, core::Id> copiedElements_;
    core::Array<core::Id> absolutePathChangedElements_;
};

struct PathUpdater {

    VGC_DOM_API
    static void preparePathForUpdate(const Path& path, const Node* owner);

    VGC_DOM_API
    static void updatePath(Path& path, const Node* owner, const PathUpdateData& data);
};

} // namespace detail

/// This is a type trait that should be specialized for any type that stores
/// `vgc::dom::Path` data, so that these paths can be updated by the `Document`
/// in case a referenced element is moved, copied, or its ID changed.
///
/// See implementation of `PathVisitor<detail::DomFaceCycles>` for an example.
///
template<typename T, typename SFINAE = void>
struct PathVisitor {};

/// Type trait for `hasPaths<T>`.
///
template<typename T, typename SFINAE = void>
struct HasPaths : std::false_type {};

template<typename T>
struct HasPaths<
    T,
    core::RequiresValid<decltype(&PathVisitor<T>::template visit<T&, void (*)(Path&)>)>>
    : std::true_type {};

/// Checks whether the given type `T` has a `PathVisitor<T>` specialization.
///
template<typename T>
inline constexpr bool hasPaths = HasPaths<T>::value;

template<>
struct PathVisitor<Path> {

    // Note: we use a forwarding reference `Fn&&` instead of a copy since:
    //
    // - The functor can be an `std::function` which is slow to copy (heap
    // allocation) if the state is larger than the storage size of its
    // small-value optimization. This would be really bad since the visitor
    // typically calls downstream visitors, which would result in more copies
    // of the functor than there are paths to visit.
    //
    // - This allows to use functors that have mutable state, for example, a
    // visitor that counts the number of paths.
    //
    // However, we must not use `std::forward<Fn>(fn)` in the implementation of
    // `visit()`, since we never want to forward the functor as an rvalue
    // reference (`Fn&&`). It would indeed be incorrect to do so, since we
    // typically call `visit(x, fn)` multiple times, so none of these calls
    // should consume the resources of `fn`, otherwise `fn` would not be usable
    // for subsequent calls.
    //
    // In other words, the only reason we use a forwarding reference is to
    // allow passing the functor as either `Fn&` or `const Fn&`, and this is
    // properly deduced without using `std::forward`. passing `fn` as a rvalue
    // would be incorrect.
    //
    // Similarly, we use a forwarding reference for the type to visit so that
    // we support `Path&&`, `const Path&`, and `Path&` as input, but we do not
    // need to forward it as a rvalue reference.
    //
    // Note: using the following:
    //
    //   template<typename MaybeConst, typename Fn>
    //   static void visit(MaybeConst& path, Fn&& fn);
    //
    // would not allow to use `PathVisitor<Path>::visit(Path(), fn);`.
    //
    template<typename Path, typename Fn>
    static void visit(Path&& path, Fn&& fn) {
        fn(path);
    }
};

template<typename T>
struct PathVisitor<NoneOr<T>, core::Requires<hasPaths<T>>> {
    template<typename NoneOrT, typename Fn>
    static void visit(NoneOrT&& noneOr, Fn&& fn) {
        if (noneOr.has_value()) {
            PathVisitor<T>::visit(noneOr.value(), fn);
        }
    }
};

static_assert(std::is_copy_constructible_v<Path>);
static_assert(std::is_move_constructible_v<Path>);
static_assert(std::is_copy_assignable_v<Path>);
static_assert(std::is_move_assignable_v<Path>);

/// Alias for `vgc::core::Array<vgc::dom::Path>`.
///
using PathArray = core::Array<Path>;

/// Alias for `vgc::core::SharedConstArray<vgc::dom::Path>`.
///
using SharedConstPathArray = core::SharedConstArray<Path>;

constexpr bool isValidIdFirstChar(char c) {
    constexpr std::string_view specials = "_-";
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z')
           || specials.find(c) != std::string::npos;
}

constexpr bool isValidIdChar(char c) {
    constexpr std::string_view specials = "_-";
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9')
           || specials.find(c) != std::string::npos;
}

constexpr bool isValidPathFirstChar(char c) {
    return (c == '#' || c == '@');
}

/// Reads a `Path` from the input stream, and stores it in the given output
/// parameter `v`. Leading whitespaces are allowed. Raises `ParseError` if the
/// stream does not start with a `Path`.
///
template<typename IStream>
void readTo(Path& v, IStream& in) {
    core::skipWhitespaceCharacters(in);
    char c = core::readExpectedCharacter(in, {'#', '@'});
    if (c == '#') {
        std::string s;
        // appends '#' first
        s.push_back(c);
        if (in.get(c)) {
            bool allowed = isValidIdFirstChar(c);
            while (allowed) {
                s.push_back(c);
                if (!in.get(c)) {
                    break;
                }
                allowed = isValidIdChar(c);
            }
            if (!allowed) {
                in.unget();
            }
        }
        v = Path(s);
        return;
    }
    // else '@'
    core::skipExpectedCharacter(in, '\'');
    std::string s = core::readStringUntilExpectedCharacter(in, '\'');
    v = Path(s);
}

} // namespace vgc::dom

template<>
struct fmt::formatter<vgc::dom::Path> : fmt::formatter<std::string_view> {
    template<typename FormatContext>
    auto format(const vgc::dom::Path& x, FormatContext& ctx) {
        return fmt::formatter<std::string_view>::format(x.toString(), ctx);
    }
};

#endif // VGC_DOM_PATH_H
