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

#include "zip_file_reader_io.h"

namespace panda {
namespace ecmascript {
std::string ZipFileReaderIo::ReadBuffer(size_t startPos, size_t bufferSize)
{
    std::string result;
    if (fd_ < 0 || startPos + bufferSize > fileLen_) {
        return result;
    }

    result.resize(bufferSize);
    if (!ReadBuffer(reinterpret_cast<uint8_t*>(result.data()), startPos, bufferSize)) {
        result.clear();
    }

    return result;
}

bool ZipFileReaderIo::ReadBuffer(uint8_t *dst, size_t startPos, size_t bufferSize)
{
    if (dst == nullptr || fd_ < 0 || startPos + bufferSize > fileLen_) {
        return false;
    }

    auto const readCount = pread(fd_, dst, bufferSize, startPos);
    if (readCount < 0 || static_cast<size_t>(readCount) < bufferSize) {
        return false;
    }

    return true;
}
}
}