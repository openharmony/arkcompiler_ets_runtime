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

#include "ecmascript/base/file_header.h"
#include "ecmascript/tests/test_helper.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/snapshot/mem/snapshot.h"
#include "ecmascript/snapshot/mem/snapshot_processor.h"
#include "ecmascript/ts_types/ts_manager.h"

using namespace panda::ecmascript;

namespace panda::test {
class SnapshotTest : public testing::Test {
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
        ecmaVm = JSNApi::CreateEcmaVM(options);
        ASSERT_TRUE(ecmaVm != nullptr) << "Cannot create EcmaVM";
        thread = ecmaVm->GetJSThread();
        scope = new EcmaHandleScope(thread);
    }

    void TearDown() override
    {
        delete scope;
        scope = nullptr;
        ecmaVm->SetEnableForceGC(false);
        thread->ClearException();
        JSNApi::DestroyJSVM(ecmaVm);
    }

    EcmaVM *ecmaVm {nullptr};
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(SnapshotTest, SerializeConstPool)
{
    auto factory = ecmaVm->GetFactory();
    auto env = ecmaVm->GetGlobalEnv();

    JSHandle<ConstantPool> constpool = factory->NewConstantPool(6);
    JSHandle<JSFunction> funcFunc(env->GetFunctionFunction());
    JSHandle<JSFunction> dateFunc(env->GetDateFunction());
    JSHandle<JSFunction> numberFunc(env->GetNumberFunction());
    JSHandle<EcmaString> str1 = factory->NewFromASCII("str11");
    JSHandle<EcmaString> str2 = factory->NewFromASCII("str22");
    constpool->SetObjectToCache(thread, 0, funcFunc.GetTaggedValue());
    constpool->SetObjectToCache(thread, 1, dateFunc.GetTaggedValue());
    constpool->SetObjectToCache(thread, 2, str1.GetTaggedValue());
    constpool->SetObjectToCache(thread, 3, numberFunc.GetTaggedValue());
    constpool->SetObjectToCache(thread, 4, str2.GetTaggedValue());
    constpool->SetObjectToCache(thread, 5, str1.GetTaggedValue());

    CString fileName = "snapshot";
    Snapshot snapshotSerialize(ecmaVm);
    // serialize
    snapshotSerialize.Serialize(*constpool, nullptr, fileName);
    // deserialize
    Snapshot snapshotDeserialize(ecmaVm);
    snapshotDeserialize.Deserialize(SnapshotType::VM_ROOT, fileName);

    auto beginRegion = const_cast<Heap *>(ecmaVm->GetHeap())->GetOldSpace()->GetCurrentRegion();
    auto constpool1 = reinterpret_cast<ConstantPool *>(beginRegion->GetBegin());
    EXPECT_EQ(constpool->GetClass()->SizeFromJSHClass(*constpool),
              constpool1->GetClass()->SizeFromJSHClass(constpool1));
    EXPECT_TRUE(constpool1->GetObjectFromCache(0).IsJSFunction());
    EXPECT_TRUE(constpool1->GetObjectFromCache(1).IsJSFunction());
    EXPECT_TRUE(constpool1->GetObjectFromCache(3).IsJSFunction());
    EcmaString *str11 = reinterpret_cast<EcmaString *>(constpool1->Get(2).GetTaggedObject());
    EcmaString *str22 = reinterpret_cast<EcmaString *>(constpool1->Get(4).GetTaggedObject());
    EcmaString *str33 = reinterpret_cast<EcmaString *>(constpool1->Get(5).GetTaggedObject());
    EXPECT_EQ(std::strcmp(EcmaStringAccessor(str11).ToCString().c_str(), "str11"), 0);
    EXPECT_EQ(std::strcmp(EcmaStringAccessor(str22).ToCString().c_str(), "str22"), 0);
    EXPECT_EQ(std::strcmp(EcmaStringAccessor(str33).ToCString().c_str(), "str11"), 0);
    std::remove(fileName.c_str());
}

HWTEST_F_L0(SnapshotTest, SerializeDifferentSpace)
{
    auto factory = ecmaVm->GetFactory();
    JSHandle<ConstantPool> constpool = factory->NewConstantPool(400);
    for (int i = 0; i < 100; i++) {
        JSHandle<TaggedArray> array = factory->NewTaggedArray(10, JSTaggedValue::Hole(), MemSpaceType::SEMI_SPACE);
        constpool->SetObjectToCache(thread, i, array.GetTaggedValue());
    }
    for (int i = 0; i < 100; i++) {
        JSHandle<TaggedArray> array = factory->NewTaggedArray(10, JSTaggedValue::Hole(), MemSpaceType::OLD_SPACE);
        constpool->SetObjectToCache(thread, i + 100, array.GetTaggedValue());
    }
    for (int i = 0; i < 100; i++) {
        JSHandle<MachineCode> codeObj = factory->NewMachineCodeObject(0, nullptr);
        constpool->SetObjectToCache(thread, i + 200, codeObj.GetTaggedValue());
    }
    for (int i = 0; i < 100; i++) {
        JSHandle<ConstantPool> constpool1 = factory->NewConstantPool(10);
        constpool->SetObjectToCache(thread, i + 300, constpool1.GetTaggedValue());
    }

    CString fileName = "snapshot";
    Snapshot snapshotSerialize(ecmaVm);
    // serialize
    snapshotSerialize.Serialize(*constpool, nullptr, fileName);
    // deserialize
    Snapshot snapshotDeserialize(ecmaVm);
    snapshotDeserialize.Deserialize(SnapshotType::VM_ROOT, fileName);

    auto beginRegion = const_cast<Heap *>(ecmaVm->GetHeap())->GetOldSpace()->GetCurrentRegion();
    auto constpool1 = reinterpret_cast<ConstantPool *>(beginRegion->GetBegin());
    EXPECT_EQ(constpool->GetClass()->SizeFromJSHClass(*constpool),
              constpool1->GetClass()->SizeFromJSHClass(constpool1));
    EXPECT_TRUE(constpool1->GetObjectFromCache(0).IsTaggedArray());
    EXPECT_TRUE(constpool1->GetObjectFromCache(100).IsTaggedArray());
    EXPECT_TRUE(constpool1->GetObjectFromCache(200).IsMachineCodeObject());
    EXPECT_TRUE(constpool1->GetObjectFromCache(300).IsTaggedArray());
    auto obj1 = constpool1->GetObjectFromCache(0).GetTaggedObject();
    EXPECT_TRUE(Region::ObjectAddressToRange(obj1)->InOldSpace());
    auto obj2 = constpool1->GetObjectFromCache(100).GetTaggedObject();
    EXPECT_TRUE(Region::ObjectAddressToRange(obj2)->InOldSpace());
    auto obj3 = constpool1->GetObjectFromCache(200).GetTaggedObject();
    auto region = Region::ObjectAddressToRange(obj3);
    EXPECT_TRUE(region->InMachineCodeSpace());

    std::remove(fileName.c_str());
}

HWTEST_F_L0(SnapshotTest, SerializeMultiFile)
{
    auto factory = ecmaVm->GetFactory();
    JSHandle<ConstantPool> constpool1 = factory->NewConstantPool(400);
    JSHandle<ConstantPool> constpool2 = factory->NewConstantPool(400);
    for (int i = 0; i < 100; i++) {
        JSHandle<TaggedArray> array = factory->NewTaggedArray(10, JSTaggedValue::Hole(), MemSpaceType::SEMI_SPACE);
        constpool1->SetObjectToCache(thread, i, array.GetTaggedValue());
        constpool2->SetObjectToCache(thread, i, array.GetTaggedValue());
    }
    for (int i = 0; i < 100; i++) {
        JSHandle<TaggedArray> array = factory->NewTaggedArray(10, JSTaggedValue::Hole(), MemSpaceType::OLD_SPACE);
        constpool1->SetObjectToCache(thread, i + 100, array.GetTaggedValue());
        constpool2->SetObjectToCache(thread, i + 100, array.GetTaggedValue());
    }
    for (int i = 0; i < 100; i++) {
        JSHandle<MachineCode> codeObj = factory->NewMachineCodeObject(0, nullptr);
        constpool1->SetObjectToCache(thread, i + 200, codeObj.GetTaggedValue());
        constpool2->SetObjectToCache(thread, i + 200, codeObj.GetTaggedValue());
    }
    for (int i = 0; i < 100; i++) {
        JSHandle<ConstantPool> constpool3 = factory->NewConstantPool(10);
        constpool1->SetObjectToCache(thread, i + 300, constpool3.GetTaggedValue());
        constpool2->SetObjectToCache(thread, i + 300, constpool3.GetTaggedValue());
    }

    CString fileName1 = "snapshot1";
    CString fileName2 = "snapshot2";
    Snapshot snapshotSerialize(ecmaVm);
    // serialize
    snapshotSerialize.Serialize(*constpool1, nullptr, fileName1);
    snapshotSerialize.Serialize(*constpool2, nullptr, fileName2);
    // deserialize
    Snapshot snapshotDeserialize(ecmaVm);
    snapshotDeserialize.Deserialize(SnapshotType::VM_ROOT, fileName1);
    snapshotDeserialize.Deserialize(SnapshotType::VM_ROOT, fileName2);

    auto beginRegion = const_cast<Heap *>(ecmaVm->GetHeap())->GetOldSpace()->GetCurrentRegion();
    auto constpool = reinterpret_cast<ConstantPool *>(beginRegion->GetBegin());
    EXPECT_TRUE(constpool->GetObjectFromCache(0).IsTaggedArray());
    EXPECT_TRUE(constpool->GetObjectFromCache(100).IsTaggedArray());
    EXPECT_TRUE(constpool->GetObjectFromCache(200).IsMachineCodeObject());
    auto obj1 = constpool->GetObjectFromCache(0).GetTaggedObject();
    EXPECT_TRUE(Region::ObjectAddressToRange(obj1)->InOldSpace());
    auto obj2 = constpool->GetObjectFromCache(100).GetTaggedObject();
    EXPECT_TRUE(Region::ObjectAddressToRange(obj2)->InOldSpace());
    auto obj3 = constpool->GetObjectFromCache(200).GetTaggedObject();
    auto region = Region::ObjectAddressToRange(obj3);
    EXPECT_TRUE(region->InMachineCodeSpace());

    std::remove(fileName1.c_str());
    std::remove(fileName2.c_str());
}

HWTEST_F_L0(SnapshotTest, SerializeBuiltins)
{
    // remove builtins.snapshot file first if exist
    CString fileName = "builtins.snapshot";
    std::remove(fileName.c_str());
    // generate builtins.snapshot file
    JSRuntimeOptions options1;
    options1.SetArkProperties(ArkProperties::ENABLE_SNAPSHOT_SERIALIZE);
    EcmaVM *ecmaVm1 = JSNApi::CreateEcmaVM(options1);
    JSNApi::DestroyJSVM(ecmaVm1);

    // create EcmaVM use builtins deserialzie
    JSRuntimeOptions options2;
    options2.SetArkProperties(ArkProperties::ENABLE_SNAPSHOT_DESERIALIZE);
    EcmaVM *ecmaVm2 = JSNApi::CreateEcmaVM(options2);
    EXPECT_TRUE(ecmaVm2->GetGlobalEnv()->GetClass()->GetObjectType() == JSType::GLOBAL_ENV);
    auto globalConst = const_cast<GlobalEnvConstants *>(ecmaVm2->GetJSThread()->GlobalConstants());
    size_t hclassEndIndex = static_cast<size_t>(ConstantIndex::UNDEFINED_INDEX);
    size_t hclassIndex = 0;
    globalConst->VisitRangeSlot([&hclassIndex, &hclassEndIndex]([[maybe_unused]]Root type,
                                                                ObjectSlot start, ObjectSlot end) {
        while (start < end) {
            JSTaggedValue object(start.GetTaggedType());
            start++;
            if (hclassIndex < hclassEndIndex) {
                EXPECT_TRUE(object.IsJSHClass());
            }
            hclassIndex++;
        }
    });
    JSNApi::DestroyJSVM(ecmaVm2);

    std::remove(fileName.c_str());
}

HWTEST_F_L0(SnapshotTest, SerializeHugeObject)
{
    auto factory = ecmaVm->GetFactory();
    auto env = ecmaVm->GetGlobalEnv();

    JSHandle<TaggedArray> array1 = factory->NewTaggedArray(300 * 1024 / 8);
    JSHandle<TaggedArray> array2 = factory->NewTaggedArray(300 * 1024 / 8);
    JSHandle<TaggedArray> array3 = factory->NewTaggedArray(100);

    JSHandle<JSFunction> funcFunc(env->GetFunctionFunction());
    JSHandle<JSFunction> dateFunc(env->GetDateFunction());
    JSHandle<JSFunction> numberFunc(env->GetNumberFunction());
    array1->Set(thread, 0, array2.GetTaggedValue());
    array1->Set(thread, 1, funcFunc.GetTaggedValue());
    array1->Set(thread, 2, dateFunc.GetTaggedValue());
    array1->Set(thread, 3, numberFunc.GetTaggedValue());
    array2->Set(thread, 0, array3.GetTaggedValue());
    array2->Set(thread, 1, funcFunc.GetTaggedValue());
    array2->Set(thread, 2, dateFunc.GetTaggedValue());
    array2->Set(thread, 3, numberFunc.GetTaggedValue());

    CString fileName = "snapshot";
    Snapshot snapshotSerialize(ecmaVm);
    // serialize
    snapshotSerialize.Serialize(*array1, nullptr, fileName);
    // deserialize
    Snapshot snapshotDeserialize(ecmaVm);
    snapshotDeserialize.Deserialize(SnapshotType::VM_ROOT, fileName);

    auto lastRegion = const_cast<Heap *>(ecmaVm->GetHeap())->GetHugeObjectSpace()->GetCurrentRegion();
    auto array4 = reinterpret_cast<TaggedArray *>(lastRegion->GetBegin());
    EXPECT_TRUE(array4->Get(0).IsTaggedArray());
    EXPECT_TRUE(array4->Get(1).IsJSFunction());
    EXPECT_TRUE(array4->Get(2).IsJSFunction());
    EXPECT_TRUE(array4->Get(3).IsJSFunction());
    std::remove(fileName.c_str());
}

HWTEST_F_L0(SnapshotTest, VersionTest)
{
    base::FileHeader::VersionType version = {4, 3, 2, 1};
    uint32_t versionNumber = 0x04030201U;
    EXPECT_EQ(version, base::FileHeader::ToVersion(versionNumber));
    EXPECT_EQ(versionNumber, base::FileHeader::ToVersionNumber(version));
}
}  // namespace panda::test
