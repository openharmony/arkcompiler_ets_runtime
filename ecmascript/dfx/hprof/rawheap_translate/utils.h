/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef RAWHEAP_TRANSLATE_UTILS_H
#define RAWHEAP_TRANSLATE_UTILS_H

#include <iostream>
#include <iomanip>
#include <functional>
#include <cstdint>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <securec.h>
#include <sstream>

namespace rawheap_translate {
#define LOG_INFO_ Logger(0) << std::left << std::setw(24) << __func__
#define LOG_ERROR_ Logger(1) << std::left << std::setw(24) << __func__
#ifdef PATH_MAX
#undef PATH_MAX
#endif
#define PATH_MAX 4096
#define MAX_FILE_SIZE (4 * 1024 * 1024 * 1024ULL) // 4 * 1024 * 1024 * 1024 : file size bigger than 4GB
#define MAX_OBJ_SIZE (MAX_FILE_SIZE >> 1)

bool RealPath(const std::string &filename, std::string &realpath);

bool GenerateDumpFileName(std::string &filename);

bool EndsWith(const std::string &str, const std::string &suffix);

bool IsLittleEndian();

uint16_t ByteToU16(char *data);

uint32_t ByteToU32(char *data);

uint64_t ByteToU64(char *data);

void ByteToU32Array(char *data, uint32_t *array, uint32_t size);

void ByteToU64Array(char *data, uint64_t *array, uint32_t size);

class Logger {
public:
    Logger(int level) : level_(level) {}
    ~Logger()
    {
        switch (level_) {
            case 1:
                // 8: level width format
                std::cout << std::left << std::setw(8) << "[ERROR] " << ss.str() << std::endl;
                break;
            default:
                // 8: level width format
                std::cout << std::left << std::setw(8) << "[INFO ] " << ss.str() << std::endl;
                break;
        }
    }

    template<typename T>
    Logger& operator<<(const T& value)
    {
        ss << value;
        return *this;
    }

private:
    int level_ {0};
    std::stringstream ss;
};

class FileReader {
public:
    FileReader() = default;
    ~FileReader() = default;

    bool Initialize(const std::string &path);
    bool Read(char *buf, uint32_t size);
    bool Seek(uint32_t offset);
    bool ReadArray(std::vector<uint32_t> &array, uint32_t size);
    bool ReadArray(std::vector<uint64_t> &array, uint32_t size);
    bool CheckAndGetHeaderAt(uint32_t offset, uint32_t assertNum);

    uint32_t GetHeaderLeft()
    {
        return left_;
    }

    uint32_t GetHeaderRight()
    {
        return right_;
    }

    uint32_t GetFileSize()
    {
        return fileSize_;
    }

    static uint32_t GetFileSize(const std::string &path);

private:
    std::ifstream file_;
    uint32_t left_ {0};
    uint32_t right_ {0};
    uint32_t fileSize_ {0};
};

class Version {
public:
    Version(int major, int minor, int build) : major_(major), minor_(minor), build_(build) {}
    Version() = default;
    ~Version() = default;

    bool Parse(const std::string &version);

    bool operator<(const Version &version) const
    {
        if (major_ > version.major_) {
            return false;
        }

        if (major_ == version.major_ && minor_ > version.minor_) {
            return false;
        }

        if (major_ == version.major_ && minor_ == version.minor_ && build_ >= version.build_) {
            return false;
        }

        return true;
    }

    std::string ToString() const
    {
        return std::to_string(major_) + '.' + std::to_string(minor_) + '.' + std::to_string(build_);
    }

private:
    int major_ {0};
    int minor_ {0};
    int build_ {0};
};
}  // namespace rawheap_translate
#endif
