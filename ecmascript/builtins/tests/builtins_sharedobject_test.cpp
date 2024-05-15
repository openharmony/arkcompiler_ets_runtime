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
class BuiltinsSharedObjectTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(BuiltinsSharedObjectTest, SharedObject)
{
    DISALLOW_GARBAGE_COLLECTION;
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    auto globalConst = thread->GlobalConstants();
    auto sharedObjectPrototype = env->GetSObjectFunctionPrototype();
    JSHClass *hclass = sharedObjectPrototype->GetTaggedObject()->GetClass();
    ASSERT_FALSE(hclass->IsExtensible());
    ASSERT_FALSE(hclass->IsConstructor());
    ASSERT_FALSE(hclass->IsCallable());
    ASSERT_TRUE(hclass->IsJSSharedObject());
    ASSERT_TRUE(hclass->GetProto().IsNull());

    // SharedObject
    PropertyDescriptor desc(thread);
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::GetOwnProperty(thread, JSHandle<JSObject>(sharedObjectPrototype), constructorKey, desc);
    auto ctor = desc.GetValue();
    ASSERT_EQ(ctor.GetTaggedValue(), env->GetSObjectFunction().GetTaggedValue());
    JSHClass *ctorHClass = ctor->GetTaggedObject()->GetClass();
    ASSERT_FALSE(ctorHClass->IsExtensible());
    ASSERT_TRUE(ctorHClass->IsConstructor());
    ASSERT_TRUE(ctorHClass->IsCallable());
    ASSERT_TRUE(ctorHClass->IsJSSharedFunction());

    // SharedFunction.prototype
    auto proto = ctorHClass->GetProto();
    ASSERT_EQ(proto, env->GetSFunctionPrototype().GetTaggedValue());
    ASSERT_TRUE(proto.IsJSSharedFunction());
    JSHClass *protoHClass = proto.GetTaggedObject()->GetClass();
    ASSERT_FALSE(protoHClass->IsExtensible());
    ASSERT_FALSE(protoHClass->IsConstructor());
    ASSERT_TRUE(protoHClass->IsCallable());

    // SharedObject.prototype
    auto sObjProto = protoHClass->GetProto();
    ASSERT_EQ(sObjProto, sharedObjectPrototype.GetTaggedValue());
    ASSERT_TRUE(sObjProto.IsJSSharedObject());
    JSHClass *sObjProtoHClass = sObjProto.GetTaggedObject()->GetClass();
    ASSERT_FALSE(sObjProtoHClass->IsExtensible());
    ASSERT_FALSE(sObjProtoHClass->IsConstructor());
    ASSERT_FALSE(sObjProtoHClass->IsCallable());
}

HWTEST_F_L0(BuiltinsSharedObjectTest, SharedFunction)
{
    DISALLOW_GARBAGE_COLLECTION;
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    auto globalConst = thread->GlobalConstants();
    auto sFunctionPrototype = env->GetSFunctionPrototype();
    // SharedFunction
    PropertyDescriptor desc(thread);
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::GetOwnProperty(thread, JSHandle<JSObject>(sFunctionPrototype), constructorKey, desc);
    auto ctor = desc.GetValue();
    ASSERT_EQ(ctor.GetTaggedValue(), env->GetSFunctionFunction().GetTaggedValue());
    JSHClass *ctorHClass = ctor->GetTaggedObject()->GetClass();
    ASSERT_FALSE(ctorHClass->IsExtensible());
    ASSERT_TRUE(ctorHClass->IsConstructor());
    ASSERT_TRUE(ctorHClass->IsCallable());
    ASSERT_TRUE(ctorHClass->IsJSSharedFunction());

    JSHandle<JSHClass> normalFunctionClass(env->GetSFunctionClassWithoutProto());
    ASSERT_FALSE(normalFunctionClass->IsExtensible());
    ASSERT_FALSE(normalFunctionClass->IsConstructor());
    ASSERT_TRUE(ctorHClass->IsCallable());
    ASSERT_TRUE(ctorHClass->IsJSSharedFunction());
}

HWTEST_F_L0(BuiltinsSharedObjectTest, SharedBuiltinsMethod)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    auto factory = thread->GetEcmaVM()->GetFactory();
    auto sFunctionPrototype = env->GetSFunctionPrototype();
    auto callStr = factory->NewFromASCII("call");
    PropertyDescriptor desc(thread);
    JSObject::GetOwnProperty(thread, JSHandle<JSObject>(sFunctionPrototype), JSHandle<JSTaggedValue>(callStr), desc);
    auto method = desc.GetValue();
    JSHClass *hclass = method->GetTaggedObject()->GetClass();
    ASSERT_FALSE(hclass->IsExtensible());
    ASSERT_TRUE(!hclass->IsConstructor());
    ASSERT_TRUE(hclass->IsCallable());
    ASSERT_TRUE(hclass->IsJSSharedFunction());
}
}  // namespace panda::test
