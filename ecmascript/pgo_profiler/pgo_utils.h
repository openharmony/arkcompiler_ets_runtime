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

#ifndef ECMASCRIPT_PGO_PROFILER_PGO_UTILS_H
#define ECMASCRIPT_PGO_PROFILER_PGO_UTILS_H

#include <string>

#include "ecmascript/common.h"
#include "ecmascript/log.h"
#include "ecmascript/log_wrapper.h"
#include "libpandafile/file.h"
#include "mem/mem.h"

namespace panda::ecmascript::pgo {
static constexpr Alignment ALIGN_SIZE = Alignment::LOG_ALIGN_4;
using PGOMethodId = panda_file::File::EntityId;
using ApEntityId = uint32_t;

class DumpUtils {
public:
    static const std::string ELEMENT_SEPARATOR;
    static const std::string BLOCK_SEPARATOR;
    static const std::string TYPE_SEPARATOR;
    static const std::string BLOCK_START;
    static const std::string ARRAY_START;
    static const std::string ARRAY_END;
    static const std::string NEW_LINE;
    static const std::string SPACE;
    static const std::string BLOCK_AND_ARRAY_START;
    static const std::string VERSION_HEADER;
    static const std::string PANDA_FILE_INFO_HEADER;
    static const uint32_t HEX_FORMAT_WIDTH_FOR_32BITS;
};

class ApNameUtils {
public:
    static const std::string AP_SUFFIX;
    static const std::string RUNTIME_AP_PREFIX;
    static const std::string MERGED_AP_PREFIX;
    static const std::string DEFAULT_AP_NAME;
    static std::string PUBLIC_API GetRuntimeApName(const std::string &ohosModuleName);
    static std::string PUBLIC_API GetMergedApName(const std::string &ohosModuleName);
    static std::string PUBLIC_API GetOhosPkgApName(const std::string &ohosModuleName);

private:
    static std::string GetBriefApName(const std::string &ohosModuleName);
};

struct ConcurrentGuardValues {
    mutable std::atomic_int last_tid_ {0};
    mutable std::atomic_int count_ {0};
};

class ConcurrentGuard {
public:
    ConcurrentGuard(ConcurrentGuardValues& v): v_(v)
    {
        auto tid = Gettid();
        auto except = 0;
        if (!v_.count_.compare_exchange_strong(except, 1)) {
            LOG_ECMA(FATAL) << "[ConcurrentGuard] total thead count should be 0, but get " << except
                            << ", current tid: " << tid << ", last tid: " << v_.last_tid_;
        }
        v_.last_tid_ = tid;
    }
    ~ConcurrentGuard()
    {
        auto tid = Gettid();
        auto except = 1;
        if (!v_.count_.compare_exchange_strong(except, 0)) {
            LOG_ECMA(FATAL) << "[ConcurrentGuard] total thead count should be 1, but get " << except
                            << ", current tid: " << tid << ", last tid: " << v_.last_tid_;
        }
    };

private:
    ConcurrentGuardValues& v_;
    int Gettid()
    {
        return os::thread::GetCurrentThreadId();
    }
};
}  // namespace panda::ecmascript::pgo
#endif  // ECMASCRIPT_PGO_PROFILER_PGO_UTILS_H