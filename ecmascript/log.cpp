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
#include "generated/base_options.h"

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
    if (level == "verbose") {
        level_ = VERBOSE;
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
        if (component == "ecmascript") {
            components_ |= Component::ECMASCRIPT;
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
        if (component == "builtins") {
            components_ |= Component::BUILTINS;
            continue;
        }
    }
}

int32_t Log::PrintLogger(int32_t, int32_t level, const char *, const char *, const char *message)
{
    switch (level) {
        case Logger::PandaLog2MobileLog::VERBOSE:
            LOG_ECMA(VERBOSE) << message;
            break;
        case Logger::PandaLog2MobileLog::DEBUG:
            LOG_ECMA(DEBUG) << message;
            break;
        case Logger::PandaLog2MobileLog::INFO:
            LOG_ECMA(INFO) << message;
            break;
        case Logger::PandaLog2MobileLog::WARN:
            LOG_ECMA(WARN) << message;
            break;
        case Logger::PandaLog2MobileLog::ERROR:
            LOG_ECMA(ERROR) << message;
            break;
        case Logger::PandaLog2MobileLog::FATAL:
            LOG_ECMA(FATAL) << message;
            break;
        default:
            LOG_ECMA(DEBUG) << message;
            break;
    }
    return 0;
}

void Log::Initialize(const JSRuntimeOptions &options)
{
    // For ArkTS runtime log
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

    // For runtime core log
    base_options::Options baseOptions("");
    baseOptions.SetLogLevel(options.GetLogLevel());
    baseOptions.SetLogComponents({ "all" });
    Logger::Initialize(baseOptions);
    Logger::SetMobileLogPrintEntryPointByPtr(reinterpret_cast<void *>(Log::PrintLogger));
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