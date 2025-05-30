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

#ifndef MOCK_ECMASCRIPT_LOG_WRAPPER_H
#define MOCK_ECMASCRIPT_LOG_WRAPPER_H

/** @file mock for ecmascript/log_wrapper.h
 *  This file is the mock of ecmascript/log_wrapper.h
 */
#include <iostream>
#include <sstream>

namespace OHOS::ArkCompiler {
enum Level {
    VERBOSE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
};
std::ostringstream LOG_SA([[maybe_unused]] const Level level);
} // namespace OHOS::ArkCompiler
#endif  // MOCK_ECMASCRIPT_LOG_WRAPPER_H
