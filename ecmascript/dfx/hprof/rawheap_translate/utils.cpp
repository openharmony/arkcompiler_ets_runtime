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

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <ctime>
#include <sstream>
#include "ecmascript/dfx/hprof/rawheap_translate/utils.h"

namespace rawheap_translate {
bool CheckVersion(const std::string &version)
{
    std::vector<int> result {};
    std::stringstream ss(version);
    std::string token;
    while (getline(ss, token, '.')) {
        int number;
        std::istringstream(token) >> number;
        result.push_back(number);
    }
    
    // 3: means the version format is x.x.x
    if (result.size() != 3) {
        LOG_ERROR("current version " + version + " is illegal!");
        return false;
    }

    bool majorVersionCheckFalse = result[MAJOR_VERSION_INDEX] > VERSION[MAJOR_VERSION_INDEX];
    bool minorVersionCheckFalse = result[MAJOR_VERSION_INDEX] == VERSION[MAJOR_VERSION_INDEX] &&
                                  result[MINOR_VERSION_INDEX] > VERSION[MINOR_VERSION_INDEX];
    bool buildVersionCheckFalse = result[MAJOR_VERSION_INDEX] == VERSION[MAJOR_VERSION_INDEX] &&
                                  result[MINOR_VERSION_INDEX] == VERSION[MINOR_VERSION_INDEX] &&
                                  result[BUILD_VERSION_INDEX] > VERSION[BUILD_VERSION_INDEX];
    if (majorVersionCheckFalse || minorVersionCheckFalse || buildVersionCheckFalse) {
        std::ostringstream oss;
        oss << "The rawheap file's version " << version;
        oss << " is not matched the current rawheap translator, please use the newest version of the translator!";
        LOG_ERROR(oss.str());
        return false;
    };
    return true;
}

bool RealPath(const std::string &filename, std::string &realpath)
{
    if (filename.empty() || filename.size() > PATH_MAX) {
        LOG_ERROR("filename is illegal!");
        return false;
    }

#ifdef _WIN32
    if (_access(filename.c_str(), 0) < 0) {
#else
    if (access(filename.c_str(), 0) < 0) {
#endif
        LOG_ERROR("file can not access! filePath=" + filename);
        return false;
    }

    char resolvedPath[PATH_MAX];
    if (strcpy_s(resolvedPath, PATH_MAX, filename.c_str()) != 0) {
        return false;
    }

    realpath = resolvedPath;
    return true;
}

uint64_t GetFileSize(std::string &inputFilePath)
{
    if (inputFilePath.empty()) {
        return 0;
    }
    struct stat fileInfo;
    if (stat(inputFilePath.c_str(), &fileInfo) == 0) {
        return fileInfo.st_size;
    }
    return 0;
}

bool FileCheckAndOpenBinary(const std::string &rawheapPath, std::ifstream &file, uint32_t &fileSize)
{
    std::string realpath {};
    if (!RealPath(rawheapPath, realpath)) {
        LOG_ERROR("file path is illegal! filePath: " + rawheapPath);
        return false;
    }

    uint64_t size = GetFileSize(realpath);
    if (size == 0 || size >= MAX_FILE_SIZE) {
        LOG_ERROR("file size >= 4GB or size = 0, unsupported!");
        return false;
    }

    fileSize = static_cast<uint32_t>(size);
    file.open(realpath, std::ios::binary);
    return true;
}

void GenerateDumpFileName(std::string &filename)
{
    filename = "hprof_";
    std::time_t t = std::time(nullptr);
    struct tm *now = localtime(&t);

    const int timeStart = 1900;  // 1900: means the start of timestamp
    filename += std::to_string(now->tm_year + timeStart);
    filename += '-' + std::to_string(now->tm_mon + 1);
    filename += '-' + std::to_string(now->tm_mday);
    filename += '-' + std::to_string(now->tm_hour);
    filename += '-' + std::to_string(now->tm_min);
    filename += '-' + std::to_string(now->tm_sec);
    filename += ".heapsnapshot";
}

bool EndsWith(const std::string &str, const std::string &suffix)
{
    if (str.length() < suffix.length()) {
        return false;
    }
    std::string subStr = str.substr(str.length() - suffix.length(), str.length());
    return subStr == suffix;
}

bool IsLittleEndian()
{
    int i = 1;
    return *reinterpret_cast<char *>(&i) == 1;
}

uint32_t ByteToU32(char *data)
{
    uint32_t value = *reinterpret_cast<uint32_t *>(data);
    if (!IsLittleEndian()) {
        value = (value & 0x000000FF) << 24 |
                (value & 0x0000FF00) << 8 |
                (value & 0x00FF0000) >> 8 |
                (value & 0xFF000000) >> 24;
    }
    return value;
}

uint64_t ByteToU64(char *data)
{
    uint64_t value = *reinterpret_cast<uint64_t *>(data);
    if (!IsLittleEndian()) {
        value = (value & 0x00000000000000FF) << 56 |
                (value & 0x000000000000FF00) << 40 |
                (value & 0x0000000000FF0000) << 24 |
                (value & 0x00000000FF000000) << 8 |
                (value & 0x000000FF00000000) >> 8 |
                (value & 0x0000FF00000000) >> 24 |
                (value & 0x00FF0000000000) >> 40 |
                (value & 0xFF000000000000) >> 56;
    }
    return value;
}

void ByteToU32Array(char *data, uint32_t *array, uint32_t size)
{
    char *num = data;
    for (uint32_t i = 0; i < size; i++) {
        array[i] = ByteToU32(num);
        num += sizeof(uint32_t);
    }
}

void ByteToU64Array(char *data, uint64_t *array, uint32_t size)
{
    char *num = data;
    for (uint32_t i = 0; i < size; i++) {
        array[i] = ByteToU64(num);
        num += sizeof(uint64_t);
    }
}
} // namespace rawheap_translate