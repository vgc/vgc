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

#ifndef VGC_CORE_PATHS_H
#define VGC_CORE_PATHS_H

#include <string>
#include <vgc/core/api.h>

namespace vgc::core {

/// Sets the absolute path of the base directory of this VGC installation. This
/// must be called before any other call to VGC functions, so that they can
/// locate their runtime resources, if any.
///
/// Example:
/// \code
/// int main(int argc, char* argv[])
/// {
///     QCoreApplication app(argc, argv);
///
///     // Set base path by removing "/bin" from the application dir path
///     QString binPath = QCoreApplication::applicationDirPath();
///     QString basePath = binPath.chopped(4);
///     vgc::core::setBasePath(basePath.toStdString());
///
///     [...]
/// }
/// \endcode
///
/// \sa basePath()
///
VGC_CORE_API
void setBasePath(const std::string& path);

/// Returns the absolute path of the base directory of this VGC installation.
///
/// VGC installations have the following structure:
///
/// \verbatim
/// <base>/bin        Contains VGC executables       (example: <base>/bin/vgcillustration)
/// <base>/lib        Contains VGC shared libraries  (example: <base>/lib/libvgccore.so)
/// <base>/python     Contains VGC python bindings   (example: <base>/python/vgc/core.cpython-36m-x86_64-linux-gnu.so)
/// <base>/resources  Contains VGC runtime resources (example: <base>/resources/graphics/opengl/shader.f.glsl)
/// \endverbatim
///
VGC_CORE_API
std::string basePath();

/// Returns the absolute path where VGC python bindings are located
///
VGC_CORE_API
std::string pythonPath();

/// Returns the absolute path where VGC runtime resources are located.
///
VGC_CORE_API
std::string resourcesPath();

/// Returns the absolute path of the runtime resource specified by its \p name.
///
/// Example:
/// \code
/// std::string vertPath = core::resourcePath("graphics/opengl/shader.v.glsl");
/// std::string fragPath = core::resourcePath("graphics/opengl/shader.f.glsl");
/// \endcode
///
VGC_CORE_API
std::string resourcePath(const std::string& name);

} // namespace vgc::core

#endif // VGC_CORE_PATHS_H
