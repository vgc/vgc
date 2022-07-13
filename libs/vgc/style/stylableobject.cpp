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

#include <vgc/style/stylableobject.h>

namespace vgc::style {

StylableObject::StylableObject() {
}

void StylableObject::addStyleClass(core::StringId class_) {
    styleClasses_.add(class_);
    onStyleClassesChanged_();
}

void StylableObject::removeStyleClass(core::StringId class_) {
    styleClasses_.remove(class_);
    onStyleClassesChanged_();
}

void StylableObject::toggleStyleClass(core::StringId class_) {
    styleClasses_.toggle(class_);
    onStyleClassesChanged_();
}

StyleValue StylableObject::style(core::StringId property) const {
    return style_.computedValue(property, this);
}

void StylableObject::onStyleClassesChanged_() {
    style_ = styleSheet()->computeStyle(this);
    // TODO: notify the object of the change of style?
    // (e.g., so that it can be re-rendered)
}

} // namespace vgc::style
