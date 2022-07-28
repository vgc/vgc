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

#ifndef VGC_GRAPHICS_RICHTEXT_H
#define VGC_GRAPHICS_RICHTEXT_H

#include <vgc/style/stylableobject.h>

#include <vgc/geometry/rect2f.h>

#include <vgc/graphics/api.h>
#include <vgc/graphics/engine.h>
#include <vgc/graphics/text.h>

namespace vgc::graphics {

VGC_DECLARE_OBJECT(RichText);
VGC_DECLARE_OBJECT(RichTextSpan);

/// \class vgc::graphics::RichTextSpan
/// \brief One element in a RichText tree
///
/// A `RichText` is represented as a tree of `RichTextSpan` objects, where each
/// span can have a specific style. For example, `"this contains <b>bold</d>
/// text."` would be represented as the following tree of spans:
///
/// \verbatim
///                   (root)
///            _______| | |_______
///           |         |        |
/// "this contains"   "bold"   "text"
/// \verbatim
///
class VGC_GRAPHICS_API RichTextSpan : public style::StylableObject {
private:
    VGC_OBJECT(RichTextSpan, style::StylableObject)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

    friend class RichText;
    RichTextSpan(RichTextSpan* parent = nullptr);
    static RichTextSpanPtr createRoot();
    RichTextSpan* createChild();

public:
    /// Returns the parent `RichTextSpan` of this `RichTextSpan`. This can be
    /// `nullptr` for root spans.
    ///
    /// \sa firstChild(), lastChild(), previousSibling(), and nextSibling().
    ///
    RichTextSpan* parent() const
    {
        return parent_;
    }

    /// Returns the root of this `RichTextSpan` tree.
    ///
    /// \sa parent().
    ///
    RichTextSpan* root() const
    {
        return root_;
    }

    /// Returns the first child `RichTextSpan` of this `RichTextSpan`. Returns
    /// `nullptr` if this `RichTextSpan` has no children.
    ///
    /// \sa lastChild(), previousSibling(), nextSibling(), and parent().
    ///
    RichTextSpan* firstChild() const
    {
        return children_->first();
    }

    /// Returns the last child `RichTextSpan` of this `RichTextSpan`. Returns
    /// `nullptr` if this `RichTextSpan` has no children.
    ///
    /// \sa firstChild(), previousSibling(), nextSibling(), and parent().
    ///
    RichTextSpan* lastChild() const
    {
        return children_->last();
    }

    /// Returns the previous sibling of this `RichTextSpan`. Returns `nullptr`
    /// if this `RichTextSpan` is a root span, or if it is the first child of
    /// its parent.
    ///
    /// \sa nextSibling(), parent(), firstChild(), and lastChild().
    ///
    RichTextSpan* previousSibling() const
    {
        return (this == root_)
                ? nullptr
                : static_cast<RichTextSpan*>(previousSiblingObject());
    }

    /// Returns the next sibling of this `RichTextSpan`. Returns `nullptr` if
    /// this `RichTextSpan` is a root span, or if it is the last child of its
    /// parent.
    ///
    /// \sa previousSibling(), parent(), firstChild(), and lastChild().
    ///
    RichTextSpan* nextSibling() const
    {
        return (this == root_)
                ? nullptr
                : static_cast<RichTextSpan*>(nextSiblingObject());
    }

    /// Returns all children of this `RichTextSpan` as an iterable range.
    ///
    /// Example:
    ///
    /// \code
    /// for (RichTextSpan* child : span->children()) {
    ///     // ...
    /// }
    /// \endcode
    ///
    RichTextSpanListView children() const
    {
        return RichTextSpanListView(children_);
    }

    // implements StylableObject interface
    style::StylableObject* parentStylableObject() const override;
    style::StylableObject* firstChildStylableObject() const override;
    style::StylableObject* lastChildStylableObject() const override;
    style::StylableObject* previousSiblingStylableObject() const override;
    style::StylableObject* nextSiblingStylableObject() const override;
    const style::StyleSheet* defaultStyleSheet() const override;

    static const style::StylePropertySpecTable* stylePropertySpecs();

private:
    RichTextSpan* parent_;
    RichTextSpan* root_;
    RichTextSpanList* children_;
};

/// \class vgc::graphics::RichText
/// \brief Represents text with complex layout and style.
///
class VGC_GRAPHICS_API RichText : public RichTextSpan {
private:
    VGC_OBJECT(RichText, RichTextSpan)
    RichText(std::string_view text);

public:
    static RichTextPtr create();
    static RichTextPtr create(std::string_view text);

    /// Manually specifies a parent `StylableObject` for this `RichText`.
    ///
    /// This can be used to make this RichText part of an existing hierarchy of
    /// `StylableObject`, for example, a widget tree.
    ///
    void setParentStylableObject(style::StylableObject* parent) {
        parentStylableObject_ = parent;
    }

