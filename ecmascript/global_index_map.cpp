/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/ecma_context.h"
#include "ecmascript/global_env.h"
#include "ecmascript/global_index_map.h"

namespace panda::ecmascript {
void GlobalIndexMap::Initialize(const JSThread *thread,
                                JSMutableHandle<PointerToIndexDictionary> globalIndexMap)
{
    // GlobalIndex map should be created when it is first used.
    InitGlobalIndexMap(thread, globalIndexMap);

    // Init GlobalIndex map
    InitGlobalConst(thread, globalIndexMap);
    InitGlobalEnv(thread, globalIndexMap);
    InitBuiltinEntries(thread, globalIndexMap);
}

void GlobalIndexMap::InitGlobalIndexMap(const JSThread *thread,
                                        JSMutableHandle<PointerToIndexDictionary> globalIndexMap)
{
    if (globalIndexMap.GetTaggedValue().IsHeapObject()) {
        return;
    }
    globalIndexMap.Update(PointerToIndexDictionary::Create(thread));
}

void GlobalIndexMap::InitGlobalConst(const JSThread *thread,
                                     JSMutableHandle<PointerToIndexDictionary> globalIndexMap)
{
    ASSERT_PRINT(globalIndexMap.GetTaggedValue().IsHeapObject(), "Global's IndexMap is not existed.");
    auto globalConst = const_cast<GlobalEnvConstants *>(thread->GlobalConstants());
    uint32_t constantCount = globalConst->GetConstantCount();
    JSMutableHandle<PointerToIndexDictionary> globalIndexMapHandle(thread, globalIndexMap);
    JSMutableHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> valueHandle(thread, JSTaggedValue::Undefined());
    for (uint32_t index = 0; index < constantCount; index++) {
        JSTaggedValue objectValue = globalConst->GetGlobalConstantObject(index);
        if (objectValue.IsHeapObject() && !objectValue.IsString()) {
            keyHandle.Update(objectValue);

            GlobalIndex globalIndex;
            globalIndex.UpdateGlobalConstId(index);
            valueHandle.Update(JSTaggedValue(globalIndex.GetGlobalIndex()));
            JSHandle<PointerToIndexDictionary> newDict =
                PointerToIndexDictionary::PutIfAbsent(thread, globalIndexMapHandle, keyHandle, valueHandle);
            globalIndexMapHandle.Update(newDict);
        }
    }
    globalIndexMap.Update(globalIndexMapHandle);
}

void GlobalIndexMap::InitGlobalEnv(const JSThread *thread,
                                   JSMutableHandle<PointerToIndexDictionary> globalIndexMap)
{
    ASSERT_PRINT(globalIndexMap.GetTaggedValue().IsHeapObject(), "Global's IndexMap is not existed.");
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    uint32_t globalEnvFieldSize = globalEnv->GetGlobalEnvFieldSize();
    JSMutableHandle<PointerToIndexDictionary> globalIndexMapHandle(thread, globalIndexMap);
    JSMutableHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> valueHandle(thread, JSTaggedValue::Undefined());
    for (uint32_t index = 0; index < globalEnvFieldSize; index++) {
        JSTaggedValue objectValue = globalEnv->GetGlobalEnvObjectByIndex(index).GetTaggedValue();
        if (objectValue.IsHeapObject()) {
            keyHandle.Update(objectValue);

            GlobalIndex globalIndex;
            globalIndex.UpdateGlobalEnvId(index);
            valueHandle.Update(JSTaggedValue(globalIndex.GetGlobalIndex()));
            JSHandle<PointerToIndexDictionary> newDict =
                PointerToIndexDictionary::PutIfAbsent(thread, globalIndexMapHandle, keyHandle, valueHandle);
            globalIndexMapHandle.Update(newDict);
        }
    }
    globalIndexMap.Update(globalIndexMapHandle);
}

void GlobalIndexMap::InitBuiltinEntries(const JSThread *thread,
                                        JSMutableHandle<PointerToIndexDictionary> globalIndexMap)
{
    ASSERT_PRINT(globalIndexMap.GetTaggedValue().IsHeapObject(), "Global's IndexMap is not existed.");
    auto builtinEntries = thread->GetBuiltinEntries();
    uint32_t builtinEntriesCount = BuiltinEntries::COUNT;
    JSMutableHandle<PointerToIndexDictionary> globalIndexMapHandle(thread, globalIndexMap);
    JSMutableHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> valueHandle(thread, JSTaggedValue::Undefined());
    for (uint32_t index = 0; index < builtinEntriesCount; index++) {
        JSTaggedValue objectValue = builtinEntries.builtin_[index].hClass_;
        keyHandle.Update(objectValue);
        if (objectValue.IsHeapObject()) {
            GlobalIndex globalIndex;
            globalIndex.UpdateBuiltinEntriesId(index);
            valueHandle.Update(JSTaggedValue(globalIndex.GetGlobalIndex()));
            JSHandle<PointerToIndexDictionary> newDict =
                PointerToIndexDictionary::PutIfAbsent(thread, globalIndexMapHandle, keyHandle, valueHandle);
            globalIndexMapHandle.Update(newDict);
        }
    }
    globalIndexMap.Update(globalIndexMapHandle);
}

void GlobalIndexMap::FindGlobalIndex(JSHandle<PointerToIndexDictionary> globalIndexMap,
                                     JSTaggedValue objAddress, GlobalIndex *globalIndex)
{
    int entry = globalIndexMap->FindEntry(objAddress);
    if (entry != -1) {
        *globalIndex = GlobalIndex(globalIndexMap->GetValue(entry).GetInt());
    }
}
} // namespace panda::ecmascript
