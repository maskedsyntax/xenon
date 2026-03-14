#include "core/file_manager.hpp"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>

namespace xenon::core {

std::string FileManager::readFile(const std::string& path) {
    QFile file(QString::fromStdString(path));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw FileError("Cannot open file: " + path);
    }
    return file.readAll().toStdString();
}

void FileManager::writeFile(const std::string& path, const std::string& content) {
    QFile file(QString::fromStdString(path));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        throw FileError("Cannot open file for writing: " + path);
    }
    file.write(content.data(), static_cast<qint64>(content.size()));
}

bool FileManager::fileExists(const std::string& path) {
    QFileInfo check_file(QString::fromStdString(path));
    return check_file.exists() && check_file.isFile();
}

bool FileManager::isDirectory(const std::string& path) {
    QFileInfo check_file(QString::fromStdString(path));
    return check_file.exists() && check_file.isDir();
}

std::string FileManager::getFileName(const std::string& path) {
    return QFileInfo(QString::fromStdString(path)).fileName().toStdString();
}

std::string FileManager::getFileExtension(const std::string& path) {
    return QFileInfo(QString::fromStdString(path)).suffix().toStdString();
}

std::string FileManager::getDirectory(const std::string& path) {
    return QFileInfo(QString::fromStdString(path)).absolutePath().toStdString();
}

std::string FileManager::detectEncoding(const std::string& content) {
    // Basic detection logic remains similar or can be improved with Qt
    if (content.empty()) return "UTF-8";
    return "UTF-8"; 
}

std::string FileManager::detectLineEnding(const std::string& content) {
    if (content.find("\r\n") != std::string::npos) return "\r\n";
    if (content.find('\r') != std::string::npos) return "\r";
    return "\n";
}

} // namespace xenon::core
