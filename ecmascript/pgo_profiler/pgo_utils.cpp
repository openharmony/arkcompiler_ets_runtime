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

#include "ecmascript/pgo_profiler/pgo_utils.h"
#include <cstdint>

namespace panda::ecmascript::pgo {

const std::string DumpUtils::ELEMENT_SEPARATOR = "/";
const std::string DumpUtils::BLOCK_SEPARATOR = ",";
const std::string DumpUtils::TYPE_SEPARATOR = "|";
const std::string DumpUtils::BLOCK_START = ":";
const std::string DumpUtils::ARRAY_START = "[";
const std::string DumpUtils::ARRAY_END = "]";
const std::string DumpUtils::NEW_LINE = "\n";
const std::string DumpUtils::SPACE = " ";
const std::string DumpUtils::BLOCK_AND_ARRAY_START = BLOCK_START + SPACE + ARRAY_START + SPACE;
const std::string DumpUtils::VERSION_HEADER = "Profiler Version" + BLOCK_START + SPACE;
const std::string DumpUtils::PANDA_FILE_INFO_HEADER = "Panda file sumcheck list" + BLOCK_AND_ARRAY_START;
const uint32_t DumpUtils::HEX_FORMAT_WIDTH_FOR_32BITS = 10;  // for example, 0xffffffff is 10 characters

} // namespace panda::ecmascript::pgo