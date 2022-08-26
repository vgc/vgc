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

#ifndef VGC_UI_FLEXLAYOUT_H
#define VGC_UI_FLEXLAYOUT_H

#include <vgc/ui/widget.h>

namespace vgc::ui {

/// \enum vgc::ui::FlexDirection
/// \brief The direction of a flex layout
///
enum class FlexDirection {
    Row,
    RowReverse,
    Column,
    ColumnReverse
};

/// \enum vgc::ui::FlexWrap
/// \brief How to wrap widgets in a Flex
///
/// For now, only NoWrap is supported. Wrap and WrapReverse will
/// be added in the future.
///
enum class FlexWrap {
    NoWrap //,
    //Wrap,
    //WrapReverse
};

// TODO: FlexJustifyContent, FlexAlignItems, FlexAlignContent
// Great resource: https://css-tricks.com/snippets/css/a-guide-to-flexbox/

VGC_DECLARE_OBJECT(Flex);

/// \class vgc::ui::Flex
/// \brief Arrange a sequence of widgets in rows and/or columns.
///
class VGC_UI_API Flex : public Widget {
private:
    VGC_OBJECT(Flex, Widget)

protected:
    /// This is an implementation details. Please use
    /// Flex::create() instead.
    ///
    Flex(FlexDirection direction = FlexDirection::Row, FlexWrap wrap = FlexWrap::NoWrap);

public:
    /// Creates a Row.
    ///
    static FlexPtr create(
        FlexDirection direction = FlexDirection::Row,
        FlexWrap wrap = FlexWrap::NoWrap);

    /// Returns the FlexDirection of this Flex.
    ///
    FlexDirection direction() const {
        return direction_;
    }

    /// Sets the FlexDirection of this Flex.
    ///
    void setDirection(FlexDirection direction);

    /// Returns the FlexWrap of this Flex.
    ///
    FlexWrap wrap() const {
        return wrap_;
    }

    /// Sets the FlexWrap of this Flex.
    ///
    void setWrap(FlexWrap wrap);

    // reimpl
    float preferredWidthForHeight(float height) const override;
    float preferredHeightForWidth(float width) const override;

protected:
    void onWidgetAdded(Widget* child) override;
    void onWidgetRemoved(Widget* child) override;
    geometry::Vec2f computePreferredSize() const override;
    void updateChildrenGeometry() override;

private:
    FlexDirection direction_;
    FlexWrap wrap_;
};

} // namespace vgc::ui

#endif // VGC_UI_FLEXLAYOUT_H
