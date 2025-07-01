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

#include "ecmascript/base/config.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/lexical_env.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;

using namespace panda::ecmascript;

namespace panda::test {
class ObjectFactoryTest : public BaseTestWithScope<false> {
};

JSHandle<GlobalEnv> GetGlobal(JSThread *thread)
{
    return thread->GetEcmaVM()->GetGlobalEnv();
}

HWTEST_F_L0(ObjectFactoryTest, NewJSObjectByConstructor)
{
    thread->GetEcmaVM()->SetEnableForceGC(false);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> objFun = GetGlobal(thread)->GetObjectFunction();

    // check mem alloc
    JSHandle<JSObject> newObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSHClass> newObjCls(thread, newObj->GetJSHClass());
    EXPECT_TRUE(*newObj != nullptr);
    EXPECT_TRUE(*newObjCls != nullptr);

    // check feild
    EXPECT_EQ(newObj->GetProperties(thread), thread->GlobalConstants()->GetEmptyArray());
    EXPECT_EQ(newObj->GetElements(thread), thread->GlobalConstants()->GetEmptyArray());
    EXPECT_TRUE(JSTaggedValue(*newObj).IsECMAObject());

    // check jshclass
    JSHClass *cls = *newObjCls;
    EXPECT_TRUE(cls->GetObjectSize() ==
                JSObject::SIZE + JSHClass::DEFAULT_CAPACITY_OF_IN_OBJECTS * JSTaggedValue::TaggedTypeSize());
    EXPECT_TRUE(cls->GetPrototype(thread) == GetGlobal(thread)->GetObjectFunctionPrototype().GetTaggedValue());
    EXPECT_TRUE(cls->GetObjectType() == JSType::JS_OBJECT);

    // check gc handle update
    auto *prototype = cls->GetPrototype(thread).GetTaggedObject();
    thread->GetEcmaVM()->CollectGarbage(TriggerGCType::FULL_GC);
    // After FullGC
    // Test condition reply on gc.It is not suitable.
    if (!g_isEnableCMCGC) {
        if (thread->GetEcmaVM()->GetJSOptions().EnableSnapshotDeserialize()) {
            EXPECT_TRUE(prototype == newObjCls->GetPrototype(thread).GetTaggedObject());
        } else {
            EXPECT_TRUE(prototype != newObjCls->GetPrototype(thread).GetTaggedObject());
        }
    }
    thread->GetEcmaVM()->SetEnableForceGC(true);
}

HWTEST_F_L0(ObjectFactoryTest, NewJSFunction)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    // check mem alloc
    JSHandle<JSFunction> newFun = factory->NewJSFunction(env);
    JSHandle<JSHClass> newFunCls(thread, newFun->GetJSHClass());
    EXPECT_TRUE(*newFun != nullptr);
    EXPECT_TRUE(*newFunCls != nullptr);

    // check feild
    EXPECT_EQ(newFun->GetProperties(thread), thread->GlobalConstants()->GetEmptyArray());
    EXPECT_EQ(newFun->GetElements(thread), thread->GlobalConstants()->GetEmptyArray());
    EXPECT_EQ(newFun->GetProtoOrHClass(thread), JSTaggedValue::Hole());
    EXPECT_EQ(newFun->GetHomeObject(thread), JSTaggedValue::Undefined());
    EXPECT_TRUE(JSTaggedValue(*newFun).IsJSFunction());

    // check jshclass
    JSHClass *cls = *newFunCls;
    EXPECT_TRUE(cls->GetObjectSize() ==
                JSFunction::SIZE + JSHClass::DEFAULT_CAPACITY_OF_IN_OBJECTS * JSTaggedValue::TaggedTypeSize());
    EXPECT_TRUE(cls->GetPrototype(thread) == GetGlobal(thread)->GetFunctionPrototype().GetTaggedValue());
    EXPECT_TRUE(cls->GetObjectType() == JSType::JS_FUNCTION);
    EXPECT_TRUE(cls->IsCallable());
    EXPECT_TRUE(cls->IsExtensible());
    EXPECT_TRUE(!cls->IsConstructor());
}

HWTEST_F_L0(ObjectFactoryTest, NewJSBoundFunction)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    // test prepare
    JSHandle<JSFunction> funFun(GetGlobal(thread)->GetObjectFunction());
    JSHandle<JSTaggedValue> bound(thread, GetGlobal(thread)->GetObjectFunctionPrototype().GetTaggedValue());
    const JSHandle<TaggedArray> array(thread->GlobalConstants()->GetHandledEmptyArray());

    // check mem alloc
    JSHandle<JSBoundFunction> newBoundFun =
        factory->NewJSBoundFunction(JSHandle<JSTaggedValue>::Cast(funFun), bound, array);
    JSHandle<JSHClass> newBoundFunCls(thread, newBoundFun->GetJSHClass());
    EXPECT_TRUE(*newBoundFun != nullptr);
    EXPECT_TRUE(*newBoundFunCls != nullptr);
}

