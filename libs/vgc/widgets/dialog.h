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

#ifndef VGC_WIDGETS_DIALOG_H
#define VGC_WIDGETS_DIALOG_H

#include <QDialog>
#include <vgc/widgets/api.h>

namespace vgc::widgets {

/// \class vgc::widgets::Dialog
/// \brief Subclass of QDialog specialized for VGC software
///
/// Unlike with QDialog, if you invoke the show() function after hiding a
/// vgc::widgets::Dialog, the position of the dialog will be preserve, even
/// after being moved by the user.
///
class VGC_WIDGETS_API Dialog : public QDialog {
private:
    Q_OBJECT

public:
    /// Creates a ColorDialog.
    ///
    Dialog(QWidget* parent = nullptr);

protected:
    // We reimplement these to preserve dialog position on hide/show
    void closeEvent(QCloseEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void showEvent(QShowEvent* event) override;

private Q_SLOTS:
    void onFinished_(int result);

private:
    void saveGeometry_();
    void restoreGeometry_();
    bool isGeometrySaved_;
    QRect savedGeometry_;
};

} // namespace vgc::widgets

#endif // VGC_WIDGETS_DIALOG_H
