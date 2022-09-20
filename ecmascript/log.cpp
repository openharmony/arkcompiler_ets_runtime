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

#ifdef PANDA_TARGET_ANDROID
#include <android/log.h>
#endif

namespace panda::ecmascript {
Level Log::level_ = Level::ERROR;
ComponentMark Log::components_ = Component::ALL;
void Log::SetLogLevelFromString(const std::string& level)
{
    if (level == "fatal") {
        level_ = FATAL;
    }
    if (level == "error") {
        level_ = ERROR;
    }
    if (level == "warning") {
        level_ = WARN;
    }
    if (level == "info") {
        level_ = INFO;
    }
    if (level == "debug") {
        level_ = DEBUG;
    }
}

void Log::SetLogComponentFromString(const std::vector<std::string>& components)
{
    components_ = Component::NONE;
    for (const auto &component : components) {
        if (component == "all") {
            components_ = Component::ALL;
            return;
        }
        if (component == "gc") {
            components_ |= Component::GC;
            continue;
        }
        if (component == "ecma") {
            components_ |= Component::ECMA;
            continue;
        }
        if (component == "interpreter") {
            components_ |= Component::INTERPRETER;
            continue;
        }
        if (component == "debugger") {
            components_ |= Component::DEBUGGER;
            continue;
        }
        if (component == "compiler") {
            components_ |= Component::COMPILER;
            continue;
        }
    }
}

void Log::Initialize(const JSRuntimeOptions &options)
{
    if (options.WasSetLogFatal()) {
        level_ = FATAL;
        SetLogComponentFromString(options.GetLogFatal());
    } else if (options.WasSetLogError()) {
        level_ = ERROR;
        SetLogComponentFromString(options.GetLogError());
    } else if (options.WasSetLogWarning()) {
        level_ = WARN;
        SetLogComponentFromString(options.GetLogWarning());
    } else if (options.WasSetLogInfo()) {
        level_ = INFO;
        SetLogComponentFromString(options.GetLogInfo());
    } else if (options.WasSetLogDebug()) {
        level_ = DEBUG;
        SetLogComponentFromString(options.GetLogDebug());
    } else {
        SetLogLevelFromString(options.GetLogLevel());
        SetLogComponentFromString(options.GetLogComponents());
    }
}

#ifdef PANDA_TARGET_ANDROID
const char *tag = "ArkCompiler";
template<>
AndroidLog<VERBOSE>::~AndroidLog()
{
    __android_log_write(ANDROID_LOG_VERBOSE, tag, stream_.str().c_str());
}

template<>
AndroidLog<DEBUG>::~AndroidLog()
{
    __android_log_write(ANDROID_LOG_DEBUG, tag, stream_.str().c_str());
}

template<>
AndroidLog<INFO>::~AndroidLog()
{
    __android_log_write(ANDROID_LOG_INFO, tag, stream_.str().c_str());
}

template<>
AndroidLog<WARN>::~AndroidLog()
{
    __android_log_write(ANDROID_LOG_WARN, tag, stream_.str().c_str());
}

template<>
AndroidLog<ERROR>::~AndroidLog()
{
    __android_log_write(ANDROID_LOG_ERROR, tag, stream_.str().c_str());
}

template<>
AndroidLog<FATAL>::~AndroidLog()
{
    __android_log_write(ANDROID_LOG_FATAL, tag, stream_.str().c_str());
    std::abort();
}
#endif
}  // namespace panda::ecmascript