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

#ifndef VGC_WIDGETS_PANELTITLEBAR_H
#define VGC_WIDGETS_PANELTITLEBAR_H

#include <QFrame>
#include <QLabel>

#include <vgc/widgets/api.h>

namespace vgc::widgets {

/// \class vgc::widgets::PanelTitleBar
/// \brief The title bar on top of each Panel.
///
class VGC_WIDGETS_API PanelTitleBar : public QFrame {
private:
    Q_OBJECT

public:
    /// Constructs a PanelTitleBar.
    ///
    explicit PanelTitleBar(const QString& title, QWidget* parent = nullptr);

    /// Destructs the PanelTitleBar.
    ///
    ~PanelTitleBar() override;

private:
    QLabel* title_;
};

} // namespace vgc::widgets

#endif // VGC_WIDGETS_PANELTITLEBAR_H
