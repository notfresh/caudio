# CAudio - 专业命令行音频播放器

## 🎵 产品简介

CAudio 是一款轻量级、高性能的命令行音频播放器，专为追求效率和简洁的用户设计。

基于 C++ 开发，采用 miniaudio 音频库，支持多种主流音频格式，提供流畅的播放体验和强大的目录管理功能。

代码有点粗糙，各位酌情自行调整和编译。


## 怎么使用

下载项目，安装支持包含C++ 17以上的编译器，执行make命令，得到 caudio.exe

把目录下的 caudio.exe 加入可执行目录中即可。

windows的预编译可执行文件我已经放在Github Release上, MacOS和Linux版本暂未提供预编译文件。

喜欢折腾的可以自己手动修改/编译/玩耍。


## ✨ 核心特性

### 🎶 多格式支持
- **广泛兼容**：支持 WAV、MP3、FLAC、OGG、M4A、AAC 等主流音频格式
- **自动解码**：智能识别音频格式，无需手动配置
- **高质量播放**：保持原始音频质量，无损播放体验

### ⏯️ 灵活播放控制
- **精确跳转**：支持从指定时间点开始播放（格式：`HH:MM:SS` 或 `MM:SS`）
- **暂停/继续**：按 Enter 键随时暂停或继续播放
- **实时进度**：显示当前播放进度和总时长
- **优雅停止**：支持 Ctrl+C 安全停止播放

### 📁 智能目录管理
- **多目录管理**：添加多个音乐目录，轻松切换
- **自动扫描**：自动识别目录中的所有音频文件
- **批量播放**：一键播放目录中的所有音频文件，自动顺序播放
- **配置持久化**：目录配置自动保存，下次启动无需重新设置

### 🚀 性能优势
- **轻量高效**：单文件可执行程序，无需安装依赖
- **低资源占用**：内存占用极小，CPU 使用率低
- **快速启动**：毫秒级启动速度，即开即用
- **跨平台**：支持 Windows、Linux、macOS 等主流操作系统

## 📖 使用指南

### 基本播放

```bash
# 播放单个音频文件
caudio play song.mp3

# 从指定时间点开始播放
caudio play song.mp3 --jump 1:30
caudio play song.mp3 --jump 0:05:30
```

### 目录管理

```bash
# 添加音乐目录
caudio directory add C:\Music
caudio dir add /home/user/Music

# 列出所有已添加的目录
caudio directory list

# 选择要使用的目录（通过索引）
caudio directory select 0

# 查看目录中的音频文件
caudio directory files

# 播放目录中的所有音频文件
caudio directory play

# 从指定时间点开始播放目录中的第一个文件（directory play 只是指定第一个文件）
caudio directory play --jump 2:15

# 删除目录（通过索引）
caudio directory remove 0
```

### 快捷命令

`dir` 是 `directory` 的简写别名，可以互换使用：

```bash
caudio dir add C:\Music
caudio dir list
caudio dir select 0
caudio dir files
caudio dir play
```

### 相对路径支持

如果已选择目录，可以直接使用文件名播放：

```bash
caudio dir select 0
caudio play song.mp3  # 自动从选中的目录查找
```

## 🎮 播放控制

播放过程中：
- **Enter 键**：暂停/继续播放
- **Ctrl+C**：停止播放并退出

播放界面实时显示：
- 当前播放状态（PLAYING / PAUSED）
- 当前播放时间
- 总时长

## 💡 使用场景

### 个人音乐管理
- 管理多个音乐收藏目录
- 快速播放整张专辑或播放列表
- 精确跳转到喜欢的片段

### 开发调试
- 测试音频文件播放
- 验证音频文件完整性
- 快速预览音频内容

### 自动化脚本
- 集成到自动化工作流
- 批量处理音频文件
- 定时播放任务

### 服务器环境
- 无图形界面的服务器环境
- SSH 远程播放
- 资源受限的设备

## 🔧 技术特点

- **C++17 标准**：现代 C++ 特性，代码高效可靠
- **miniaudio 音频库**：轻量级、跨平台音频处理
- **配置持久化**：自动保存用户配置到 `caudio_config.txt`
- **错误处理**：完善的错误提示和异常处理
- **跨平台兼容**：Windows、Linux、macOS 全平台支持

## 📦 编译安装

### 编译要求
- C++17 兼容的编译器（GCC、Clang、MSVC）
- Make 工具（可选）

### 编译步骤

```bash
# 使用 Makefile 编译
make

# 或手动编译
g++ -std=c++17 -Wall -O2 caudio.cpp directory_manager.cpp -o caudio
```

### Windows 编译

```powershell
# 使用 MinGW 或 MSVC
g++ -std=c++17 -Wall -O2 caudio.cpp directory_manager.cpp -o caudio.exe
```

## 📝 配置说明

程序会自动创建 `caudio_config.txt` 文件保存配置：
- 已添加的目录列表
- 当前选中的目录索引

配置文件格式简单，可直接编辑。

## 🎯 产品优势

1. **极简设计**：命令行界面，专注核心功能，无冗余
2. **即开即用**：无需安装，下载即用
3. **资源友好**：低内存占用，适合长时间运行
4. **稳定可靠**：完善的错误处理，异常情况友好提示
5. **灵活扩展**：易于集成到其他工具和脚本中

## 📄 许可证

本项目采用MIT开源许可证，欢迎自由使用和修改。

## 🤝 支持与反馈

如有问题或建议，欢迎提交 Issue 或 Pull Request。

---

**CAudio** - 让音频播放回归简单高效 🎵

