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

#include <vgc/ui/imagebox.h>

#include <QImageReader>

#include <vgc/core/array.h>
#include <vgc/graphics/strings.h>
#include <vgc/ui/logcategories.h>
#include <vgc/ui/strings.h>

#include <vgc/ui/detail/paintutil.h>

namespace vgc::ui {

ImageBox::ImageBox(std::string_view relativePath)
    : Widget() {

    QImageReader reader(relativePath.data());
    reader.setAutoTransform(true);
    qimage_ = reader.read();

    //addStyleClass(strings::ImageBox);
}

ImageBoxPtr ImageBox::create(std::string_view relativePath) {
    return ImageBoxPtr(new ImageBox(relativePath));
}

void ImageBox::setImage(std::string_view /*relativePath*/) {
    //if (...) {
    //    reload_ = true;
    //    requestRepaint();
    //}
}

void ImageBox::onResize() {
    SuperClass::onResize();
    reload_ = true;
}

void ImageBox::onPaintCreate(graphics::Engine* engine) {
    SuperClass::onPaintCreate(engine);
    quad_ =
        engine->createDynamicTriangleStripView(graphics::BuiltinGeometryLayout::XYUVRGBA);
}

void ImageBox::onPaintDraw(graphics::Engine* engine, PaintOptions options) {

    SuperClass::onPaintDraw(engine, options);

    namespace gs = graphics::strings;

    if (reloadImg_) {
        reload_ = true;
        reloadImg_ = false;
        qimage_ = qimage_.convertToFormat(QImage::Format_RGBA8888);

        const Int w = qimage_.width();
        const Int h = qimage_.height();
        Int size = w * h * 4;
        core::Array<char> bits(size);
        std::copy(qimage_.bits(), qimage_.bits() + size, bits.data());

        {
            graphics::ImageCreateInfo info = {};
            info.setWidth(w);
            info.setHeight(h);
            info.setRank(graphics::ImageRank::_2D);
            info.setPixelFormat(graphics::PixelFormat::RGBA_8_UNORM);
            info.setUsage(graphics::Usage::Default);
            info.setBindFlags(
                graphics::ImageBindFlag::ShaderResource
                | graphics::ImageBindFlag::RenderTarget);
            info.setCpuAccessFlags(graphics::CpuAccessFlag::None);
            // enable mips
            info.setResourceMiscFlags(graphics::ResourceMiscFlag::GenerateMips);
            info.setNumMipLevels(0);
            image_ = engine->createImage(info, std::move(bits));
        }
        {
            graphics::ImageViewCreateInfo info = {};
            info.setBindFlags(graphics::ImageBindFlag::ShaderResource);
            imageView_ = engine->createImageView(info, image_);
            engine->generateMips(imageView_);
        }
        {
            graphics::SamplerStateCreateInfo info = {};
            samplerState_ = engine->createSamplerState(info);
        }
    }

    if (reload_) {
        reload_ = false;
        core::FloatArray a;
        const Int w = qimage_.width();
        const Int h = qimage_.height();
        geometry::Vec2f siz = size();
        float scale = siz.y() / static_cast<float>(h);
        if (w * scale >= siz.x()) {
            scale = siz.x() / static_cast<float>(w);
        }
        geometry::Vec2f brc = {scale * w, scale * h};
        geometry::Vec2f tlc = {(siz.x() - brc.x()) * 0.5f, (siz.y() - brc.y()) * 0.5f};
        brc += tlc;
        a.extend({
            tlc.x(), tlc.y(), 0.f, 0.f, 1.f, 1.f, 1.f, 1.f, //
            tlc.x(), brc.y(), 0.f, 1.f, 1.f, 1.f, 1.f, 1.f, //
            brc.x(), tlc.y(), 1.f, 0.f, 1.f, 1.f, 1.f, 1.f, //
            brc.x(), brc.y(), 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, //
        });
        engine->updateVertexBufferData(quad_, std::move(a));
    }

    engine->pushProgram(graphics::BuiltinProgram::SimpleTextured);
    engine->setStageImageViews(&imageView_, 0, 1, graphics::ShaderStage::Pixel);
    engine->setStageSamplers(&samplerState_, 0, 1, graphics::ShaderStage::Pixel);
    engine->draw(quad_, -1, 0);
    engine->popProgram();
}

void ImageBox::onPaintDestroy(graphics::Engine* engine) {
    quad_.reset();
    SuperClass::onPaintDestroy(engine);
}

geometry::Vec2f ImageBox::computePreferredSize() const {

    namespace gs = graphics::strings;

    PreferredSizeType auto_ = PreferredSizeType::Auto;
    PreferredSize w = preferredWidth();
    PreferredSize h = preferredHeight();

    geometry::Vec2f res(0, 0);
    geometry::Vec2f imgPreferredSize(0, 0);
    if (w.type() == auto_ || h.type() == auto_) {
        imgPreferredSize = geometry::Vec2f{100, 100};
    }
    if (w.type() == auto_) {
        float paddingLeft = detail::getLength(this, gs::padding_left);
        float paddingRight = detail::getLength(this, gs::padding_right);
        res[0] = imgPreferredSize[0] + paddingLeft + paddingRight;
    }
    else {
        res[0] = w.value();
    }
    if (h.type() == auto_) {
        float paddingTop = detail::getLength(this, gs::padding_top);
        float paddingBottom = detail::getLength(this, gs::padding_bottom);
        res[1] = imgPreferredSize[1] + paddingTop + paddingBottom;
    }
    else {
        res[1] = h.value();
    }
    return res;
}

} // namespace vgc::ui
