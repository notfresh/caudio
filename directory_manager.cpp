#include "directory_manager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define access _access
#define F_OK 0
#else
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#endif

DirectoryManager::DirectoryManager() : current_index_(-1) {
    loadConfig(); // 启动时自动加载配置
}

DirectoryManager::~DirectoryManager() {
    saveConfig(); // 退出时自动保存配置
}

bool DirectoryManager::isValidDirectory(const std::string& path) const {
#ifdef _WIN32
    DWORD dwAttrib = GetFileAttributesA(path.c_str());
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && 
            (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat info;
    if (stat(path.c_str(), &info) != 0) return false;
    return S_ISDIR(info.st_mode);
#endif
}

bool DirectoryManager::isAudioFile(const std::string& filename) const {
    std::string lower = filename;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    auto endsWith = [](const std::string& str, const std::string& suffix) {
        if (suffix.length() > str.length()) return false;
        return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
    };
    
    return endsWith(lower, ".wav") || 
           endsWith(lower, ".mp3") || 
           endsWith(lower, ".flac") ||
           endsWith(lower, ".ogg") ||
           endsWith(lower, ".m4a") ||
           endsWith(lower, ".aac");
}

std::vector<std::string> DirectoryManager::scanAudioFiles(const std::string& dir) const {
    std::vector<std::string> files;
    
#ifdef _WIN32
    std::string pattern = dir + "\\*";
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(pattern.c_str(), &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                std::string filename = findData.cFileName;
                if (isAudioFile(filename)) {
                    files.push_back(dir + "\\" + filename);
                }
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
#else
    DIR* dp = opendir(dir.c_str());
    if (dp != nullptr) {
        struct dirent* entry;
        while ((entry = readdir(dp)) != nullptr) {
            std::string filename = entry->d_name;
            if (filename != "." && filename != "..") {
                std::string fullpath = dir + "/" + filename;
                struct stat info;
                if (stat(fullpath.c_str(), &info) == 0 && S_ISREG(info.st_mode)) {
                    if (isAudioFile(filename)) {
                        files.push_back(fullpath);
                    }
                }
            }
        }
        closedir(dp);
    }
#endif
    
    std::sort(files.begin(), files.end());
    return files;
}

bool DirectoryManager::addDirectory(const std::string& path) {
    if (!isValidDirectory(path)) {
        std::cerr << "Error: Invalid directory: " << path << "\n";
        return false;
    }
    
    // 检查是否已存在
    for (const auto& dir : directories_) {
        if (dir == path) {
            std::cerr << "Directory already exists in list.\n";
            return false;
        }
    }
    
    directories_.push_back(path);
    std::cout << "Added directory: " << path << "\n";
    return true;
}

bool DirectoryManager::removeDirectory(int index) {
    if (index < 0 || index >= (int)directories_.size()) {
        std::cerr << "Error: Invalid index " << index << "\n";
        return false;
    }
    
    std::cout << "Removed directory: " << directories_[index] << "\n";
    directories_.erase(directories_.begin() + index);
    
    // 如果删除的是当前选中的目录，重置选中状态
    if (current_index_ == index) {
        current_index_ = -1;
    } else if (current_index_ > index) {
        current_index_--; // 调整索引
    }
    
    return true;
}

void DirectoryManager::listDirectories() const {
    if (directories_.empty()) {
        std::cout << "No directories in list.\n";
        return;
    }
    
    std::cout << "Directories:\n";
    for (size_t i = 0; i < directories_.size(); ++i) {
        std::string marker = (i == current_index_) ? " [SELECTED]" : "";
        std::cout << "  " << i << ". " << directories_[i] << marker << "\n";
    }
}

bool DirectoryManager::selectDirectory(int index) {
    if (index < 0 || index >= (int)directories_.size()) {
        std::cerr << "Error: Invalid index " << index << "\n";
        return false;
    }
    
    current_index_ = index;
    std::cout << "Selected directory: " << directories_[index] << "\n";
    
    // 显示该目录下的音频文件数量
    auto files = getAudioFiles();
    std::cout << "Found " << files.size() << " audio file(s).\n";
    
    return true;
}

std::string DirectoryManager::getCurrentDirectory() const {
    if (current_index_ < 0 || current_index_ >= (int)directories_.size()) {
        return "";
    }
    return directories_[current_index_];
}

std::vector<std::string> DirectoryManager::getAudioFiles() const {
    std::string dir = getCurrentDirectory();
    if (dir.empty()) {
        return {};
    }
    return scanAudioFiles(dir);
}

bool DirectoryManager::saveConfig(const std::string& config_file) const {
    std::ofstream file(config_file);
    if (!file.is_open()) {
        return false;
    }
    
    file << current_index_ << "\n";
    file << directories_.size() << "\n";
    for (const auto& dir : directories_) {
        file << dir << "\n";
    }
    
    return true;
}

bool DirectoryManager::loadConfig(const std::string& config_file) {
    std::ifstream file(config_file);
    if (!file.is_open()) {
        return false;
    }
    
    int count;
    file >> current_index_;
    file >> count;
    file.ignore(); // 跳过换行符
    
    directories_.clear();
    for (int i = 0; i < count; ++i) {
        std::string dir;
        std::getline(file, dir);
        if (!dir.empty() && isValidDirectory(dir)) {
            directories_.push_back(dir);
        }
    }
    
    // 验证当前索引是否有效
    if (current_index_ >= (int)directories_.size()) {
        current_index_ = -1;
    }
    
    return true;
}

