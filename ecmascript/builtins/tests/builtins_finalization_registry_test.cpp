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

#include "ecmascript/builtins/builtins_finalization_registry.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_array_iterator.h"
#include "ecmascript/js_finalization_registry.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"

#include "ecmascript/object_factory.h"
#include "ecmascript/object_operator.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;
using BuiltinsBase = panda::ecmascript::base::BuiltinsBase;
static int testValue = 0;

namespace panda::test {
class BuiltinsFinalizationRegistryTest : public BaseTestWithScope<false> {
public:
    class TestClass : public base::BuiltinsBase {
    public:
        static JSTaggedValue cleanupCallback()
        {
            ++testValue;
            return JSTaggedValue::Undefined();
        }
    };
};

JSTaggedValue CreateFinalizationRegistryConstructor(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSFunction> finalizationRegistry(env->GetBuiltinsFinalizationRegistryFunction());
    JSHandle<JSFunction> handleFunc = factory->NewJSFunction(
        env, reinterpret_cast<void *>(BuiltinsFinalizationRegistryTest::TestClass::cleanupCallback));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*finalizationRegistry), 6);
    ecmaRuntimeCallInfo->SetFunction(finalizationRegistry.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, handleFunc.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue res = BuiltinsFinalizationRegistry::FinalizationRegistryConstructor(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    return res;
}

// new FinalizationRegistry (cleanupCallback)
HWTEST_F_L0(BuiltinsFinalizationRegistryTest, FinalizationRegistryConstructor)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSFunction> finalizationRegistry(env->GetBuiltinsFinalizationRegistryFunction());
    JSHandle<JSFunction> handleFunc = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::cleanupCallback));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*finalizationRegistry), 6);
    ecmaRuntimeCallInfo->SetFunction(finalizationRegistry.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, handleFunc.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsFinalizationRegistry::FinalizationRegistryConstructor(ecmaRuntimeCallInfo);
    ASSERT_TRUE(result.IsECMAObject());
}

void KeyValueCommon(JSThread* thread, JSHandle<JSTaggedValue>& target)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("1"));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
    JSObject::SetProperty(thread, target, key, value);
}

JSHandle<JSFinalizationRegistry> Common(JSThread* thread)
{
    JSTaggedValue result = CreateFinalizationRegistryConstructor(thread);
    JSHandle<JSFinalizationRegistry> jsfinalizationRegistry(thread, result);
    return jsfinalizationRegistry;
}

void RegisterUnRegisterCommon(JSThread *thread, JSHandle<JSFinalizationRegistry> &jsfinalizationRegistry,
                              std::vector<JSTaggedValue> &args, int32_t maxArg, bool unRegister = false)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), maxArg);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsfinalizationRegistry.GetTaggedValue());
    for (size_t i = 0; i < args.size(); i++) {
        ecmaRuntimeCallInfo->SetCallArg(i, args[i]);
    }

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    if (unRegister) {
        BuiltinsFinalizationRegistry::Unregister(ecmaRuntimeCallInfo);
    } else {
        BuiltinsFinalizationRegistry::Register(ecmaRuntimeCallInfo);
    }
    TestHelper::TearDownFrame(thread, prev);
}

// finalizationRegistry.Register(target, heldValue)
HWTEST_F_L0(BuiltinsFinalizationRegistryTest, Register0)
{
    testValue = 0;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objectFunc = env->GetObjectFunction();

    auto jsfinalizationRegistry = Common(thread);
    JSHandle<JSTaggedValue> target(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc));
    KeyValueCommon(thread, target);

    std::vector<JSTaggedValue> args{target.GetTaggedValue(), JSTaggedValue(10)};

    RegisterUnRegisterCommon(thread, jsfinalizationRegistry, args, 8);
    ASSERT_EQ(testValue, 0);
}

// finalizationRegistry.Register(target, heldValue [ , unregisterToken ])
HWTEST_F_L0(BuiltinsFinalizationRegistryTest, Register1)
{
    testValue = 0;
    auto jsfinalizationRegistry = Common(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> objectFunc = thread->GetEcmaVM()->GetGlobalEnv()->GetObjectFunction();
    JSHandle<JSTaggedValue> target(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc));
    KeyValueCommon(thread, target);
    std::vector<JSTaggedValue> args{target.GetTaggedValue(), JSTaggedValue(10), target.GetTaggedValue()};
    RegisterUnRegisterCommon(thread, jsfinalizationRegistry, args, 10);
    ASSERT_EQ(testValue, 0);
}

