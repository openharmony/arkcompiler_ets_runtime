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

#include "crt.h"

#include <cstddef>
#include <cstdint>
#include <dlfcn.h>

#include "common_components/common_runtime/src/heap/allocator/region_desc.h"
#include "ecmascript/mem/cmc_gc/gc_hooks.h"

namespace panda {

CommonRuntime* CommonRuntime::instance_ = nullptr;
size_t CommonRuntime::LARGE_OBJECT_THRESHOLD = 0;

void CommonRuntime::Create()
{
    instance_ = new CommonRuntime();

    LARGE_OBJECT_THRESHOLD = RegionDesc::LARGE_OBJECT_DEFAULT_THRESHOLD;
}

void CommonRuntime::Destroy()
{
    delete instance_;
}

void CommonRuntime::Init()
{
    // Runtime will check is it has initilized
    ArkRegisterSetBaseAddrHook(SetBaseAddress);
    ArkCommonRuntimeInit();
    ArkRegisterFillFreeObjectHook(FillFreeObject);
    ArkRegisterJSGCCallbackHook(JSGCCallback);
}

void CommonRuntime::Fini()
{
    ArkCommonRuntimeFini();
    ArkRegisterEnumThreadStackRootHook(nullptr);
    ArkRegisterEnumStaticRootsHook(nullptr);
    ArkRegisterFillFreeObjectHook(nullptr);
    RegisterPreforwardStaticRootsHook(nullptr);
}
} // namespace panda
