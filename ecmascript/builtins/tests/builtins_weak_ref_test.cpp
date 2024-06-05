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

#include "ecmascript/builtins/builtins_weak_ref.h"

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/global_env.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/js_weak_ref.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/tagged_array-inl.h"


using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;
using BuiltinsBase = panda::ecmascript::base::BuiltinsBase;

namespace panda::test {
using BuiltinsWeakRef = ecmascript::builtins::BuiltinsWeakRef;

class BuiltinsWeakRefTest : public BaseTestWithScope<false> {
};

JSTaggedValue CreateWeakRefConstructor(JSThread *thread, JSTaggedValue target)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    JSHandle<JSFunction> weakRef(env->GetBuiltinsWeakRefFunction());

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*weakRef), 6);
    ecmaRuntimeCallInfo->SetFunction(weakRef.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, target);

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsWeakRef::WeakRefConstructor(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    return result;
}

// new WeakRef(target)
HWTEST_F_L0(BuiltinsWeakRefTest, WeakRefConstructor)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> objectFunc(env->GetObjectFunction());

    JSHandle<JSObject> target(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc));

    JSHandle<JSFunction> weakRef(env->GetBuiltinsWeakRefFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, weakRef.GetTaggedValue(), 6);
    ecmaRuntimeCallInfo->SetFunction(weakRef.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, target.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsWeakRef::WeakRefConstructor(ecmaRuntimeCallInfo);

    ASSERT_TRUE(result.IsECMAObject());
}

// weakRef.Deref()
HWTEST_F_L0(BuiltinsWeakRefTest, Deref1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> objectFunc(env->GetObjectFunction());

    JSHandle<JSObject> target(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc));
    JSTaggedValue result = CreateWeakRefConstructor(thread, target.GetTaggedValue());
    JSHandle<JSWeakRef> jsWeakRef(thread, JSWeakRef::Cast(reinterpret_cast<TaggedObject *>(result.GetRawData())));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsWeakRef.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue result1 = BuiltinsWeakRef::Deref(ecmaRuntimeCallInfo);
    ASSERT_EQ(result1, target.GetTaggedValue());
}

// weakRef.Deref()
HWTEST_F_L0(BuiltinsWeakRefTest, Deref2)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> objectFunc(env->GetObjectFunction());
    JSHandle<JSTaggedValue> formatStyle = thread->GlobalConstants()->GetHandledStyleString();
    JSHandle<JSTaggedValue> styleKey(factory->NewFromASCII("currency"));
    JSHandle<JSTaggedValue> styleValue(factory->NewFromASCII("EUR"));
    JSHandle<JSObject> target(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc));
    JSObject::SetProperty(thread, target, formatStyle, styleKey);
    JSObject::SetProperty(thread, target, styleKey, styleValue);

    JSTaggedValue result = CreateWeakRefConstructor(thread, target.GetTaggedValue());
    JSHandle<JSWeakRef> jsWeakRef(thread, JSWeakRef::Cast(reinterpret_cast<TaggedObject *>(result.GetRawData())));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsWeakRef.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue result1 = BuiltinsWeakRef::Deref(ecmaRuntimeCallInfo);
    ASSERT_EQ(result1, target.GetTaggedValue());

    JSObject::SetProperty(thread, target, styleKey, styleValue);
    ASSERT_EQ(result1, target.GetTaggedValue());
}

// weakRef.Deref()
HWTEST_F_L0(BuiltinsWeakRefTest, Deref3)
{
    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> objectFunc(env->GetObjectFunction());

    JSTaggedValue target =
	    factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc).GetTaggedValue();
    JSTaggedValue result = CreateWeakRefConstructor(thread, target);
    JSHandle<JSWeakRef> jsWeakRef(thread, JSWeakRef::Cast(reinterpret_cast<TaggedObject *>(result.GetRawData())));
    JSTaggedValue result2 = JSTaggedValue::Undefined();

    {
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        auto obj =
	        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc);
        target = obj.GetTaggedValue();
        auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo1->SetThis(jsWeakRef.GetTaggedValue());

        [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
        result2 = BuiltinsWeakRef::Deref(ecmaRuntimeCallInfo1);
        TestHelper::TearDownFrame(thread, prev1);
    }
    vm->CollectGarbage(TriggerGCType::FULL_GC);
    if (!thread->HasPendingException()) {
        job::MicroJobQueue::ExecutePendingJob(thread, vm->GetJSThread()->GetCurrentEcmaContext()->GetMicroJobQueue());
    }
    vm->SetEnableForceGC(true);
    ASSERT_TRUE(!result2.IsUndefined());
}

// symbol target
HWTEST_F_L0(BuiltinsWeakRefTest, SymbolTarget)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSSymbol> symbolTarget = thread->GetEcmaVM()->GetFactory()->NewJSSymbol();
    JSHandle<JSTaggedValue> target(symbolTarget);

    JSHandle<JSFunction> weakRef(env->GetBuiltinsWeakRefFunction());

    auto ecmaRuntimeCallInfo1 =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, weakRef.GetTaggedValue(), 6); // 6 means 1 call arg
    ecmaRuntimeCallInfo1->SetFunction(weakRef.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetCallArg(0, target.GetTaggedValue());

    // constructor
    [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result1 = BuiltinsWeakRef::WeakRefConstructor(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev1);
    ASSERT_TRUE(result1.IsECMAObject());

    JSHandle<JSWeakRef> jsWeakRef(thread, JSWeakRef::Cast(reinterpret_cast<TaggedObject *>(result1.GetRawData())));
    auto ecmaRuntimeCallInfo2 =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4); // 4 means 0 call arg
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(jsWeakRef.GetTaggedValue());

    // weakRef.Deref()
    [[maybe_unused]] auto prev2 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2);
    JSTaggedValue result2 = BuiltinsWeakRef::Deref(ecmaRuntimeCallInfo2);
    TestHelper::TearDownFrame(thread, prev2);
    ASSERT_EQ(result2, target.GetTaggedValue());
}
}  // namespace panda::test
