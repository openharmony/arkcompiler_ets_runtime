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

#ifndef ECMASCRIPT_LOG_H
#define ECMASCRIPT_LOG_H

#include <cstdint>
#include <iostream>
#include <sstream>

#include "ecmascript/common.h"
#include "ecmascript/napi/include/jsnapi.h"

#ifdef ENABLE_HILOG
#include "hilog/log.h"
#endif

using LOG_LEVEL = panda::RuntimeOption::LOG_LEVEL;
enum Level {
    VERBOSE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
};

using ComponentMark = uint64_t;
enum Component {
    NONE = 0ULL,
    GC = 1ULL << 0ULL,
    INTERPRETER = 1ULL << 1ULL,
    COMPILER = 1ULL << 2ULL,
    DEBUGGER = 1ULL << 3ULL,
    ECMASCRIPT = 1ULL << 4ULL,
    BUILTINS = 1ULL << 5ULL,
    TRACE = 1ULL << 6ULL,
    NO_TAG = 0xFFFFFFFFULL >> 1ULL,
    ALL = 0xFFFFFFFFULL,
};

namespace panda::ecmascript {
#ifdef ENABLE_HILOG
constexpr static unsigned int ARK_DOMAIN = 0xD003F00;
constexpr static auto TAG = "ArkCompiler";
constexpr static OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, ARK_DOMAIN, TAG};

#if ECMASCRIPT_ENABLE_VERBOSE_LEVEL_LOG
// print Debug level log if enable Verbose log
#define LOG_VERBOSE LOG_DEBUG
#else
#define LOG_VERBOSE LOG_LEVEL_MIN
#endif
#endif  // ENABLE_HILOG

class JSRuntimeOptions;
class PUBLIC_API Log {
public:
    static void Initialize(const JSRuntimeOptions &options);
    static inline bool LogIsLoggable(Level level, Component component)
    {
        return (level >= level_) && ((components_ & component) != 0ULL);
    }
    static inline std::string GetComponentStr(Component component)
    {
        switch (component)
        {
            case Component::NO_TAG:
                return "";
            case Component::GC:
                return "[gc] ";
            case Component::ECMASCRIPT:
                return "[ecmascript] ";
            case Component::INTERPRETER:
                return "[interpreter] ";
            case Component::DEBUGGER:
                return "[debugger] ";
            case Component::COMPILER:
                return "[compiler] ";
            case Component::BUILTINS:
                return "[builtins] ";
            case Component::TRACE:
                return "[trace] ";
            case Component::ALL:
                return "[default] ";
            default:
                return "[unknown] ";
        }
    }
    static std::string LevelToString(Level level);
    static Level ConvertFromRuntime(LOG_LEVEL level);

private:
    static void SetLogLevelFromString(const std::string& level);
    static void SetLogComponentFromString(const std::vector<std::string>& components);
    static int32_t PrintLogger(int32_t, int32_t level, const char *, const char *, const char *message);

    static Level level_;
    static ComponentMark components_;
};

#if defined(ENABLE_HILOG)
template<LogLevel level, Component component>
class HiLog {
public:
    HiLog()
    {
        std::string str = Log::GetComponentStr(component);
        stream_ << str;
    }
    ~HiLog()
    {
        if constexpr (level == LOG_LEVEL_MIN) {
            // print nothing
        } else if constexpr (level == LOG_DEBUG) {
            OHOS::HiviewDFX::HiLog::Debug(LABEL, "%{public}s", stream_.str().c_str());
        } else if constexpr (level == LOG_INFO) {
            OHOS::HiviewDFX::HiLog::Info(LABEL, "%{public}s", stream_.str().c_str());
        } else if constexpr (level == LOG_WARN) {
            OHOS::HiviewDFX::HiLog::Warn(LABEL, "%{public}s", stream_.str().c_str());
        } else if constexpr (level == LOG_ERROR) {
            OHOS::HiviewDFX::HiLog::Error(LABEL, "%{public}s", stream_.str().c_str());
        } else {
            OHOS::HiviewDFX::HiLog::Fatal(LABEL, "%{public}s", stream_.str().c_str());
            std::abort();
        }
    }
    template<class type>
    std::ostream &operator <<(type input)
    {
        stream_ << input;
        return stream_;
    }

private:
    std::ostringstream stream_;
};
#elif defined(ENABLE_ANLOG)  // ENABLE_ANLOG
template<Level level>
class PUBLIC_API AndroidLog {
public:
    AndroidLog()
    {
        std::string str = "[default] ";
        stream_ << str;
    }
    ~AndroidLog();

    template<class type>
    std::ostream &operator <<(type input)
    {
        stream_ << input;
        return stream_;
    }

private:
    std::ostringstream stream_;
};
#else
template<Level level, Component component>
class StdLog {
public:
    StdLog()
    {
        std::string str = Log::GetComponentStr(component);
        stream_ << str;
    }
    ~StdLog()
    {
        if constexpr (level == FATAL || level == ERROR) {
            std::cerr << stream_.str().c_str() << std::endl;
        } else {
            std::cout << stream_.str().c_str() << std::endl;
        }

        if constexpr (level == FATAL) {
            std::abort();
        }
    }

    template<class type>
    std::ostream &operator <<(type input)
    {
        stream_ << input;
        return stream_;
    }

private:
    std::ostringstream stream_;
};
#endif

#if defined(ENABLE_HILOG)
#define ARK_LOG(level, component) panda::ecmascript::Log::LogIsLoggable(level, component) && \
                                  panda::ecmascript::HiLog<LOG_##level, (component)>()
#elif defined(ENABLE_ANLOG)
#define ARK_LOG(level, component) panda::ecmascript::AndroidLog<(level)>()
#else
#if defined(OHOS_UNIT_TEST)
#define ARK_LOG(level, component) ((level >= INFO) || panda::ecmascript::Log::LogIsLoggable(level, component)) && \
                                  panda::ecmascript::StdLog<(level), (component)>()
#else
#define ARK_LOG(level, component) panda::ecmascript::Log::LogIsLoggable(level, component) && \
                                  panda::ecmascript::StdLog<(level), (component)>()
#endif
#endif
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_LOG_H
