// Copyright 2022 The VGC Developers
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

#ifndef VGC_GRAPHICS_DETAIL_COMMANDS_H
#define VGC_GRAPHICS_DETAIL_COMMANDS_H

#include <string_view>
#include <type_traits>
#include <utility>

#include <vgc/core/format.h>
#include <vgc/graphics/api.h>

namespace vgc::graphics {

VGC_DECLARE_OBJECT(Engine);

} // namespace vgc::graphics

namespace vgc::graphics::detail {

// Abstract render command.
//
class VGC_GRAPHICS_API Command {
protected:
    explicit Command(std::string_view name)
        : name_(name) {}

public:
    virtual ~Command() = default;

    virtual void execute(Engine* engine) = 0;

    virtual std::string repr()  {
        return std::string(name());
    }

    std::string_view name() const {
        return name_;
    }

private:
    std::string_view name_;
};

template<typename Lambda>
class LambdaCommand : public Command, private Lambda {
public:
    LambdaCommand(std::string_view name, Lambda&& lambda)
        : Command(name)
        , Lambda(std::move(lambda)) {}

    LambdaCommand(std::string_view, const Lambda&) = delete;

    void execute(Engine* engine) override {
        Lambda::operator()(engine);
    }
};

template<typename U, typename Lambda>
LambdaCommand(U, Lambda) -> LambdaCommand<std::decay_t<Lambda>>;

template<typename Data, typename Lambda>
class LambdaCommandWithParameters : public Command, private Lambda {
public:
    template<typename... Args>
    LambdaCommandWithParameters(std::string_view name, Lambda&& lambda, Args&&... args)
        : Command(name)
        , Lambda(std::move(lambda))
        , data_{std::forward<Args>(args)...} {}

    template<typename... Args>
    LambdaCommandWithParameters(std::string_view, const Lambda&, Args&&...) = delete;

    void execute(Engine* engine) {
        Lambda::operator()(engine, data_);
    }

    const Data& data() const {
        return data_;
    }

    // XXX add special repr if data has stream op

private:
    Data data_;
};

template<typename U, typename Lambda, typename Data>
LambdaCommandWithParameters(U, Lambda, Data) -> LambdaCommandWithParameters<std::decay_t<Data>, std::decay_t<Lambda>>;

} // namespace vgc::graphics::detail

#endif // VGC_GRAPHICS_DETAIL_COMMANDS_H
