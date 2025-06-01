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

#ifndef COMMON_COMPONENTS_BASE_RUNTIME_BASE_RUNTIME_PARAM_H
#define COMMON_COMPONENTS_BASE_RUNTIME_BASE_RUNTIME_PARAM_H

#include "common_interfaces/base/common.h"
#include "common_interfaces/base/runtime_param.h"

namespace panda {
class BaseRuntimeParam {
public:
    static RuntimeParam DefaultRuntimeParam();
    static size_t InitHeapSize();

private:
    BaseRuntimeParam() = delete;
    ~BaseRuntimeParam() = delete;
    NO_COPY_SEMANTIC_CC(BaseRuntimeParam);
    NO_MOVE_SEMANTIC_CC(BaseRuntimeParam);
};

#if defined(PANDA_TARGET_OHOS)
#define RUNTIME_PARAM_LIST(V)                                                                               \
    /*  KEY         SUB_KEY                 TYPE        MIN     MAX         DEFAULT        */ /*  UNIT */   \
    V(heapParam,    heapSize,               size_t,     65536,  4194304,                                    \
                                                        BaseRuntimeParam::InitHeapSize()    ) /*    KB */;  \
    V(heapParam,    regionSize,             size_t,     4,      2048,       1024            ) /*    KB */;  \
    V(heapParam,    exemptionThreshold,     double,     0.0,    1.0,        0.8             ) /*     % */;  \
    V(heapParam,    heapUtilization,        double,     0.0,    1.0,        0.8             ) /*     % */;  \
    V(heapParam,    heapGrowth,             double,     0.0,    INT64_MAX,  1.15            ) /* times */;  \
    V(heapParam,    allocationRate,         double,     0.0,    INT64_MAX,  10240           ) /*  rate */;  \
    V(heapParam,    allocationWaitTime,     uint64_t,   0,      INT64_MAX,  1000            ) /*    ns */;  \
    V(gcParam,      enableGC,               bool,       0,      1,          1                ) /*   y/n */; \
    V(gcParam,      enableStwGC,            bool,       0,      1,          0                ) /*   y/n */; \
    V(gcParam,      gcThreads,              uint32_t,   1,      32,         5               ) /*   NUM */;  \
    V(gcParam,      garbageThreshold,       double,     0.1,    1.0,        0.5             ) /*     % */;; \
    V(gcParam,      gcThreshold,            size_t,     0,      INT64_MAX,                                  \
                                                        BaseRuntimeParam::InitHeapSize()    ) /*    KB */;  \
    V(gcParam,      gcInterval,             uint64_t,   0,      INT64_MAX,  150000          ) /*    us */;  \
    V(gcParam,      backupGCInterval,       uint64_t,   0,      INT64_MAX,  240000          ) /*    ms */
#else  // PANDA_TARGET_OHOS
#define RUNTIME_PARAM_LIST(V)                                                                               \
    /*  KEY         SUB_KEY                 TYPE        MIN     MAX         DEFAULT        */ /*  UNIT */   \
    V(heapParam,    heapSize,               size_t,     4096,   4194304,                                    \
                                                        BaseRuntimeParam::InitHeapSize()    ) /*    KB */;  \
    V(heapParam,    regionSize,             size_t,     4,      2048,       64              ) /*    KB */;  \
    V(heapParam,    exemptionThreshold,     double,     0.0,    1.0,        0.8             ) /*     % */;  \
    V(heapParam,    heapUtilization,        double,     0.0,    1.0,        0.8             ) /*     % */;  \
    V(heapParam,    heapGrowth,             double,     0.0,    INT64_MAX,  1.15            ) /* times */;  \
    V(heapParam,    allocationRate,         double,     0.0,    INT64_MAX,  10240           ) /*  rate */;  \
    V(heapParam,    allocationWaitTime,     uint64_t,   0,      INT64_MAX,  1000            ) /*    ns */;  \
    V(gcParam,      enableGC,               bool,       0,      1,          1               ) /*   y/n */;  \
    V(gcParam,      enableStwGC,            bool,       0,      1,          0               ) /*   y/n */;  \
    V(gcParam,      gcThreads,              uint32_t,   1,      64,         5               ) /*   NUM */;  \
    V(gcParam,      garbageThreshold,       double,     0.1,    1.0,        0.5             ) /*     % */;  \
    V(gcParam,      gcThreshold,            size_t,     0,      INT64_MAX,                                  \
                                                        BaseRuntimeParam::InitHeapSize()    ) /*    KB */;  \
    V(gcParam,      gcInterval,             uint64_t,   0,      INT64_MAX,  150000          ) /*    us */;  \
    V(gcParam,      backupGCInterval,       uint64_t,   0,      INT64_MAX,  240000          ) /*    ms */
#endif  // PANDA_TARGET_OHOS
} // namespace panda

#endif // COMMON_COMPONENTS_BASE_RUNTIME_BASE_RUNTIME_PARAM_H
