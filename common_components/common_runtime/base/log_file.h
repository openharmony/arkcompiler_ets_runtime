/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ARK_COMMON_LOG_FILE_H
#define ARK_COMMON_LOG_FILE_H

#include <mutex>

#include "common_components/base/time_utils.h"


namespace common {
enum LogType {
    // for overall brief report
    REPORT = 0,

    // for debug purpose
    DEBUGY,

    // for allocator
    ALLOC,
    REGION,
    FRAGMENT,

    // for barriers
    BARRIER,  // idle phase
    EBARRIER, // enum phase
    TBARRIER, // marking phase
    PBARRIER, // preforward phase
    FBARRIER, // copy phase

    // for gc
    GCPHASE,
    ENUM,
    TRACE,
    PREFORWARD,
    COPY,
    FIX,
    FINALIZE,

    UNWIND,
    EXCEPTION,
    SIGNAL,

    ARKTHREAD,

#ifdef ARKCOMMON_SANITIZER_SUPPORT
    SANITIZER,
#endif

    LOG_TYPE_NUMBER
};

#ifndef DEFAULT_ARK_REPORT
#define DEFAULT_ARK_REPORT 0
#endif // DEFAULT_ARK_REPORT

#ifndef DEFAULT_ARK_LOG_ALLOC
#define DEFAULT_ARK_LOG_ALLOC 0
#endif // DEFAULT_ARK_LOG_ALLOC

#ifndef DEFAULT_ARK_LOG_REGION
#define DEFAULT_ARK_LOG_REGION 0
#endif // DEFAULT_ARK_LOG_REGION

#ifndef DEFAULT_ARK_LOG_FRAGMENT
#define DEFAULT_ARK_LOG_FRAGMENT 0
#endif // DEFAULT_ARK_LOG_FRAGMENT

#ifndef DEFAULT_ARK_LOG_DEBUG
#define DEFAULT_ARK_LOG_DEBUG 0
#endif // DEFAULT_ARK_LOG_DEBUG

#ifndef DEFAULT_ARK_LOG_BARRIER
#define DEFAULT_ARK_LOG_BARRIER 0
#endif // DEFAULT_ARK_LOG_BARRIER

#ifndef DEFAULT_ARK_LOG_EBARRIER
#define DEFAULT_ARK_LOG_EBARRIER 0
#endif // DEFAULT_ARK_LOG_EBARRIER

#ifndef DEFAULT_ARK_LOG_TBARRIER
#define DEFAULT_ARK_LOG_TBARRIER 0
#endif // DEFAULT_ARK_LOG_TBARRIER

#ifndef DEFAULT_ARK_LOG_PBARRIER
#define DEFAULT_ARK_LOG_PBARRIER 0
#endif // DEFAULT_ARK_LOG_PBARRIER

#ifndef DEFAULT_ARK_LOG_FBARRIER
#define DEFAULT_ARK_LOG_FBARRIER 0
#endif // DEFAULT_ARK_LOG_FBARRIER

#ifndef DEFAULT_ARK_LOG_COMMONPHASE
#define DEFAULT_ARK_LOG_COMMONPHASE 0
#endif // DEFAULT_ARK_LOG_COMMONPHASE

#ifndef DEFAULT_ARK_LOG_ENUM
#define DEFAULT_ARK_LOG_ENUM 0
#endif // DEFAULT_ARK_LOG_ENUM

#ifndef DEFAULT_ARK_LOG_TRACE
#define DEFAULT_ARK_LOG_TRACE 0
#endif // DEFAULT_ARK_LOG_TRACE

#ifndef DEFAULT_ARK_LOG_PREFORWARD
#define DEFAULT_ARK_LOG_PREFORWARD 0
#endif // DEFAULT_ARK_LOG_PREFORWARD

#ifndef DEFAULT_ARK_LOG_COPY
#define DEFAULT_ARK_LOG_COPY 0
#endif // DEFAULT_ARK_LOG_COPY

#ifndef DEFAULT_ARK_LOG_FIX
#define DEFAULT_ARK_LOG_FIX 0
#endif // DEFAULT_ARK_LOG_FIX

#ifndef DEFAULT_ARK_LOG_FINALIZE
#define DEFAULT_ARK_LOG_FINALIZE 0
#endif // DEFAULT_ARK_LOG_FINALIZE

#ifndef DEFAULT_ARK_LOG_UNWIND
#define DEFAULT_ARK_LOG_UNWIND 0
#endif // DEFAULT_ARK_LOG_UNWIND

#ifndef DEFAULT_ARK_LOG_EXCEPTION
#define DEFAULT_ARK_LOG_EXCEPTION 0
#endif // DEFAULT_ARK_LOG_EXCEPTION

#ifndef DEFAULT_ARK_LOG_SIGNAL
#define DEFAULT_ARK_LOG_SIGNAL 0
#endif // DEFAULT_ARK_LOG_SIGNAL

#ifndef DEFAULT_ARK_LOG_THREAD
#define DEFAULT_ARK_LOG_THREAD 0
#endif // DEFAULT_ARK_LOG_THREAD

#ifndef DEFAULT_ARK_LOG2STDOUT
#define DEFAULT_ARK_LOG2STDOUT 0
#endif // DEFAULT_ARK_LOG2STDOUT

#ifdef ARKCOMMON_SANITIZER_SUPPORT
#ifndef DEFAULT_ARK_LOG_SANITIZER
#define DEFAULT_ARK_LOG_SANITIZER 0
#endif // DEFAULT_ARK_LOG_SANITIZER
#endif

#ifndef NDEBUG
long GetEnv(const char* envName, long defaultValue); // do not use directly

// Use this macro to get environment variable for log.
// For example: ARK_ENABLED_LOG(ARK_REPORT)
// Will first check if an environment variable "ARK_REPORT" is present,
// and is a valid integer, and use its value. If not present or not a valid
// integer, it will fall back to the default value of the ARK_REPORT
// macro.  This lets the user override configuration at run time, which is useful
// for debugging.
#define ARK_ENABLED_LOG(conf) (panda::GetEnv(#conf, DEFAULT_##conf) == 1)
#else
#define ARK_ENABLED_LOG(conf) (0)
#endif

CString Pretty(uint64_t number) noexcept;
CString PrettyOrderInfo(uint64_t number, const char* unit);
CString PrettyOrderMathNano(uint64_t number, const char* unit);
Level InitLogLevel();

void WriteLog(bool addPrefix, LogType type, const char* format, ...) noexcept;

#define ENABLE_LOG(type) LogFile::LogIsEnabled(type)

#ifndef NDEBUG
#define DLOG(type, format...) \
    if (LogFile::LogIsEnabled(type)) { \
        WriteLog(true, type, format); \
    }
#define VLOG(type, format...) \
    if (LogFile::LogIsEnabled(type)) { \
        WriteLog(true, type, format); \
    }
#else
#define DLOG(type, format...) (void)(0)
#define VLOG(type, format...) \
    if (LogFile::LogIsEnabled(type)) { \
        WriteLog(true, type, format); \
    }
#endif

constexpr size_t DEFAULT_MAX_FILE_SIZE = 10 * 1024 * 1024;
constexpr size_t LOG_BUFFER_SIZE = 1024;

class LogFile {
public:
    LogFile() = default;
    ~LogFile() = default;
    static void Init();
    static void Fini();