HWTEST_F_L0(ObjectFactoryTest, NewJSPrimitiveRef)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    // test prepare
    JSHandle<JSFunction> numberFun(GetGlobal(thread)->GetNumberFunction());
    JSHandle<JSTaggedValue> primitive(thread, JSTaggedValue(1));

    // check mem alloc
    JSHandle<JSPrimitiveRef> newPrimitive = factory->NewJSPrimitiveRef(numberFun, primitive);
    JSHandle<JSHClass> newPrimitiveCls(thread, newPrimitive->GetJSHClass());
    EXPECT_TRUE(*newPrimitive != nullptr);
    EXPECT_TRUE(*newPrimitiveCls != nullptr);

    EXPECT_TRUE(newPrimitive->GetValue(thread) == JSTaggedValue(1));
}

HWTEST_F_L0(ObjectFactoryTest, NewLexicalEnv)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    // check mem alloc
    JSHandle<LexicalEnv> newLexicalEnv = factory->NewLexicalEnv(0);
    JSHandle<JSHClass> newLexicalEnvCls(thread, newLexicalEnv->GetClass());
    EXPECT_TRUE(*newLexicalEnv != nullptr);
    EXPECT_TRUE(*newLexicalEnvCls != nullptr);
}

HWTEST_F_L0(ObjectFactoryTest, NewJSArray)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    // check mem alloc
    JSHandle<JSArray> newJSAarray = factory->NewJSArray();
    JSHandle<JSHClass> newJSArrayCls(thread, newJSAarray->GetJSHClass());
    EXPECT_TRUE(*newJSAarray != nullptr);
    EXPECT_TRUE(*newJSArrayCls != nullptr);
}

HWTEST_F_L0(ObjectFactoryTest, NewAndCopyTaggedArray)
{
    constexpr uint32_t SHORT_ELEMENT_NUMS = 20;
    constexpr uint32_t LONG_ELEMENT_NUMS = 100;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> shortTaggedarray = factory->NewTaggedArray(SHORT_ELEMENT_NUMS);
    // init tagggedArray
    for (uint32_t i = 0; i < SHORT_ELEMENT_NUMS; i++) {
        shortTaggedarray->Set(thread, i, JSTaggedValue(i));
    }
    JSHandle<TaggedArray> copiedShort = factory->NewAndCopyTaggedArray(shortTaggedarray, SHORT_ELEMENT_NUMS,
                                                                       SHORT_ELEMENT_NUMS);
    JSHandle<TaggedArray> copiedLong = factory->NewAndCopyTaggedArray(shortTaggedarray, LONG_ELEMENT_NUMS,
                                                                       SHORT_ELEMENT_NUMS);
    // check
    for (uint32_t i = 0; i < SHORT_ELEMENT_NUMS; i++) {
        EXPECT_EQ(copiedShort->Get(thread, i), shortTaggedarray->Get(thread, i));
        EXPECT_EQ(copiedLong->Get(thread, i), shortTaggedarray->Get(thread, i));
    }
    for (uint32_t i = SHORT_ELEMENT_NUMS; i < LONG_ELEMENT_NUMS; i++) {
        EXPECT_EQ(copiedLong->Get(thread, i), JSTaggedValue::Hole());
    }
}

HWTEST_F_L0(ObjectFactoryTest, NewAndCopyTaggedArrayNeedBarrier)
{
    constexpr uint32_t SHORT_ELEMENT_NUMS = 20;
    constexpr uint32_t LONG_ELEMENT_NUMS = 100;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> shortTaggedarray = factory->NewTaggedArray(SHORT_ELEMENT_NUMS);
    // init tagggedArray
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    for (uint32_t i = 0; i < SHORT_ELEMENT_NUMS; i++) {
        JSHandle<JSFunction> newFun = factory->NewJSFunction(env);
        shortTaggedarray->Set(thread, i, newFun);
    }
    JSHandle<TaggedArray> copiedShort = factory->NewAndCopyTaggedArray(shortTaggedarray, SHORT_ELEMENT_NUMS,
                                                                       SHORT_ELEMENT_NUMS);
    JSHandle<TaggedArray> copiedLong = factory->NewAndCopyTaggedArray(shortTaggedarray, LONG_ELEMENT_NUMS,
                                                                       SHORT_ELEMENT_NUMS);
    // check
    for (uint32_t i = 0; i < SHORT_ELEMENT_NUMS; i++) {
        EXPECT_EQ(copiedShort->Get(thread, i), shortTaggedarray->Get(thread, i));
        EXPECT_EQ(copiedLong->Get(thread, i), shortTaggedarray->Get(thread, i));
    }
    for (uint32_t i = SHORT_ELEMENT_NUMS; i < LONG_ELEMENT_NUMS; i++) {
        EXPECT_EQ(copiedLong->Get(thread, i), JSTaggedValue::Hole());
    }
}
}  // namespace panda::test
