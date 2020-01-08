// Copyright 2020 The VGC Developers
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

#ifndef VGC_CORE_RANDOM_H
#define VGC_CORE_RANDOM_H

#include <random>
#include <vgc/core/api.h>

namespace vgc {
namespace core {

class VGC_CORE_API UniformDistribution {
public:
    UniformDistribution(double min, double max);
    double operator()();

private:
    std::default_random_engine e_;
    std::uniform_real_distribution<double> d_;
};

} // namespace core
} // namespace vgc

#endif // VGC_CORE_RANDOM_H
