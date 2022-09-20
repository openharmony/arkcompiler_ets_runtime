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

#ifdef ENABLE_HILOG
#include "hilog/log.h"

constexpr static unsigned int ARK_DOMAIN = 0xD003F00;
constexpr static auto TAG = "ArkCompiler";
constexpr static OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, ARK_DOMAIN, TAG};

#if ECMASCRIPT_ENABLE_VERBOSE_LEVEL_LOG
// print Debug level log if enable Verbose log
#define LOG_VERBOSE LOG_DEBUG
#else
#define LOG_VERBOSE LOG_LEVEL_MIN
#endif

static bool LOGGABLE_VERBOSE = HiLogIsLoggable(ARK_DOMAIN, TAG, LOG_VERBOSE);
static bool LOGGABLE_DEBUG = HiLogIsLoggable(ARK_DOMAIN, TAG, LOG_DEBUG);
static bool LOGGABLE_INFO = HiLogIsLoggable(ARK_DOMAIN, TAG, LOG_INFO);
static bool LOGGABLE_WARN = HiLogIsLoggable(ARK_DOMAIN, TAG, LOG_WARN);
static bool LOGGABLE_ERROR = HiLogIsLoggable(ARK_DOMAIN, TAG, LOG_ERROR);
static bool LOGGABLE_FATAL = HiLogIsLoggable(ARK_DOMAIN, TAG, LOG_FATAL);
#endif // ENABLE_HILOG

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
    ECMA = 1ULL << 4ULL,
    ALL = 0xFFFFFFFFULL,
};

namespace panda::ecmascript {
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
        if (component == Component::ALL) {
            return "default";
        }
        if (component == Component::GC) {
            return "gc";
        }
        if (component == Component::ECMA) {
            return "ecma";
        }
        if (component == Component::INTERPRETER) {
            return "interpreter";
        }
        if (component == Component::DEBUGGER) {
            return "debugger";
        }
        if (component == Component::COMPILER) {
            return "compiler";
        }
        return "unknown";
    }

private:
    static void SetLogLevelFromString(const std::string& level);
    static void SetLogComponentFromString(const std::vector<std::string>& components);

    static Level level_;
    static ComponentMark components_;
};

#if defined(ENABLE_HILOG)
template<LogLevel level>
class HiLog {
public:
    HiLog() = default;
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
#elif defined(PANDA_TARGET_ANDROID)  // PANDA_TARGET_ANDROID
template<Level level>
class PUBLIC_API AndroidLog {
public:
    AndroidLog() = default;
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
        stream_ << std::string("[") << str << std::string("]: ");
    }
    ~StdLog()
    {
        std::cerr << stream_.str().c_str() << std::endl;
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
#define ARK_LOG(level) panda::ecmascript::LOGGABLE_##level && panda::ecmascript::HiLog<LOG_##level>()
#elif defined(PANDA_TARGET_ANDROID)
#define ARK_LOG(level) panda::ecmascript::AndroidLog<(level)>()
#else
#define ARK_LOG(level, component) panda::ecmascript::Log::LogIsLoggable(level, component) && \
                                  panda::ecmascript::StdLog<(level), (component)>()
#endif
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_LOG_H
