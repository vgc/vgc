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
#include <vgc/dom/logcategories.h>
#include <vgc/dom/operation.h>
#include <vgc/dom/schema.h>
#include <vgc/dom/strings.h>

namespace vgc::dom {

using core::XmlSyntaxError;

Document::Document(CreateKey key)
    : Node(key, ProtectedKey{}, this, NodeType::Document)
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
    Document* document_ = nullptr;
    Node* currentNode_ = nullptr;
    const ElementSpec* elementSpec_ = nullptr;
    std::string tagName_;
    std::string attributeName_;
    std::string attributeValue_;
    std::string referenceName_;

    // Create the parser object
    Parser(std::string_view filePath, Document* document)
        : document_(document)
        , currentNode_(document) {

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

        Value value = {};
        if (name == strings::name) {
            value = core::StringId();
        }
        else if (name == strings::id) {
            value = core::StringId();
        }
        else {
            const AttributeSpec* spec = elementSpec_->findAttributeSpec(name);
            if (!spec) {
                throw VgcSyntaxError(
                    "Unknown attribute '" + attributeName_ + "' for element '" + tagName_
                    + "'. Expected an attribute name defined in the VGC schema.");
            }
            value = spec->defaultValue();
        }

        parseValue(value, attributeValue_);
        Element::cast(currentNode_)->setAttribute(name, std::move(value));
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

/* static */
DocumentPtr Document::copy(core::ConstSpan<Node*> nodes) {

    static_assert(core::isRange<core::Span<Node*>>);

    struct NodeAndDepth {
        NodeAndDepth(Node* node)
            : node(node)
            , depth(node->depth()) {
        }
        Node* node;
        Int depth;
    };

    core::Array<NodeAndDepth> copyNodes(nodes);

    // Sort by depth from low (root) to high (leaf).
    std::sort(
        copyNodes.begin(),
        copyNodes.end(),
        [](const NodeAndDepth& a, const NodeAndDepth& b) { return a.depth < b.depth; });

    // Keep only topmost nodes.
    // Thanks to the ordering, a node can never be visited before its parent.

    for (auto it1 = copyNodes.begin(); it1 != copyNodes.end(); ++it1) {
        NodeAndDepth& n1 = *it1;
        for (auto it2 = copyNodes.begin(); it2 != it1; ++it2) {
            NodeAndDepth& n2 = *it2;
            if (n1.node->isDescendantOf(n2.node)) {
                // n2 will be copied with n1 copy
                it1 = copyNodes.erase(it1);
                break;
            }
        }
    }

    if (copyNodes.isEmpty()) {
        return nullptr;
    }

    Node* n0 = copyNodes.first().node;

    // Fix order to match original document order
    Int i = 0;
    n0->document()->numberNodesDepthFirstRec_(n0->document(), i);
    std::sort(
        copyNodes.begin(),
        copyNodes.end(),
        [](const NodeAndDepth& a, const NodeAndDepth& b) {
            return a.node->temporaryIndex_ < b.node->temporaryIndex_;
        });

    DocumentPtr result = Document::create();

    // Copy in order
    Document* srcDoc = n0->document();
    Document* tgtDoc = result.get();
    PathUpdateData pud = {};

    Node* copyContainer = Element::create(tgtDoc, "copyContainer");

    srcDoc->preparePathsUpdateRec_(srcDoc);

    for (const NodeAndDepth& n1 : copyNodes) {
        // TODO: copy other node types than Element.
        Element* element = Element::cast(n1.node);
        if (element) {
            Element::createCopy_(copyContainer, element, nullptr, pud);
        }
    }

    tgtDoc->updatePathsRec_(copyContainer, pud);

    return result;
}

/* static */
core::Array<Node*> Document::paste(DocumentPtr document, Node* parent) {

    core::Array<Node*> res;

    // Copy in order
    Document* sourceDoc = document.get();
    Document* targetDoc = parent->document();
    PathUpdateData pathUpdateData = {};

    if (!sourceDoc || sourceDoc == targetDoc) {
        return res;
    }

    Node* copyContainer = sourceDoc->rootElement();
    if (!copyContainer) {
        return res;
    }

    Node* rootElement = targetDoc->rootElement();

    sourceDoc->preparePathsUpdateRec_(copyContainer);

    DocumentPtr result = Document::create();
    for (Node* n1 : copyContainer->children()) {
        // TODO: copy other node types than Element.
        Element* element = Element::cast(n1);
        if (element) {
            Element* pastedElement =
                Element::createCopy_(rootElement, element, nullptr, pathUpdateData);
            res.append(pastedElement);
        }
    }

    targetDoc->updatePathsRec_(rootElement, pathUpdateData);

    return res;
}

Element* Document::elementFromId(core::StringId id) const {
    auto it = elementByIdMap_.find(id);
    if (it != elementByIdMap_.end()) {
        return it->second;
    }
    return nullptr;
}

Element* Document::elementFromInternalId(core::Id id) const {
    auto it = elementByInternalIdMap_.find(id);
    if (it != elementByInternalIdMap_.end()) {
        return it->second;
    }
    return nullptr;
}

// static
Element* Document::elementFromPath(
    const Path& path,
    const Node* workingNode,
    core::StringId tagNameFilter) {

    const core::Array<PathSegment>& segments = path.segments();
    auto segIt = segments.begin();
    auto segEnd = segments.end();

    return resolveElementPartOfPath_(path, segIt, segEnd, workingNode, tagNameFilter);
}

// static
Value Document::valueFromPath(
    const Path& path,
    const Node* workingNode,
    core::StringId tagNameFilter) {

    const core::Array<PathSegment>& segments = path.segments();
    auto segIt = segments.begin();
    auto segEnd = segments.end();

    Element* element =
        resolveElementPartOfPath_(path, segIt, segEnd, workingNode, tagNameFilter);
    if (!element) {
        return Value();
    }

    return resolveAttributePartOfPath_(path, segIt, segEnd, element);
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

void Document::onElementIdChanged_(Element* element, core::StringId oldId) {
    if (!oldId.isEmpty()) {
        elementByIdMap_.erase(oldId);
    }
    core::StringId id = element->id();
    if (!id.isEmpty()) {
        // in case of conflict, the current element becomes the one searchable.
        elementByIdMap_[id] = element;
    }
    // TODO: use dependents list to do the path update instead of
    // iterating on all elements.
    if (!oldId.isEmpty()) {
        for (const auto& [eId, e] : elementByInternalIdMap_) {
            std::ignore = eId;
            detail::prepareInternalPathsForUpdate(e);
        }
        PathUpdateData pud;
        pud.addAbsolutePathChangedElement(element->internalId());
        for (const auto& [eId, e] : elementByInternalIdMap_) {
            std::ignore = eId;
            detail::updateInternalPaths(e, pud);
        }
    }
}

void Document::onElementNameChanged_(Element* /*element*/) {
    // TODO: path update when paths use names
}

void Document::onElementAboutToBeDestroyed_(Element* element) {
    core::StringId id = element->id();
    if (!id.isEmpty()) {
        elementByIdMap_.erase(id);
    }
    elementByInternalIdMap_.erase(element->internalId());
}

void Document::numberNodesDepthFirstRec_(Node* node, Int& i) {
    node->temporaryIndex_ = i++;
    for (Node* c : node->children()) {
        numberNodesDepthFirstRec_(c, i);
    }
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
    for (Node* child : node->children()) {
        onCreateNode_(child);
    }
}

void Document::onRemoveNode_(Node* node) {
    for (Node* child : node->children()) {
        onRemoveNode_(child);
    }
    if (!pendingDiff_.createdNodes_.removeOne(node)) {
        pendingDiff_.removedNodes_.emplaceLast(node);
        pendingDiffKeepAllocPointers_.emplaceLast(node);
    }
    else {
        previousRelativesMap_.erase(node);
        pendingDiff_.removeModifiedElementAttributeRecords(static_cast<Element*>(node));
        pendingDiff_.reparentedNodes_.erase(node);
        pendingDiff_.childrenReorderedNodes_.erase(node);
    }
}

void Document::onMoveNode_(Node* node, const NodeRelatives& savedRelatives) {
    previousRelativesMap_.try_emplace(node, savedRelatives);
}

void Document::onChangeAttribute_(Element* element, core::StringId name) {
    pendingDiff_.insertModifiedElementAttributeRecord(element, name);
}

namespace {

Element* firstChildElementWithName(const Element* element, core::StringId name) {
    Element* childElement = element->firstChildElement();
    while (childElement) {
        if (childElement->name() == name) {
            break;
        }
        childElement = childElement->nextSiblingElement();
    }
    return childElement; // nullptr if not found
}

} // namespace

// static
Element* Document::resolveElementPartOfPath_(
    const Path& path,
    Path::ConstSegmentIterator& segIt,
    Path::ConstSegmentIterator segEnd,
    const Node* workingNode,
    core::StringId tagNameFilter) {

    Document* document = workingNode->document();

    Element* element = nullptr;
    bool hasError = false;
    bool firstAttributeEncountered = false;
    for (; segIt != segEnd; ++segIt) {
        switch (segIt->type()) {
        case PathSegmentType::Root:
            // only first segment can be root
            element = document->rootElement();
            break;
        case PathSegmentType::Id:
            // only first segment can be root
            // nullptr if not found, handled out of switch
            element = document->elementFromId(segIt->nameOrId());
            break;
        case PathSegmentType::Element:
            if (!element) {
                element = Element::cast(const_cast<Node*>(workingNode));
            }
            if (element) {
                // nullptr if not found, handled out of switch
                element = firstChildElementWithName(element, segIt->nameOrId());
            }
            break;
        case PathSegmentType::Attribute:
            if (!element) {
                element = Element::cast(const_cast<Node*>(workingNode));
            }
            firstAttributeEncountered = true;
            break;
        }
        // Break out of loop if there is an error or we reached the end of
        // the "element part" of the path.
        if (!element /* <- error */) {
            hasError = true;
            break;
        }
        else if (firstAttributeEncountered) {
            break;
        }
    }
    if (hasError) {
        for (; segIt != segEnd; ++segIt) {
            if (segIt->type() == PathSegmentType::Attribute) {
                break;
            }
        }
        return nullptr;
    }

    if (element && !tagNameFilter.isEmpty() && element->tagName() != tagNameFilter) {
        VGC_WARNING(
            LogVgcDom,
            "Path `{}` resolved to an element `{}` but `{}` was expected.",
            path,
            element->tagName(),
            tagNameFilter);
        element = nullptr;
    }

    return element;
}

// static
Value Document::resolveAttributePartOfPath_(
    const Path& /*path*/,
    Path::ConstSegmentIterator& segIt,
    Path::ConstSegmentIterator segEnd,
    const Element* element) {

    Value result = {};
    for (; segIt != segEnd; ++segIt) {
        switch (segIt->type()) {
        case PathSegmentType::Root:
        case PathSegmentType::Id:
        case PathSegmentType::Element: {
            // error, no elements should be in the attribute part
            return Value();
        }
        case PathSegmentType::Attribute: {
            result = element->getAttribute(segIt->nameOrId());
            if (result.isValid() && segIt->isIndexed()) {
                result = result.getItemWrapped(segIt->arrayIndex());
            }
            break;
        }
        }
    }
    return result;
}

void Document::preparePathsUpdateRec_(const Node* node) {
    detail::prepareInternalPathsForUpdate(node);
    for (Node* c : node->children()) {
        preparePathsUpdateRec_(c);
    }
}

void Document::updatePathsRec_(const Node* node, const PathUpdateData& pud) {
    detail::updateInternalPaths(node, pud);
    for (Node* c : node->children()) {
        updatePathsRec_(c, pud);
    }
}

} // namespace vgc::dom
