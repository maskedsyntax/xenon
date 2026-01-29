#include "services/settings_manager.hpp"

namespace xenon::services {

SettingsManager& SettingsManager::instance() {
    static SettingsManager manager;
    return manager;
}

void SettingsManager::loadSettings(const std::string& /* configPath */) {
    // TODO: Load from JSON file
}

void SettingsManager::saveSettings(const std::string& /* configPath */) {
    // TODO: Save to JSON file
}

std::string SettingsManager::getString(const std::string& key, const std::string& defaultValue) {
    auto it = settings_.find(key);
    return it != settings_.end() ? it->second : defaultValue;
}

int SettingsManager::getInt(const std::string& key, int defaultValue) {
    auto it = settings_.find(key);
    if (it != settings_.end()) {
        try {
            return std::stoi(it->second);
        } catch (...) {
            return defaultValue;
        }
    }
    return defaultValue;
}

bool SettingsManager::getBool(const std::string& key, bool defaultValue) {
    auto it = settings_.find(key);
    if (it != settings_.end()) {
        return it->second == "true" || it->second == "1";
    }
    return defaultValue;
}

void SettingsManager::setString(const std::string& key, const std::string& value) {
    settings_[key] = value;
}

void SettingsManager::setInt(const std::string& key, int value) {
    settings_[key] = std::to_string(value);
}

void SettingsManager::setBool(const std::string& key, bool value) {
    settings_[key] = value ? "true" : "false";
}

} // namespace xenon::services
