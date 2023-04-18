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
#ifndef ECMASCRIPT_COMPILER_AOT_FILE_BINARY_BUFFER_PARSER_H
#define ECMASCRIPT_COMPILER_AOT_FILE_BINARY_BUFFER_PARSER_H

#include <cstdint>

namespace panda::ecmascript {
class BinaryBufferParser {
public:
    BinaryBufferParser(uint8_t *buffer, uint32_t length) : buffer_(buffer), length_(length) {}
    ~BinaryBufferParser() = default;
    void ParseBuffer(void *dst, uint32_t count);
    void ParseBuffer(void *dst, uint32_t count, uint32_t offset);
    void ParseBuffer(uint8_t *dst, uint32_t count, uint8_t *src);
    uint8_t *GetAddr() const
    {
        return buffer_;
    }

private:
    uint8_t *buffer_ {nullptr};
    uint32_t length_ {0};
    uint32_t offset_ {0};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_COMPILER_AOT_FILE_BINARY_BUFFER_PARSER_H
