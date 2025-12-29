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

namespace panda::ecmascript::pgo {

const std::string ApNameUtils::AP_SUFFIX = ".ap";
const std::string ApNameUtils::RUNTIME_AP_PREFIX = "rt_";
const std::string ApNameUtils::MERGED_AP_PREFIX = "merged_";
const std::string ApNameUtils::DEFAULT_AP_NAME = "modules" + AP_SUFFIX;

std::string ApNameUtils::GetRuntimeApName(const std::string &ohosModuleName)
{
    return RUNTIME_AP_PREFIX + GetBriefApName(ohosModuleName);
}

std::string ApNameUtils::GetMergedApName(const std::string &ohosModuleName)
{
    return MERGED_AP_PREFIX + GetBriefApName(ohosModuleName);
}

std::string ApNameUtils::GetOhosPkgApName(const std::string &ohosModuleName)
{
    return GetBriefApName(ohosModuleName);
}

std::string ApNameUtils::GetBriefApName(const std::string &ohosModuleName)
{
    return ohosModuleName + AP_SUFFIX;
}
} // namespace panda::ecmascript::pgo