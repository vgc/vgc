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

#ifndef VGC_CORE_RANDOM_H
#define VGC_CORE_RANDOM_H

#include <random>

#include <vgc/core/api.h>
#include <vgc/core/arithmetic.h>

namespace vgc::core {

namespace detail {

template<typename T, typename SFINAE = void>
struct UniformDistribution_;

template<typename T>
struct UniformDistribution_<T, Requires<std::is_integral_v<T>>> {
    using type = std::uniform_int_distribution<T>;
};

template<typename T>
struct UniformDistribution_<T, Requires<std::is_floating_point_v<T>>> {
    using type = std::uniform_real_distribution<T>;
};

template<typename T>
using UniformDistribution = typename UniformDistribution_<T>::type;

// Generates a non-deterministic uniformly-distributed random value
// in the range `(std::random_device::min)()` and `(std::random_device::max)()`.
//
// This simply calls operator() on a thread-local instance of std::random_device.
//
VGC_CORE_API
UInt32 generateRandomInteger();

} // namespace detail

template<typename T>
class PseudoRandomUniform {
public:
    /// Creates a pseudo-random number generator over a uniform distribution,
    /// initialized with with a non-deterministic hardware-generated random
    /// seed (if available).
    ///
    PseudoRandomUniform(T min, T max)
        : engine_(detail::generateRandomInteger())
        , distribution_(min, max) {
    }

    /// Creates a pseudo-random number generator over a uniform distribution,
    /// initialized with the given seed.
    ///
    PseudoRandomUniform(T min, T max, UInt32 seed)
        : engine_(seed)
        , distribution_(min, max) {
    }

    /// Initializes the pseudo-random engine with the given seed.
    ///
    void seed(UInt32 value) {
        engine_.seed(value);
    }

    /// Generates a pseudo-random number.
    ///
    T operator()() {
        return distribution_(engine_);
    }

private:
    std::mt19937 engine_;
    detail::UniformDistribution<T> distribution_;
};

} // namespace vgc::core

#endif // VGC_CORE_RANDOM_H
