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

#include "common_components/common_runtime/src/base/log_file.h"

#include <unistd.h>

#include "common_components/base/sys_call.h"
#include "securec.h"

namespace common {
LogFile::LogFileItem LogFile::logFile_[LOG_TYPE_NUMBER];

const char* LOG_TYPE_NAMES[LOG_TYPE_NUMBER] = {
    "report",

    "debug",

    "alloc",            "region",       "fragment",

    "barrier",          "ebarrier",     "tbarrier", "pbarrier",     "fbarrier",

    "gcphase",          "enum",         "trace",    "preforward",   "copy", "fix", "finalize",

    "unwind",           "exception",    "signal",

    "arkthread",
#ifdef ARKCOMMON_SANITIZER_SUPPORT
    "sanitizer",
#endif
};

Level LogFile::logLevel_ = InitLogLevel();

void LogFile::Init()
{
    SetFlags();
#ifndef NDEBUG
    OpenLogFiles();
#endif
}

void LogFile::Fini() { CloseLogFiles(); }

void LogFile::SetFlags()
{
#ifndef NDEBUG
    logFile_[REPORT].enableLog = ARK_ENABLED_LOG(ARK_REPORT);
    logFile_[ALLOC].enableLog = ARK_ENABLED_LOG(ARK_LOG_ALLOC);
    logFile_[REGION].enableLog = ARK_ENABLED_LOG(ARK_LOG_REGION);
    logFile_[FRAGMENT].enableLog = ARK_ENABLED_LOG(ARK_LOG_FRAGMENT);
    logFile_[DEBUGY].enableLog = ARK_ENABLED_LOG(ARK_LOG_DEBUG);
    logFile_[BARRIER].enableLog = ARK_ENABLED_LOG(ARK_LOG_BARRIER);
    logFile_[EBARRIER].enableLog = ARK_ENABLED_LOG(ARK_LOG_EBARRIER);
    logFile_[TBARRIER].enableLog = ARK_ENABLED_LOG(ARK_LOG_TBARRIER);
    logFile_[PBARRIER].enableLog = ARK_ENABLED_LOG(ARK_LOG_PBARRIER);
    logFile_[FBARRIER].enableLog = ARK_ENABLED_LOG(ARK_LOG_FBARRIER);
    logFile_[GCPHASE].enableLog = ARK_ENABLED_LOG(ARK_LOG_COMMONPHASE);
    logFile_[ENUM].enableLog = ARK_ENABLED_LOG(ARK_LOG_ENUM);
    logFile_[TRACE].enableLog = ARK_ENABLED_LOG(ARK_LOG_TRACE);
    logFile_[PREFORWARD].enableLog = ARK_ENABLED_LOG(ARK_LOG_PREFORWARD);
    logFile_[COPY].enableLog = ARK_ENABLED_LOG(ARK_LOG_COPY);
    logFile_[FIX].enableLog = ARK_ENABLED_LOG(ARK_LOG_FIX);
    logFile_[FINALIZE].enableLog = ARK_ENABLED_LOG(ARK_LOG_FINALIZE);
    logFile_[UNWIND].enableLog = ARK_ENABLED_LOG(ARK_LOG_UNWIND);
    logFile_[EXCEPTION].enableLog = ARK_ENABLED_LOG(ARK_LOG_EXCEPTION);
    logFile_[SIGNAL].enableLog = ARK_ENABLED_LOG(ARK_LOG_SIGNAL);
    logFile_[ARKTHREAD].enableLog = ARK_ENABLED_LOG(ARK_LOG_THREAD);
#ifdef ARKCOMMON_SANITIZER_SUPPORT
    logFile_[SANITIZER].enableLog = ARK_ENABLED_LOG(ARK_LOG_SANITIZER);
#endif
#endif
}

#ifndef NDEBUG
void LogFile::OpenLogFiles()
{
    CString pid = CString(GetPid());
    CString dateDigit = "";
    CString dirName = ".";

    for (int i = 0; i < LOG_TYPE_NUMBER; ++i) {
        if (logFile_[i].enableLog) {
            if (GetEnv("ARK_LOG_STDOUT", 0) == 1) {
                logFile_[i].file = fopen("/dev/stdout", "w");
                if (logFile_[i].file == nullptr) {
                    LOG_COMMON(ERROR) << "LogFile::OpenLogFiles(): fail to set file /dev/stdout";
                    continue;
                }
            } else {
                CString fileName = dirName + "/" + dateDigit + "_" + pid + "." + LOG_TYPE_NAMES[i] + ".txt";
                LOG_COMMON(INFO) << "create log file " << fileName.Str();
                logFile_[i].file = fopen(fileName.Str(), "a+"); // Assignment closes the old file.
                if (logFile_[i].file == nullptr) {
                    LOG_COMMON(ERROR) << "LogFile::OpenLogFiles(): fail to open the file";
                    continue;
                }
            }
        }
    }
}
#endif

void LogFile::CloseLogFiles()
{
    for (int i = 0; i < LOG_TYPE_NUMBER; ++i) {
        if (logFile_[i].enableLog) {
            logFile_[i].enableLog = false;
            fclose(logFile_[i].file);
        }
    }
}

static bool MaybeRotate(size_t curPos, size_t maxSize, FILE* file)
{
    if (curPos < maxSize) {
        return false;
    }
    (void)fflush(file);
    (void)ftruncate(fileno(file), ftell(file));
    rewind(file);
    return true;
}


static void WriteLogImpl(bool addPrefix, LogType type, const char* format, va_list& args)
{
    char buf[LOG_BUFFER_SIZE];
    if (!LogFile::LogIsEnabled(type)) {
        return;
    }
    int index = 0;
    if (addPrefix) {
        index = sprintf_s(buf, sizeof(buf), "%s %d ", TimeUtil::GetTimestamp().Str(), GetTid());
        if (index == -1) {
            LOG_COMMON(ERROR) << "WriteLogImpl sprintf_s failed. msg: " << strerror(errno);
            return;
        }
    }

    int ret = vsprintf_s(buf + index, sizeof(buf) - index, format, args);
    if (ret == -1) {
        LOG_COMMON(ERROR) << "WriteLogImpl vsprintf_s failed. msg: " << strerror(errno);
        return;
    }
    index += ret;

    LogFile::LogFileLock(type);
#ifdef PANDA_TARGET_OHOS
    auto env = CString(std::getenv("ARK_REPORT"));
    if (env.Str() != nullptr) {
#endif
    FILE* file = LogFile::GetFile(type);
    if (file == nullptr) {
        LOG_COMMON(ERROR) << "WriteLog failed. ARK_REPORT is not a valid path. Please check again.";
        LogFile::LogFileUnLock(type);
        return;
    }
    int err = fprintf(file, "%s\n", buf);
    if ((err - 1) != index) { // 1 = '\n'
        LOG_COMMON(ERROR) << "WriteLogImpl fprintf failed. msg: %s\n", strerror(errno);
        LogFile::LogFileUnLock(type);
        return;
    }
#ifdef PANDA_TARGET_OHOS
    fflush(file);
    }
#else
#ifdef NDEBUG
    size_t curPos = LogFile::GetCurPosLocation(type);
    LogFile::SetCurPosLocation(type, curPos + index);
    if (MaybeRotate(curPos + index, LogFile::GetMaxFileSize(type), file)) {
        LogFile::SetCurPosLocation(type, 0);
    }
#endif
    fflush(file);
#endif
    LogFile::LogFileUnLock(type);
}

void WriteLog(bool addPrefix, LogType type, const char* format, ...) noexcept
{
    va_list args;
    va_start(args, format);
    WriteLogImpl(addPrefix, type, format, args);
    va_end(args);
}

// Orders of magnitudes.  Note: The upperbound of uint64_t is 16E (16 * (1024 ^ 6))
const char* g_orderOfMagnitude[] = { "", "K", "M", "G", "T", "P", "E" };

// Orders of magnitudes.  Note: The upperbound of uint64_t is 16E (16 * (1024 ^ 6))
const char* g_orderOfMagnitudeFromNano[] = { "n", "u", "m", nullptr };

// number of digits in a pretty format segment (100,000,000 each has three digits)
constexpr int NUM_DIGITS_PER_SEGMENT = 3;

CString Pretty(uint64_t number) noexcept
{
    CString orig = CString(number);
    int pos = static_cast<int>(orig.Length()) - NUM_DIGITS_PER_SEGMENT;
    while (pos > 0) {
        orig.Insert(pos, ",");
        pos -= NUM_DIGITS_PER_SEGMENT;
    }
    return orig;
}

// Useful for informatic units, such as KiB, MiB, GiB, ...
CString PrettyOrderInfo(uint64_t number, const char* unit)
{
    size_t order = 0;
    const uint64_t factor = 1024;

    while (number > factor) {
        number /= factor;
        order += 1;
    }

    const char* prefix = g_orderOfMagnitude[order];
    const char* infix = order > 0 ? "i" : ""; // 1KiB = 1024B, but there is no "1iB"

    return CString(number) + prefix + infix + unit;
}

// Useful for scientific units where number is in nanos: ns, us, ms, s
CString PrettyOrderMathNano(uint64_t number, const char* unit)
{
    size_t order = 0;
    const uint64_t factor = 1000; // show in us if under 10ms

    while (number > factor && g_orderOfMagnitudeFromNano[order] != nullptr) {
        number /= factor;
        order += 1;
    }

    const char* prefix = g_orderOfMagnitudeFromNano[order];
    if (prefix == nullptr) {
        prefix = "";
    }

    return CString(number) + prefix + unit;
}

#ifndef NDEBUG
long GetEnv(const char* envName, long defaultValue)
{
    const char* ev = getenv(envName);
    if (ev != nullptr) {
        char* endptr = nullptr;
        long rv = std::strtol(ev, &endptr, 0); // support dec, oct and hex
        if (*endptr == '\0') {
            return rv;
        }
    }

    return defaultValue;
}
#endif

Level InitLogLevel()
{
    auto env = CString(std::getenv("ARK_LOG_LEVEL"));
    if (env.Str() == nullptr) {
        return Level::ERROR;
    }

    CString logLevel = env.RemoveBlankSpace();
    if (logLevel.Length() != 1) {
        LOG_COMMON(ERROR) << "Unsupported in ARK_LOG_LEVEL length. Valid length must be 1."
                    " Valid ARK_LOG_LEVEL must be in ['v', 'd', 'i', 'w', 'e', 'f' 's'].";
        return Level::ERROR;
    }

    switch (logLevel.Str()[0]) {
        case 'v':
            return Level::VERBOSE;
        case 'd':
            return Level::DEBUG;
        case 'i':
            return Level::INFO;
        case 'w':
            return Level::WARN;
        case 'e':
            return Level::ERROR;
        case 'f':
            return Level::FATAL;
        case 's':
            return Level::FATAL_WITHOUT_ABORT;
        default:
            LOG_COMMON(ERROR) << "Unsupported in ARK_LOG_LEVEL. Valid ARK_LOG_LEVEL must be in"
                "['v', 'd', 'i', 'w', 'e', 'f' 's'].\n";
    }
    return Level::ERROR;
}
} // namespace common
