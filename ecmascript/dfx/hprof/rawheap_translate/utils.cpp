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
bool RealPath(const std::string &filename, std::string &realpath)
{
    if (filename.empty() || filename.size() > PATH_MAX) {
        LOG_ERROR_ << "filename is illegal!";
        return false;
    }

#ifdef _WIN32
    if (_access(filename.c_str(), 0) < 0) {
#else
    if (access(filename.c_str(), 0) < 0) {
#endif
        LOG_ERROR_ << "file can not access! filePath=" << filename;
        return false;
    }

    char resolvedPath[PATH_MAX];
    if (strcpy_s(resolvedPath, PATH_MAX, filename.c_str()) != 0) {
        return false;
    }

    realpath = resolvedPath;
    return true;
}

bool FileCheckAndOpenBinary(const std::string &rawheapPath, std::ifstream &file, uint32_t &fileSize)
{
    std::string realpath {};
    if (!RealPath(rawheapPath, realpath)) {
        LOG_ERROR_ << "file path is illegal! filePath: " << rawheapPath;
        return false;
    }

    uint64_t size = FileReader::GetFileSize(realpath);
    if (size == 0 || size >= MAX_FILE_SIZE) {
        LOG_ERROR_ << "file size >= 4GB or size = 0, unsupported!";
        return false;
    }

    fileSize = static_cast<uint32_t>(size);
    file.open(realpath, std::ios::binary);
    return true;
}

bool GenerateDumpFileName(std::string &filename)
{
    std::time_t t = std::time(nullptr);
    struct tm *now = localtime(&t);
    if (now == nullptr) {
        LOG_ERROR_ << "failed to converting time to local time!";
        return false;
    }

    filename = "hprof_";
    const int timeStart = 1900;  // 1900: means the start of timestamp
    filename += std::to_string(now->tm_year + timeStart);
    filename += '-' + std::to_string(now->tm_mon + 1);
    filename += '-' + std::to_string(now->tm_mday);
    filename += '-' + std::to_string(now->tm_hour);
    filename += '-' + std::to_string(now->tm_min);
    filename += '-' + std::to_string(now->tm_sec);
    filename += ".heapsnapshot";
    return true;
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

uint16_t ByteToU16(char *data)
{
    uint32_t value = *reinterpret_cast<uint16_t *>(data);
    if (!IsLittleEndian()) {
        value = (value & 0x00FF) << 8 |
                (value & 0xFF00) >> 8;
    }
    return value;
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

bool FileReader::Initialize(const std::string &path)
{
    std::string realPath;
    if (!RealPath(path, realPath)) {
        return false;
    }

    file_.open(realPath, std::ios::binary);
    fileSize_= GetFileSize(realPath);
    return true;
}

bool FileReader::Read(char *buf, uint32_t size)
{
    if (buf == nullptr) {
        LOG_ERROR_ << "file buf is nullptr!";
        return false;
    }
    if (file_.read(buf, size).fail()) {
        LOG_ERROR_ << "read failed!";
        return false;
    }
    return true;
}

bool FileReader::Seek(uint32_t offset)
{
    if (!file_.is_open()) {
        LOG_ERROR_ << "file not open!";
        return false;
    }
    file_.clear();
    if (!file_.seekg(offset)) {
        LOG_ERROR_ << "set file offset failed, offset=" << offset;
        return false;
    }
    return true;
}

bool FileReader::ReadArray(std::vector<uint32_t> &array, uint32_t size)
{
    uint32_t dataSize = size * sizeof(uint32_t);
    std::vector<char> data(dataSize);
    if (!Read(data.data(), dataSize)) {
        return false;
    }
    ByteToU32Array(data.data(), array.data(), size);
    return true;
}

bool FileReader::ReadArray(std::vector<uint64_t> &array, uint32_t size)
{
    uint32_t dataSize = size * sizeof(uint64_t);
    std::vector<char> data(dataSize);
    if (!Read(data.data(), dataSize)) {
        return false;
    }
    ByteToU64Array(data.data(), array.data(), size);
    return true;
}

bool FileReader::CheckAndGetHeaderAt(uint32_t offset, uint32_t assertNum)
{
    constexpr int HEADER_SIZE = sizeof(uint64_t) / sizeof(uint32_t);
    std::vector<uint32_t> header(HEADER_SIZE);
    if (!Seek(offset) || !ReadArray(header, HEADER_SIZE)) {
        return false;
    }

    uint32_t first = header[0];
    uint32_t second = header[1];
    if (assertNum != 0 && second != assertNum) {
        return false;
    }

    left_ = first;
    right_ = second;
    return true;
}

uint32_t FileReader::GetFileSize(const std::string &path)
{
    if (path.empty()) {
        return 0;
    }
    struct stat fileInfo;
    if (stat(path.c_str(), &fileInfo) == 0) {
        return static_cast<uint32_t>(fileInfo.st_size);
    }
    return 0;
}

bool Version::Parse(const std::string &version)
{
    std::vector<int> result {};
    std::stringstream ss(version);
    std::string token;
    while (getline(ss, token, '.')) {
        int number;
        std::istringstream(token) >> number;
        result.push_back(number);
    }

    if (result.size() != 3) {
        LOG_ERROR_ << "parse version failed! version=" << version;
        return false;
    }

    major_ = result[0];
    minor_ = result[1];
    build_ = result[2];
    return true;
}
} // namespace rawheap_translate
