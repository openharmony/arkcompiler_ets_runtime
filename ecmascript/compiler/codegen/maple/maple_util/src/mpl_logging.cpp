/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "mpl_logging.h"
#include <unistd.h>
#include <cstring>
#include <ctime>
#ifndef _WIN32
#include <sys/syscall.h>
#endif
#include "securec.h"

namespace {
constexpr uint32_t kMaxLogLen = 10000;
}

namespace maple {
LogInfo logInfo;
LogInfo &log = logInfo;

const char *kLongLogLevels[] = {
    [kLlDbg] = "D", [kLlLog] = "L", [kLlInfo] = "Info ", [kLlWarn] = "Warn ", [kLlErr] = "Error", [kLlFatal] = "Fatal"};

const char *tags[] = {
    [kLtThread] = "TR",
    [kLtLooper] = "LP",
};

SECUREC_ATTRIBUTE(4, 5)
void LogInfo::EmitLogForUser(enum LogNumberCode num, enum LogLevel ll, const char *fmt, ...) const
{
    char buf[kMaxLogLen];
    int len = snprintf_s(buf, kMaxLogLen, kMaxLogLen - 1, "%s %02d: ", kLongLogLevels[ll], num);
    if (len == -1) {
        WARN(kLncWarn, "snprintf_s failed");
        return;
    }
    if (outMode) {
        va_list l;
        va_start(l, fmt);
        int eNum = vsnprintf_s(buf + len, kMaxLogLen - len, static_cast<size_t>(kMaxLogLen - len - 1), fmt, l);
        if (eNum == -1) {
            WARN(kLncWarn, "vsnprintf_s failed");
            va_end(l);
            return;
        }
        va_end(l);
    }
    std::cerr << buf << '\n';
    return;
}

void LogInfo::EmitLogForUser(enum LogNumberCode num, enum LogLevel ll, const std::string &message) const
{
    EmitLogForUser(num, ll, "%s", message.c_str());
}

SECUREC_ATTRIBUTE(5, 6)
void LogInfo::EmitErrorMessage(const std::string &cond, const std::string &file, unsigned int line, const char *fmt,
                               ...) const
{
    char buf[kMaxLogLen];
#ifdef _WIN32
    int len = snprintf_s(buf, kMaxLogLen, kMaxLogLen - 1, "CHECK/CHECK_FATAL failure: %s at [%s:%d] ", cond.c_str(),
                         file.c_str(), line);
#else
    pid_t tid = syscall(SYS_gettid);
    int len = snprintf_s(buf, kMaxLogLen, kMaxLogLen - 1, "Tid(%d): CHECK/CHECK_FATAL failure: %s at [%s:%d] ", tid,
                         cond.c_str(), file.c_str(), line);
#endif
    if (len == -1) {
        WARN(kLncWarn, "snprintf_s failed");
        return;
    }
    va_list l;
    va_start(l, fmt);
    int eNum = vsnprintf_s(buf + len, kMaxLogLen - len, static_cast<size_t>(kMaxLogLen - len - 1), fmt, l);
    if (eNum == -1) {
        WARN(kLncWarn, "vsnprintf_s failed");
        va_end(l);
        return;
    }
    va_end(l);
    std::cerr << buf;
}

std::ostream &LogInfo::MapleLogger(LogLevel level)
{
    if (level == kLlLog) {
        return std::cout;
    }
    return std::cerr;
}
}  // namespace maple
