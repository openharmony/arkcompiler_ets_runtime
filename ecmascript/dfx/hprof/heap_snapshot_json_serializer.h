/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_DFX_HPROF_HEAP_SNAPSHOT_SERIALIZER_H
#define ECMASCRIPT_DFX_HPROF_HEAP_SNAPSHOT_SERIALIZER_H

#include <fstream>
#include <limits.h>
#include <sstream>

#include "ecmascript/dfx/hprof/file_stream.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/dfx/hprof/string_hashmap.h"

#include "securec.h"

namespace panda::ecmascript {
using fstream = std::fstream;
using stringstream = std::stringstream;

class HeapSnapshot;
class TraceNode;

class StreamWriter {
public:
    explicit StreamWriter(Stream *stream)
        : stream_(stream), chunkSize_(stream->GetSize()), chunk_(chunkSize_), current_(0)
    {
    }

    void WriteString(const CString &str)
    {
        auto len = str.size();
        ASSERT(len <= static_cast<size_t>(INT_MAX));
        if (len == 0) {
            return;
        }
        const char *cur = str.c_str();
        const char *end = cur + len;
        while (cur < end) {
            int dstSize = chunkSize_ - current_;
            int writeSize = std::min(static_cast<int>(end - cur), dstSize);
            ASSERT(writeSize > 0);
            if (memcpy_s(chunk_.data() + current_, dstSize, cur, writeSize) != EOK) {
                LOG_FULL(FATAL) << "memcpy_s failed";
            }
            cur += writeSize;
            current_ += writeSize;
            MaybeWriteChunk();
        }
    }

    void WriteBinBlock(char *block, int size)
    {
        char *cur = block;
        while (size > 0) {
            MaybeWriteBinBlock();
            int dstSize = chunkSize_ - current_;
            int writeSize = std::min(dstSize, size);
            if (memcpy_s(chunk_.data() + current_, dstSize, cur, writeSize) != EOK) {
                LOG_FULL(FATAL) << "memcpy_s failed!";
            }
            cur += writeSize;
            current_ += writeSize;
            size -= writeSize;
        }
    }

    void WriteChar(char c)
    {
        ASSERT(c != '\0');
        ASSERT(current_ < chunkSize_);
        chunk_[current_++] = c;
        MaybeWriteChunk();
    }

    void WriteNumber(uint64_t num)
    {
        WriteString(ToCString(num));
    }

    void End()
    {
        if (current_ > 0) {
            WriteChunk();
        }
        stream_->EndOfStream();
    }

    void EndOfWriteBinBlock()
    {
        if (current_ > 0) {
            WriteBinBlock();
        }
        stream_->EndOfStream();
    }

private:
    void MaybeWriteChunk()
    {
        ASSERT(current_ <= chunkSize_);
        if (current_ == chunkSize_) {
            WriteChunk();
        }
    }

    void MaybeWriteBinBlock()
    {
        ASSERT(current_ <= chunkSize_);
        if (current_ == chunkSize_) {
            WriteBinBlock();
        }
    }

    void WriteChunk()
    {
        stream_->WriteChunk(chunk_.data(), current_);
        current_ = 0;
    }

    void WriteBinBlock()
    {
        stream_->WriteBinBlock(chunk_.data(), current_);
        current_ = 0;
    }

    Stream *stream_ {nullptr};
    int chunkSize_ {0};
    CVector<char> chunk_;
    int current_ {0};
};

class HeapSnapshotJSONSerializer {
public:
    explicit HeapSnapshotJSONSerializer() = default;
    ~HeapSnapshotJSONSerializer() = default;
    NO_MOVE_SEMANTIC(HeapSnapshotJSONSerializer);
    NO_COPY_SEMANTIC(HeapSnapshotJSONSerializer);
    static bool Serialize(HeapSnapshot *snapshot, Stream *stream);

private:
    static constexpr char ASCII_US = 31;
    static constexpr char ASCII_DEL = 127;
    static constexpr uint8_t UTF8_MAX_BYTES = 4;

private:
    static void SerializeSnapshotHeader(HeapSnapshot *snapshot, StreamWriter *writer);
    static void SerializeNodes(HeapSnapshot *snapshot, StreamWriter *writer);
    static void SerializeEdges(HeapSnapshot *snapshot, StreamWriter *writer);
    static void SerializeTraceFunctionInfo(HeapSnapshot *snapshot, StreamWriter *writer);
    static void SerializeTraceTree(HeapSnapshot *snapshot, StreamWriter *writer);
    static void SerializeTraceNode(TraceNode *node, StreamWriter *writer);
    static void SerializeSamples(HeapSnapshot *snapshot, StreamWriter *writer);
    static void SerializeLocations(StreamWriter *writer);
    static void SerializeStringTable(HeapSnapshot *snapshot, StreamWriter *writer);
    static void SerializeString(CString *str, StreamWriter *writer);
    static void SerializeUnicodeChar(uint32_t unicodeChar, StreamWriter *writer);
    static void SerializerSnapshotClosure(StreamWriter *writer);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_DFX_HPROF_HEAP_SNAPSHOT_SERIALIZER_H