// finalizationRegistry.Register(target, heldValue [ , unregisterToken ])
HWTEST_F_L0(BuiltinsFinalizationRegistryTest, Register2)
{
    testValue = 0;
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objectFunc = env->GetObjectFunction();

    auto jsfinalizationRegistry = Common(thread);
    vm->SetEnableForceGC(false);
    JSTaggedValue target = JSTaggedValue::Undefined();
    {
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        auto obj =
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc);
        target = obj.GetTaggedValue();
        std::vector<JSTaggedValue> args{target, JSTaggedValue(10), target};
        RegisterUnRegisterCommon(thread, jsfinalizationRegistry, args, 10);
    }
    vm->CollectGarbage(TriggerGCType::FULL_GC);
    if (!thread->HasPendingException()) {
        job::MicroJobQueue::ExecutePendingJob(thread, vm->GetJSThread()->GetCurrentEcmaContext()->GetMicroJobQueue());
    }
    vm->SetEnableForceGC(true);
    ASSERT_EQ(testValue, 1);
}

// finalizationRegistry.Register(target, heldValue [ , unregisterToken ])
HWTEST_F_L0(BuiltinsFinalizationRegistryTest, Register3)
{
    testValue = 0;
    EcmaVM *vm = thread->GetEcmaVM();
    auto jsfinalizationRegistry = Common(thread);

    vm->SetEnableForceGC(false);
    JSTaggedValue target = JSTaggedValue::Undefined();
    JSTaggedValue target1 = JSTaggedValue::Undefined();
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSTaggedValue> objectFunc = env->GetObjectFunction();
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        auto obj =
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc);
        auto obj1 =
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc);
        target = obj.GetTaggedValue();

        std::vector<JSTaggedValue> args{target, JSTaggedValue(10), target};
        RegisterUnRegisterCommon(thread, jsfinalizationRegistry, args, 10);

        target1 = obj1.GetTaggedValue();
        std::vector<JSTaggedValue> args1{target1, JSTaggedValue(10), target1};
        RegisterUnRegisterCommon(thread, jsfinalizationRegistry, args, 10);
    }
    vm->CollectGarbage(TriggerGCType::FULL_GC);
    if (!thread->HasPendingException()) {
        job::MicroJobQueue::ExecutePendingJob(thread, vm->GetJSThread()->GetCurrentEcmaContext()->GetMicroJobQueue());
    }
    vm->SetEnableForceGC(true);
    ASSERT_EQ(testValue, 2);
}

// finalizationRegistry.Register(target, heldValue [ , unregisterToken ])
HWTEST_F_L0(BuiltinsFinalizationRegistryTest, Register4)
{
    testValue = 0;
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objectFunc = env->GetObjectFunction();

    auto jsfinalizationRegistry = Common(thread);
    auto jsfinalizationRegistry1 = Common(thread);
    vm->SetEnableForceGC(false);
    JSTaggedValue target = JSTaggedValue::Undefined();
    JSTaggedValue target1 = JSTaggedValue::Undefined();
    {
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        auto obj =
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc);
        auto obj1 =
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc);
        target = obj.GetTaggedValue();
        target1 = obj1.GetTaggedValue();

        std::vector<JSTaggedValue> args{target, JSTaggedValue(10), target};
        RegisterUnRegisterCommon(thread, jsfinalizationRegistry, args, 10);

        std::vector<JSTaggedValue> args1{target1, JSTaggedValue(10), target1};
        RegisterUnRegisterCommon(thread, jsfinalizationRegistry1, args, 10);
    }
    vm->CollectGarbage(TriggerGCType::FULL_GC);
    if (!thread->HasPendingException()) {
        job::MicroJobQueue::ExecutePendingJob(thread, vm->GetJSThread()->GetCurrentEcmaContext()->GetMicroJobQueue());
    }
    vm->SetEnableForceGC(true);
    ASSERT_EQ(testValue, 2);
}

// finalizationRegistry.Register(target, heldValue [ , unregisterToken ])
HWTEST_F_L0(BuiltinsFinalizationRegistryTest, Register5)
{
    testValue = 0;
    EcmaVM *vm = thread->GetEcmaVM();
    auto jsfinalizationRegistry = Common(thread);
    vm->SetEnableForceGC(false);
    JSTaggedValue target = JSTaggedValue::Undefined();
    JSTaggedValue target1 = JSTaggedValue::Undefined();
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<JSTaggedValue> objectFunc = thread->GetEcmaVM()->GetGlobalEnv()->GetObjectFunction();
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        auto obj =
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc);
        auto obj1 =
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc);
        target = obj.GetTaggedValue();
        target1 = obj1.GetTaggedValue();

        std::vector<JSTaggedValue> args{target, JSTaggedValue(10), target};
        RegisterUnRegisterCommon(thread, jsfinalizationRegistry, args, 10);

        std::vector<JSTaggedValue> args1{target1, JSTaggedValue(10), target};
        RegisterUnRegisterCommon(thread, jsfinalizationRegistry, args, 10);
    }
    vm->CollectGarbage(TriggerGCType::FULL_GC);
    if (!thread->HasPendingException()) {
        job::MicroJobQueue::ExecutePendingJob(thread, vm->GetJSThread()->GetCurrentEcmaContext()->GetMicroJobQueue());
    }
    vm->SetEnableForceGC(true);
    ASSERT_EQ(testValue, 2);
}

