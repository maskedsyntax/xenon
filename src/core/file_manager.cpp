#include "core/file_manager.hpp"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <algorithm>

namespace fs = std::filesystem;

namespace xenon::core {

std::string FileManager::readFile(const std::string& path) {
    try {
        if (!fs::exists(path)) {
            throw FileError("File not found: " + path);
        }

        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            throw FileError("Cannot open file: " + path);
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    } catch (const fs::filesystem_error& e) {
        throw FileError(std::string(e.what()));
    }
}

void FileManager::writeFile(const std::string& path, const std::string& content) {
    try {
        std::ofstream file(path, std::ios::binary);
        if (!file.is_open()) {
            throw FileError("Cannot open file for writing: " + path);
        }

        file.write(content.data(), content.size());
        file.close();

        if (!file.good()) {
            throw FileError("Failed to write file: " + path);
        }
    } catch (const fs::filesystem_error& e) {
        throw FileError(std::string(e.what()));
    }
}

bool FileManager::fileExists(const std::string& path) {
    return fs::exists(path) && fs::is_regular_file(path);
}

bool FileManager::isDirectory(const std::string& path) {
    return fs::exists(path) && fs::is_directory(path);
}

std::string FileManager::getFileName(const std::string& path) {
    return fs::path(path).filename().string();
}

std::string FileManager::getFileExtension(const std::string& path) {
    std::string ext = fs::path(path).extension().string();
    if (!ext.empty() && ext[0] == '.') {
        ext = ext.substr(1);
    }
    return ext;
}

std::string FileManager::getDirectory(const std::string& path) {
    return fs::path(path).parent_path().string();
}

std::string FileManager::detectEncoding(const std::string& content) {
    if (content.empty()) {
        return "UTF-8";
    }

    if (content.size() >= 3 &&
        static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF) {
        return "UTF-8-BOM";
    }

    if (content.size() >= 2 &&
        static_cast<unsigned char>(content[0]) == 0xFE &&
        static_cast<unsigned char>(content[1]) == 0xFF) {
        return "UTF-16-BE";
    }

    if (content.size() >= 2 &&
        static_cast<unsigned char>(content[0]) == 0xFF &&
        static_cast<unsigned char>(content[1]) == 0xFE) {
        return "UTF-16-LE";
    }

    return "UTF-8";
}

std::string FileManager::detectLineEnding(const std::string& content) {
    const size_t crlfCount = std::count(content.begin(), content.end(), '\r');
    const size_t lfCount = std::count(content.begin(), content.end(), '\n');

    if (crlfCount > 0 && lfCount > 0) {
        return "\r\n";
    } else if (crlfCount > 0) {
        return "\r";
    }
    return "\n";
}

} // namespace xenon::core
