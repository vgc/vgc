The Document Object Model that we use here roughly
follows the spirit of the W3C DOM Specifications:

https://www.w3.org/TR/2004/REC-DOM-Level-3-Core-20040407/

However, there are a few differences:

1. In vgc::dom, attributes do not inherit the Node interface.

2. In vgc::dom, we do not have Element::tagName but simply Element::name.

3. In vgc::dom, we do not have Node::nodeName().

4. In vgc::dom, at least for now, there are no namespaces in the API.
