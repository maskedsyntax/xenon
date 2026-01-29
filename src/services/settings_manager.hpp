#pragma once

#include <string>
#include <unordered_map>

namespace xenon::services {

class SettingsManager {
public:
    static SettingsManager& instance();

    void loadSettings(const std::string& configPath);
    void saveSettings(const std::string& configPath);

    std::string getString(const std::string& key, const std::string& defaultValue = "");
    int getInt(const std::string& key, int defaultValue = 0);
    bool getBool(const std::string& key, bool defaultValue = false);

    void setString(const std::string& key, const std::string& value);
    void setInt(const std::string& key, int value);
    void setBool(const std::string& key, bool value);

private:
    SettingsManager() = default;
    std::unordered_map<std::string, std::string> settings_;
};

} // namespace xenon::services
