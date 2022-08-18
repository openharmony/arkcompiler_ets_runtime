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

#include "ecmascript/js_runtime_options.h"
#include "ecmascript/log.h"

namespace panda::ecmascript {
Level Log::level_ = Level::ERROR;
void Log::SetLogLevelFromString(const std::string& level)
{
    if (level == "fatal") {
        SetLevel(FATAL);
    }
    if (level == "error") {
        SetLevel(ERROR);
    }
    if (level == "warning") {
        SetLevel(WARN);
    }
    if (level == "info") {
        SetLevel(INFO);
    }
    if (level == "debug") {
        SetLevel(DEBUG);
    }
}

void Log::Initialize(const JSRuntimeOptions &options)
{
    if (options.WasSetLogFatal()) {
        SetLevel(FATAL);
    } else if (options.WasSetLogError()) {
        SetLevel(ERROR);
    } else if (options.WasSetLogWarning()) {
        SetLevel(WARN);
    } else if (options.WasSetLogInfo()) {
        SetLevel(INFO);
    } else if (options.WasSetLogDebug()) {
        SetLevel(DEBUG);
    } else {
        SetLogLevelFromString(options.GetLogLevel());
    }
}
}  // namespace panda::ecmascript