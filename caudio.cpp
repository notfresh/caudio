// caudio.cpp
#define MINIAUDIO_IMPLEMENTATION
#include "third-party/miniaudio.h"
#include "directory_manager.h"

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <chrono>
#include <thread>
#include <cstdio>
#include <csignal>
#include <cstring>
#include <fstream>

#ifdef _WIN32
#include <conio.h>  // for _kbhit (optional)
#else
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#endif

bool g_stop = false;
bool g_paused = false;  // 暂停状态

void signal_handler(int sig) {
    g_stop = true;
}

// 播放状态结构
struct PlaybackState {
    ma_decoder* decoder;
    ma_uint64 current_frame;
    bool paused;
};

// 解析时间字符串 "MM:SS" 或 "HH:MM:SS" → 秒数
double parse_time(const std::string& time_str) {
    std::vector<int> parts;
    std::stringstream ss(time_str);
    std::string part;
    while (std::getline(ss, part, ':')) {
        parts.push_back(std::stoi(part));
    }

    if (parts.size() == 2) {
        return parts[0] * 60 + parts[1];           // MM:SS
    } else if (parts.size() == 3) {
        return parts[0] * 3600 + parts[1] * 60 + parts[2]; // HH:MM:SS
    } else if (parts.size() == 1) {
        return parts[0];                           // SS
    }
    return 0.0;
}

// 格式化时间显示
std::string format_time(double seconds) {
    int h = (int)seconds / 3600;
    int m = ((int)seconds % 3600) / 60;
    int s = (int)seconds % 60;
    
    if (h > 0) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d:%02d:%02d", h, m, s);
        return buf;
    } else {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d:%02d", m, s);
        return buf;
    }
}

// 简易非阻塞键盘检测（用于 Enter 暂停/继续）
bool check_keyboard() {
#ifdef _WIN32
    return _kbhit() != 0;
#else
    termios term;
    tcgetattr(0, &term);
    termios term2 = term;
    term2.c_lflag &= ~ICANON;
    tcsetattr(0, TCSANOW, &term2);
    int byteswaiting;
    ioctl(0, FIONREAD, &byteswaiting);
    tcsetattr(0, TCSANOW, &term);
    return byteswaiting > 0;
#endif
}