    struct LogFileItem {
        bool enableLog = false;
        std::mutex fileMutex;
        FILE* file = nullptr;
        size_t maxFileSize = DEFAULT_MAX_FILE_SIZE;
        size_t curPosLocation = 0;
    };

    static FILE* GetFile(LogType type) { return logFile_[type].file; }

    static void LogFileLock(LogType type) { logFile_[type].fileMutex.lock(); }

    static void LogFileUnLock(LogType type) { logFile_[type].fileMutex.unlock(); }

    static bool LogIsEnabled(LogType type) noexcept
    {
#ifdef PANDA_TARGET_OHOS
        if (type == REPORT) {
            return true;
        }
#endif
        return logFile_[type].enableLog;
    }

    static void EnableLog(LogType type, bool key) { logFile_[type].enableLog = key; }

    static size_t GetMaxFileSize(LogType type) { return logFile_[type].maxFileSize; }

    static size_t GetCurPosLocation(LogType type) { return logFile_[type].curPosLocation; }

    static void SetCurPosLocation(LogType type, size_t curPos) { logFile_[type].curPosLocation = curPos; }

    static Level GetLogLevel() { return logLevel_; }

private:
#ifndef NDEBUG
    static void OpenLogFiles();
#endif
    static void CloseLogFiles();

    static void SetFlags();
    static LogFileItem logFile_[LOG_TYPE_NUMBER];

    static Level logLevel_;
};

#define ARK_COMMON_PHASE_TIMER(...) Timer ARK_pt_##__LINE__(__VA_ARGS__)

class Timer {
public:
    explicit Timer(const CString& pName, LogType type = REPORT) : name_(pName), logType_(type)
    {
        if (ENABLE_LOG(type)) {
            startTime_ = TimeUtil::MicroSeconds();
        }
    }

    ~Timer()
    {
        if (ENABLE_LOG(logType_)) {
            uint64_t stopTime = TimeUtil::MicroSeconds();
            uint64_t diffTime = stopTime - startTime_;
            WriteLog(true, logType_, "%s time: %sus", name_.Str(), Pretty(diffTime).Str());
        }
    }

private:
    CString name_;
    uint64_t startTime_ = 0;
    LogType logType_;
};
} // namespace common
#endif // ARK_COMMON_LOG_FILE_H
