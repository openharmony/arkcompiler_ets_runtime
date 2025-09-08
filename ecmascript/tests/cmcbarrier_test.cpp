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

#include "ecmascript/js_array.h"
#include "ecmascript/global_env.h"
#include "ecmascript/tests/test_helper.h"
#include "common_components/heap/allocator/region_desc.h"
using namespace panda::ecmascript;

namespace panda::test {
class CMCBarrierTest : public CMCBaseTestWithScope {
};

HWTEST_F_L0(CMCBarrierTest, OldToNewBatchCopy)
{
    ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
    uint32_t arrayLength = 10;
    // length 50 will be in old
    JSHandle<TaggedArray> srcArray = factory->NewOldSpaceTaggedArray(arrayLength);
    JSHandle<TaggedArray> dstArray = factory->NewOldSpaceTaggedArray(arrayLength);

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newFun = factory->NewJSFunction(env); // in young
    for (uint32_t i = 0; i < 10; i++) {
        srcArray->Set(thread, i, newFun);
    }

    for (uint32_t i = 0; i < arrayLength; i++) {
        auto val = srcArray->Get(thread, i).GetTaggedObject();
        common::RegionDesc::InlinedRegionMetaData *objMetaRegion =
            common::RegionDesc::InlinedRegionMetaData::GetInlinedRegionMetaData(reinterpret_cast<uintptr_t>(val));
        EXPECT_TRUE(objMetaRegion->IsInYoungSpaceForWB());
    }

    thread->SetCMCGCPhase(common::GCPhase::GC_PHASE_FINAL_MARK);
    JSTaggedValue* to = reinterpret_cast<JSTaggedValue*>(ToUintPtr(dstArray->GetData()));
    JSTaggedValue* from = reinterpret_cast<JSTaggedValue*>(ToUintPtr(srcArray->GetData()));
    Barriers::CopyObject<true, false>(thread, *dstArray, to, from, arrayLength);

    // young to young, all the bitset should not be changed.
    for (uint32_t i = 0; i < arrayLength; i++) {
        auto val = dstArray->Get(thread, i).GetTaggedObject();
        common::RegionDesc::InlinedRegionMetaData *objMetaRegion =
            common::RegionDesc::InlinedRegionMetaData::GetInlinedRegionMetaData(reinterpret_cast<uintptr_t>(val));
        EXPECT_TRUE(objMetaRegion->IsInYoungSpaceForWB());
    }

    // check
    for (uint32_t i = 0; i < arrayLength; i++) {
        EXPECT_EQ(dstArray->Get(thread, i), srcArray->Get(thread, i));
    }
}
} // namespace panda::ecmascript
