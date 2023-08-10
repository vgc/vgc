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

#include <vgc/core/algorithm.h>
#include <vgc/core/flags.h>
#include <vgc/core/format.h>
#include <vgc/core/id.h>
#include <vgc/core/object.h>
#include <vgc/core/parse.h>
#include <vgc/core/stringid.h>
#include <vgc/dom/api.h>

namespace vgc::dom {

VGC_DECLARE_OBJECT(Node);
VGC_DECLARE_OBJECT(Document);
VGC_DECLARE_OBJECT(Element);

class Path;
class PathUpdateData;

using StreamReader = core::StringReader;
using StreamWriter = core::StringWriter;

namespace detail {

void preparePathForUpdate(const Path& path, const Node* workingNode);
void updatePath(Path& path, const Node* workingNode, const PathUpdateData& data);

} // namespace detail

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
            path->write_(out);
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

    friend void detail::preparePathForUpdate(const Path&, const Node*);
    friend void detail::updatePath(Path&, const Node*, const PathUpdateData&);
    // Used for path updates.
    // Could use a wrapper InternalPath in Value/Schema
    // if the size of Path becomes an issue.
    mutable core::Id baseInternalId_ = {};
    mutable core::Id targetInternalId_ = {};

    Path(core::Array<PathSegment>&& segments)
        : segments_(std::move(segments)) {
    }

    void write_(fmt::memory_buffer& out) const;
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
