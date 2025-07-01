/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/weak_vector.h"
#include "ecmascript/global_env.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class WeakVectorTest : public BaseTestWithScope<false> {
};

/**
 * @tc.name: Create
 * @tc.desc: Check whether the returned value through "Create" function is within expectations by "GetEnd"
 *           function and "Empty" function. convert to tagedarray, and check whether the length attribute
 *           meets the expectation.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(WeakVectorTest, Create)
{
    uint32_t weakVectorCapacity = 100;
    JSHandle<WeakVector> weakVector = WeakVector::Create(thread, weakVectorCapacity);
    EXPECT_TRUE(*weakVector != nullptr);
    JSHandle<TaggedArray> weakVectArr(weakVector);
    EXPECT_EQ(weakVector->GetEnd(), 0U);
    EXPECT_TRUE(weakVector->Empty());
    EXPECT_EQ(weakVectArr->GetLength(), weakVectorCapacity + 1);
}

/**
 * @tc.name: SetAndGet
 * @tc.desc: Check whether each value in the vector container is the same as the value of set by "set" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(WeakVectorTest, SetAndGet)
{
    uint32_t weakVectorCapacity = 100;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<WeakVector> weakVector = WeakVector::Create(thread, weakVectorCapacity);

    JSHandle<JSObject> weakObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSTaggedValue> weakObjclassVal(thread, weakObj->GetJSHClass());
    JSTaggedValue weakValue = JSTaggedValue(weakObjclassVal.GetTaggedValue().CreateAndGetWeakRef());
    // set weak value
    for (uint32_t i = 0; i < weakVectorCapacity; i++) {
        weakVector->Set(thread, i, weakValue);
        EXPECT_EQ(weakVector->Get(thread, i), weakValue);
    }
}

/**
 * @tc.name: Grow
 * @tc.desc: Create a weak vector with a certain length through the "Create" function, and then expand the weak vector
 *           with the "Grow" function to increase its length.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(WeakVectorTest, Grow)
{
    thread->GetEcmaVM()->SetEnableForceGC(false); // turn off gc
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetStringFunction();

    JSHandle<JSObject> weakObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSTaggedValue> weakObjclassVal(thread, weakObj->GetJSHClass());
    JSTaggedValue weakValue = JSTaggedValue(weakObjclassVal.GetTaggedValue().CreateAndGetWeakRef());
    // test growing vector with null value
    uint32_t oldWeakVectorCapacity = 0;
    uint32_t newWeakVectorCapacity = 100;
    JSHandle<WeakVector> oldWeakVector = WeakVector::Create(thread, oldWeakVectorCapacity);
    JSHandle<WeakVector> newWeakVector = oldWeakVector->Grow(thread, oldWeakVector, newWeakVectorCapacity);
    EXPECT_EQ(newWeakVector->GetCapacity(), newWeakVectorCapacity);
    EXPECT_TRUE(newWeakVector->Empty());
    // test growing vector with no null value
    oldWeakVectorCapacity = 100;
    newWeakVectorCapacity = 200;
    for (uint32_t i = 0; i < oldWeakVectorCapacity; i++) {
        newWeakVector->Set(thread, i, weakValue);
    }
    newWeakVector = newWeakVector->Grow(thread, newWeakVector, newWeakVectorCapacity);
    EXPECT_EQ(newWeakVector->GetCapacity(), newWeakVectorCapacity);
    for (uint32_t i = 0; i < oldWeakVectorCapacity; i++) {
        EXPECT_EQ(newWeakVector->Get(thread, i), weakValue);
    }
    EXPECT_EQ(newWeakVector->GetEnd(), 0U);
    thread->GetEcmaVM()->SetEnableForceGC(true);  // turn on gc
}

/**
 * @tc.name: PushBack
 * @tc.desc: Change the value in the weak vector through "pushback" function,and compare whether the original value of
 *           the weak vector and the pushback value are the same in the pushback position.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(WeakVectorTest, PushBack)
{
    uint32_t weakVectorCapacity = 100;
    uint32_t pushWeakVectorCapacity = weakVectorCapacity >> 2;  // 2: means half value
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetPromiseFunction();
    JSHandle<WeakVector> weakVector = WeakVector::Create(thread, weakVectorCapacity);

    JSHandle<JSObject> weakObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSTaggedValue> weakObjclassVal(thread, weakObj->GetJSHClass());
    JSTaggedValue weakValue = JSTaggedValue(weakObjclassVal.GetTaggedValue().CreateAndGetWeakRef());
    // fill the value into the vector
    for (uint32_t i = 0; i < weakVectorCapacity; i++) {
        weakVector->Set(thread, i, weakValue);
    }
    // push the value into the vector and reset end
    for (uint32_t i = 0; i < pushWeakVectorCapacity; i++) {
        weakVector->PushBack(thread, JSTaggedValue::Hole());
    }
    EXPECT_EQ(weakVector->GetEnd(), pushWeakVectorCapacity);

    for (uint32_t i = 0; i < pushWeakVectorCapacity; i++) {
        EXPECT_TRUE(weakVector->Get(thread, i).IsHole());
    }
    for (uint32_t i = pushWeakVectorCapacity; i < weakVectorCapacity; i++) {
        EXPECT_EQ(weakVector->Get(thread, i), weakValue);
    }
}

/**
 * @tc.name: Delete
 * @tc.desc: Check whether the value in the vector container is hole when deleted, and whether the container
 *           is not empty when all are deleted by "delete" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(WeakVectorTest, Delete)
{
    uint32_t weakVectorCapacity = 100;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetPromiseFunction();
    JSHandle<WeakVector> weakVector = WeakVector::Create(thread, weakVectorCapacity);

    JSHandle<JSObject> weakObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSTaggedValue> weakObjclassVal(thread, weakObj->GetJSHClass());
    JSTaggedValue weakValue = JSTaggedValue(weakObjclassVal.GetTaggedValue().CreateAndGetWeakRef());

    for (uint32_t i = 0; i < weakVectorCapacity; i++) {
        weakVector->PushBack(thread, weakValue);
    }
    EXPECT_TRUE(weakVector->Full());
    EXPECT_FALSE(weakVector->Delete(thread, weakVectorCapacity));

    for (uint32_t i = 0; i < weakVectorCapacity; i++) {
        EXPECT_TRUE(weakVector->Delete(thread, i));
        EXPECT_TRUE(weakVector->Get(thread, i).IsHole());
    }
    EXPECT_FALSE(weakVector->Empty());
    EXPECT_EQ(weakVector->GetCapacity(), weakVectorCapacity);
}
}  // namespace panda::test