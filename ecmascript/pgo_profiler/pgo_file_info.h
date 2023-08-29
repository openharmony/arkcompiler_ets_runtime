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

#ifndef ECMASCRIPT_PGO_PROFILER_BASE_INFO_H
#define ECMASCRIPT_PGO_PROFILER_BASE_INFO_H

#include <cstdint>

#include "ecmascript/common.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "macros.h"
#include "mem/mem.h"

namespace panda::ecmascript {

class PUBLIC_API PGODumpUtils {
public:
    static const std::string ELEMENT_SEPARATOR;
    static const std::string BLOCK_SEPARATOR;
    static const std::string TYPE_SEPARATOR;
    static const std::string BLOCK_START;
    static const std::string ARRAY_START;
    static const std::string ARRAY_END;
    static const std::string NEW_LINE;
    static const std::string SPACE;
    static const std::string BLOCK_AND_ARRAY_START;
    static const std::string VERSION_HEADER;
    static const std::string PANDA_FILE_INFO_HEADER;
    static const uint32_t HEX_FORMAT_WIDTH_FOR_32BITS;
};

class PGOProfilerHeader;

struct SectionInfo {
    uint32_t offset_ {0};
    // reserve
    uint32_t size_ {0};
    uint32_t number_ {0};
};
static constexpr Alignment PGO_ALIGN_SIZE = Alignment::LOG_ALIGN_4;
using PGOMethodId = EntityId;
using PGORecordId = EntityId;

class PGOFileDataInterface {
public:
    PGOFileDataInterface() = default;
    virtual ~PGOFileDataInterface() = default;
    virtual uint32_t ProcessToBinary(std::fstream &stream) = 0;
    virtual uint32_t ParseFromBinary(void **buffer, PGOProfilerHeader *const header) = 0;
    virtual bool ProcessToText(std::ofstream &stream) = 0;
    // not support yet
    virtual bool ParseFromText([[maybe_unused]] std::ifstream &stream)
    {
        return true;
    };

private:
    NO_COPY_SEMANTIC(PGOFileDataInterface);
    NO_MOVE_SEMANTIC(PGOFileDataInterface);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_PGO_PROFILER_BASE_INFO_H