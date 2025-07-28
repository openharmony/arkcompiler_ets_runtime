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
#ifndef ECMASCRIPT_CROSS_VM_HEAP_HYBRID_H
#define ECMASCRIPT_CROSS_VM_HEAP_HYBRID_H

#define BASEHEAP_PUBLIC_HYBRID_EXTENSION()                                                                        \
    void SetGCType(TriggerGCType gcType)                                                                          \
    {                                                                                                             \
        gcType_ = gcType;                                                                                         \
    }                                                                                                             \
    uint32_t GetRunningTaskCount()

#define SHAREDHEAP_PUBLIC_HYBRID_EXTENSION()                                                                      \
    UnifiedGC *GetUnifiedGC() const                                                                               \
    {                                                                                                             \
        return unifiedGC_;                                                                                        \
    }                                                                                                             \
    template<TriggerGCType gcType, GCReason gcReason>                                                             \
    bool TriggerUnifiedGCMark(JSThread *thread) const;                                                            \
    void StartUnifiedGCMark(TriggerGCType gcType, GCReason gcReason);                                             \
    void CreateUnifiedGC()


#define SHAREDHEAP_PRIVATE_HYBRID_EXTENSION()                                                                  \
    UnifiedGC *unifiedGC_ {nullptr}


#define HEAP_PUBLIC_HYBRID_EXTENSION()                                                                          \
    void UnifiedGCPrepare();                                                                                  \
    UnifiedGCMarker *GetUnifiedGCMarker() const                                                               \
    {                                                                                                         \
        return unifiedGCMarker_;                                                                              \
    }                                                                                                         \
    bool IsUnifiedGC() const                                                                                  \
    {                                                                                                         \
        return gcType_ == TriggerGCType::UNIFIED_GC;                                                          \
    }                                                                                                         \
    void CreateUnifiedGCMarker()

#define HEAP_PRIVATE_HYBRID_EXTENSION()                                                                        \
    UnifiedGCMarker *unifiedGCMarker_ {nullptr}

#endif  // ECMASCRIPT_CROSS_VM_HEAP_HYBRID_H
