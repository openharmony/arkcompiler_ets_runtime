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

#ifndef COMMON_COMPONENTS_LOG_LOG_H
#define COMMON_COMPONENTS_LOG_LOG_H

#include "common_components/log/log_base.h"

#ifdef ENABLE_HILOG
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
#include "hilog/log.h"
#endif
#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD003F00
#undef LOG_TAG
#define LOG_TAG "ArkCompiler"

#if defined(ENABLE_HITRACE)
    #include "hitrace_meter.h"

    #define OHOS_HITRACE(level, name, customArgs)         HITRACE_METER_NAME_EX(level, HITRACE_TAG_ARK, name, customArgs)
    #define OHOS_HITRACE_START(level, name, customArgs)   StartTraceEx(level, HITRACE_TAG_ARK, name, customArgs)
    #define OHOS_HITRACE_FINISH(level)                    FinishTraceEx(level, HITRACE_TAG_ARK)
    #define OHOS_HITRACE_COUNT(level, name, count)        CountTraceEx(level, HITRACE_TAG_ARK, name, count)
#else
    #define OHOS_HITRACE(level, name, customArgs)
    #define OHOS_HITRACE_START(level, name, customArgs)
    #define OHOS_HITRACE_FINISH(level)
    #define OHOS_HITRACE_COUNT(level, name, count)
#endif

namespace panda {
class PUBLIC_API Log {
public:
    static void Initialize(const LogOptions &options);
    static inline bool LogIsLoggable(Level level, Component component)
    {
        switch (component)
        {
            case Component::SA:
                return ((components_ & static_cast<ComponentMark>(component)) != 0ULL);
            default:
                return (level >= level_) && ((components_ & static_cast<ComponentMark>(component)) != 0ULL);
        }
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
            case Component::PGO:
                return "[pgo] ";
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
            case Component::JIT:
                return "[jit] ";
            case Component::BASELINEJIT:
                return "[baselinejit] ";
            case Component::SA:
                return "[sa] ";
            case Component::COMMON:
                return "[common] ";
            case Component::ALL:
                return "[default] ";
            default:
                return "[unknown] ";
        }
    }
    static std::string LevelToString(Level level);
    static Level ConvertFromRuntime(LOG_LEVEL level);

private:
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
            HILOG_DEBUG(LOG_CORE, "%{public}s", stream_.str().c_str());
        } else if constexpr (level == LOG_INFO) {
            HILOG_INFO(LOG_CORE, "%{public}s", stream_.str().c_str());
        } else if constexpr (level == LOG_WARN) {
            HILOG_WARN(LOG_CORE, "%{public}s", stream_.str().c_str());
        } else if constexpr (level == LOG_ERROR) {
            HILOG_ERROR(LOG_CORE, "%{public}s", stream_.str().c_str());
        } else {
            HILOG_FATAL(LOG_CORE, "%{public}s", stream_.str().c_str());
            if (level == LOG_FATAL) {
                std::abort();
            }
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
        if constexpr (level >= Level::ERROR) {
            std::cerr << stream_.str().c_str() << std::endl;
        } else {
            std::cout << stream_.str().c_str() << std::endl;
        }

        if constexpr (level == Level::FATAL) {
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
#define ARK_LOG(level, component) panda::Log::LogIsLoggable(Level::level, component) && \
                                  panda::HiLog<LOG_##level, (component)>()
#elif defined(ENABLE_ANLOG)
#define ARK_LOG(level, component) panda::AndroidLog<(Level::level)>()
#else
#if defined(OHOS_UNIT_TEST)
#define ARK_LOG(level, component) ((Level::level >= Level::INFO) ||                      \
                                  panda::Log::LogIsLoggable(Level::level, component)) && \
                                  panda::StdLog<(Level::level), (component)>()
#else
#define ARK_LOG(level, component) panda::Log::LogIsLoggable(Level::level, component) && \
                                  panda::StdLog<(Level::level), (component)>()
#endif
#endif

#define LOG_COMMON(level) ARK_LOG(level, Component::COMMON)

#define LOGD_IF(cond) (UNLIKELY_CC(cond)) && LOG_COMMON(DEBUG)
#define LOGI_IF(cond) (UNLIKELY_CC(cond)) && LOG_COMMON(INFO)
#define LOGW_IF(cond) (UNLIKELY_CC(cond)) && LOG_COMMON(WARN)
#define LOGE_IF(cond) (UNLIKELY_CC(cond)) && LOG_COMMON(ERROR)
#define LOGF_IF(cond) (UNLIKELY_CC(cond)) && LOG_COMMON(ERROR) << "Check failed: " << #cond && LOG_COMMON(FATAL)

#define CHECKF(cond) (UNLIKELY_CC(!(cond))) && LOG_COMMON(FATAL) << "Check failed: " << #cond
#define LOGF_CHECK(cond) LOGF_IF(!(cond))

#ifndef NDEBUG
#define ASSERT_LOGF(cond, msg) LOGF_IF(!(cond)) << (msg)
#else  // NDEBUG
#define ASSERT_LOGF(cond, msg)
#endif  // NDEBUG

#define CHECK_CALL(call, args, what)        \
    do {                                    \
        int rc = call args;                 \
        if (UNLIKELY_CC(rc != 0)) {            \
            errno = rc;                     \
            LOG_COMMON(FATAL) << #call <<   \
                " failed for " <<           \
                (what) <<" reason " <<      \
                 strerror(errno) <<         \
                 " return " << errno;       \
        }                                   \
    } while (false)

}  // namespace panda
#endif  // COMMON_COMPONENTS_LOG_LOG_H
