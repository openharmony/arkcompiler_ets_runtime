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
#include <functional>
#include <cstdint>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <securec.h>

namespace rawheap_translate {
#define LOG_ERROR(msg) std::cerr << "[ERRO] " << (msg) << std::endl
#define LOG_INFO(msg) std::cout << "[INFO] " << (msg) << std::endl
#ifdef PATH_MAX
#undef PATH_MAX
#endif
#define PATH_MAX 4096
#define MAX_FILE_SIZE (4 * 1024 * 1024 * 1024ULL) // 4 * 1024 * 1024 * 1024 : file size bigger than 4GB
#define MAX_OBJ_SIZE (MAX_FILE_SIZE >> 1)

static const int MAJOR_VERSION_INDEX = 0;
static const int MINOR_VERSION_INDEX = 1;
static const int BUILD_VERSION_INDEX = 2;
static const int VERSION[3] = {1, 0, 0};

bool CheckVersion(const std::string &version);

bool RealPath(const std::string &filename, std::string &realpath);

uint64_t GetFileSize(std::string &inputFilePath);

bool FileCheckAndOpenBinary(const std::string &rawheapPath, std::ifstream &file, uint32_t &fileSize);

void GenerateDumpFileName(std::string &filename);

bool EndsWith(const std::string &str, const std::string &suffix);

bool IsLittleEndian();

uint32_t ByteToU32(char *data);

uint64_t ByteToU64(char *data);

void ByteToU32Array(char *data, uint32_t *array, uint32_t size);

void ByteToU64Array(char *data, uint64_t *array, uint32_t size);
}  // namespace rawheap_translate
#endif