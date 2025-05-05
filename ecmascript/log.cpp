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

#ifdef ENABLE_ANLOG
#include <android/log.h>
#endif

namespace panda::ecmascript {
#ifdef ENABLE_HILOG
namespace {
Level ConvertToLevel(LogLevel hilogLevel)
{
    Level level = Level::ERROR;
    std::string logLevel;
    switch (hilogLevel) {
        case LogLevel::LOG_INFO:
            level = Level::INFO;
            break;
        case LogLevel::LOG_WARN:
            level = Level::WARN;
            break;
        case LogLevel::LOG_ERROR:
            level = Level::ERROR;
            break;
        case LogLevel::LOG_FATAL:
        case LogLevel::LOG_LEVEL_MAX:
            level = Level::FATAL;
            break;
        case LogLevel::LOG_DEBUG:
        default:
            level = Level::DEBUG;
            break;
    }

    return level;
}

LogLevel GetHiLogLevel()
{
    for (int32_t level = LogLevel::LOG_LEVEL_MIN; level <= LogLevel::LOG_LEVEL_MAX; level++) {
        if (HiLogIsLoggable(LOG_DOMAIN, LOG_TAG, static_cast<LogLevel>(level))) {
            return static_cast<LogLevel>(level);
        }
    }
    return LogLevel::LOG_LEVEL_MAX;
}
}  // namespace
#endif

Level Log::level_ = Level::ERROR;
ComponentMark Log::components_ = Component::ALL;

Level Log::ConvertFromRuntime(LOG_LEVEL level)
{
    Level logLevel = Level::INFO;
    switch (level) {
        case LOG_LEVEL::FOLLOW:
#ifdef ENABLE_HILOG
            logLevel = ConvertToLevel(GetHiLogLevel());
            break;
#endif
        case LOG_LEVEL::INFO:
            logLevel = Level::INFO;
            break;
        case LOG_LEVEL::WARN:
            logLevel = Level::WARN;
            break;
        case LOG_LEVEL::ERROR:
            logLevel = Level::ERROR;
            break;
        case LOG_LEVEL::FATAL:
            logLevel = Level::FATAL;
            break;
        case LOG_LEVEL::DEBUG:
        default:
            logLevel = Level::DEBUG;
            break;
    }

    return logLevel;
}

std::string Log::LevelToString(Level level)
{
    std::string logLevel;
    switch (level) {
        case Level::INFO:
            logLevel = "info";
            break;
        case Level::WARN:
            logLevel = "warning";
            break;
        case Level::ERROR:
            logLevel = "error";
            break;
        case Level::FATAL:
            logLevel = "fatal";
            break;
        case Level::DEBUG:
        default:
            logLevel = "debug";
            break;
    }

    return logLevel;
}

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
        if (component == "pgo") {
            components_ |= Component::PGO;
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
        if (component == "jit") {
            components_ |= Component::JIT;
            continue;
        }
        if (component == "baselinejit") {
            components_ |= Component::BASELINEJIT;
            continue;
        }
        if (component == "trace") {
            components_ |= Component::TRACE;
            continue;
        }
        if (component == "sa") {
            components_ |= Component::SA;
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

#ifdef ENABLE_ANLOG
const char *tag = "ArkCompiler";
template<>
PUBLIC_API AndroidLog<VERBOSE>::~AndroidLog()
{
    __android_log_write(ANDROID_LOG_VERBOSE, tag, stream_.str().c_str());
}

template<>
PUBLIC_API AndroidLog<DEBUG>::~AndroidLog()
{
    __android_log_write(ANDROID_LOG_DEBUG, tag, stream_.str().c_str());
}

template<>
PUBLIC_API AndroidLog<INFO>::~AndroidLog()
{
    __android_log_write(ANDROID_LOG_INFO, tag, stream_.str().c_str());
}

template<>
PUBLIC_API AndroidLog<WARN>::~AndroidLog()
{
    __android_log_write(ANDROID_LOG_WARN, tag, stream_.str().c_str());
}

template<>
PUBLIC_API AndroidLog<ERROR>::~AndroidLog()
{
    __android_log_write(ANDROID_LOG_ERROR, tag, stream_.str().c_str());
}

template<>
PUBLIC_API AndroidLog<FATAL>::~AndroidLog()
{
    __android_log_write(ANDROID_LOG_FATAL, tag, stream_.str().c_str());
    std::abort();
}
#endif
}  // namespace panda::ecmascript