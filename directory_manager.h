#ifndef DIRECTORY_MANAGER_H
#define DIRECTORY_MANAGER_H

#include <string>
#include <vector>

class DirectoryManager {
public:
    DirectoryManager();
    ~DirectoryManager();

    // 添加目录到列表
    bool addDirectory(const std::string& path);
    
    // 从列表移除目录
    bool removeDirectory(int index);
    
    // 列出所有目录
    void listDirectories() const;
    
    // 选择目录（返回索引）
    bool selectDirectory(int index);
    
    // 获取当前选中的目录
    std::string getCurrentDirectory() const;
    
    // 获取当前选中目录的所有音频文件
    std::vector<std::string> getAudioFiles() const;
    
    // 获取目录列表
    std::vector<std::string> getDirectories() const { return directories_; }
    
    // 获取当前选中的索引
    int getCurrentIndex() const { return current_index_; }
    
    // 保存配置到文件
    bool saveConfig(const std::string& config_file = "caudio_config.txt") const;
    
    // 从文件加载配置
    bool loadConfig(const std::string& config_file = "caudio_config.txt");

private:
    std::vector<std::string> directories_;
    int current_index_;  // -1 表示未选中
    
    // 检查路径是否存在且为目录
    bool isValidDirectory(const std::string& path) const;
    
    // 获取目录中的所有音频文件
    std::vector<std::string> scanAudioFiles(const std::string& dir) const;
    
    // 检查文件是否为音频文件
    bool isAudioFile(const std::string& filename) const;
};

#endif // DIRECTORY_MANAGER_H

