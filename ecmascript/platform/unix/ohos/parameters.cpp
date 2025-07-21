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

#include "ecmascript/platform/parameters.h"

#if !defined(STANDALONE_MODE)
#include "parameters.h"
#endif

namespace panda::ecmascript {
    uint64_t GetImportDuration(uint64_t defaultTime)
    {
#if !defined(STANDALONE_MODE)
        return OHOS::system::GetUintParameter<uint64_t>("persist.ark.importDuration",
                                                        defaultTime);
#else
        return defaultTime;
#endif
    }

    size_t GetPoolSize(size_t defaultSize)
    {
#if !defined(STANDALONE_MODE)
        return OHOS::system::GetUintParameter<size_t>("persist.ark.heap.poolsize", defaultSize / 1_MB) * 1_MB;
#else
        return defaultSize;
#endif
    }

    bool IsEnableCMCGC(bool defaultValue)
    {
#if !defined(STANDALONE_MODE)
        return OHOS::system::GetBoolParameter("persist.ark.enable.cmc.gc", defaultValue);
#else
        return defaultValue;
#endif
    }

    uint64_t GetCMCMaxGarbageCacheSize(uint64_t defaultSize)
    {
#if !defined(STANDALONE_MODE)
        return OHOS::system::GetUintParameter<uint64_t>("persist.ark.cmc.max.garbage.cache.size", defaultSize);
#else
        return defaultSize;
#endif
    }
}  // namespace panda::ecmascript
