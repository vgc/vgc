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

#include <vgc/dom/document.h>

#include <cerrno>
#include <cstring>
#include <fstream>

#include <vgc/core/logging.h>
#include <vgc/core/xml.h>
#include <vgc/dom/element.h>
#include <vgc/dom/io.h>
#include <vgc/dom/operation.h>
#include <vgc/dom/schema.h>
#include <vgc/dom/strings.h>

namespace vgc::dom {

using core::XmlSyntaxError;

Document::Document(CreateKey key)
    : Node(key, this, NodeType::Document)
    , hasXmlDeclaration_(true)
    , hasXmlEncoding_(true)
    , hasXmlStandalone_(false)
    , xmlVersion_("1.0")
    , xmlEncoding_("UTF-8")
    , xmlStandalone_(false)
    , versionId_(core::genId()) {

    generateXmlDeclaration_();
}

/* static */
DocumentPtr Document::create() {
    return core::createObject<Document>();
}

namespace {

class Parser {
public:
    static DocumentPtr parse(std::string_view filePath) {
        DocumentPtr res = Document::create();
        Parser parser(filePath, res.get());
        return res;
    }

private:
    Document* document_;
    Node* currentNode_;
    std::string tagName_;
    const ElementSpec* elementSpec_;
    std::string attributeName_;
    std::string attributeValue_;
    std::string referenceName_;

    // Create the parser object
    Parser(std::string_view filePath, Document* document)
        : document_(document)
        , currentNode_(document)
        , elementSpec_(nullptr) {

        auto xml_ = core::XmlStreamReader::fromFile(filePath);
        while (xml_.readNext()) {
            switch (xml_.eventType()) {
            case core::XmlEventType::StartDocument:
                if (xml_.hasXmlDeclaration()) {
                    document_->setXmlDeclaration();
                    document_->setXmlVersion(xml_.version());
                    if (xml_.isEncodingSet()) {
                        document_->setXmlEncoding(xml_.encoding());
                    }
                    else {
                        document_->setNoXmlEncoding();
                    }
                    if (xml_.isStandaloneSet()) {
                        document_->setXmlStandalone(xml_.isStandalone());
                    }
                    else {
                        document_->setNoXmlStandalone();
                    }
                }
                else {
                    document_->setNoXmlDeclaration();
                }
                break;
            case core::XmlEventType::StartElement:
                tagName_ = xml_.name();
                onStartTag_();
                for (const core::XmlStreamAttributeView& attr : xml_.attributes()) {
                    attributeName_ = attr.name();
                    attributeValue_ = attr.value();
                    onAttribute_();
                }
                break;
            case core::XmlEventType::EndElement:
                tagName_ = xml_.name();
                onEndTag_();
                break;
            default:
                break;
            }
        }
    }

    // Action to be performed when a start tag is encountered.
    // The name of the tag is available in tagName_.
    void onStartTag_() {
        elementSpec_ = schema().findElementSpec(tagName_);
        if (!elementSpec_) {
            throw VgcSyntaxError(
                "Unknown element name '" + tagName_
                + "'. Expected an element name defined in the VGC schema.");
        }

        if (currentNode_->nodeType() == NodeType::Document) {
            Document* doc = Document::cast(currentNode_);
            currentNode_ = Element::create(doc, tagName_);
        }
        else if (currentNode_->nodeType() == NodeType::Element) {
            Element* element = Element::cast(currentNode_);
            currentNode_ = Element::create(element, tagName_);
        }
        else {
            // Note: this cannot happen yet, but we keep it as safeguard for
            // the future when adding more node types, or when restricting
            // in the schema which elements are allowed to be children of other
            // elements.
            throw VgcSyntaxError(
                "Unexpected element '" + tagName_
                + "'. Elements of this type are not allowed as children of the current "
                  "node type '"
                + core::toString(currentNode_->nodeType()) + "'.");
        }
    }

    // Action to be performed when an end tag (or the closing '/>' of an empty
    // element tag) is encountered. The name of the tag is available in
    // tagName_.
    void onEndTag_() {
        currentNode_ = currentNode_->parent();
        if (currentNode_->nodeType() == NodeType::Element) {
            tagName_ = Element::cast(currentNode_)->tagName();
            elementSpec_ = schema().findElementSpec(tagName_);
        }
        else {
            tagName_.clear();
            elementSpec_ = nullptr;
        }
    }

