/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ECMASCRIPT_DFX_HPROF_FILE_STREAM_H
#define ECMASCRIPT_DFX_HPROF_FILE_STREAM_H

#include <cstdint>
#include <fstream>
#include <iosfwd>
#include <utility>

#include "ecmascript/dfx/hprof/stream.h"
#include "ecmascript/mem/mem_common.h"

namespace panda::ecmascript {
class FileStream : public Stream {
public:
    explicit FileStream(const std::string &fileName);
    ~FileStream() override;

    void EndOfStream() override;

    // Get chunk's size
    int GetSize() override
    {
        const static int fileChunkSize = static_cast<int>(1_MB);
        return fileChunkSize;
    }

    // Writes the chunk of data into the stream
    bool WriteChunk(char* data, int32_t size) override;
    bool WriteBinBlock(char *data, int32_t size) override
    {
        return WriteChunk(data, size);
    }
    bool Good() override;
    void UpdateHeapStats([[maybe_unused]] HeapStat* data, [[maybe_unused]] int32_t count) override
    {
    }
    void UpdateLastSeenObjectId([[maybe_unused]]int32_t lastSeenObjectId, [[maybe_unused]]int64_t timeStampUs) override
    {
    }

private:
    void Initialize(const std::string &fileName);
    std::pair<bool, std::string> FilePathValid(const std::string &fileName);

    std::fstream fileStream_;
};

class FileDescriptorStream : public Stream {
public:
    explicit FileDescriptorStream(int32_t fd): fd_(fd) {}
    ~FileDescriptorStream() override = default;

    void EndOfStream() override;

    // Get chunk's size
    int GetSize() override
    {
        const static int fileChunkSize = static_cast<int>(1_MB);
        return fileChunkSize;
    }

    // Writes the chunk of data into the stream
    bool WriteChunk(char *data, int32_t size) override;
    bool WriteBinBlock(char *data, int32_t size) override;
    bool Good() override;
    void UpdateHeapStats([[maybe_unused]] HeapStat* data, [[maybe_unused]] int32_t count) override
    {
    }
    void UpdateLastSeenObjectId([[maybe_unused]]int32_t lastSeenObjectId, [[maybe_unused]]int64_t timeStampUs) override
    {
    }

private:
    int32_t fd_;
};
}  // namespace panda::ecmascript::tooling

#endif  // ECMASCRIPT_DFX_HPROF_FILE_STREAM_H
