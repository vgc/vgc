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
#include <vgc/style/style.h>

#include <vgc/geometry/rect2f.h>

#include <vgc/graphics/api.h>
#include <vgc/graphics/engine.h>
#include <vgc/graphics/text.h>

namespace vgc::graphics {

VGC_GRAPHICS_API
style::StyleValue
parsePixelHinting(style::StyleTokenIterator begin, style::StyleTokenIterator end);

VGC_GRAPHICS_API
style::StyleValue
parseTextHorizontalAlign(style::StyleTokenIterator begin, style::StyleTokenIterator end);

VGC_GRAPHICS_API
style::StyleValue
parseTextVerticalAlign(style::StyleTokenIterator begin, style::StyleTokenIterator end);

VGC_DECLARE_OBJECT(RichText);
VGC_DECLARE_OBJECT(RichTextSpan);

enum class RichTextMoveOperation {
    NoMove,
    StartOfLine,
    StartOfText,
    StartOfSelection,
    EndOfLine,
    EndOfText,
    EndOfSelection,
    PreviousCharacter,
    PreviousWord,
    NextCharacter,
    NextWord,
    LeftOneCharacter,
    LeftOneWord,
    LeftOfSelection,
    RightOneCharacter,
    RightOneWord,
    RightOfSelection
};

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
    RichTextSpan* parent() const {
        return parent_;
    }

    /// Returns the root of this `RichTextSpan` tree.
    ///
    /// \sa parent().
    ///
    RichTextSpan* root() const {
        return root_;
    }

    /// Returns the first child `RichTextSpan` of this `RichTextSpan`. Returns
    /// `nullptr` if this `RichTextSpan` has no children.
    ///
    /// \sa lastChild(), previousSibling(), nextSibling(), and parent().
    ///
    RichTextSpan* firstChild() const {
        return children_->first();
    }

    /// Returns the last child `RichTextSpan` of this `RichTextSpan`. Returns
    /// `nullptr` if this `RichTextSpan` has no children.
    ///
    /// \sa firstChild(), previousSibling(), nextSibling(), and parent().
    ///
    RichTextSpan* lastChild() const {
        return children_->last();
    }

    /// Returns the previous sibling of this `RichTextSpan`. Returns `nullptr`
    /// if this `RichTextSpan` is a root span, or if it is the first child of
    /// its parent.
    ///
    /// \sa nextSibling(), parent(), firstChild(), and lastChild().
    ///
    RichTextSpan* previousSibling() const {
        return (this == root_) ? nullptr
                               : static_cast<RichTextSpan*>(previousSiblingObject());
    }

