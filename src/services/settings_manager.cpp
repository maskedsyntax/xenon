#include "services/settings_manager.hpp"
#include <fstream>
#include <cctype>
#include <sys/stat.h>

namespace xenon::services {

namespace {

std::string escapeJson(const std::string& s) {
    std::string result;
    result.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n";  break;
            case '\r': result += "\\r";  break;
            case '\t': result += "\\t";  break;
            default:   result += c;      break;
        }
    }
    return result;
}

std::string unescapeJson(const std::string& s) {
    std::string result;
    result.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            switch (s[i + 1]) {
                case '"':  result += '"';  ++i; break;
                case '\\': result += '\\'; ++i; break;
                case 'n':  result += '\n'; ++i; break;
                case 'r':  result += '\r'; ++i; break;
                case 't':  result += '\t'; ++i; break;
                default:   result += s[i]; break;
            }
        } else {
            result += s[i];
        }
    }
    return result;
}

void mkdirRecursive(const std::string& path) {
    std::string current;
    for (size_t i = 0; i < path.size(); ++i) {
        current += path[i];
        if (path[i] == '/' && !current.empty()) {
            mkdir(current.c_str(), 0755);
        }
    }
    if (!current.empty()) {
        mkdir(current.c_str(), 0755);
    }
}

} // anonymous namespace

SettingsManager& SettingsManager::instance() {
    static SettingsManager manager;
    return manager;
}

void SettingsManager::loadSettings(const std::string& configPath) {
    std::ifstream file(configPath);
    if (!file.is_open()) {
        return; // No config file yet, use defaults
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();

    // Simple JSON object parser for {"key": "value", ...}
    settings_.clear();
    size_t pos = content.find('{');
    if (pos == std::string::npos) return;
    pos++;

    while (pos < content.size()) {
        // Skip whitespace
        while (pos < content.size() && std::isspace(static_cast<unsigned char>(content[pos]))) pos++;
        if (pos >= content.size() || content[pos] == '}') break;

        // Expect opening quote for key
        if (content[pos] != '"') break;
        pos++;

        // Read key
        std::string key;
        while (pos < content.size() && content[pos] != '"') {
            if (content[pos] == '\\' && pos + 1 < content.size()) {
                key += content[pos];
                key += content[pos + 1];
                pos += 2;
            } else {
                key += content[pos];
                pos++;
            }
        }
        if (pos >= content.size()) break;
        pos++; // skip closing quote

        key = unescapeJson(key);

        // Skip whitespace and colon
        while (pos < content.size() && (std::isspace(static_cast<unsigned char>(content[pos])) || content[pos] == ':')) pos++;

        // Expect opening quote for value
        if (pos >= content.size() || content[pos] != '"') break;
        pos++;

        // Read value
        std::string value;
        while (pos < content.size() && content[pos] != '"') {
            if (content[pos] == '\\' && pos + 1 < content.size()) {
                value += content[pos];
                value += content[pos + 1];
                pos += 2;
            } else {
                value += content[pos];
                pos++;
            }
        }
        if (pos >= content.size()) break;
        pos++; // skip closing quote

        value = unescapeJson(value);

        settings_[key] = value;

        // Skip whitespace and comma
        while (pos < content.size() && (std::isspace(static_cast<unsigned char>(content[pos])) || content[pos] == ',')) pos++;
    }
}

void SettingsManager::saveSettings(const std::string& configPath) {
    // Ensure parent directory exists
    auto lastSlash = configPath.rfind('/');
    if (lastSlash != std::string::npos) {
        mkdirRecursive(configPath.substr(0, lastSlash));
    }

    std::ofstream file(configPath);
    if (!file.is_open()) {
        return;
    }

    file << "{\n";
    size_t count = 0;
    for (const auto& [key, value] : settings_) {
        file << "  \"" << escapeJson(key) << "\": \"" << escapeJson(value) << "\"";
        if (++count < settings_.size()) {
            file << ",";
        }
        file << "\n";
    }
    file << "}\n";
    file.close();
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
