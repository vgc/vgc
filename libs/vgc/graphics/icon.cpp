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

namespace {

enum class ColorType {
    Custom,
    Foreground,
    Accent
};

struct ColorSpec {
    ColorType type = ColorType::Custom;
    core::Color color; // only alpha is used if non-custom

    bool operator==(const ColorSpec& other) {
        return (type == other.type) && (color == other.color);
    }

    bool operator!=(const ColorSpec& other) {
        return !(*this == other);
    }
};

struct Batch {

    explicit Batch(const ColorSpec& colorSpec)
        : colorSpec(colorSpec) {
    }

    // Data that can be reused on Engine change
    ColorSpec colorSpec;
    core::FloatArray vertices;

    // Data that needs to be recreatesd on Engine change
    BufferPtr instanceBuffer;
    GeometryViewPtr geometryView;
};

} // namespace

namespace detail {

class IconData {
public:
    core::Array<SvgSimplePath> paths;
};

class IconResources {
public:
    core::Array<Batch> batches;
};

void IconDataDeleter::operator()(IconData* p) {
    delete p;
}

void IconResourcesDeleter::operator()(IconResources* p) {
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

// Create the batches, leaving Engine resources uninitialized for now.
//
core::Array<Batch> createBatchesFromPaths(const core::Array<SvgSimplePath>& paths) {

    core::Array<Batch> res;

    // Use pixelSize to avoid having too many triangles for
    // curves that span less than a pixel.
    //
    // TODO: what if pixelSize is not 1.0? How to handle icon displayed
    // zoomed in? We may want to have SizedIcon vs. Icon, similarly to
    // SizedFont vs. Font.
    //
    double pixelSize = 1.0;
    auto params = geometry::Curves2dSampleParams::semiAdaptive(pixelSize);

    for (const SvgSimplePath& path : paths) {

        // Determine which color to draw the path
        ColorSpec colorSpec;
        if (path.styleClasses().contains("background")) {
            continue;
        }
        else if (path.styleClasses().contains("fill-foreground-color")) {
            colorSpec.type = ColorType::Foreground;
            if (path.fill().paintType() == SvgPaintType::Color) {
                colorSpec.color.setA(path.fill().color().a());
            }
        }
        else if (path.styleClasses().contains("fill-accent-color")) {
            colorSpec.type = ColorType::Accent;
            if (path.fill().paintType() == SvgPaintType::Color) {
                colorSpec.color.setA(path.fill().color().a());
            }
        }
        else if (path.fill().paintType() == SvgPaintType::Color) {
            colorSpec.color = path.fill().color();
        }
        else {
            continue;
        }

        // Create a new batch if necessary, or keep using the same batch
        if (res.isEmpty() || res.last().colorSpec != colorSpec) {
            res.emplaceLast(colorSpec);
        }
        Batch& batch = res.last();

        // Triangulate the path (Layout: XY)
        path.curves().fill(batch.vertices, params);
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

    // Triangulate the icon data and convert to batches
    resources_.reset(new detail::IconResources());
    resources_->batches = createBatchesFromPaths(data_->paths);
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
    shouldUpdateInstanceBuffers_ = true;
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

    for (Batch& batch : resources_->batches) {

        // Vertex buffer: XY
        BufferPtr vertexBuffer = engine->createVertexBuffer(batch.vertices, false);

        // Instance Buffer: RGBA
        batch.instanceBuffer = engine->createVertexBuffer(core::FloatArray(), true);

        // Create GeometryView
        graphics::GeometryViewCreateInfo createInfo = {};
        createInfo.setBuiltinGeometryLayout(graphics::BuiltinGeometryLayout::XY_iRGBA);
        createInfo.setPrimitiveType(graphics::PrimitiveType::TriangleList);
        createInfo.setVertexBuffer(0, vertexBuffer);
        createInfo.setVertexBuffer(1, batch.instanceBuffer);
        batch.geometryView = engine->createGeometryView(createInfo);
    }

    shouldUpdateInstanceBuffers_ = true;
}

void Icon::onPaintDraw_(graphics::Engine* engine) {
    if (shouldUpdateInstanceBuffers_) {
        shouldUpdateInstanceBuffers_ = false;
        core::Color foregroundColor = getColor(this, strings::icon_foreground_color);
        core::Color accentColor = getColor(this, strings::icon_accent_color);
        for (Batch& batch : resources_->batches) {
            core::Color c;
            switch (batch.colorSpec.type) {
            case ColorType::Custom:
                c = batch.colorSpec.color;
                break;
            case ColorType::Foreground:
                c = foregroundColor;
                c.setA(batch.colorSpec.color.a());
                break;
            case ColorType::Accent:
                c = accentColor;
                c.setA(batch.colorSpec.color.a());
                break;
            }
            core::FloatArray instanceData({c.r(), c.g(), c.b(), c.a()});
            engine->updateBufferData(batch.instanceBuffer, std::move(instanceData));
        }
    }
    engine->setProgram(graphics::BuiltinProgram::Simple);
    for (const Batch& batch : resources_->batches) {
        engine->draw(batch.geometryView);
    }
}

void Icon::onPaintDestroy_(graphics::Engine*) {
    for (Batch& batch : resources_->batches) {
        batch.instanceBuffer.reset();
        batch.geometryView.reset();
    }
}

} // namespace vgc::graphics
