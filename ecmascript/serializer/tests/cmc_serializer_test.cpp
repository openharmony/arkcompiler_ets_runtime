/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <thread>

#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

#include "ecmascript/serializer/value_serializer.h"
#include "ecmascript/serializer/base_deserializer.h"

using namespace panda::ecmascript;
using namespace testing::ext;

namespace panda::test {

class CMCJSDeserializerTest {
public:
    CMCJSDeserializerTest() : ecmaVm(nullptr), scope(nullptr), thread(nullptr) {}

    void Init()
    {
        JSRuntimeOptions options;
        options.SetEnableCMCGC(true);
        ecmaVm = JSNApi::CreateEcmaVM(options);
        EXPECT_TRUE(ecmaVm != nullptr) << "Cannot create Runtime";
        thread = ecmaVm->GetJSThread();
        scope = new EcmaHandleScope(thread);
        thread->ManagedCodeBegin();
    }
    void Destroy()
    {
        thread->ManagedCodeEnd();
        delete scope;
        scope = nullptr;
        thread->ClearException();
        JSNApi::DestroyJSVM(ecmaVm);
    }

    void SerializeTaggedArrayTest(SerializeData *data)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        EXPECT_TRUE(!res.IsEmpty());
        EXPECT_TRUE(res->IsTaggedArray());
        Destroy();
    }
private:
    EcmaVM *ecmaVm = nullptr;
    EcmaHandleScope *scope = nullptr;
    JSThread *thread = nullptr;
};

class CMCJSSerializerTest : public testing::Test {
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
        JSRuntimeOptions options;
        options.SetEnableCMCGC(true);
        ecmaVm = JSNApi::CreateEcmaVM(options);
        EXPECT_TRUE(ecmaVm != nullptr) << "Cannot create Runtime";
        thread = ecmaVm->GetJSThread();
        scope = new EcmaHandleScope(thread);
        thread->ManagedCodeBegin();
    }

    void TearDown() override
    {
        thread->ManagedCodeEnd();
        delete scope;
        scope = nullptr;
        thread->ClearException();
        JSNApi::DestroyJSVM(ecmaVm);
    }

    JSThread *thread {nullptr};
    EcmaVM *ecmaVm {nullptr};
    EcmaHandleScope *scope {nullptr};
};

HWTEST_F_L0(CMCJSSerializerTest, SerializeTaggedArrayTest)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<TaggedArray> array = factory->NewTaggedArray(10, JSTaggedValue::Hole());  // 10: array length
    for (int i = 0; i < 5; i++) { // 5: new array cycles
        JSHandle<TaggedArray> array1 =
            factory->NewTaggedArray(10, JSTaggedValue::Hole()); // 10: array length
        JSHandle<TaggedArray> array2 =
            factory->NewTaggedArray(10, JSTaggedValue::Hole(), MemSpaceType::NON_MOVABLE); // 10: array length
        array->Set(thread, i, array1);
        array->Set(thread, i + 1, array2);
    }
    ValueSerializer *serializer = new ValueSerializer(thread, false, true);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(array),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success);
    std::unique_ptr<SerializeData> data = serializer->Release();
    CMCJSDeserializerTest jsDeserializerTest;
    std::thread t1(&CMCJSDeserializerTest::SerializeTaggedArrayTest, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
};
}  // namespace panda::test
