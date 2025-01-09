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

#include <list>
#include <string>

#include "ecmascript/common.h"
#include "ecmascript/log.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/platform/mutex.h"
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
    static const std::string ALIGN;
    static const std::string BLOCK_AND_ARRAY_START;
    static const std::string VERSION_HEADER;
    static const std::string PANDA_FILE_INFO_HEADER;
    static const uint32_t HEX_FORMAT_WIDTH_FOR_32BITS;
};

class DumpJsonUtils {
public:
    static inline const std::string ABC_FILE_POOL = "abcFilePool";
    static inline const std::string ABC_FILE = "abcFile";
    static inline const std::string RECORD_DETAIL = "recordDetail";
    static inline const std::string MODULE_NAME = "moduleName";
    static inline const std::string FUNCTION = "function";
    static inline const std::string FUNCTION_NAME = "functionName";
    static inline const std::string TYPE = "type";
    static inline const std::string TYPE_OFFSET = "typeOffset";
    static inline const std::string TYPE_NAME = "typeName";
    static inline const std::string IS_ROOT = "isRoot";
    static inline const std::string KIND = "kind";
    static inline const std::string ABC_ID = "abcId";
    static inline const std::string ID = "id";
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

class ConcurrentGuardValue {
public:
    mutable std::atomic_int last_tid {0};
    mutable std::atomic_int count {0};

    int Gettid()
    {
        return os::thread::GetCurrentThreadId();
    }
};

class ConcurrentGuard {
private:
    std::string operation_;

public:
    ConcurrentGuard(ConcurrentGuardValue& v, std::string operation = ""): operation_(operation), v_(v)
    {
        auto tid = v_.Gettid();
        auto except = 0;
        auto desired = 1;
        if (!v_.count.compare_exchange_strong(except, desired) && v_.last_tid != tid) {
            LOG_ECMA(FATAL) << "[" << operation_ << "] total thead count should be 0, but get " << except
                            << ", current tid: " << tid << ", last tid: " << v_.last_tid;
        }
        v_.last_tid = tid;
    }
    ~ConcurrentGuard()
    {
        auto tid = v_.Gettid();
        auto except = 1;
        auto desired = 0;
        if (!v_.count.compare_exchange_strong(except, desired) && v_.last_tid != tid) {
            LOG_ECMA(FATAL) << "[" << operation_ << "] total thead count should be 1, but get " << except
                            << ", current tid: " << tid << ", last tid: " << v_.last_tid;
        }
    };

private:
    ConcurrentGuardValue& v_;
};
}  // namespace panda::ecmascript::pgo
#endif  // ECMASCRIPT_PGO_PROFILER_PGO_UTILS_H
