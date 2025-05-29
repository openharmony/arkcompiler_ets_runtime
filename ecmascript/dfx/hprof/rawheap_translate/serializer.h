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

#ifndef RAWHEAP_TRANSLATE_SERIALIZER_H
#define RAWHEAP_TRANSLATE_SERIALIZER_H

#define NODE_FIELD_COUNT 8
#include "ecmascript/dfx/hprof/rawheap_translate/rawheap_translate.h"
#include "ecmascript/dfx/hprof/rawheap_translate/utils.h"


namespace rawheap_translate {
class StreamWriter {
public:
    StreamWriter()
    {
        chunk_.resize(chunkSize_);
    }

    ~StreamWriter()
    {
        EndOfStream();
    }

    bool Initialize(const std::string &filePath);
    void WriteString(const std::string &str);
    void EndOfStream();

    void WriteChar(char c)
    {
        if (c == '\0') {
            return;
        }
        MaybeWriteChunk();
        chunk_[current_++] = c;
    }

    void WriteNumber(uint64_t num)
    {
        WriteString(std::to_string(num));
    }

private:
    void MaybeWriteChunk()
    {
        if (current_ == chunkSize_) {
            std::string str(chunk_.begin(), chunk_.end());
            fileStream_ << str;
            current_ = 0;
        }
    }

    std::ofstream fileStream_;
    std::vector<char> chunk_;
    int current_ {0};
    int chunkSize_ {1024 * 1024};
};

class HeapSnapshotJSONSerializer {
public:
    explicit HeapSnapshotJSONSerializer() = default;
    ~HeapSnapshotJSONSerializer() = default;

    static bool Serialize(RawHeap *rawheap, StreamWriter *writer);

private:
    static constexpr char ASCII_US = 31;
    static constexpr char ASCII_DEL = 127;
    static constexpr uint8_t UTF8_MAX_BYTES = 4;

private:
    static void SerializeSnapshotHeader(RawHeap *rawheap, StreamWriter *writer);
    static void SerializeNodes(RawHeap *rawheap, StreamWriter *writer);
    static void SerializeEdges(RawHeap *rawheap, StreamWriter *writer);
    static void SerializeStringTable(RawHeap *rawheap, StreamWriter *writer);
    static void SerializeString(const char *str, StreamWriter *writer);
    static void SerializerSnapshotClosure(StreamWriter *writer);
};
}  // namespace rawheap_translate
#endif