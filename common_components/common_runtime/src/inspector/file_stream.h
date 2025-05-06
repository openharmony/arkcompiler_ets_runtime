/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ARK_COMMON_FILE_STREAM_H
#define ARK_COMMON_FILE_STREAM_H

#include <cstdint>
#include <fstream>
#include <iosfwd>
#include <utility>

#include "common_components/common_runtime/src/inspector/stream.h"
#include "common_components/log/log.h"

namespace panda {
class StreamWriter {
public:
    explicit StreamWriter(Stream* stream)
        : stream_(stream), chunkSize_(stream->GetSize()), chunk_(chunkSize_), current_(0)
    {
    }

    void WriteString(const CString &str)
    {
        auto len = str.Length();
        DCHECK_CC(len <= static_cast<size_t>(INT_MAX));
        if (len == 0) {
            return;
        }
        const char* cur = str.Str();
        const char* end = cur + len;
        while (cur < end) {
            int dstSize = chunkSize_ - current_;
            int writeSize = std::min(static_cast<int>(end - cur), dstSize);
            DCHECK_CC(writeSize > 0);
            if (memcpy_s(chunk_.data() + current_, dstSize, cur, writeSize) != EOK) {
                LOG_COMMON(ERROR) << "memcpy_s failed";
            }
            cur += writeSize;
            current_ += writeSize;
            MaybeWriteChunk();
        }
    }

    void WriteChar(char c)
    {
        DCHECK_CC(c != '\0');
        DCHECK_CC(current_ < chunkSize_);
        chunk_[current_++] = c;
        MaybeWriteChunk();
    }

    void WriteNumber(int32_t num)
    {
        WriteString(CString(num));
    }

    void End()
    {
        if (current_ > 0) {
            WriteChunk();
        }
        stream_->EndOfStream();
    }

private:
    void MaybeWriteChunk()
    {
        DCHECK_CC(current_ <= chunkSize_);
        if (current_ >= chunkSize_) {
            WriteChunk();
        }
    }

    void WriteChunk()
    {
        stream_->WriteChunk(chunk_.data(), current_);
        current_ = 0;
    }

    Stream* stream_ {nullptr};
    int chunkSize_ {0};
    std::vector<char> chunk_;
    int current_ {0};
};

enum MsgType : uint32_t {
    DUMPHEAPSNAPSHOT = 0,
    ALLOCATION = 1,
    HEAPUSAGE = 2,
    DISABLE = 3,
    END = 4,
    STATSUPDATE = 5,
};

class HeapProfilerStream : public Stream {
public:
    static HeapProfilerStream& GetInstance()
    {
        static HeapProfilerStream instance;
        return instance;
    }
    using SendMsgCB =  std::function<void(const std::string& message)>;
    HeapProfilerStream() : sendMsg_(nullptr) {}
    void SetHandler(SendMsgCB sendMsgInit);
    bool SetMessageID(const std::string &message);
    CString GetMessageID();
    void SetContext(MsgType type);
    void EndOfStream() override {}
    int GetSize() override;
    bool WriteChunk(char* data, int32_t size) override;

private:
    std::string start_;
    std::string end_;
    SendMsgCB sendMsg_;
    CString messageId_;
};
}
#endif
