#pragma once

#include <string>
#include <stdexcept>

namespace xenon::core {

class FileError : public std::runtime_error {
public:
    explicit FileError(const std::string& message)
        : std::runtime_error(message) {
    }
};

class FileManager {
public:
    static std::string readFile(const std::string& path);
    static void writeFile(const std::string& path, const std::string& content);
    static bool fileExists(const std::string& path);
    static bool isDirectory(const std::string& path);
    static std::string getFileName(const std::string& path);
    static std::string getFileExtension(const std::string& path);
    static std::string getDirectory(const std::string& path);

    static std::string detectEncoding(const std::string& content);
    static std::string detectLineEnding(const std::string& content);

private:
    FileManager() = default;
};

} // namespace xenon::core
