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
#ifndef ECMASCRIPT_CROSS_VM_JS_THREAD_HYBRID_H
#define ECMASCRIPT_CROSS_VM_JS_THREAD_HYBRID_H

#define JSTHREAD_PUBLIC_HYBRID_EXTENSION()                                 \
    inline uintptr_t NewXRefGlobalHandle(JSTaggedType value)               \
    {                                                                      \
        return newXRefGlobalHandle_(value);                                \
    }                                                                      \
    inline void DisposeXRefGlobalHandle(uintptr_t nodeAddr)                \
    {                                                                      \
        disposeXRefGlobalHandle_(nodeAddr);                                \
    }                                                                      \
    inline void SetNodeKind(NodeKind nodeKind)                             \
    {                                                                      \
        setNodeKind_(nodeKind);                                            \
    }                                                                      \
    void SetStackStart(uint64_t stackStart)                                \
    {                                                                      \
        glueData_.stackStart_ = stackStart;                                \
    }                                                                      \
    void SetStackLimit(uint64_t stackLimit)                                \
    {                                                                      \
        glueData_.stackLimit_ = stackLimit;                                \
    }                                                                      \
    void SetupXRefFields()

#define JSTHREAD_PRIVATE_HYBRID_EXTENSION()                              \
    std::function<uintptr_t(JSTaggedType value)> newXRefGlobalHandle_;   \
    std::function<void(uintptr_t nodeAddr)> disposeXRefGlobalHandle_;    \
    std::function<void(NodeKind nodeKind)> setNodeKind_
#endif  // ECMASCRIPT_CROSS_VM_JS_THREAD_HYBRID_H