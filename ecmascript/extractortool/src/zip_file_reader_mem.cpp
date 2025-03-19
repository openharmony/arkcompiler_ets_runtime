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

#include "zip_file_reader_mem.h"

#include "securec.h"


namespace panda {
namespace ecmascript {
bool ZipFileReaderMem::init()
{
    if (!ZipFileReader::init()) {
        return false;
    }
    posix_fadvise(fd_, 0, 0, POSIX_FADV_SEQUENTIAL);

    fileContent_.resize(fileLen_);
    auto pos = std::string::size_type{};
    while (true) {
        auto const readCount = read(fd_, fileContent_.data() + pos, fileContent_.size() - pos);
        if (readCount < 0) {
            fileContent_.clear();
            return false;
        } else if (readCount == 0) {
            break;
        }

        pos += readCount;
    }
    return true;
}

std::string ZipFileReaderMem::ReadBuffer(size_t startPos, size_t bufferSize)
{
    if (startPos + bufferSize > fileContent_.length()) {
        return "";
    }

    return fileContent_.substr(startPos, bufferSize);
}

bool ZipFileReaderMem::ReadBuffer(uint8_t *dst, size_t startPos, size_t bufferSize)
{
    if (!dst || startPos + bufferSize > fileContent_.length()) {
        return false;
    }

    if (memcpy_s(dst, bufferSize, fileContent_.data() + startPos, bufferSize) != EOK) {
        return false;
    }

    return true;
}
}
}