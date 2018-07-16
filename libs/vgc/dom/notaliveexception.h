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

#ifndef VGC_DOM_NOTALIVEEXCEPTION_H
#define VGC_DOM_NOTALIVEEXCEPTION_H

#include <exception>
#include <string>
#include <vgc/dom/api.h>

namespace vgc {
namespace dom {

class Node;

/// \class vgc.dom.NotAliveException
/// \brief Exception thrown when accessing a node which is not alive.
///
/// This exception is thrown by most member methods of Node when `this` node is
/// not "alive", that is, if it has already been destroyed.
///
/// More precisely, the lifetime of a Node goes through three stages:
///
/// 1. Node::create() has been called: the node is "alive".
///
/// 2. Node::destroy() has been called: the node is not "alive" anymore, but all
///    pointers to the node are still valid non-dangling pointers. Dereferencing
///    the pointer is safe, and calling node->isAlive() is safe, but calling
///    most other member methods will cause NotAliveException to be thrown.
///
/// 3. Node::~Node() has been called: all raw pointers to the node are now
///    dangling pointers, and all weak pointers to the node are now expired.
///
class VGC_DOM_API NotAliveException : public std::exception {
    std::string what_;

public:
    NotAliveException(Node* node);
    const char* what() const throw() {
      return what_.c_str();
    }
};

} // namespace dom
} // namespace vgc

#endif // VGC_DOM_NOTALIVEEXCEPTION_H
