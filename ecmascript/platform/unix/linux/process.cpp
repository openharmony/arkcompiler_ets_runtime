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

#include "ecmascript/platform/process.h"
#include "ecmascript/log_wrapper.h"

namespace panda::ecmascript {

bool Process::IsolateSubProcess([[maybe_unused]]const std::string &packageName,
    [[maybe_unused]]int mainProcPid, [[maybe_unused]]int subProcPid)
{
    LOG_ECMA(WARN) << "IsolateSubProcess is not support in linux.";
    return false;
}
}  // namespace panda::ecmascript