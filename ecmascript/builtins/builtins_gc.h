/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_GC_H
#define ECMASCRIPT_BUILTINS_BUILTINS_GC_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/cross_vm/builtins_gc_hybrid.h"
#include "ecmascript/js_thread.h"

namespace panda::ecmascript::builtins {
class BuiltinsGc : public base::BuiltinsBase {
public:
    static JSTaggedValue GetFreeHeapSize(EcmaRuntimeCallInfo *info);

    static JSTaggedValue GetReservedHeapSize(EcmaRuntimeCallInfo *info);

    static JSTaggedValue GetUsedHeapSize(EcmaRuntimeCallInfo *info);

    static JSTaggedValue GetObjectAddress(EcmaRuntimeCallInfo *info);

    static JSTaggedValue GetObjectSpaceType(EcmaRuntimeCallInfo *info);

    static JSTaggedValue RegisterNativeAllocation(EcmaRuntimeCallInfo *info);

    static JSTaggedValue RegisterNativeFree(EcmaRuntimeCallInfo *info);

    static JSTaggedValue WaitForFinishGC(EcmaRuntimeCallInfo *info);

    static JSTaggedValue StartGC(EcmaRuntimeCallInfo *info);

    static JSTaggedValue AllocateArrayObject(EcmaRuntimeCallInfo *info);

    static Span<const base::BuiltinFunctionEntry> GetGcFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(GC_FUNCTIONS);
    }

    BUILDINSGC_PUBLIC_HYBRID_EXTENSION()

private:
#define BUILTINS_GC_FUNCTION_ENTRY(name, method, length, id) \
    base::BuiltinFunctionEntry::Create(name, BuiltinsGc::method, length, BUILTINS_STUB_ID(id)),

// List of functions in ArkTools.GC, extension of JS engine.
// where BuiltinsGc::func refers to the native implementation of GC[name].
//       kungfu::BuiltinsStubCSigns::stubIndex refers to the builtin stub index, or INVALID if no stub available.

    static constexpr std::array GC_FUNCTIONS = {
        BUILTINS_GC_FUNCTION_ENTRY("getFreeHeapSize",             GetFreeHeapSize,               0, INVALID)
        BUILTINS_GC_FUNCTION_ENTRY("getReservedHeapSize",         GetReservedHeapSize,           0, INVALID)
        BUILTINS_GC_FUNCTION_ENTRY("getUsedHeapSize",             GetUsedHeapSize,               0, INVALID)
        BUILTINS_GC_FUNCTION_ENTRY("getObjectAddress",            GetObjectAddress,              1, INVALID)
        BUILTINS_GC_FUNCTION_ENTRY("getObjectSpaceType",          GetObjectSpaceType,            1, INVALID)
        BUILTINS_GC_FUNCTION_ENTRY("registerNativeAllocation",    RegisterNativeAllocation,      1, INVALID)
        BUILTINS_GC_FUNCTION_ENTRY("registerNativeFree",          RegisterNativeFree,            1, INVALID)
        BUILTINS_GC_FUNCTION_ENTRY("waitForFinishGC",             WaitForFinishGC,               1, INVALID)
        BUILTINS_GC_FUNCTION_ENTRY("startGC",                     StartGC,                       3, INVALID)
        BUILTINS_GC_FUNCTION_ENTRY("allocateArrayObject",         AllocateArrayObject,           1, INVALID)
    };

    BUILTINSGC_PUBLIC_FUNCTION_ENTRY_HYBRID_EXTENSION();
#undef BUILTINS_GC_FUNCTION_ENTRY

    static void WaitAndHandleConcurrentMarkingFinished(Heap *heap);
    static TriggerGCType StringToGcType(JSThread *thread, JSTaggedValue cause);
};
}  // namespace panda::ecmascript::builtins

#endif  // ECMASCRIPT_BUILTINS_BUILTINS_GC_H