    // Action to be performed when an element attribute is encountered. The
    // attribute name and string value are available in attributeName_ and
    // attributeValue_.
    void onAttribute_() {
        core::StringId name(attributeName_);

        ValueType valueType = {};
        if (name == strings::name) {
            valueType = ValueType::StringId;
        }
        else if (name == strings::id) {
            valueType = ValueType::StringId;
        }
        else {
            const AttributeSpec* spec = elementSpec_->findAttributeSpec(name);
            if (!spec) {
                throw VgcSyntaxError(
                    "Unknown attribute '" + attributeName_ + "' for element '" + tagName_
                    + "'. Expected an attribute name defined in the VGC schema.");
            }
            valueType = spec->valueType();
        }

        Value value = parseValue(attributeValue_, valueType);
        Element::cast(currentNode_)->setAttribute(name, value);
    }
};

} // namespace

/* static */
DocumentPtr Document::open(std::string_view filePath) {
    // Note: in the future, we want to be able to detect formatting style of
    // input XML files, and preserve this style, as well as existing
    // non-significant whitespaces, etc. This is why we write our own parser,
    // since XML parsers typically discard all non-significant data. For
    // example, <foo name="hello"> and <foo    name="hello"> would typically
    // generate the same XmlStream events and we wouldn't be able to preserve
    // the formatting when saving back.

    return Parser::parse(filePath);
}

Element* Document::rootElement() const {
    for (Node* node : children()) {
        if (node->nodeType() == NodeType::Element) {
            return Element::cast(node);
        }
    }
    return nullptr;
}

std::string Document::xmlDeclaration() const {
    return xmlDeclaration_;
}

bool Document::hasXmlDeclaration() const {
    return hasXmlDeclaration_;
}

void Document::setXmlDeclaration() {
    hasXmlDeclaration_ = true;
    generateXmlDeclaration_();
}

void Document::setNoXmlDeclaration() {
    hasXmlDeclaration_ = false;
    hasXmlEncoding_ = false;
    hasXmlStandalone_ = false;
    generateXmlDeclaration_();
}

std::string Document::xmlVersion() const {
    return xmlVersion_;
}

void Document::setXmlVersion(std::string_view version) {
    xmlVersion_ = version;
    hasXmlDeclaration_ = true;
    generateXmlDeclaration_();
}

std::string Document::xmlEncoding() const {
    return xmlEncoding_;
}

bool Document::hasXmlEncoding() const {
    return hasXmlEncoding_;
}

void Document::setXmlEncoding(std::string_view encoding) {
    xmlEncoding_ = encoding;
    hasXmlEncoding_ = true;
    hasXmlDeclaration_ = true;
    generateXmlDeclaration_();
}

void Document::setNoXmlEncoding() {
    xmlEncoding_ = "UTF-8";
    hasXmlEncoding_ = false;
    generateXmlDeclaration_();
}

bool Document::xmlStandalone() const {
    return xmlStandalone_;
}

bool Document::hasXmlStandalone() const {
    return hasXmlStandalone_;
}

void Document::setXmlStandalone(bool standalone) {
    xmlStandalone_ = standalone;
    hasXmlStandalone_ = true;
    hasXmlDeclaration_ = true;
    generateXmlDeclaration_();
}

void Document::setNoXmlStandalone() {
    xmlStandalone_ = false;
    hasXmlStandalone_ = false;
    generateXmlDeclaration_();
}

void Document::generateXmlDeclaration_() {
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

void Document::save(const std::string& filePath, const XmlFormattingStyle& style) const {
    std::ofstream out(filePath);
    if (!out.is_open()) {
        throw FileError("Cannot save file " + filePath + ": " + std::strerror(errno));
    }

    if (hasXmlDeclaration()) {
        out << xmlDeclaration_ << std::endl;
    }
    writeChildren(out, style, 0, this);
}

Element* Document::elementById(core::StringId id) const {
    auto it = elementByIdMap_.find(id);
    if (it != elementByIdMap_.end()) {
        return it->second;
    }
    return nullptr;
}

Element* Document::elementByInternalId(core::Id id) const {
    auto it = elementByInternalIdMap_.find(id);
    if (it != elementByInternalIdMap_.end()) {
        return it->second;
    }
    return nullptr;
}

core::History* Document::enableHistory(core::StringId entrypointName) {
    if (!history_) {
        history_ = core::History::create(entrypointName);
        history_->headChanged().connect(onHistoryHeadChanged());
        history_->aboutToUndo().connect(onHistoryAboutToUndo());
        history_->undone().connect(onHistoryUndone());
        history_->aboutToRedo().connect(onHistoryAboutToRedo());
        history_->redone().connect(onHistoryRedone());
    }
    return history_.get();
}

void Document::disableHistory() {
    if (history_) {
        history_->disconnect(this);
        history_ = nullptr;
    }
}

bool Document::hasPendingDiff() {
    return !pendingDiff_.isEmpty();
}

bool Document::emitPendingDiff() {

    // Do nothing if there is no pending diff.
    //
    if (!hasPendingDiff()) {
        return false;
    }

    // Compress the diff.
    //
    // Note that pendingDiff_ might be non-empty before compression, but empty
    // after compression. If pendingDiff_ is empty after compression, we still
    // emits this empty diff since we want to provide the invariant that if
    // hasPendingDiff() is true, then a signal is emitted. This allows clients
    // to ignore the "next" document changed() signal if they know that the
    // change comes from them.
    //
    compressPendingDiff_();

    // Emits the diff now.
    //
    changed().emit(pendingDiff_);
    pendingDiff_.reset();
    pendingDiffKeepAllocPointers_.clear();
    return true;
}

void Document::compressPendingDiff_() {

    for (const auto& [node, oldRelatives] : previousRelativesMap_) {
        if (pendingDiff_.createdNodes_.contains(node)) {
            continue;
        }
        if (pendingDiff_.removedNodes_.contains(node)) {
            continue;
        }

        NodeRelatives newRelatives(node);
        if (oldRelatives.parent_ != newRelatives.parent_) {
            pendingDiff_.reparentedNodes_.insert(node);
            //pendingDiff_.childrenReorderedNodes_.insert(newRelatives.parent_);
        }
        else if (
            oldRelatives.nextSibling_ != newRelatives.nextSibling_
            || oldRelatives.previousSibling_ != newRelatives.previousSibling_) {
            // this introduces false positives if old siblings were removed or
            // new siblings were added.
            pendingDiff_.childrenReorderedNodes_.insert(newRelatives.parent_);
        }
    }
    previousRelativesMap_.clear();

    // remove created and removed nodes from modified elements
    auto& modifiedElements = pendingDiff_.modifiedElements_;
    for (auto it = modifiedElements.begin(), last = modifiedElements.end(); it != last;) {
        Node* node = it->first;
        if (pendingDiff_.createdNodes_.contains(node)) {
            it = modifiedElements.erase(it);
            continue;
        }
        if (pendingDiff_.removedNodes_.contains(node)) {
            it = modifiedElements.erase(it);
            continue;
        }
        ++it;
    }
}

void Document::onHistoryHeadChanged_() {
    //emitPendingDiff();
}

void Document::onHistoryAboutToUndo_() {
    emitPendingDiff();
    pendingDiff_.isUndoOrRedo_ = true;
}

void Document::onHistoryUndone_() {
    emitPendingDiff();
    pendingDiff_.isUndoOrRedo_ = false;
}

void Document::onHistoryAboutToRedo_() {
    emitPendingDiff();
    pendingDiff_.isUndoOrRedo_ = true;
}

void Document::onHistoryRedone_() {
    emitPendingDiff();
    pendingDiff_.isUndoOrRedo_ = false;
}

void Document::onCreateNode_(Node* node) {
    pendingDiff_.createdNodes_.emplaceLast(node);
}

void Document::onRemoveNode_(Node* node) {
    if (!pendingDiff_.createdNodes_.removeOne(node)) {
        pendingDiff_.removedNodes_.emplaceLast(node);
        pendingDiffKeepAllocPointers_.emplaceLast(node);
    }
    else {
        previousRelativesMap_.erase(node);
        pendingDiff_.modifiedElements_.erase(static_cast<Element*>(node));
        pendingDiff_.reparentedNodes_.erase(node);
        pendingDiff_.childrenReorderedNodes_.erase(node);
    }
}

void Document::onMoveNode_(Node* node, const NodeRelatives& savedRelatives) {
    previousRelativesMap_.try_emplace(node, savedRelatives);
}

void Document::onChangeAttribute_(Element* element, core::StringId name) {
    pendingDiff_.modifiedElements_[element].insert(name);
}

} // namespace vgc::dom
