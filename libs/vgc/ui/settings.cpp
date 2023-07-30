// Copyright 2023 The VGC Developers
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

#include <vgc/ui/settings.h>

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

#include <vgc/ui/logcategories.h>
#include <vgc/ui/qtutil.h>

namespace vgc::ui {

namespace {

SettingsPtr createGlobalSettings(std::string_view name) {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir dir(path);
    QString filePath = dir.filePath(toQt(name));
    return Settings::create(fromQt(filePath));
}

template<typename T>
T get(const detail::SettingsMap& map, std::string_view key, const T& fallback) {
    auto it = map.find(std::string(key));
    bool found = it != map.end();
    if (found && std::holds_alternative<T>(it->second)) {
        return std::get<T>(it->second);
    }
    else {
        return fallback;
    }
}

template<typename T>
void set(detail::SettingsMap& map, std::string_view key, const T& value) {
    map[std::string(key)] = value;
}

template<typename T>
T getOrSet(detail::SettingsMap& map, std::string_view key, const T& fallback) {
    auto [it, inserted] = map.try_emplace(std::string(key), fallback);
    bool found = !inserted;
    if (found && std::holds_alternative<T>(it->second)) {
        return std::get<T>(it->second);
    }
    else {
        return fallback;
    }
}

} // namespace

namespace settings {

Settings* preferences() {
    static SettingsPtr res = createGlobalSettings("preferences.json");
    return res.get();
}

Settings* session() {
    static SettingsPtr res = createGlobalSettings("session.json");
    return res.get();
}

} // namespace settings

Settings::Settings(CreateKey key, std::string_view filePath)
    : Object(key)
    , filePath_(filePath) {

    readFromFile();
}

SettingsPtr Settings::create(std::string_view filePath) {
    return core::createObject<Settings>(filePath);
}

void Settings::clear() {
    map_.clear();
}

void Settings::readFromFile() {
    if (filePath().empty()) {
        return;
    }
    QFile file(toQt(filePath()));
    if (!file.open(QIODevice::ReadOnly)) {
        VGC_WARNING(LogVgcUi, "Could not open settings at {}.", filePath());
        return;
    }
    QByteArray data = file.readAll();
    QJsonDocument json = QJsonDocument::fromJson(data);
    QJsonObject root = json.object();
    std::string unsupportedType;
    for (QJsonObject::iterator it = root.begin(); it != root.end(); ++it) {
        std::string key = fromQt(it.key());
        QJsonValueRef value = it.value();
        switch (value.type()) {
        case QJsonValue::Bool:
            setBoolValue(key, value.toBool());
            break;
        case QJsonValue::Double:
            setDoubleValue(key, value.toDouble());
            break;
        case QJsonValue::String:
            setStringValue(key, fromQt(value.toString()));
            break;
        case QJsonValue::Null:
            unsupportedType = "Null";
            [[fallthrough]];
        case QJsonValue::Array:
            unsupportedType = "Array";
            [[fallthrough]];
        case QJsonValue::Object:
            unsupportedType = "Object";
            [[fallthrough]];
        case QJsonValue::Undefined:
            unsupportedType = "Undefined";
            VGC_WARNING(
                LogVgcUi,
                "JSON value for key '{}' is of unsupported type '{}'.",
                key,
                unsupportedType);
            break;
        }
    }
}

void Settings::writeToFile() const {
    if (filePath().empty()) {
        return;
    }
    QFileInfo fileInfo(toQt(filePath()));
    QDir dir = fileInfo.dir();
    if (!dir.mkpath(".")) {
        VGC_WARNING(LogVgcUi, "Could not create directory at {}.", fromQt(dir.path()));
        return;
    }
    QFile file(toQt(filePath()));
    if (!file.open(QIODevice::WriteOnly)) {
        VGC_WARNING(LogVgcUi, "Could not write settings at {}.", filePath());
        return;
    }

    QJsonObject root;
    for (auto& [key, value] : map_) {
        QString qkey = toQt(key);
        if (std::holds_alternative<bool>(value)) {
            root[qkey] = std::get<bool>(value);
        }
        else if (std::holds_alternative<double>(value)) {
            root[qkey] = std::get<double>(value);
        }
        else if (std::holds_alternative<std::string>(value)) {
            root[qkey] = toQt(std::get<std::string>(value));
        }
    }
    file.write(QJsonDocument(root).toJson());
}

bool Settings::contains(std::string_view key) {

    // Note: In C++20, we will be able to compare directly the strings in the
    // map with the string_view argument, without creating a temporary
    // std::string.
    //
    return map_.find(std::string(key)) != map_.end();
}

bool Settings::getBoolValue(std::string_view key, bool fallback) const {
    return get<bool>(map_, key, fallback);
}

void Settings::setBoolValue(std::string_view key, bool value) {
    set<bool>(map_, key, value);
}

bool Settings::getOrSetBoolValue(std::string_view key, bool fallback) {
    return getOrSet<bool>(map_, key, fallback);
}

double Settings::getDoubleValue(std::string_view key, double fallback) const {
    return get<double>(map_, key, fallback);
}

void Settings::setDoubleValue(std::string_view key, double value) {
    set<double>(map_, key, value);
}

double Settings::getOrSetDoubleValue(std::string_view key, double fallback) {
    return getOrSet<double>(map_, key, fallback);
}

float Settings::getFloatValue(std::string_view key, float fallback) const {
    double fallback_ = fallback;
    double value = get<double>(map_, key, fallback_);
    return static_cast<float>(value);
}

void Settings::setFloatValue(std::string_view key, float value) {
    double value_ = value;
    set<double>(map_, key, value_);
}

float Settings::getOrSetFloatValue(std::string_view key, float fallback) {
    double fallback_ = fallback;
    double value = getOrSet<double>(map_, key, fallback_);
    return static_cast<float>(value);
}

Int Settings::getIntValue(std::string_view key, Int fallback) const {
    double fallback_ = fallback;
    double value = get<double>(map_, key, fallback_);
    return static_cast<Int>(std::round(value));
}

void Settings::setIntValue(std::string_view key, Int value) {
    double value_ = value;
    set<double>(map_, key, value_);
}

Int Settings::getOrSetIntValue(std::string_view key, Int fallback) {
    double fallback_ = fallback;
    double value = getOrSet<double>(map_, key, fallback_);
    return static_cast<Int>(std::round(value));
}

std::string
Settings::getStringValue(std::string_view key, std::string_view fallback) const {
    std::string fallback_(fallback);
    return get<std::string>(map_, key, fallback_);
}

void Settings::setStringValue(std::string_view key, std::string_view value) {
    std::string value_(value);
    set<std::string>(map_, key, value_);
}

std::string
Settings::getOrSetStringFloatValue(std::string_view key, std::string_view fallback) {
    std::string fallback_(fallback);
    return getOrSet<std::string>(map_, key, fallback_);
}

void Settings::onDestroyed() {
    writeToFile();
}

} // namespace vgc::ui
