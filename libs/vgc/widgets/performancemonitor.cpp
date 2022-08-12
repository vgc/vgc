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

#include <vgc/widgets/performancemonitor.h>

#include <QLabel>

#include <vgc/core/arithmetic.h>
#include <vgc/core/format.h>
#include <vgc/ui/qtutil.h>

namespace vgc::widgets {

PerformanceMonitor::PerformanceMonitor(QWidget* parent)
    : QWidget(parent)
    , log_(core::PerformanceLog::create()) {

    // Grid layout for displaying the logs' values
    layout_ = new QGridLayout();
    layout_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    layout_->setHorizontalSpacing(30);
    setLayout(layout_);
}

PerformanceMonitor::~PerformanceMonitor() {
}

namespace {

// Add some indentation to the log's name, and convert to a QString.
QString captionText_(core::PerformanceLog* log, int indentLevel) {
    QString res;
    for (int i = 0; i < indentLevel; ++i) {
        res += "  ";
    }
    res += ui::toQt(log->name());
    return res;
}

} // namespace

void PerformanceMonitor::refresh() {
    const auto unit = core::TimeUnit::Milliseconds;
    const int decimals = 2;

    // Iterative depth-first traversal using a stack of (depth, log) pairs
    using TraversalData = std::pair<int, core::PerformanceLog*>;
    int index = 0;
    size_t index_ = 0;
    std::vector<TraversalData> stack;
    stack.push_back(std::make_pair(0, log_.get()));
    while (!stack.empty()) {
        // Pop last log in the stack
        auto pair = stack.back();
        stack.pop_back();
        int depth = pair.first;
        core::PerformanceLog* log = pair.second;

        // Display the log (unless it's the root; we don't display the root)
        if (log != log_.get()) {

            QString valueText =
                ui::toQt(core::secondsToString(log->lastTime(), unit, decimals));

            // If there was already something displayed for this index,
            // update the text of the QLabels
            if (index_ < displayedLogs_.size()) {

                // Update the "value" QLabel
                displayedLogs_[index_].value->setText(valueText);

                // If the previously displayed entry was a different entry,
                // also update the "caption" QLabel
                if (displayedLogs_[index_].log != log) {
                    displayedLogs_[index_].log = log;
                    displayedLogs_[index_].caption->setText(captionText_(log, depth));
                }

                // Make sure the labels are visible (they may have been hidden
                // if unused, see the end of this function)
                displayedLogs_[index_].caption->show();
                displayedLogs_[index_].value->show();
            }
            // If there was no QLabels already for this index, create them
            else {
                DisplayedLog_ d;
                d.log = log;
                d.caption = new QLabel(captionText_(log, depth));
                d.value = new QLabel(valueText);
                displayedLogs_.push_back(d);

                layout_->addWidget(d.caption, index, 0);
                layout_->addWidget(d.value, index, 1);
            }

            // Increment index
            ++index;
            ++index_;

            // Stop recursion if there are more elements to display
            // than layout_->addWidget() supports. Unlikely.
            if (index == (std::numeric_limits<int>::max)()) {
                break;
            }
        }

        // Recurse on children
        for (core::PerformanceLog* child = log->lastChild(); //
             child != nullptr;                               //
             child = child->previousSibling()) {
            stack.push_back(std::make_pair(depth + 1, child));
        }
    }

    // Hide the remaining existing QLabels, if any
    for (size_t i = index_; i < displayedLogs_.size(); ++i) {
        displayedLogs_[i].log = nullptr;
        displayedLogs_[i].caption->hide();
        displayedLogs_[i].value->hide();
    }
}

core::PerformanceLog* PerformanceMonitor::log() const {
    return log_.get();
}

} // namespace vgc::widgets
