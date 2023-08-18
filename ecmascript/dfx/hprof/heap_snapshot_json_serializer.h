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

#include "ecmascript/mem/c_string.h"
#include "ecmascript/dfx/hprof/file_stream.h"
#include "ecmascript/mem/c_containers.h"

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

    void Write(const CString &str)
    {
        ASSERT(str.size() <= static_cast<size_t>(INT_MAX));
        auto len = static_cast<int>(str.size());
        const char *cur = str.c_str();
        const char *end = cur + len;
        while (cur < end) {
            int dstSize = chunkSize_ - current_;
            int writeSize = std::min(static_cast<int>(end - cur), dstSize);
            if (memcpy_s(chunk_.data() + current_, dstSize, cur, writeSize) != EOK) {
                LOG_FULL(FATAL) << "memcpy_s failed";
            }
            cur += writeSize;
            current_ += writeSize;

            if (current_ == chunkSize_) {
                WriteChunk();
            }
        }
    }

    void Write(uint64_t num)
    {
        Write(ToCString(num));
    }

    void End()
    {
        if (current_ > 0) {
            WriteChunk();
        }
        stream_->EndOfStream();
    }

private:
    void WriteChunk()
    {
        stream_->WriteChunk(chunk_.data(), current_);
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
    ~HeapSnapshotJSONSerializer();
    NO_MOVE_SEMANTIC(HeapSnapshotJSONSerializer);
    NO_COPY_SEMANTIC(HeapSnapshotJSONSerializer);
    bool Serialize(HeapSnapshot *snapshot, Stream *stream);

private:
    void SerializeSnapshotHeader();
    void SerializeNodes();
    void SerializeEdges();
    void SerializeTraceFunctionInfo();
    void SerializeTraceTree();
    void SerializeTraceNode(TraceNode *node);
    void SerializeSamples();
    void SerializeLocations();
    void SerializeStringTable();
    void SerializerSnapshotClosure();

    HeapSnapshot *snapshot_ {nullptr};
    StreamWriter *writer_ {nullptr};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_DFX_HPROF_HEAP_SNAPSHOT_SERIALIZER_H
