/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/pgo_profiler/pgo_file_info.h"
#include <cstdint>

namespace panda::ecmascript {

const std::string PGODumpUtils::ELEMENT_SEPARATOR = "/";
const std::string PGODumpUtils::BLOCK_SEPARATOR = ",";
const std::string PGODumpUtils::TYPE_SEPARATOR = "|";
const std::string PGODumpUtils::BLOCK_START = ":";
const std::string PGODumpUtils::ARRAY_START = "[";
const std::string PGODumpUtils::ARRAY_END = "]";
const std::string PGODumpUtils::NEW_LINE = "\n";
const std::string PGODumpUtils::SPACE = " ";
const std::string PGODumpUtils::BLOCK_AND_ARRAY_START = BLOCK_START + SPACE + ARRAY_START + SPACE;
const std::string PGODumpUtils::VERSION_HEADER = "Profiler Version" + BLOCK_START + SPACE;
const std::string PGODumpUtils::PANDA_FILE_INFO_HEADER = "Panda file sumcheck list" + BLOCK_AND_ARRAY_START;
const uint32_t PGODumpUtils::HEX_FORMAT_WIDTH_FOR_32BITS = 10;  // for example, 0xffffffff is 10 characters

}  // namespace panda::ecmascript