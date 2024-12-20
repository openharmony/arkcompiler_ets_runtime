/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "zip_file_reader.h"

#include <sys/stat.h>

#include "zip_file_reader_io.h"
#include "zip_file_reader_mem.h"

namespace panda {
namespace ecmascript {
constexpr size_t MEM_MAX_FILE_SIZE = 1u;

std::shared_ptr<ZipFileReader> ZipFileReader::CreateZipFileReader(const std::string &filePath)
{
    size_t fileSize = GetFileLen(filePath);
    if (fileSize == 0) {
        return nullptr;
    }

    std::shared_ptr<ZipFileReader> result;
    if (fileSize <= MEM_MAX_FILE_SIZE) {
        result = std::make_shared<ZipFileReaderMem>(filePath);
    } else {
        result = std::make_shared<ZipFileReaderIo>(filePath);
    }

    result->fileLen_ = fileSize;
    if (result->init()) {
        return result;
    }
    return nullptr;
}

ZipFileReader::~ZipFileReader()
{
    if (fd_ >= 0 && closable_) {
        close(fd_);
        fd_ = -1;
    }
}

size_t ZipFileReader::GetFileLen(const std::string &filePath)
{
    if (filePath.empty()) {
        return 0;
    }

    struct stat fileStat{};
    if (stat(filePath.c_str(), &fileStat) == 0) {
        return fileStat.st_size;
    }

    return 0;
}

bool ZipFileReader::init()
{
    if (filePath_.empty()) {
        return false;
    }
    fd_ = open(filePath_.c_str(), O_RDONLY);
    if (fd_ < 0) {
        return false;
    }

    return true;
}
}
}