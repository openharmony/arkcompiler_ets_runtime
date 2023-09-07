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

#ifndef ECMASCRIPT_PGO_PROFILER_PGO_UTILS_H
#define ECMASCRIPT_PGO_PROFILER_PGO_UTILS_H

#include <string>

#include "mem/mem.h"
#include "libpandafile/file.h"

namespace panda::ecmascript::pgo {
static constexpr Alignment ALIGN_SIZE = Alignment::LOG_ALIGN_4;
using PGOMethodId = panda_file::File::EntityId;
using ApEntityId = panda_file::File::EntityId;
class DumpUtils {
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
} // namespace panda::ecmascript::pgo
#endif  // ECMASCRIPT_PGO_PROFILER_PGO_UTILS_H