    /// Returns the next sibling of this `RichTextSpan`. Returns `nullptr` if
    /// this `RichTextSpan` is a root span, or if it is the last child of its
    /// parent.
    ///
    /// \sa previousSibling(), parent(), firstChild(), and lastChild().
    ///
    RichTextSpan* nextSibling() const {
        return (this == root_) ? nullptr
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
    RichTextSpanListView children() const {
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
    RichText();
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

    /// Makes the text empty
    ///
    void clear();

    /// Sets the text of this `RichText`.
    ///
    void setText(std::string_view text);

    /// Returns the text of this `RichText`.
    ///
    const std::string& text() const {
        return text_;
    }

    /// Sets the rectangle of this `RichText`, used for text alignment and
    /// word-wrapping.
    ///
    ///
    void setRect(const geometry::Rect2f& rect) {
        rect_ = rect;
        updateScroll_();
    }

    /// Returns the rectangle of this `RichText`.
    ///
    const geometry::Rect2f& rect() const {
        return rect_;
    }

    /// Inserts to the given `FloatArray` a list of triangles that can be used to draw
    /// this `RichText`.
    ///
    void fill(core::FloatArray& a) const;

    /// Returns whether the cursor is visible.
    ///
    bool isCursorVisible() const {
        return isCursorVisible_;
    }

    /// Sets whether the cursor is visible.
    ///
    void setCursorVisible(bool isVisible) {
        isCursorVisible_ = isVisible;
        updateScroll_();
    }

    /// Returns whether the selection is visible.
    ///
    bool isSelectionVisible() const {
        return isSelectionVisible_;
    }

    /// Sets whether the selection is visible.
    ///
    void setSelectionVisible(bool isVisible) {
        isSelectionVisible_ = isVisible;
        updateScroll_();
    }

    /// Returns the new position that a cursor would have if we applied
    /// the given `operation` from the given `position`.
    ///
    /// If the operation is `StartOfSelection`, `EndOfSelection`,
    /// `LeftOfSelection`, or `RightOfSelection`, the current selection is used
    /// to determine the resulting position. In case of multiple selections,
    /// the given `selectionIndex` specify which of the selections should be
    /// used.
    ///
    Int movedPosition(
        Int position,
        RichTextMoveOperation operation,
        Int selectionIndex = 0) const;

    /// Moves the cursor according to the given operation. If `select` is false
    /// (the default), then the selection is cleared. If `select` is true, then
    /// the current selection is modified to integrate the given operation
    /// (typically, this mode is used when a user presses `Shift`).
    ///
    void moveCursor(RichTextMoveOperation operation, bool select = false);

    /// Returns the start position of the selection. In case of multiple
    /// selections, the given `index` specify which of the selections
    /// should be used.
    ///
    Int selectionStart([[maybe_unused]] Int index = 0) const {
        return selectionStart_;
    }

    /// Returns the end position of the selection. In case of multiple
    /// selections, the given `index` specify which of the selections
    /// should be used.
    ///
    Int selectionEnd([[maybe_unused]] Int index = 0) const {
        return selectionEnd_;
    }

    /// Sets the start position of the selection. In case of multiple
    /// selections, the given `index` specify which of the selections
    /// should be used.
    ///
    void setSelectionStart(Int position, [[maybe_unused]] Int index = 0) {
        selectionStart_ = position;
    }

    /// Sets the end position of the selection. In case of multiple
    /// selections, the given `index` specify which of the selections
    /// should be used.
    ///
    void setSelectionEnd(Int position, [[maybe_unused]] Int index = 0) {
        selectionEnd_ = position;
        updateScroll_();
    }

    /// Unselects any selected region, preserving the cursor where it
    /// previously was, that is, at the end of the selection.
    ///
    void clearSelection() {
        selectionStart_ = selectionEnd_;
    }

    /// Returns whether there is at least one non-empty selected region.
    ///
    bool hasSelection() const {
        return selectionStart_ != selectionEnd_;
    }

    /// Changes the selection such that the entire text is selected.
    ///
    void selectAll() {
        selectionStart_ = 0;
        selectionEnd_ = static_cast<Int>(text_.size());
    }

    /// Returns the subset of `text()` that is selected, as an 'std::string`.
    ///
    std::string selectedText() const;

    /// Returns the subset of `text()` that is selected, as an `std::string_view`.
    ///
    std::string_view selectedTextView() const;

    /// Deletes the selected text, and change the selection to a cursor
    /// where the text was previously located.
    ///
    void deleteSelectedText();

    /// Deletes the selected text if there is a selection. Otherwise, deletes
    /// all the text between the current cursor position and the position
    /// reached by moving the cursor according to the given `operation`.
    ///
    void deleteFromCursor(RichTextMoveOperation operation);

    /// Inserts the given text at the current cursor location, and set the
    /// cursor at the end of the newly inserted text. If there was a selection
    /// prior to inserting the text, the selection is deleted.
    ///
    /// Note that all ASCII control characters (including tabs and newlines)
    /// are automatically removed from the given text. In future versions of
    /// the library, tabs and newlines may be supported.
    ///
    void insertText(std::string_view text);

    /// Returns the position in byte of the cursor.
    ///
    Int cursorBytePosition() const {
        return selectionEnd_;
    }

    /// Sets the position in byte of the cursor.
    ///
    void setCursorBytePosition(Int bytePosition) {
        selectionStart_ = bytePosition;
        selectionEnd_ = bytePosition;
        updateScroll_();
    }

    /// Returns how much the text is scrolled horizontally relative to its
    /// default position. Scrolling is automatically performed in order to keep
    /// the cursor within `rect()`, whenever `isCursorVisible()` is true.
    ///
    float horizontalScroll() const {
        return horizontalScroll_;
    }

    /// Manually changes how much the text is scrolled horizontally relative to
    /// its default position. Note that if `isCursorVisible()` is true, then
    /// the new `horizontalScroll()` might be different than the requested
    /// scroll to ensure that the cursor stays within `rect()`.
    ///
    void setHorizontalScroll(float horizontalScroll) {
        horizontalScroll_ = horizontalScroll;
        updateScroll_();
    }

    /// Returns the text position closest to the given 2D `point` that has
    /// all the given `boundaryMarkers`.
    ///
    Int positionFromPoint(
        const geometry::Vec2f& point,
        TextBoundaryMarkers boundaryMarkers = TextBoundaryMarker::Grapheme) const;

    /// Returns the pair of positions enclosing the given 2D `point` that has
    /// all the given `boundaryMarkers`.
    ///
    /// If no such pair of enclosing positions exist (for example, because the
    /// given `point` is outside the left-most position or right-most position
    /// of a given horizontal line of text), then the two returned positions are both
    /// equal to the position closest to the given `point`.
    ///
    std::pair<Int, Int> positionPairFromPoint(
        const geometry::Vec2f& point,
        TextBoundaryMarkers boundaryMarkers = TextBoundaryMarker::Grapheme) const;

    /// Returns the preferred size of the RichText, that is, the minimum size
    /// that can contain the text without wrapping or cropping.
    ///
    geometry::Vec2f preferredSize() const;

    // reimplementation of StylableObject virtual methods
    style::StylableObject* parentStylableObject() const override {
        return parentStylableObject_;
    }

protected:
    void onStyleChanged() override;

private:
    style::StylableObject* parentStylableObject_;
    std::string text_;
    geometry::Rect2f rect_;
    graphics::ShapedText shapedText_;
    bool isSelectionVisible_;
    bool isCursorVisible_;
    Int selectionStart_;
    Int selectionEnd_;
    float horizontalScroll_ = 0.0f;

    geometry::Vec2f advance_(Int position) const;
    float maxCursorHorizontalAdvance_() const;
    void updateScroll_();
    void insertText_(std::string_view text);
};

} // namespace vgc::graphics

#endif // VGC_GRAPHICS_RICHTEXT_H
