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

#ifndef VGC_DOM_XMLFORMATTINGSTYLE_H
#define VGC_DOM_XMLFORMATTINGSTYLE_H

#include <vgc/dom/api.h>

namespace vgc::dom {

/// \enum vgc::dom::XmlIndentStyle
/// \brief Whether tabs or spaces are used for indentation.
///
/// This enum class specifies whether tabs or spaces are used for indentation
/// of an XML file. The two possible values are XmlIndentStyle::Tabs and
/// XmlIndentStyle::Spaces.
///
/// \sa XmlFormattingStyle
///
/// Note that this is only meant for indentation purposes, not alignment.
/// Currently, this is irrelevant because we do not support any formatting
/// style requiring alignment, but in the future, spaces may still be used for
/// alignement even when indentStyle = Tabs. For example:
///
/// \code
/// <a>
/// ---><b>
/// --->---><c attr1="value1"
/// --->--->...attr2="value2"/>
/// ---></b>
/// </c>
/// \endcode
///
enum class XmlIndentStyle {
    Spaces, ///< Use spaces for indentation.
    Tabs    ///< Use tabs for indentation.
};

/// \class vgc::dom::XmlFormattingStyle
/// \brief Specifies how XML documents should be formatted (indentation, etc.)
///
/// This struct-like class allows you to specifies how output XML files are
/// formatted. For example, indentStyle specifies whether tabs or spaces are
/// used for indentation, and indentSize specifies how many of them are used
/// for each indent level. Finally, attributeIndentSize specifies how many tabs
/// or spaces are used to indent attributes.
///
/// Currently, attributes are always placed on a separate line, like this:
///
/// \code
/// <sometag
/// ....attr1="value1"
/// ....attr2="value2">
/// ..<childtag1/>
/// ..<childtag2/>
/// </sometag>
/// \endcode
///
/// However, in the future, we are planning to extend this class in order to
/// allow having attributes on a single line, like this:
///
/// \code
/// <sometag attr1="value1" attr2="value2">
/// ..<childtag1/>
/// ..<childtag2/>
/// </sometag>
/// \endcode
///
// Note: here is a good resource for inspiration for when to extend this class:
// https://www.jetbrains.com/help/resharper/EditorConfig_XML_XmlCodeStylePageSchema.html
//
struct VGC_DOM_API XmlFormattingStyle {
    /// Whether tabs or spaces are used for indentation of XML nodes. The
    /// default value is XmlIndentStyle::Spaces.
    ///
    /// Example with indentStyle = XmlIndentStyle::Spaces and indentSize = 2:
    ///
    /// \code
    /// <a>
    /// ..<b>
    /// ..</b>
    /// ..<b>
    /// ....<c/>
    /// ..</b>
    /// </a>
    /// \endcode
    ///
    /// Example with indentStyle = XmlIndentStyle::Tabs and indentSize = 1:
    ///
    /// \code
    /// <a>
    /// ---><b>
    /// ---></b>
    /// ---><b>
    /// --->---><c/>
    /// ---></b>
    /// </a>
    /// \endcode
    ///
    /// \sa indentSize
    ///
    XmlIndentStyle indentStyle = XmlIndentStyle::Spaces;

    /// Number of tabs or spaces used for each indent level of XML nodes. The
    /// default value is 2.
    ///
    /// \sa indentStyle
    ///
    int indentSize = 2;

    /// Number of tabs or spaces used for indenting attributes. The default
    /// value is 4.
    ///
    /// It is useful to have different values for indentSize and
    /// attributeIndentSize in order to make it easier for humans to recognize
    /// at a glance attributes versus child elements.
    ///
    /// Example with indentSize = 2 and attributeIndentSize = 2:
    ///
    /// \code
    /// <sometag
    /// ..attr1="value1"
    /// ..attr2="value2">
    /// ..<childtag1/>
    /// ..<childtag2/>
    /// </sometag>
    /// \endcode
    ///
    /// Example with indentSize = 2 and attributeIndentSize = 4:
    ///
    /// \code
    /// <sometag
    /// ....attr1="value1"
    /// ....attr2="value2">
    /// ..<childtag1/>
    /// ..<childtag2/>
    /// </sometag>
    /// \endcode
    ///
    int attributeIndentSize = 4;
};

} // namespace vgc::dom

#endif // VGC_DOM_XMLFORMATTINGSTYLE_H