    /// Sets the text of this `RichText`.
    ///
    void setText(std::string_view text);

    /// Returns the text of this `RichText`.
    ///
    const std::string& text() const
    {
        return text_;
    }

    /// Sets the rectangle of this `RichText`, used for text alignment and
    /// word-wrapping.
    ///
    ///
    void setRect(const geometry::Rect2f& rect)
    {
        rect_ = rect;
        updateScroll_();
    }

    /// Returns the rectangle of this `RichText`.
    ///
    const geometry::Rect2f& rect() const
    {
        return rect_;
    }

    /// Inserts to the given `FloatArray` a list of triangles that can be used to draw
    /// this `RichText`.
    ///
    void fill(core::FloatArray& a);

    /// Returns whether the cursor is visible.
    ///
    bool isCursorVisible() const
    {
        return isCursorVisible_;
    }

    /// Sets whether the cursor is visible.
    ///
    void setCursorVisible(bool isVisible)
    {
        isCursorVisible_ = isVisible;
        updateScroll_();
    }

    /// Returns whether the selection is visible.
    ///
    bool isSelectionVisible() const
    {
        return isSelectionVisible_;
    }

    /// Sets whether the selection is visible.
    ///
    void setSelectionVisible(bool isVisible)
    {
        isSelectionVisible_ = isVisible;
        updateScroll_();
    }

    void setSelectionBeginBytePosition(Int bytePosition)
    {
        selectionBegin_ = bytePosition;
    }

    void setSelectionEndBytePosition(Int bytePosition)
    {
        selectionEnd_ = bytePosition;
        updateScroll_();
    }

    void clearSelection()
    {
        selectionBegin_ = selectionEnd_;
    }

    bool hasSelection()
    {
        return selectionBegin_ != selectionEnd_;
    }

    /// Deletes the selected text, and change the selection to a cursor
    /// where the text was previously located.
    ///
    void deleteSelectedText();

    /// Deletes the selected text if there is a selection. Otherwise, deletes
    /// the `boundaryType` entity (grapheme, word, line, etc.) immediately
    /// after the cursor.
    ///
    void deleteNext(TextBoundaryType boundaryType);

    /// Deletes the selected text if there is a selection. Otherwise, deletes
    /// the `boundaryType` entity (grapheme, word, line, etc.) immediately
    /// before the cursor.
    ///
    void deletePrevious(TextBoundaryType boundaryType);

    /// Returns the position in byte of the cursor.
    ///
    Int cursorBytePosition() const
    {
        return selectionEnd_;
    }

    /// Sets the position in byte of the cursor.
    ///
    void setCursorBytePosition(Int bytePosition)
    {
        selectionBegin_ = bytePosition;
        selectionEnd_ = bytePosition;
        updateScroll_();
    }

    /// Sets the position of the cursor corresponding to the grapheme boundary
    /// closest to the given mouse position.
    ///
    void setCursorFromMousePosition(const geometry::Vec2f& mousePosition)
    {
        Int position = bytePosition(mousePosition);
        if (position != cursorBytePosition()) {
            setCursorBytePosition(position);
        }
    }

    /// Returns how much the text is scrolled horizontally relative to its
    /// default position. Scrolling is automatically performed in order to keep
    /// the cursor within `rect()`, whenever `isCursorVisible()` is true.
    ///
    float horizontalScroll()
    {
        return horizontalScroll_;
    }

    /// Manually changes how much the text is scrolled horizontally relative to
    /// its default position. Note that if `isCursorVisible()` is true, then
    /// the new `horizontalScroll()` might be different than the requested
    /// scroll to ensure that the cursor stays within `rect()`.
    ///
    void setHorizontalScroll(float horizontalScroll)
    {
        horizontalScroll_ = horizontalScroll;
        updateScroll_();
    }

    /// Returns the byte position in the original text corresponding to the
    /// grapheme boundary closest to the given mouse position.
    ///
    Int bytePosition(const geometry::Vec2f& mousePosition);

    // reimplementation of StylableObject virtual methods
    style::StylableObject* parentStylableObject() const override
    {
        return parentStylableObject_;
    }

private:
    style::StylableObject* parentStylableObject_;
    std::string text_;
    geometry::Rect2f rect_;
    graphics::ShapedText shapedText_;
    bool isSelectionVisible_;
    bool isCursorVisible_;
    Int selectionBegin_;
    Int selectionEnd_;
    float horizontalScroll_ = 0.0f;

    geometry::Vec2f graphemeAdvance_(Int bytePosition) const;
    float maxCursorHorizontalAdvance_() const;
    void updateScroll_();
};

} // namespace vgc::graphics

#endif // VGC_GRAPHICS_RICHTEXT_H
