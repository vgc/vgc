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

#include <vgc/widgets/performancemonitor.h>

#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <vgc/core/stringutil.h>
#include <vgc/widgets/qtutil.h>

namespace vgc {
namespace widgets {

PerformanceMonitor::PerformanceMonitor(QWidget* parent) :
    QWidget(parent)
{
    QGridLayout* layout = new QGridLayout();

    renderingTime_ = new QLabel("N/A");
    layout->addWidget(new QLabel("Rendering: "), 0, 0);
    layout->addWidget(renderingTime_, 0, 1);

    // Set layout. We first wrap the grid layout in a vbox layout to
    // add strecthing below the grid, to make it top-aligned
    QVBoxLayout* wrapper = new QVBoxLayout();
    wrapper->addLayout(layout);
    wrapper->addStretch();
    setLayout(wrapper);

    // Set minimum width
    setMinimumWidth(200);
}

PerformanceMonitor::~PerformanceMonitor()
{

}

void PerformanceMonitor::setRenderingTime(double t)
{
    const auto unit = core::TimeUnit::Milliseconds;
    const int decimals = 2;

    renderingTime_->setText(toQt(core::secondsToString(t, unit, decimals)));
}

} // namespace widgets
} // namespace vgc
