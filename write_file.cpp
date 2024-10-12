#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

void createFile(const std::string& filePath, size_t fileSizeGB) {
    // 计算文件大小（字节）
    size_t fileSize = fileSizeGB * 1024 * 1024 * 1024;   // GB 转换为字节

    // 创建目录（如果不存在）
    std::filesystem::path path(filePath);
    std::filesystem::create_directories(path.parent_path());

    // 创建并打开文件
    std::ofstream file(filePath, std::ios::binary);
    if (!file) {
        std::cerr << "无法创建文件: " << filePath << std::endl;
        return;
    }

    // 写入指定大小的文件
    file.seekp(fileSize - 1);   // 定位到文件末尾
    file.put('\0');             // 写入一个字节以扩展文件大小

    file.close();   // 关闭文件
    std::cout << "文件已创建: " << filePath << "，大小: " << fileSizeGB << " GB" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "用法: " << argv[0] << " <文件路径> <文件大小(GB)>" << std::endl;
        return 1;
    }

    std::string filePath = argv[1];
    size_t fileSizeGB = std::stoul(argv[2]);   // 将输入的大小转换为无符号长整型

    createFile(filePath, fileSizeGB);

    return 0;
}