// 播放音频文件
int play_audio(const std::string& audio_file, double jump_seconds = 0.0) {
    g_stop = false;
    
    // 检查文件是否存在
    std::ifstream file_check(audio_file);
    if (!file_check.good()) {
        std::cerr << "Error: File not found or cannot be accessed: " << audio_file << "\n";
        std::cerr << "  Please check:\n";
        std::cerr << "  - File path is correct\n";
        std::cerr << "  - File exists\n";
        std::cerr << "  - You have read permission\n";
        return 1;
    }
    file_check.close();
    
    // 初始化解码器
    ma_decoder decoder;
    ma_result result = ma_decoder_init_file(audio_file.c_str(), nullptr, &decoder);
    if (result != MA_SUCCESS) {
        const char* error_desc = ma_result_description(result);
        std::cerr << "Failed to open audio file: " << audio_file << "\n";
        std::cerr << "  Error code: " << result << "\n";
        std::cerr << "  Error description: " << (error_desc ? error_desc : "Unknown error") << "\n";
        std::cerr << "  Possible reasons:\n";
        std::cerr << "  - File format not supported (miniaudio supports: WAV, MP3, FLAC, OGG, M4A, AAC)\n";
        std::cerr << "  - File is corrupted\n";
        std::cerr << "  - File is not a valid audio file\n";
        std::cerr << "  - Codec not available (may need additional libraries)\n";
        return 1;
    }

    // 计算跳转帧数
    ma_uint64 total_frames;
    ma_result length_result = ma_decoder_get_length_in_pcm_frames(&decoder, &total_frames);
    if (length_result != MA_SUCCESS) {
        const char* error_desc = ma_result_description(length_result);
        std::cerr << "Failed to get audio length.\n";
        std::cerr << "  Error code: " << length_result << "\n";
        std::cerr << "  Error description: " << (error_desc ? error_desc : "Unknown error") << "\n";
        ma_decoder_uninit(&decoder);
        return 1;
    }
    double duration_sec = total_frames / (double)decoder.outputSampleRate;
    ma_uint64 jump_frames = (ma_uint64)(jump_seconds * decoder.outputSampleRate);

    if (jump_frames >= total_frames) {
        std::cerr << "Jump time exceeds audio duration (" << format_time(duration_sec) << ")\n";
        ma_decoder_uninit(&decoder);
        return 1;
    }

    // 跳转
    if (jump_frames > 0) {
        ma_decoder_seek_to_pcm_frame(&decoder, jump_frames);
    }
    
    // 显示播放信息
    std::string filename = audio_file;
    size_t pos = filename.find_last_of("/\\");
    if (pos != std::string::npos) {
        filename = filename.substr(pos + 1);
    }
    
    std::cout << "\n========================================\n";
    std::cout << "Playing: " << filename << "\n";
    if (jump_seconds > 0) {
        std::cout << "From: " << format_time(jump_seconds) << "\n";
    }
    std::cout << "Duration: " << format_time(duration_sec) << "\n";
    std::cout << "Press Enter to pause/resume, Ctrl+C to stop.\n";
    std::cout << "========================================\n";

    // 播放状态
    PlaybackState playback_state;
    playback_state.decoder = &decoder;
    playback_state.current_frame = jump_frames;
    playback_state.paused = false;
    g_paused = false;

    // 设置播放设备
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = decoder.outputFormat;
    config.playback.channels = decoder.outputChannels;
    config.sampleRate        = decoder.outputSampleRate;
    config.dataCallback      = [](ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
        PlaybackState* state = (PlaybackState*)pDevice->pUserData;
        if (state->paused) {
            // 暂停时填充静音
            memset(pOutput, 0, frameCount * ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels));
        } else {
            // 正常播放
            ma_decoder_read_pcm_frames(state->decoder, pOutput, frameCount, nullptr);
            state->current_frame += frameCount;
        }
    };
    config.pUserData = &playback_state;

    ma_device device;
    result = ma_device_init(nullptr, &config, &device);
    if (result != MA_SUCCESS) {
        const char* error_desc = ma_result_description(result);
        std::cerr << "Failed to open playback device.\n";
        std::cerr << "  Error code: " << result << "\n";
        std::cerr << "  Error description: " << (error_desc ? error_desc : "Unknown error") << "\n";
        std::cerr << "  Possible reasons:\n";
        std::cerr << "  - No audio output device available\n";
        std::cerr << "  - Audio device is in use by another application\n";
        std::cerr << "  - Audio driver issue\n";
        ma_decoder_uninit(&decoder);
        return 1;
    }

    signal(SIGINT, signal_handler); // Ctrl+C 也能停

    ma_device_start(&device);

    // 播放循环：显示进度 + 检测 Enter（暂停/继续）
    while (!g_stop && ma_device_is_started(&device)) {
        if (check_keyboard()) {
            char ch = getchar();
            if (ch == '\n' || ch == '\r') {
                // 切换暂停状态
                playback_state.paused = !playback_state.paused;
                g_paused = playback_state.paused;
                
                if (playback_state.paused) {
                    std::cout << "\n[PAUSED] ";
                } else {
                    std::cout << "\n[PLAYING] ";
                }
                fflush(stdout);
            }
        }

        // 显示进度（每0.5秒更新一次）
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // 显示进度
        double current_sec = playback_state.current_frame / (double)decoder.outputSampleRate;
        if (current_sec > duration_sec) break;

        // 打印进度（清行重写）
        std::string status = playback_state.paused ? "[PAUSED]" : "[PLAYING]";
        printf("\r%s [%s / %s]", status.c_str(), format_time(current_sec).c_str(), format_time(duration_sec).c_str());
        fflush(stdout);
    }

    ma_device_uninit(&device);
    ma_decoder_uninit(&decoder);

    std::cout << "\n\nPlayback stopped.\n";
    return 0;
}

