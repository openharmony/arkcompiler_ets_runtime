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
#include "ecmascript/js_thread.h"

// List of functions in ArkTools.GC, extension of JS engine.
// V(name, func, length, stubIndex)
// where BuiltinsGc::func refers to the native implementation of GC[name].
//       kungfu::BuiltinsStubCSigns::stubIndex refers to the builtin stub index, or INVALID if no stub available.
#define BUILTIN_GC_FUNCTIONS(V)                                             \
    V("getFreeHeapSize",                           GetFreeHeapSize,                           0, INVALID)     \
    V("getReservedHeapSize",                       GetReservedHeapSize,                       0, INVALID)     \
    V("getUsedHeapSize",                           GetUsedHeapSize,                           0, INVALID)     \
    V("getObjectAddress",                          GetObjectAddress,                          1, INVALID)     \
    V("getObjectSpaceType",                        GetObjectSpaceType,                        1, INVALID)     \
    V("registerNativeAllocation",                  RegisterNativeAllocation,                  1, INVALID)     \
    V("registerNativeFree",                        RegisterNativeFree,                        1, INVALID)

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

    static Span<const base::BuiltinFunctionEntry> GetGcFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(GC_FUNCTIONS);
    }

private:
#define BUILTINS_GC_FUNCTION_ENTRY(name, method, length, id) \
    base::BuiltinFunctionEntry::Create(name, BuiltinsGc::method, length, kungfu::BuiltinsStubCSigns::id),

    static constexpr std::array GC_FUNCTIONS  = {
        BUILTIN_GC_FUNCTIONS(BUILTINS_GC_FUNCTION_ENTRY)
    };
#undef BUILTINS_GC_FUNCTION_ENTRY
};
}  // namespace panda::ecmascript::builtins

#endif  // ECMASCRIPT_BUILTINS_BUILTINS_GC_H
