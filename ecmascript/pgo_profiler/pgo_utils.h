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
using ApEntityId = uint32_t;

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

    enum class PGONativeFunctionId : int8_t {
        // iterator function
        MAP_PROTO_ITERATOR = -9,  // 9: number of registered functions
        SET_PROTO_ITERATOR,
        STRING_PROTO_ITERATOR,
        ARRAY_PROTO_ITERATOR,
        TYPED_ARRAY_PROTO_ITERATOR,
        // next function
        MAP_ITERATOR_PROTO_NEXT,
        SET_ITERATOR_PROTO_NEXT,
        STRING_ITERATOR_PROTO_NEXT,
        ARRAY_ITERATOR_PROTO_NEXT,
        LAST,
        INVALID = 0,  // keep the same with method offset 0 to reuse calltarget offset field in pgo
    };
    static_assert(PGONativeFunctionId::LAST == PGONativeFunctionId::INVALID);
};

class ApNameUtils {
public:
    static const std::string AP_SUFFIX;
    static const std::string RUNTIME_AP_PREFIX;
    static const std::string DEFAULT_AP_NAME;
    static std::string GetRuntimeApName(const std::string &ohosModuleName);
    static std::string GetOhosPkgApName(const std::string &ohosModuleName);

private:
    static std::string GetBriefApName(const std::string &ohosModuleName);
};
}  // namespace panda::ecmascript::pgo
#endif  // ECMASCRIPT_PGO_PROFILER_PGO_UTILS_H