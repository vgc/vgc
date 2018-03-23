// Copyright 2017 The VGC Developers
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
#include <vgc/dom/element.h>

namespace {
std::string generateXmlDeclaration_(const vgc::dom::Document* doc)
{
    std::string res = "<?xml version=\"" + doc->xmlVersion() + "\"";
    if (doc->hasXmlEncoding()) {
        res += " \"" + doc->xmlEncoding() + "\"";
    }
    if (doc->hasXmlStandalone()) {
        res += " \"" + doc->xmlStandaloneString() + "\"";
    }
    res += "?>";
    return res;
}
} // namespace

namespace vgc {
namespace dom {

Document::Document() :
    Node(NodeType::Document),
    hasXmlDeclaration_(true),
    hasXmlEncoding_(true),
    hasXmlStandalone_(true),
    xmlVersion_("1.0"),
    xmlEncoding_("UTF-8"),
    xmlStandalone_(false),
    xmlDeclaration_(generateXmlDeclaration_(this))
{

}

Element* Document::setRootElement(ElementSharedPtr element)
{
    // Nothing to do if this element is already the root
    if (element->parent() == this) {
        return element.get();
    }

    // Remove exising root element if any
    if (Element* existingRootElement = rootElement()) {
        removeChild(existingRootElement);
    }

    return Element::cast(appendChild(element));
}

Element* Document::rootElement() const
{
    for (NodeSharedPtr node : children()) {
        if (node->nodeType() == NodeType::Element) {
            return Element::cast(node.get());
        }
    }
    return nullptr;
}

std::string Document::xmlDeclaration() const
{
    return xmlDeclaration_;
}

bool Document::hasXmlDeclaration() const
{
    return hasXmlDeclaration_;
}

void Document::setXmlDeclaration()
{
    hasXmlDeclaration_ = true;
    xmlDeclaration_ = generateXmlDeclaration_(this);
}

void Document::setNoXmlDeclaration()
{
    hasXmlDeclaration_ = false;
    xmlDeclaration_ = "";
}

std::string Document::xmlVersion() const
{
    return xmlVersion_;
}

void Document::setXmlVersion(const std::string& version)
{
    xmlVersion_ = version;
    hasXmlDeclaration_ = true;
    xmlDeclaration_ = generateXmlDeclaration_(this);
}

std::string Document::xmlEncoding() const
{
    return xmlEncoding_;
}

bool Document::hasXmlEncoding() const
{
    return hasXmlEncoding_;
}

void Document::setXmlEncoding(const std::string& encoding)
{
    xmlEncoding_ = encoding;
    hasXmlEncoding_ = true;
    hasXmlDeclaration_ = true;
    xmlDeclaration_ = generateXmlDeclaration_(this);
}

void Document::setNoXmlEncoding()
{
    xmlEncoding_ = "UTF-8";
    hasXmlEncoding_ = false;
    if (hasXmlDeclaration_) {
        xmlDeclaration_ = generateXmlDeclaration_(this);
    }
}

bool Document::xmlStandalone() const
{
    return xmlStandalone_;
}

std::string Document::xmlStandaloneString() const
{
    return xmlStandalone() ? "yes" : "no";
}

bool Document::hasXmlStandalone() const
{
    return hasXmlStandalone_;
}

void Document::setXmlStandalone(bool standalone)
{
    xmlStandalone_ = standalone;
    hasXmlStandalone_ = true;
    hasXmlDeclaration_ = true;
    xmlDeclaration_ = generateXmlDeclaration_(this);
}

void Document::setNoXmlStandalone()
{
    xmlStandalone_ = false;
    hasXmlStandalone_ = false;
    if (hasXmlDeclaration_) {
        xmlDeclaration_ = generateXmlDeclaration_(this);
    }
}

void Document::save(const std::string& filePath)
{
    // XXX TODO
}

} // namespace dom
} // namespace vgc
