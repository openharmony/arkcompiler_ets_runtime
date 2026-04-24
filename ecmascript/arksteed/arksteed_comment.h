/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_ARKSTEED_ARKSTEED_COMMENT_H
#define ECMASCRIPT_ARKSTEED_ARKSTEED_COMMENT_H

#include <cstdint>
#include <string>
#include <vector>

namespace panda::ecmascript::arksteed {

// A single comment entry in the code comment section
struct CommentEntry {
    uint32_t pcOffset;
    std::string comment;

    CommentEntry(uint32_t pc, std::string_view str) : pcOffset(pc), comment(str){};

    uint32_t CommentLength() const;
    uint32_t Size() const;
};

// Stores code comments and provides serialization/deserialization
class CommentList {
public:
    CommentList() = default;

    void Add(uint32_t pcOffset, std::string_view comment);

    const std::vector<CommentEntry> &GetComments() const;
    size_t Size() const;
    void Clear();

    uint32_t SectionSize() const;
    void Emit(uint8_t *buffer) const;

    class Iterator {
    public:
        Iterator(const uint8_t *data, uint32_t size);

        bool HasCurrent() const;
        uint32_t GetPCOffset() const;
        uint32_t GetCommentSize() const;
        const char *GetComment() const;
        void Next();

    private:
        const uint8_t *data_;
        uint32_t size_;
        const uint8_t *currentEntry_;
    };

    Iterator GetIterator(const uint8_t *data, uint32_t size) const;
    uint32_t GetCommentsDataSize() const;
    uint8_t *EmitToBuffer() const;

    // Find the comment with largest pcOffset that is <= given pcOffset
    const CommentEntry *FindComment(uint32_t pcOffset) const;

private:
    std::vector<CommentEntry> comments_;
    uint32_t byteCount_ = 0;
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_ARKSTEED_COMMENT_H
