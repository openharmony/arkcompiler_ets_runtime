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
#ifndef ECMASCRIPT_CROSS_VM_GLOBAL_ENV_CONSTANTS_HYBRID_H
#define ECMASCRIPT_CROSS_VM_GLOBAL_ENV_CONSTANTS_HYBRID_H

#define SHARED_GLOBAL_ENV_CONSTANT_STRING_HYBRID(V)                                                         \
    V(ProxyNapiWrapperString,         PROXY_NAPI_WRAPPER_INDEX,              "_proxynapiwrapper")           \
    V(UnifiedGcCause,                 UNIFIED_GC_CAUSE,                      "unified")

#define GLOBAL_ENV_CONSTANT_CLASS_HYBRID(V)                                        \
    V(JSTaggedValue, XRefObjectClass, XREF_OBJECT_HCLASS_INDEX, ecma_roots_class)

#endif  // ECMASCRIPT_CROSS_VM_GLOBAL_ENV_CONSTANTS_HYBRID_H
