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

#include "ecmascript/arksteed/arksteed_comment.h"

#include "ecmascript/log_wrapper.h"
#include "securec.h"

#include <cstring>

namespace panda::ecmascript::arksteed {

// Comment section format:
//   4 bytes: total section size
//   For each comment entry:
//     4 bytes: pc offset
//     4 bytes: comment length (including null terminator)
//     N bytes: comment string (null-terminated)
static constexpr uint32_t OFFSET_TO_FIRST_COMMENT_ENTRY = sizeof(uint32_t);
static constexpr uint32_t OFFSET_TO_PC = 0;
static constexpr uint32_t OFFSET_TO_COMMENT_SIZE = OFFSET_TO_PC + sizeof(uint32_t);
static constexpr uint32_t OFFSET_TO_COMMENT_STRING = OFFSET_TO_COMMENT_SIZE + sizeof(uint32_t);

uint32_t CommentEntry::CommentLength() const
{
    return static_cast<uint32_t>(comment.size() + 1);
}

uint32_t CommentEntry::Size() const
{
    return OFFSET_TO_COMMENT_STRING + CommentLength();
}

void CommentList::Add(uint32_t pcOffset, std::string_view comment)
{
    CommentEntry entry(pcOffset, comment);
    byteCount_ += entry.Size();
    comments_.push_back(std::move(entry));
}

const std::vector<CommentEntry> &CommentList::GetComments() const
{
    return comments_;
}

size_t CommentList::Size() const
{
    return comments_.size();
}

void CommentList::Clear()
{
    comments_.clear();
    byteCount_ = 0;
}

uint32_t CommentList::SectionSize() const
{
    return OFFSET_TO_FIRST_COMMENT_ENTRY + byteCount_;
}

void CommentList::Emit(uint8_t *buffer) const
{
    uint32_t totalSize = SectionSize();
    *reinterpret_cast<uint32_t *>(buffer) = totalSize;

    uint8_t *current = buffer + OFFSET_TO_FIRST_COMMENT_ENTRY;
    for (const auto &entry : comments_) {
        *reinterpret_cast<uint32_t *>(current + OFFSET_TO_PC) = entry.pcOffset;
        *reinterpret_cast<uint32_t *>(current + OFFSET_TO_COMMENT_SIZE) = entry.CommentLength();
        if (memcpy_s(current + OFFSET_TO_COMMENT_STRING, entry.CommentLength(),
            entry.comment.c_str(), entry.CommentLength()) != EOK) {
            LOG_JIT(FATAL) << "memcpy failed in Emit";
        }
        current += entry.Size();
    }
}

CommentList::Iterator::Iterator(const uint8_t *data, uint32_t size)
    : data_(data), size_(size), currentEntry_(data != nullptr ? data + OFFSET_TO_FIRST_COMMENT_ENTRY : nullptr)
{}

bool CommentList::Iterator::HasCurrent() const
{
    return data_ != nullptr && currentEntry_ < data_ + size_;
}

uint32_t CommentList::Iterator::GetPCOffset() const
{
    return HasCurrent() ? *reinterpret_cast<const uint32_t *>(currentEntry_ + OFFSET_TO_PC) : 0;
}

uint32_t CommentList::Iterator::GetCommentSize() const
{
    return HasCurrent() ? *reinterpret_cast<const uint32_t *>(currentEntry_ + OFFSET_TO_COMMENT_SIZE) : 0;
}

const char *CommentList::Iterator::GetComment() const
{
    return HasCurrent() ? reinterpret_cast<const char *>(currentEntry_ + OFFSET_TO_COMMENT_STRING) : nullptr;
}

void CommentList::Iterator::Next()
{
    if (HasCurrent()) {
        currentEntry_ += OFFSET_TO_COMMENT_STRING + GetCommentSize();
    }
}

CommentList::Iterator CommentList::GetIterator(const uint8_t *data, uint32_t size) const
{
    return Iterator(data, size);
}

uint32_t CommentList::GetCommentsDataSize() const
{
    return SectionSize();
}

uint8_t *CommentList::EmitToBuffer() const
{
    if (comments_.empty()) {
        return nullptr;
    }
    uint8_t *buffer = new uint8_t[SectionSize()];
    Emit(buffer);
    return buffer;
}

const CommentEntry *CommentList::FindComment(uint32_t pcOffset) const
{
    for (auto it = comments_.rbegin(); it != comments_.rend(); ++it) {
        if (it->pcOffset <= pcOffset) {
            return &(*it);
        }
    }
    return nullptr;
}

}  // namespace panda::ecmascript::arksteed
