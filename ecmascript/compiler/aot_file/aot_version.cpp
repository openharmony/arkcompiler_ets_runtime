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

#include "ecmascript/compiler/aot_file/aot_version.h"

namespace panda::ecmascript {
using FileHeaderBase = panda::ecmascript::base::FileHeaderBase;
extern "C" __attribute__((visibility("default"))) int32_t GetAOTVersion(std::string &aotVersion)
{
    aotVersion = panda::ecmascript::AOTFileVersion::GetAOTVersion();
    if (aotVersion.size() != (FileHeaderBase::VERSION_SIZE * AOTFileVersion::VERSION_MULTIPLE_SIZE) - 1) {
        return -1;
    }
    return 0;
}
};  // namespace panda::ecmascript
