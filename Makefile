# Makefile for caudio project

# 编译器
CXX = g++

# 编译选项
CXXFLAGS = -std=c++17 -Wall -O2

# 目标文件
TARGET = caudio

# 源文件
SOURCES = caudio.cpp directory_manager.cpp

# 对象文件
OBJECTS = $(SOURCES:.cpp=.o)

# 默认目标
all: $(TARGET)

# 链接目标文件生成可执行文件
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET)

# 编译源文件为目标文件
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理生成的文件
clean:
	rm -f $(OBJECTS) $(TARGET) $(TARGET).exe

# Windows 下的清理
clean-win:
	del /Q $(OBJECTS) $(TARGET).exe 2>nul || true

# 重新编译
rebuild: clean all

# 帮助信息
help:
	@echo "Makefile for caudio project"
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build the project (default)"
	@echo "  clean    - Remove object files and executable (Unix)"
	@echo "  clean-win - Remove object files and executable (Windows)"
	@echo "  rebuild  - Clean and rebuild"
	@echo "  help     - Show this help message"

.PHONY: all clean clean-win rebuild help

