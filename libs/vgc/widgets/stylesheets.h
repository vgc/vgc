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

#ifndef VGC_WIDGETS_STYLESHEETS_H
#define VGC_WIDGETS_STYLESHEETS_H

#include <string>
#include <vgc/widgets/api.h>

namespace vgc::widgets {

/// Sets the application stylesheet from a *.qss file specified by its resource
/// \p name.
///
/// This loads the stylesheet from file, replaces every occurence of "vgc:/" by
/// the absolute path of the VGC resources, then calls
/// QApplication::setStyleSheet().
///
/// Example:
/// \code
/// setApplicationStyleSheet("widgets/stylesheets/dark.qss")
/// \endcode
///
VGC_WIDGETS_API
void setApplicationStyleSheet(const std::string& name);

} // namespace vgc::widgets

#endif // VGC_WIDGETS_STYLESHEETS_H