// finalizationRegistry.Unregister(unregisterToken ])
HWTEST_F_L0(BuiltinsFinalizationRegistryTest, Unregister1)
{
    testValue = 0;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objectFunc = env->GetObjectFunction();

    auto jsfinalizationRegistry = Common(thread);
    JSHandle<JSTaggedValue> target(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc));
    KeyValueCommon(thread, target);

    std::vector<JSTaggedValue> args{target.GetTaggedValue(), JSTaggedValue(10), target.GetTaggedValue()};
    RegisterUnRegisterCommon(thread, jsfinalizationRegistry, args, 10);

    std::vector<JSTaggedValue> args1{target.GetTaggedValue()};
    RegisterUnRegisterCommon(thread, jsfinalizationRegistry, args1, 6, true);
    ASSERT_EQ(testValue, 0);
}

HWTEST_F_L0(BuiltinsFinalizationRegistryTest, Unregister2)
{
    testValue = 0;
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> objectFunc = thread->GetEcmaVM()->GetGlobalEnv()->GetObjectFunction();

    auto jsfinalizationRegistry = Common(thread);
    vm->SetEnableForceGC(false);
    JSTaggedValue target = JSTaggedValue::Undefined();
    {
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        auto obj =
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc);
        target = obj.GetTaggedValue();

        std::vector<JSTaggedValue> args{target, JSTaggedValue(10), target};
        RegisterUnRegisterCommon(thread, jsfinalizationRegistry, args, 10);

        std::vector<JSTaggedValue> args1{target};
        RegisterUnRegisterCommon(thread, jsfinalizationRegistry, args1, 6, true);
    }
    vm->CollectGarbage(TriggerGCType::FULL_GC);
    if (!thread->HasPendingException()) {
        job::MicroJobQueue::ExecutePendingJob(thread, vm->GetJSThread()->GetCurrentEcmaContext()->GetMicroJobQueue());
    }
    vm->SetEnableForceGC(true);
    ASSERT_EQ(testValue, 0);
}

// finalizationRegistry.Register(target, heldValue [ , unregisterToken ]) target and unregisterToken Symbol
HWTEST_F_L0(BuiltinsFinalizationRegistryTest, RegisterTargetSymbol)
{
    testValue = 0;
    EcmaVM *vm = thread->GetEcmaVM();

    auto jsfinalizationRegistry = Common(thread);

    vm->SetEnableForceGC(false);
    JSTaggedValue target = JSTaggedValue::Undefined();
    JSTaggedValue target1 = JSTaggedValue::Undefined();
    {
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        JSHandle<JSSymbol> symbol1 = thread->GetEcmaVM()->GetFactory()->NewJSSymbol();
        JSHandle<JSSymbol> symbol2 = thread->GetEcmaVM()->GetFactory()->NewJSSymbol();
        target = symbol1.GetTaggedValue();
        target1 = symbol2.GetTaggedValue();
        std::vector<JSTaggedValue> args{target, JSTaggedValue(10), target};
        RegisterUnRegisterCommon(thread, jsfinalizationRegistry, args, 10);

        std::vector<JSTaggedValue> args1{target1, JSTaggedValue(10), target1};
        RegisterUnRegisterCommon(thread, jsfinalizationRegistry, args1, 10);
    }
    vm->CollectGarbage(TriggerGCType::FULL_GC);
    if (!thread->HasPendingException()) {
        job::MicroJobQueue::ExecutePendingJob(thread, vm->GetJSThread()->GetCurrentEcmaContext()->GetMicroJobQueue());
    }
    vm->SetEnableForceGC(true);
    ASSERT_EQ(testValue, 2);
}

// finalizationRegistry.Unregister(unregisterToken) unregisterToken Symbol
HWTEST_F_L0(BuiltinsFinalizationRegistryTest, UnregisterTokenSymbol)
{
    testValue = 0;
    EcmaVM *vm = thread->GetEcmaVM();

    auto jsfinalizationRegistry = Common(thread);
    vm->SetEnableForceGC(false);
    JSTaggedValue target = JSTaggedValue::Undefined();
    {
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        JSHandle<JSSymbol> symbol = thread->GetEcmaVM()->GetFactory()->NewJSSymbol();
        target = symbol.GetTaggedValue();

        std::vector<JSTaggedValue> args{target, JSTaggedValue(10), target};
        RegisterUnRegisterCommon(thread, jsfinalizationRegistry, args, 10);

        std::vector<JSTaggedValue> args1{target};
        RegisterUnRegisterCommon(thread, jsfinalizationRegistry, args1, 6, true);
    }
    vm->CollectGarbage(TriggerGCType::FULL_GC);
    if (!thread->HasPendingException()) {
        job::MicroJobQueue::ExecutePendingJob(thread, vm->GetJSThread()->GetCurrentEcmaContext()->GetMicroJobQueue());
    }
    vm->SetEnableForceGC(true);
    ASSERT_EQ(testValue, 0);
}
}  // namespace panda::test
