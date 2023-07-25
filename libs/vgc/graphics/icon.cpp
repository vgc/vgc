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

#include <vgc/graphics/icon.h>

#include <vgc/core/colors.h>
#include <vgc/core/io.h>
#include <vgc/graphics/strings.h>
#include <vgc/graphics/svg.h>
#include <vgc/style/parse.h>

namespace vgc::graphics {

namespace detail {

class IconData {
public:
    core::Array<graphics::SvgSimplePath> paths;
};

void IconDataDeleter::operator()(IconData* p) {
    delete p;
}

} // namespace detail

namespace {

core::Color getColor(const Icon* icon, core::StringId property) {
    core::Color res;
    style::Value value = icon->style(property);
    if (value.has<core::Color>()) {
        res = value.to<core::Color>();
    }
    return res;
}

} // namespace

Icon::Icon(std::string_view filePath) {

    data_.reset(new detail::IconData());
    std::string svg = core::readFile(filePath);
    data_->paths = getSvgSimplePaths(svg);

    geometry::Vec2d sized = getSvgViewBox(svg).size();
    size_[0] = core::narrow_cast<float>(sized[0]);
    size_[1] = core::narrow_cast<float>(sized[1]);
}

IconPtr Icon::create(std::string_view filePath) {
    return IconPtr(new Icon(filePath));
}

void Icon::draw(graphics::Engine* engine) {
    updateEngine_(engine);
    onPaintDraw_(engine);

    // XXX: do we want to support clipping in this class, or defer
    // that to client code? See implementation of ui::Widget::paint().
}

void Icon::populateStyleSpecTable(style::SpecTable* table) {
    if (!table->setRegistered(staticClassName())) {
        return;
    }
    auto black = style::Value::custom(core::colors::black);
    table->insert(strings::icon_foreground_color, black, true, &style::parseColor);
    table->insert(strings::icon_accent_color, black, true, &style::parseColor);
    SuperClass::populateStyleSpecTable(table);
}

void Icon::onStyleChanged() {
    isInstanceBufferDirty_ = true;
}

void Icon::updateEngine_(graphics::Engine* engine) {
    if (engine != lastPaintEngine_) {
        setEngine_(engine);
    }
}

void Icon::setEngine_(graphics::Engine* engine) {
    if (lastPaintEngine_) {
        releaseEngine_();
    }
    lastPaintEngine_ = engine;
    engine->aboutToBeDestroyed().connect(releaseEngineSlot_());
    onPaintCreate_(engine);
}

void Icon::releaseEngine_() {
    onPaintDestroy_(lastPaintEngine_);
    lastPaintEngine_->aboutToBeDestroyed().disconnect(releaseEngineSlot_());
    lastPaintEngine_ = nullptr;
}

void Icon::onPaintCreate_(graphics::Engine* engine) {

    // Vertex Buffer: XY
    core::FloatArray vertices;
    if (data_) {
        // Use pixelSize to avoid having too many triangles for
        // curves that span less than a pixel.
        //
        // TODO: what if pixelSize is not 1.0? How to handle icon displayed
        // zoomed in? We may want to have SizedIcon vs. Icon, similarly to
        // SizedFont vs. Font.
        //
        double pixelSize = 1.0;
        auto params = geometry::Curves2dSampleParams::semiAdaptive(pixelSize);

        // Triangulate all curves
        for (const SvgSimplePath& path : data_->paths) {
            path.curves().fill(vertices, params);
        }
    }
    BufferPtr vertexBuffer = engine->createVertexBuffer(std::move(vertices), false);

    // Instance Buffer: RGBA
    instanceBuffer_ = engine->createVertexBuffer(core::FloatArray(), true);

    // Create GeometryView
    graphics::GeometryViewCreateInfo createInfo = {};
    createInfo.setBuiltinGeometryLayout(graphics::BuiltinGeometryLayout::XY_iRGBA);
    createInfo.setPrimitiveType(graphics::PrimitiveType::TriangleList);
    createInfo.setVertexBuffer(0, vertexBuffer);
    createInfo.setVertexBuffer(1, instanceBuffer_);
    geometryView_ = engine->createGeometryView(createInfo);
}

void Icon::onPaintDraw_(graphics::Engine* engine) {
    if (isInstanceBufferDirty_) {
        isInstanceBufferDirty_ = false;
        core::Color c = getColor(this, strings::icon_foreground_color);
        core::FloatArray instanceData({c.r(), c.g(), c.b(), c.a()});
        engine->updateBufferData(instanceBuffer_, std::move(instanceData));
    }
    engine->setProgram(graphics::BuiltinProgram::Simple);
    engine->draw(geometryView_);
}

void Icon::onPaintDestroy_(graphics::Engine*) {
    geometryView_.reset();
}

} // namespace vgc::graphics
