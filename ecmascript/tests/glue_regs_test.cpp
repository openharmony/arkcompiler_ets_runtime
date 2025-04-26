/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/builtins/builtins.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
class GlueRegsTest :  public BaseTestWithScope<false> {
};

HWTEST_F_L0(GlueRegsTest, ConstantClassTest)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ASSERT_NE(globalConst, nullptr);

    const JSTaggedValue *address = globalConst->BeginSlot();
    size_t curIndex = static_cast<size_t>(ConstantIndex::CONSTANT_BEGIN);
    size_t holeIndex = static_cast<size_t>(ConstantIndex::HOLE_INDEX);
    while (address < globalConst->EndSlot()) {
        if (curIndex != holeIndex) {
        }
        address += 1;
        curIndex += 1;
    }
}

HWTEST_F_L0(GlueRegsTest, ConstantSpecialTest)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ASSERT_NE(globalConst, nullptr);

    EXPECT_TRUE(globalConst->GetUndefined().IsUndefined());
    EXPECT_TRUE(globalConst->GetHandledUndefined()->IsUndefined());
    EXPECT_TRUE(globalConst->GetNull().IsNull());
    EXPECT_TRUE(globalConst->GetHandledNull()->IsNull());
    EXPECT_TRUE(globalConst->GetEmptyString().IsString());
    EXPECT_TRUE(globalConst->GetHandledEmptyString()->IsString());
}

HWTEST_F_L0(GlueRegsTest, ConstantStringTest)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ASSERT_NE(globalConst, nullptr);

#define CONSTANT_STRING_ITERATOR(Name, Index, Desc)                                     \
    JSTaggedValue Name##value = globalConst->Get##Name();                               \
    EXPECT_TRUE(Name##value.IsString());                                                \
    Region *Name##region = Region::ObjectAddressToRange(Name##value.GetTaggedObject()); \
    EXPECT_TRUE(Name##region->InSharedReadOnlySpace());
    SHARED_GLOBAL_ENV_CONSTANT_STRING(CONSTANT_STRING_ITERATOR)
#undef CONSTANT_STRING_ITERATOR
}

HWTEST_F_L0(GlueRegsTest, ConstantAccessorTest)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ASSERT_NE(globalConst, nullptr);

#define CONSTANT_ACCESSOR_ITERATOR(Type, Name, Index, Desc)              \
    Type Name##value = globalConst->Get##Name();                         \
    EXPECT_TRUE(!Name##value.IsNull());                                  \
    JSHandle<Type> Name##handledValue = globalConst->GetHandled##Name(); \
    EXPECT_TRUE(!Name##handledValue->IsNull());
    SHARED_GLOBAL_ENV_CONSTANT_ACCESSOR(CONSTANT_ACCESSOR_ITERATOR)
#undef CONSTANT_ACCESSOR_ITERATOR
}
}  // namespace panda::test