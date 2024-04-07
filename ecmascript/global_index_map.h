/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_GLOBAL_INDEX_MAP_H
#define ECMASCRIPT_GLOBAL_INDEX_MAP_H

#include "utils/bit_field.h"
#include "ecmascript/global_index.h"
#include "ecmascript/tagged_dictionary.h"

namespace panda::ecmascript {
// HeapObject's Pointer To IndexHashmap
class GlobalIndexMap {
public:
    static void Initialize(const JSThread *thread,
                           JSMutableHandle<PointerToIndexDictionary> globalIndexMap);
    static void InitGlobalIndexMap(const JSThread *thread,
                                   JSMutableHandle<PointerToIndexDictionary> globalIndexMap);
    static void InitGlobalConst(const JSThread *thread,
                                JSMutableHandle<PointerToIndexDictionary> globalIndexMap);
    static void InitGlobalEnv(const JSThread *thread,
                              JSMutableHandle<PointerToIndexDictionary> globalIndexMap);
    static void InitBuiltinEntries(const JSThread *thread,
                                   JSMutableHandle<PointerToIndexDictionary> globalIndexMap);
    static void FindGlobalIndex(JSHandle<PointerToIndexDictionary> globalIndexMap,
                                JSTaggedValue obj, GlobalIndex *globalIndex);
    static inline JSHandle<PointerToIndexDictionary> GetGlobalIndexMap(EcmaContext *context)
    {
        return JSHandle<PointerToIndexDictionary>(reinterpret_cast<uintptr_t>(&context->pointerToIndexDictionary_));
    }
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_GLOBAL_INDEX_MAP_H