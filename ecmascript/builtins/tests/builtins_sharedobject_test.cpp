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

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class BuiltinsSharedObjectTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override
    {
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    EcmaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(BuiltinsSharedObjectTest, SharedObject)
{
    DISALLOW_GARBAGE_COLLECTION;
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    auto globalConst = thread->GlobalConstants();
    auto sharedObjectPrototype = env->GetSharedObjectFunctionPrototype();
    JSHClass *hclass = sharedObjectPrototype->GetTaggedObject()->GetClass();
    ASSERT_FALSE(hclass->IsExtensible());
    ASSERT_TRUE(hclass->IsJSSharedObject());
    ASSERT_TRUE(hclass->GetProto().IsNull());

    // SharedObject
    PropertyDescriptor desc(thread);
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::GetOwnProperty(thread, JSHandle<JSObject>(sharedObjectPrototype), constructorKey, desc);
    auto ctor = desc.GetValue();
    ASSERT_EQ(ctor.GetTaggedValue(), env->GetSharedObjectFunction().GetTaggedValue());
    JSHClass *ctorHClass = ctor->GetTaggedObject()->GetClass();
    ASSERT_FALSE(ctorHClass->IsExtensible());
    ASSERT_TRUE(ctorHClass->IsConstructor());
    ASSERT_TRUE(ctorHClass->IsJSSharedFunction());

    // SharedFunction.prototype
    auto proto = ctorHClass->GetProto();
    ASSERT_EQ(proto, env->GetSharedFunctionPrototype().GetTaggedValue());
    ASSERT_TRUE(proto.IsJSSharedFunction());
    JSHClass *protoHClass = proto.GetTaggedObject()->GetClass();
    ASSERT_FALSE(protoHClass->IsExtensible());
    ASSERT_TRUE(!protoHClass->IsConstructor());

    // SharedObject.prototype
    auto sobjProto = protoHClass->GetProto();
    ASSERT_EQ(sobjProto, sharedObjectPrototype.GetTaggedValue());
    ASSERT_TRUE(sobjProto.IsJSSharedObject());
    JSHClass *sobjProtoHClass = sobjProto.GetTaggedObject()->GetClass();
    ASSERT_FALSE(sobjProtoHClass->IsExtensible());
    ASSERT_TRUE(!sobjProtoHClass->IsConstructor());
}

HWTEST_F_L0(BuiltinsSharedObjectTest, SharedFunction)
{
    DISALLOW_GARBAGE_COLLECTION;
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    auto globalConst = thread->GlobalConstants();
    auto sharedFunctionPrototype = env->GetSharedFunctionPrototype();
    // SharedFunction
    PropertyDescriptor desc(thread);
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::GetOwnProperty(thread, JSHandle<JSObject>(sharedFunctionPrototype), constructorKey, desc);
    auto ctor = desc.GetValue();
    ASSERT_EQ(ctor.GetTaggedValue(), env->GetSharedFunctionFunction().GetTaggedValue());
    JSHClass *ctorHClass = ctor->GetTaggedObject()->GetClass();
    ASSERT_FALSE(ctorHClass->IsExtensible());
    ASSERT_TRUE(ctorHClass->IsConstructor());
    ASSERT_TRUE(ctorHClass->IsJSSharedFunction());

    JSHandle<JSHClass> normalFunctionClass(env->GetSharedFunctionClassWithoutProto());
    ASSERT_FALSE(normalFunctionClass->IsExtensible());
    ASSERT_FALSE(normalFunctionClass->IsConstructor());
    ASSERT_TRUE(ctorHClass->IsJSSharedFunction());
}
}  // namespace panda::test
