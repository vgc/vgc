// Copyright 2018 The VGC Developers
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

#include <vgc/dom/document.h>

#include <fstream>
#include <vgc/core/logging.h>
#include <vgc/dom/element.h>
#include <vgc/dom/io.h>

namespace vgc {
namespace dom {

Document::Document() :
    Node(this, NodeType::Document),
    hasXmlDeclaration_(true),
    hasXmlEncoding_(true),
    hasXmlStandalone_(true),
    xmlVersion_("1.0"),
    xmlEncoding_("UTF-8"),
    xmlStandalone_(false)
{
    generateXmlDeclaration_();
}

Element* Document::rootElement() const
{
    checkAlive();

    for (Node* node : children()) {
        if (node->nodeType() == NodeType::Element) {
            return Element::cast(node);
        }
    }
    return nullptr;
}

std::string Document::xmlDeclaration() const
{
    checkAlive();

    return xmlDeclaration_;
}

bool Document::hasXmlDeclaration() const
{
    checkAlive();

    return hasXmlDeclaration_;
}

void Document::setXmlDeclaration()
{
    checkAlive();

    hasXmlDeclaration_ = true;
    generateXmlDeclaration_();
}

void Document::setNoXmlDeclaration()
{
    checkAlive();

    hasXmlDeclaration_ = false;
    generateXmlDeclaration_();
}

std::string Document::xmlVersion() const
{
    checkAlive();

    return xmlVersion_;
}

void Document::setXmlVersion(const std::string& version)
{
    checkAlive();

    xmlVersion_ = version;
    hasXmlDeclaration_ = true;
    generateXmlDeclaration_();
}

std::string Document::xmlEncoding() const
{
    checkAlive();

    return xmlEncoding_;
}

bool Document::hasXmlEncoding() const
{
    checkAlive();

    return hasXmlEncoding_;
}

void Document::setXmlEncoding(const std::string& encoding)
{
    checkAlive();

    xmlEncoding_ = encoding;
    hasXmlEncoding_ = true;
    hasXmlDeclaration_ = true;
    generateXmlDeclaration_();
}

void Document::setNoXmlEncoding()
{
    checkAlive();

    xmlEncoding_ = "UTF-8";
    hasXmlEncoding_ = false;
    generateXmlDeclaration_();
}

bool Document::xmlStandalone() const
{
    checkAlive();

    return xmlStandalone_;
}

bool Document::hasXmlStandalone() const
{
    checkAlive();

    return hasXmlStandalone_;
}

void Document::setXmlStandalone(bool standalone)
{
    checkAlive();

    xmlStandalone_ = standalone;
    hasXmlStandalone_ = true;
    hasXmlDeclaration_ = true;
    generateXmlDeclaration_();
}

void Document::setNoXmlStandalone()
{
    checkAlive();

    xmlStandalone_ = false;
    hasXmlStandalone_ = false;
    generateXmlDeclaration_();
}

void Document::generateXmlDeclaration_()
{
    xmlDeclaration_.clear();
    if (hasXmlDeclaration_) {
        xmlDeclaration_.append("<?xml version=\"");
        xmlDeclaration_.append(xmlVersion_);
        xmlDeclaration_.append("\"");
        if (hasXmlEncoding_) {
            xmlDeclaration_.append(" encoding=\"");
            xmlDeclaration_.append(xmlEncoding_);
            xmlDeclaration_.append("\"");
        }
        if (hasXmlStandalone_) {
            xmlDeclaration_.append(" standalone=\"");
            xmlDeclaration_.append(xmlStandalone_ ? "yes" : "no");
            xmlDeclaration_.append("\"");
        }
        xmlDeclaration_.append("?>");
    }
}

bool Document::save(const std::string& filePath,
                    const XmlFormattingStyle& style) const
{
    checkAlive();

    std::ofstream out(filePath);
    if (!out.is_open()) {
        vgc::core::warning() << "Could not write file " << filePath << std::endl;
        return false;
    }

    out << xmlDeclaration_ << std::endl;    
    writeChildren(out, style, 0, this);

    return true;
}

} // namespace dom
} // namespace vgc
