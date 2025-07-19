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
#ifndef ECMASCRIPT_CROSS_VM_BUILTINS_GC_HYBRID_H
#define ECMASCRIPT_CROSS_VM_BUILTINS_GC_HYBRID_H

#define BUILDINSGC_PUBLIC_HYBRID_EXTENSION() \
    static JSTaggedValue ClearWeakRefForTest(EcmaRuntimeCallInfo *info);    \
    static Span<const base::BuiltinFunctionEntry> GetGcFunctionsHybrid()    \
    {                                                                       \
        return Span<const base::BuiltinFunctionEntry>(GC_FUNCTIONS_HYBRID); \
    }

#define BUILTINSGC_PUBLIC_FUNCTION_ENTRY_HYBRID_EXTENSION()                                                  \
    static constexpr std::array GC_FUNCTIONS_HYBRID = {                                                      \
        BUILTINS_GC_FUNCTION_ENTRY("clearWeakRefForTest",         ClearWeakRefForTest,           1, INVALID) \
    }
    

#endif  // ECMASCRIPT_CROSS_VM_BUILTINS_GC_HYBRID_H