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

#include <vgc/dom/path.h>

#include <array>

#include <vgc/dom/element.h>
#include <vgc/dom/logcategories.h>
#include <vgc/dom/strings.h>

namespace vgc::dom {

namespace {

const core::StringId nameAttrName("name");

} // namespace

PathSegment::PathSegment(
    core::StringId name,
    PathSegmentType type,
    PathSegmentFlags flags,
    Int arrayIndex) noexcept

    : name_(name)
    , type_(type)
    , flags_(flags)
    , arrayIndex_(arrayIndex) {
}

bool PathSegment::operator==(const PathSegment& other) const noexcept {
    if (name_ != other.name_ || type_ != other.type_ || flags_ != other.flags_) {
        return false;
    }
    if (flags_.has(PathSegmentFlag::Indexed) && arrayIndex_ != other.arrayIndex_) {
        return false;
    }
    return true;
}

bool PathSegment::operator<(const PathSegment& other) const noexcept {
    if (type_ != other.type_) {
        return core::toUnderlying(type_) < core::toUnderlying(other.type_);
    }
    int c = name_.compare(other.name());
    if (c != 0) {
        return c < 0;
    }
    if (flags_.has(PathSegmentFlag::Indexed) && arrayIndex_ != other.arrayIndex_) {
        return arrayIndex_ < other.arrayIndex_;
    }
    if (flags_ != other.flags_) {
        return flags_.toUnderlying() < other.flags_.toUnderlying();
    }
    return false;
}

namespace {

bool isReservedChar(char c) {
    constexpr std::string_view reserved = "/.#[]";
    return reserved.find(c) != std::string::npos;
}

size_t findReservedCharOrEnd(std::string_view path, size_t start) {
    const size_t n = path.size();
    size_t i = start;
    while (i < n && !isReservedChar(path[i])) {
        ++i;
    }
    return i;
}

} // namespace

Path::Path(std::string_view path) {
    const size_t n = path.size();
    size_t i = 0;
    size_t j = 0;

    // Empty path is equivalent to dot path.
    if (n == 0) {
        return;
    }

    char firstChar = path[0];
    if (firstChar == '/') {
        // Full path.
        segments_.emplaceLast(core::StringId(), PathSegmentType::Root);
        i = 1;
        if (i < n && !isReservedChar(path[1])) {
            j = findReservedCharOrEnd(path, i);
            segments_.emplaceLast(
                core::StringId(path.substr(i, j - i)), PathSegmentType::Element);
            i = j;
        }
    }
    else if (firstChar == '#') {
        // Based path.
        ++i;
        j = findReservedCharOrEnd(path, i);
        if (j == i) {
            VGC_ERROR(
                LogVgcDom, "Empty unique name (starts with '#') in path \"{}\".", path);
            segments_.clear();
            return;
        }
        segments_.emplaceLast(core::StringId(path.substr(i, j - i)), PathSegmentType::Id);
        i = j;
    }
    else if (firstChar == '.') {
        // Relative path.
        if (n > 2 && path[1] == '/') {
            // Skip only if it is not an attribute dot.
            i += 2;
        }
        else if (n == 1) {
            // "."
            return;
        }
    }
    else if (!isReservedChar(firstChar)) {
        // Relative path.
        j = findReservedCharOrEnd(path, i + 1);
        segments_.emplaceLast(
            core::StringId(path.substr(i, j - i)), PathSegmentType::Element);
        i = j;
    }

    while (i < n && path[i] == '/') {
        ++i;
        j = findReservedCharOrEnd(path, i);
        if (j == i) {
            VGC_ERROR(LogVgcDom, "Empty element name in path \"{}\".", path);
            segments_.clear();
            return;
        }
        segments_.emplaceLast(
            core::StringId(path.substr(i, j - i)), PathSegmentType::Element);
        i = j;
    }

    if (i < n && path[i] == '.') {
        ++i;
        j = findReservedCharOrEnd(path, i);
        if (j == i) {
            VGC_ERROR(LogVgcDom, "Empty attribute name in path \"{}\".", path);
            segments_.clear();
            return;
        }
        std::string_view attrName = path.substr(i, j - i);
        i = j;

        if (i < n && path[i] == '[') {
            ++i;
            j = findReservedCharOrEnd(path, i);
            if (j == n || path[j] != ']') {
                VGC_ERROR(LogVgcDom, "Expected ']' after index in path \"{}\".", path);
                segments_.clear();
                return;
            }
            if (j == i) {
                VGC_ERROR(LogVgcDom, "Empty index in path \"{}\".", path);
                segments_.clear();
                return;
            }

            Int index = 0;
            try {
                // XXX temp patch, core::parse currently allows leading and trailing whitespaces.
                if (core::isWhitespace(path[i]) || core::isWhitespace(path[j - 1])) {
                    throw core::RuntimeError("");
                }
                index = core::parse<Int>(path, i, j - i);
            }
            catch (core::RuntimeError&) {
                VGC_ERROR(LogVgcDom, "Invalid index format in path \"{}\".", path);
                segments_.clear();
                return;
            }

            segments_.emplaceLast(
                core::StringId(attrName),
                PathSegmentType::Attribute,
                PathSegmentFlag::Indexed,
                index);
            i = j + 1;
        }
        else {
            segments_.emplaceLast(core::StringId(attrName), PathSegmentType::Attribute);
        }
    }

    if (i != n) {
        VGC_ERROR(
            LogVgcDom,
            "Unexpected character '{}' at index {} in path \"{}\".",
            path[i],
            i,
            path);
        segments_.clear();
        return;
    }
}

Path Path::getElementPath() const {
    Path ret = {};
    for (auto it = segments_.rbegin(); it != segments_.rend(); ++it) {
        if (it->type() != PathSegmentType::Attribute) {
            ret.segments_.assign(segments_.begin(), it.base());
            break;
        }
    }
    return ret;
}

Path Path::getElementRelativeAttributePath() const {
    Path ret = {};
    for (auto it = segments_.rbegin(); it != segments_.rend(); ++it) {
        if (it->type() != PathSegmentType::Attribute) {
            ret.segments_.assign(it.base(), segments_.end());
            break;
        }
    }
    return ret;
}

void Path::writeTo_(fmt::memory_buffer& out) const {
    if (segments_.isEmpty()) {
        out.push_back('.');
        return;
    }

    size_t oldSize = out.size();

    bool skipSlash = true;
    Int i = 0;

    const PathSegment& seg0 = segments_[0];
    PathSegmentType type0 = seg0.type();
    if (type0 == PathSegmentType::Root) {
        out.push_back('/');
        ++i;
    }
    else if (type0 == PathSegmentType::Id) {
        out.push_back('#');
        out.append(seg0.name().string());
        skipSlash = false;
        ++i;
    }

    const Int n = segments_.length();
    for (; i < n; ++i) {
        const PathSegment& seg = segments_[i];
        PathSegmentType type = seg.type();
        if (type == PathSegmentType::Element) {
            if (skipSlash) {
                skipSlash = false;
            }
            else {
                out.push_back('/');
            }
            out.append(seg.name().string());
        }
        else if (type == PathSegmentType::Attribute) {
            out.push_back('.');
            out.append(seg.name().string());
            if (seg.isIndexed()) {
                out.push_back('[');
                // XXX better way ?
                core::formatTo(std::back_inserter(out), "{}", seg.arrayIndex());
                out.push_back(']');
            }
        }
        else {
            VGC_ERROR(
                LogVgcDom,
                "Could not convert dom::Path to string, it contains unexpected "
                "segments.");
            out.resize(oldSize);
            return;
        }
    }
}

} // namespace vgc::dom