// 显示帮助信息
void show_help(const char* program_name) {
    std::cout << "Usage:\n";
    std::cout << "  " << program_name << " play <audio_file> [--jump HH:MM:SS]\n";
    std::cout << "  " << program_name << " directory|dir add <path>\n";
    std::cout << "  " << program_name << " directory|dir remove <index>\n";
    std::cout << "  " << program_name << " directory|dir list\n";
    std::cout << "  " << program_name << " directory|dir select <index>\n";
    std::cout << "  " << program_name << " directory|dir files\n";
    std::cout << "  " << program_name << " directory|dir play [--jump HH:MM:SS]\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << program_name << " play song.wav\n";
    std::cout << "  " << program_name << " play song.wav --jump 1:30\n";
    std::cout << "  " << program_name << " dir add C:\\Music\n";
    std::cout << "  " << program_name << " dir list\n";
    std::cout << "  " << program_name << " dir select 0\n";
    std::cout << "  " << program_name << " dir files\n";
    std::cout << "  " << program_name << " dir play\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        show_help(argv[0]);
        return 1;
    }

    std::string command = argv[1];

    // dir 是 directory 的简写别名
    if (command == "dir") {
        command = "directory";
    }

    // 处理 directory 命令
    if (command == "directory") {
        if (argc < 3) {
            std::cerr << "Error: directory command requires subcommand.\n";
            show_help(argv[0]);
            return 1;
        }

        DirectoryManager manager;
        std::string subcmd = argv[2];

        if (subcmd == "add") {
            if (argc < 4) {
                std::cerr << "Error: directory add requires a path.\n";
                return 1;
            }
            return manager.addDirectory(argv[3]) ? 0 : 1;
        }
        else if (subcmd == "remove") {
            if (argc < 4) {
                std::cerr << "Error: directory remove requires an index.\n";
                return 1;
            }
            int index = std::stoi(argv[3]);
            return manager.removeDirectory(index) ? 0 : 1;
        }
        else if (subcmd == "list") {
            manager.listDirectories();
            return 0;
        }
        else if (subcmd == "select") {
            if (argc < 4) {
                std::cerr << "Error: directory select requires an index.\n";
                return 1;
            }
            int index = std::stoi(argv[3]);
            return manager.selectDirectory(index) ? 0 : 1;
        }
        else if (subcmd == "files") {
            std::string current_dir = manager.getCurrentDirectory();
            if (current_dir.empty()) {
                std::cerr << "Error: No directory selected. Use 'directory select <index>' first.\n";
                return 1;
            }
            
            auto files = manager.getAudioFiles();
            if (files.empty()) {
                std::cout << "No audio files found in: " << current_dir << "\n";
                return 0;
            }
            
            std::cout << "Audio files in: " << current_dir << "\n";
            std::cout << "Total: " << files.size() << " file(s)\n\n";
            
            for (size_t i = 0; i < files.size(); ++i) {
                // 提取文件名（去掉路径）
                std::string filename = files[i];
                size_t pos = filename.find_last_of("/\\");
                if (pos != std::string::npos) {
                    filename = filename.substr(pos + 1);
                }
                
                std::cout << "  " << (i + 1) << ". " << filename << "\n";
            }
            return 0;
        }
        else if (subcmd == "play") {
            std::string current_dir = manager.getCurrentDirectory();
            if (current_dir.empty()) {
                std::cerr << "Error: No directory selected. Use 'directory select <index>' first.\n";
                return 1;
            }

            auto files = manager.getAudioFiles();
            if (files.empty()) {
                std::cerr << "Error: No audio files found in selected directory.\n";
                return 1;
            }

            double jump_seconds = 0.0;
            // 解析 --jump 参数
            for (int i = 3; i < argc; ++i) {
                if (std::string(argv[i]) == "--jump" && i + 1 < argc) {
                    jump_seconds = parse_time(argv[i + 1]);
                    break;
                }
            }

            // 播放列表中的所有文件
            std::cout << "Playing " << files.size() << " file(s) from: " << current_dir << "\n";
            for (size_t i = 0; i < files.size(); ++i) {
                std::cout << "\n[" << (i + 1) << "/" << files.size() << "] ";
                int result = play_audio(files[i], (i == 0) ? jump_seconds : 0.0);
                if (result != 0 || g_stop) {
                    break;
                }
                g_stop = false; // 重置停止标志
            }
            return 0;
        }
        else {
            std::cerr << "Error: Unknown directory subcommand: " << subcmd << "\n";
            show_help(argv[0]);
            return 1;
        }
    }
    // 处理 play 命令
    else if (command == "play") {
        if (argc < 3) {
            std::cerr << "Error: play command requires an audio file.\n";
            show_help(argv[0]);
            return 1;
        }

        std::string audio_file = argv[2];
        double jump_seconds = 0.0;

        // 解析 --jump 参数
        for (int i = 3; i < argc; ++i) {
            if (std::string(argv[i]) == "--jump" && i + 1 < argc) {
                jump_seconds = parse_time(argv[i + 1]);
                break;
            }
        }

        // 检查是否是相对路径（不包含路径分隔符）
        bool is_relative_path = (audio_file.find('/') == std::string::npos && 
                                 audio_file.find('\\') == std::string::npos);
        
        // 如果是相对路径，尝试拼接当前选中的目录
        if (is_relative_path) {
            DirectoryManager manager;
            std::string current_dir = manager.getCurrentDirectory();
            if (!current_dir.empty()) {
                // 拼接完整路径
#ifdef _WIN32
                audio_file = current_dir + "\\" + audio_file;
#else
                audio_file = current_dir + "/" + audio_file;
#endif
            }
        }

        return play_audio(audio_file, jump_seconds);
    }
    else {
        std::cerr << "Error: Unknown command: " << command << "\n";
        show_help(argv[0]);
        return 1;
    }
}
