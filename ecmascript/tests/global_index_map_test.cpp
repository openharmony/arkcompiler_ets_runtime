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

#include "ecmascript/builtin_entries.h"
#include "ecmascript/ecma_context.h"
#include "ecmascript/global_env.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/global_index.h"
#include "ecmascript/global_index_map.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class GlobalIndexMapTest :  public BaseTestWithScope<false> {
};

/**
 * @tc.name: InitGlobalIndexMap
 * @tc.desc: Check whether InitGlobalIndexMap can initialize dictionary.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(GlobalIndexMapTest, GlobalIndexMap_initGlobalIndexMap)
{
    JSMutableHandle<PointerToIndexDictionary> globalIndexMap(
        GlobalIndexMap::GetGlobalIndexMap(thread->GetCurrentEcmaContext()).GetAddress());
    EXPECT_NE(globalIndexMap.GetTaggedValue().IsHeapObject(), true);
    GlobalIndexMap::InitGlobalIndexMap(thread, globalIndexMap);
    globalIndexMap.Update(GlobalIndexMap::GetGlobalIndexMap(thread->GetCurrentEcmaContext()));
    EXPECT_EQ(globalIndexMap.GetTaggedValue().IsHeapObject(), true);
}

/**
 * @tc.name: InitGlobalConst
 * @tc.desc: Check whether GlobalConst can be find in the GlobalMap.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(GlobalIndexMapTest, GlobalIndexMap_initGlobalConst)
{
    JSMutableHandle<PointerToIndexDictionary> globalIndexMap(
        GlobalIndexMap::GetGlobalIndexMap(thread->GetCurrentEcmaContext()).GetAddress());
    EXPECT_NE(globalIndexMap.GetTaggedValue().IsHeapObject(), true);
    GlobalIndexMap::InitGlobalIndexMap(thread, globalIndexMap);
    GlobalIndexMap::InitGlobalConst(thread, globalIndexMap);
    globalIndexMap.Update(GlobalIndexMap::GetGlobalIndexMap(thread->GetCurrentEcmaContext()));
    EXPECT_EQ(globalIndexMap.GetTaggedValue().IsHeapObject(), true);
    auto globalConst = const_cast<GlobalEnvConstants *>(thread->GlobalConstants());
    uint32_t constantCount = globalConst->GetConstantCount();
    for (uint32_t index = 0; index < constantCount; index++) {
        JSTaggedValue objectValue = globalConst->GetGlobalConstantObject(index);
        if (objectValue.IsHeapObject() && !objectValue.IsString()) {
            GlobalIndex rootIndex;
            GlobalIndexMap::FindGlobalIndex(globalIndexMap, objectValue, &rootIndex);
            EXPECT_EQ(static_cast<int>(index), rootIndex.GetGlobalConstId());
            GlobalIndex globalConstGlobalIndex;
            globalConstGlobalIndex.UpdateGlobalConstId(index);
            EXPECT_EQ(globalConstGlobalIndex, rootIndex);
        }
    }
}

/**
 * @tc.name: InitGlobalEnv
 * @tc.desc: Check whether GlobalEnv can be find in the GlobalMap.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(GlobalIndexMapTest, GlobalIndexMap_initGlobalEnv)
{
    JSMutableHandle<PointerToIndexDictionary> globalIndexMap(
        GlobalIndexMap::GetGlobalIndexMap(thread->GetCurrentEcmaContext()).GetAddress());
    EXPECT_NE(globalIndexMap.GetTaggedValue().IsHeapObject(), true);
    GlobalIndexMap::InitGlobalIndexMap(thread, globalIndexMap);
    GlobalIndexMap::InitGlobalEnv(thread, globalIndexMap);
    globalIndexMap.Update(GlobalIndexMap::GetGlobalIndexMap(thread->GetCurrentEcmaContext()));
    EXPECT_EQ(globalIndexMap.GetTaggedValue().IsHeapObject(), true);
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    uint32_t globalEnvFieldSize = globalEnv->GetGlobalEnvFieldSize();
    for (uint32_t index = 0; index < globalEnvFieldSize; index++) {
        JSTaggedValue objectValue = globalEnv->GetGlobalEnvObjectByIndex(index).GetTaggedValue();
        if (objectValue.IsHeapObject()) {
            GlobalIndex rootIndex;
            GlobalIndexMap::FindGlobalIndex(globalIndexMap, objectValue, &rootIndex);
            EXPECT_EQ(static_cast<int>(index), rootIndex.GetGlobalEnvId());
            if (index != rootIndex.GetGlobalEnvId()) {
                objectValue.D();
            }
            GlobalIndex globalEnvGlobalIndex;
            globalEnvGlobalIndex.UpdateGlobalEnvId(index);
            EXPECT_EQ(globalEnvGlobalIndex, rootIndex);
        }
    }
}
} // namespace panda::test