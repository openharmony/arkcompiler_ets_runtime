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
    static RuntimeParam InitRuntimeParam();

private:
    BaseRuntimeParam() = delete;
    ~BaseRuntimeParam() = delete;
    NO_COPY_SEMANTIC_CC(BaseRuntimeParam);
    NO_MOVE_SEMANTIC_CC(BaseRuntimeParam);

    static void CheckSysmemSize();
    static size_t InitHeapSize(size_t defaultParam);
    static size_t InitRegionSize(size_t defaultParam);
    static double InitGarbageRatio(double defaultParam);
    static double InitPercentParameter(const char* name, double minSize, double maxSize, double defaultParam);
    static size_t InitSizeParameter(const char* name, size_t minSize, size_t defaultParam);
    static size_t InitTimeParameter(const char* name, size_t minSize, size_t defaultParam);
    static double InitDecParameter(const char* name, double minSize, double defaultParam);

    static size_t sysMemSize_;
};
} // namespace panda

#endif // COMMON_COMPONENTS_BASE_RUNTIME_BASE_RUNTIME_PARAM_H
