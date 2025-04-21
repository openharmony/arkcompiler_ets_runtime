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

using namespace panda::ecmascript;

namespace panda::test {
class BarrierTest : public BaseTestWithScope<true> {
};

HWTEST_F_L0(BarrierTest, YoungToYoungBatchCopy)
{
    ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
    uint32_t arrayLength = 10;
    JSHandle<TaggedArray> srcArray = factory->NewTaggedArray(arrayLength);
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    for (uint32_t i = 0; i < arrayLength; i++) {
        JSHandle<JSFunction> newFun = factory->NewJSFunction(env);
        srcArray->Set(thread, i, newFun);
    }

    JSHandle<TaggedArray> dstArray = factory->NewTaggedArray(arrayLength);
    std::set<uintptr_t> LocalToShareBeforeCopy;

    Region* dstRegion = Region::ObjectAddressToRange(dstArray.GetObject<TaggedArray>());
    dstRegion->IterateAllLocalToShareBits([&LocalToShareBeforeCopy](void* mem) {
        LocalToShareBeforeCopy.emplace(ToUintPtr(mem));
        return true;
    });

    JSTaggedValue* to = reinterpret_cast<JSTaggedValue*>(ToUintPtr(dstArray->GetData()));
    JSTaggedValue* from = reinterpret_cast<JSTaggedValue*>(ToUintPtr(srcArray->GetData()));
    Barriers::CopyObject<true, false>(thread, *dstArray, to, from, arrayLength);

    // young to young, all the bitset should not be changed.
    dstRegion->IterateAllLocalToShareBits([&LocalToShareBeforeCopy](void* mem) {
        EXPECT_TRUE(LocalToShareBeforeCopy.count(ToUintPtr(mem)));
        return true;
    });

    // check
    for (uint32_t i = 0; i < arrayLength; i++) {
        EXPECT_EQ(dstArray->Get(thread, i), srcArray->Get(thread, i));
    }
}

HWTEST_F_L0(BarrierTest, BatchCopyNoBarrier)
{
    ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
    uint32_t arrayLength = 10;
    JSHandle<TaggedArray> srcArray = factory->NewTaggedArray(arrayLength);
    for (uint32_t i = 0; i < arrayLength; i++) {
        srcArray->Set(thread, i, JSTaggedValue(i));
    }

    JSHandle<TaggedArray> dstArray = factory->NewTaggedArray(arrayLength);
    std::set<uintptr_t> LocalToShareBeforeCopy;

    Region* dstRegion = Region::ObjectAddressToRange(dstArray.GetObject<TaggedArray>());
    dstRegion->IterateAllLocalToShareBits([&LocalToShareBeforeCopy](void* mem) {
        LocalToShareBeforeCopy.emplace(ToUintPtr(mem));
        return true;
    });

    JSTaggedValue* to = reinterpret_cast<JSTaggedValue*>(ToUintPtr(dstArray->GetData()));
    JSTaggedValue* from = reinterpret_cast<JSTaggedValue*>(ToUintPtr(srcArray->GetData()));
    Barriers::CopyObjectPrimitive<false>(to, from, arrayLength);

    dstRegion->IterateAllLocalToShareBits([&LocalToShareBeforeCopy](void* mem) {
        EXPECT_TRUE(LocalToShareBeforeCopy.count(ToUintPtr(mem)));
        return true;
    });

    // check
    for (uint32_t i = 0; i < arrayLength; i++) {
        EXPECT_EQ(dstArray->Get(thread, i), srcArray->Get(thread, i));
    }

    JSHandle<TaggedArray> dstArray2 = factory->NewTaggedArray(arrayLength);
    JSTaggedValue* to2 = reinterpret_cast<JSTaggedValue*>(ToUintPtr(dstArray2->GetData()));
    JSTaggedValue* from2 = reinterpret_cast<JSTaggedValue*>(ToUintPtr(srcArray->GetData()));
    // barrier should also work for no heap value
    Barriers::CopyObject<true, false>(thread, *dstArray, to2, from2, arrayLength);
    // check
    for (uint32_t i = 0; i < arrayLength; i++) {
        EXPECT_EQ(dstArray2->Get(thread, i), srcArray->Get(thread, i));
    }
}

HWTEST_F_L0(BarrierTest, LocalToShareBatchCopy)
{
    ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
    uint32_t arrayLength = 10;
    JSHandle<TaggedArray> srcArray = factory->NewTaggedArray(arrayLength);
    for (uint32_t i = 0; i < arrayLength; i++) {
        JSHandle<EcmaString> str = factory->NewFromStdString(std::to_string(i) + "_" + std::to_string(i));
        // string longer than 1 will be in sweepable shared heap
        srcArray->Set(thread, i, str);
    }

    JSHandle<TaggedArray> dstArray = factory->NewTaggedArray(arrayLength);
    std::set<uintptr_t> LocalToShareBeforeCopy;

    Region* dstRegion = Region::ObjectAddressToRange(dstArray.GetObject<TaggedArray>());
    dstRegion->IterateAllLocalToShareBits([&LocalToShareBeforeCopy](void* mem) {
        LocalToShareBeforeCopy.emplace(ToUintPtr(mem));
        return true;
    });

    JSTaggedValue* to = reinterpret_cast<JSTaggedValue*>(ToUintPtr(dstArray->GetData()));
    JSTaggedValue* from = reinterpret_cast<JSTaggedValue*>(ToUintPtr(srcArray->GetData()));
    Barriers::CopyObject<true, false>(thread, *dstArray, to, from, arrayLength);

    std::set<uintptr_t> LocalToShareSlot;
    for (uint32_t i = 0; i < arrayLength; i++) {
        LocalToShareSlot.insert(ToUintPtr(dstArray->GetData() + i));
    }
    // young to young, all the bitset should not be changed.
    dstRegion->IterateAllLocalToShareBits([&LocalToShareSlot, &LocalToShareBeforeCopy, &dstArray, arrayLength](
        void* mem) {
            if (!LocalToShareBeforeCopy.count(ToUintPtr(mem))) {
                EXPECT_GE(ToUintPtr(mem), ToUintPtr(dstArray->GetData()));
                EXPECT_LT(ToUintPtr(mem), ToUintPtr(dstArray->GetData()+arrayLength));
                LocalToShareSlot.erase(ToUintPtr(mem));
            } else {
                EXPECT_TRUE(LocalToShareBeforeCopy.count(ToUintPtr(mem)));
            }
            return true;
        });
    EXPECT_TRUE(LocalToShareSlot.empty());
    // check
    for (uint32_t i = 0; i < arrayLength; i++) {
        EXPECT_EQ(dstArray->Get(thread, i), srcArray->Get(thread, i));
    }
}

HWTEST_F_L0(BarrierTest, LocalToReadOnlyShareBatchCopy)
{
    ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
    uint32_t arrayLength = 10;
    JSHandle<TaggedArray> srcArray = factory->NewTaggedArray(arrayLength);
    for (uint32_t i = 0; i < arrayLength; i++) {
        JSHandle<EcmaString> str = factory->NewFromStdString(std::to_string(i));
        // One byte string is in readonly
        srcArray->Set(thread, i, str);
    }

    JSHandle<TaggedArray> dstArray = factory->NewTaggedArray(arrayLength);
    std::set<uintptr_t> LocalToShareBeforeCopy;

    Region* dstRegion = Region::ObjectAddressToRange(dstArray.GetObject<TaggedArray>());
    dstRegion->IterateAllLocalToShareBits([&LocalToShareBeforeCopy](void* mem) {
        LocalToShareBeforeCopy.emplace(ToUintPtr(mem));
        return true;
    });

    JSTaggedValue* to = reinterpret_cast<JSTaggedValue*>(ToUintPtr(dstArray->GetData()));
    JSTaggedValue* from = reinterpret_cast<JSTaggedValue*>(ToUintPtr(srcArray->GetData()));
    Barriers::CopyObject<true, false>(thread, *dstArray, to, from, arrayLength);

    std::set<uintptr_t> LocalToShareSlot;
    for (uint32_t i = 0; i < arrayLength; i++) {
        LocalToShareSlot.insert(ToUintPtr(dstArray->GetData() + i));
    }
    // young to young, all the bitset should not be changed.
    dstRegion->IterateAllLocalToShareBits([&LocalToShareSlot, &LocalToShareBeforeCopy](
        void* mem) {
            EXPECT_FALSE(LocalToShareSlot.count(ToUintPtr(mem)));
            EXPECT_TRUE(LocalToShareBeforeCopy.count(ToUintPtr(mem)));
            return true;
        });
    // check
    for (uint32_t i = 0; i < arrayLength; i++) {
        EXPECT_EQ(dstArray->Get(thread, i), srcArray->Get(thread, i));
    }
}

HWTEST_F_L0(BarrierTest, LocalToShareMixBatchCopy)
{
    ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
    uint32_t arrayLength = 10;
    JSHandle<TaggedArray> srcArray = factory->NewTaggedArray(arrayLength);
    for (uint32_t i = 0; i < arrayLength; i++) {
        if (i % 2 == 0) {
            JSHandle<EcmaString> str = factory->NewFromStdString(std::to_string(i) + "_" + std::to_string(i));
            // One byte string is in readonly
            srcArray->Set(thread, i, str);
        } else {
            srcArray->Set(thread, i, JSTaggedValue(i));
        }
    }

    JSHandle<TaggedArray> dstArray = factory->NewTaggedArray(arrayLength);
    std::set<uintptr_t> LocalToShareBeforeCopy;

    Region* dstRegion = Region::ObjectAddressToRange(dstArray.GetObject<TaggedArray>());
    dstRegion->IterateAllLocalToShareBits([&LocalToShareBeforeCopy](void* mem) {
        LocalToShareBeforeCopy.emplace(ToUintPtr(mem));
        return true;
    });

    JSTaggedValue* to = reinterpret_cast<JSTaggedValue*>(ToUintPtr(dstArray->GetData()));
    JSTaggedValue* from = reinterpret_cast<JSTaggedValue*>(ToUintPtr(srcArray->GetData()));
    Barriers::CopyObject<true, false>(thread, *dstArray, to, from, arrayLength);

    std::set<uintptr_t> LocalToShareSlot;
    for (uint32_t i = 0; i < arrayLength; i++) {
        if (i % 2 == 0) {
            LocalToShareSlot.insert(ToUintPtr(dstArray->GetData() + i));
        }
    }
    // young to young, all the bitset should not be changed.
    dstRegion->IterateAllLocalToShareBits([&LocalToShareSlot, &LocalToShareBeforeCopy, &dstArray, arrayLength](
        void* mem) {
            if (!LocalToShareBeforeCopy.count(ToUintPtr(mem))) {
                EXPECT_GE(ToUintPtr(mem), ToUintPtr(dstArray->GetData()));
                EXPECT_LT(ToUintPtr(mem), ToUintPtr(dstArray->GetData()+arrayLength));
                LocalToShareSlot.erase(ToUintPtr(mem));
            } else {
                EXPECT_TRUE(LocalToShareBeforeCopy.count(ToUintPtr(mem)));
            }
            return true;
        });
    EXPECT_TRUE(LocalToShareSlot.empty());
    // check
    for (uint32_t i = 0; i < arrayLength; i++) {
        EXPECT_EQ(dstArray->Get(thread, i), srcArray->Get(thread, i));
    }
}

HWTEST_F_L0(BarrierTest, OldToNewBatchCopy)
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

    std::set<uintptr_t> OldToNewSlot;
    for (uint32_t i = 0; i < arrayLength; i++) {
        if (Region::ObjectAddressToRange(srcArray->Get(thread, i).GetTaggedObject())->InYoungSpace()) {
            OldToNewSlot.insert(ToUintPtr(dstArray->GetData() + i));
        }
    }
    EXPECT_TRUE(!OldToNewSlot.empty());

    std::set<uintptr_t> OldToNewBeforeCopy;
    std::set<uintptr_t> LocalToShareBeforeCopy;

    Region* dstRegion = Region::ObjectAddressToRange(dstArray.GetObject<TaggedArray>());
    dstRegion->IterateAllOldToNewBits([&OldToNewBeforeCopy](void* mem) {
        OldToNewBeforeCopy.emplace(ToUintPtr(mem));
        return true;
    });
    dstRegion->IterateAllLocalToShareBits([&LocalToShareBeforeCopy](void* mem) {
        LocalToShareBeforeCopy.emplace(ToUintPtr(mem));
        return true;
    });

    JSTaggedValue* to = reinterpret_cast<JSTaggedValue*>(ToUintPtr(dstArray->GetData()));
    JSTaggedValue* from = reinterpret_cast<JSTaggedValue*>(ToUintPtr(srcArray->GetData()));
    Barriers::CopyObject<true, false>(thread, *dstArray, to, from, arrayLength);

    // young to young, all the bitset should not be changed.
    dstRegion->IterateAllLocalToShareBits([&LocalToShareBeforeCopy](void* mem) {
        EXPECT_TRUE(LocalToShareBeforeCopy.count(ToUintPtr(mem)));
        return true;
    });
    // check
    for (uint32_t i = 0; i < arrayLength; i++) {
        EXPECT_EQ(dstArray->Get(thread, i), srcArray->Get(thread, i));
    }
}

HWTEST_F_L0(BarrierTest, LocalToShareUnshift)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t arrayLength = 40;
    JSHandle<TaggedArray> array = factory->NewTaggedArray(arrayLength);
    for (uint32_t i = 0; i < arrayLength; i++) {
        JSHandle<EcmaString> str = factory->NewFromStdString(std::to_string(i) + "_" + std::to_string(i));
        // string longer than 1 will be in sweepable shared heap
        array->Set(thread, i, str);
    }
    JSHandle<JSArray> jsArray = JSArray::CreateArrayFromList(thread, array);

    JSHandle<EcmaString> unshiftStr = factory->NewFromStdString("unshift");
    auto unshiftFunc = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>(jsArray),
                                                  JSHandle<JSTaggedValue>(unshiftStr)).GetValue();
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, unshiftFunc.GetTaggedValue(),
                                                                    jsArray.GetTaggedValue(),
                                                                    thread->GlobalConstants()->GetUndefined(), 1);
    info->SetCallArg(thread->GlobalConstants()->GetUndefined());
    EcmaInterpreter::Execute(info);

    array = JSHandle<TaggedArray>(thread, jsArray->GetElements());
    Region *dstRegion = Region::ObjectAddressToRange(array.GetObject<TaggedArray>());
    std::set<uintptr_t> LocalToShareSlot;
    for (uint32_t i = 1; i < arrayLength + 1; i++) {
        LocalToShareSlot.insert(ToUintPtr(array->GetData() + i));
    }
    dstRegion->IterateAllLocalToShareBits([&LocalToShareSlot](void *mem) {
        LocalToShareSlot.erase(ToUintPtr(mem));
        return true;
    });

    EXPECT_TRUE(LocalToShareSlot.empty());
}

HWTEST_F_L0(BarrierTest, UnshiftBarrierMoveForward)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t arrayLength = 3;
    JSHandle<TaggedArray> array = factory->NewTaggedArray(arrayLength);
    for (uint32_t i = 0; i < arrayLength; i++) {
        JSHandle<EcmaString> str = factory->NewFromStdString(std::to_string(i) + "_" + std::to_string(i));
        // string longer than 1 will be in sweepable shared heap
        array->Set(thread, i, str);
    }
    JSHandle<JSArray> jsArray = JSArray::CreateArrayFromList(thread, array);

    JSHandle<EcmaString> unshiftStr = factory->NewFromStdString("unshift");
    auto unshiftFunc = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>(jsArray),
                                                  JSHandle<JSTaggedValue>(unshiftStr)).GetValue();
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, unshiftFunc.GetTaggedValue(),
                                                                    jsArray.GetTaggedValue(),
                                                                    thread->GlobalConstants()->GetUndefined(), 3);
    info->SetCallArg(thread->GlobalConstants()->GetUndefined(), thread->GlobalConstants()->GetUndefined(),
                     thread->GlobalConstants()->GetUndefined());
    EcmaInterpreter::Execute(info);

    array = JSHandle<TaggedArray>(thread, jsArray->GetElements());
    Region *dstRegion = Region::ObjectAddressToRange(array.GetObject<TaggedArray>());
    std::set<uintptr_t> LocalToShareSlot;
    for (uint32_t i = 3; i < arrayLength + 3; i++) {
        LocalToShareSlot.insert(ToUintPtr(array->GetData() + i));
    }
    dstRegion->IterateAllLocalToShareBits([&LocalToShareSlot](void *mem) {
        LocalToShareSlot.erase(ToUintPtr(mem));
        return true;
    });

    EXPECT_TRUE(LocalToShareSlot.empty());
}

HWTEST_F_L0(BarrierTest, UnshiftBarrierMoveBackward)
{
    if (!thread->IsAsmInterpreter()) {
        return;
    }
#ifdef ENABLE_NEXT_OPTIMIZATION
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t arrayLength = 20;
    JSHandle<TaggedArray> array = factory->NewTaggedArray(arrayLength);
    for (uint32_t i = 0; i < arrayLength; i++) {
        if (i % 2 == 0) {
            JSHandle<EcmaString> str = factory->NewFromStdString(std::to_string(i) + "_" + std::to_string(i));
            // string longer than 1 will be in sweepable shared heap
            array->Set(thread, i, str);
            continue;
        }
        array->Set(thread, i, JSTaggedValue(1));
    }
    JSHandle<JSArray> jsArray = JSArray::CreateArrayFromList(thread, array);

    JSHandle<EcmaString> unshiftStr = factory->NewFromStdString("unshift");
    auto unshiftFunc = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>(jsArray),
                                                  JSHandle<JSTaggedValue>(unshiftStr)).GetValue();
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, unshiftFunc.GetTaggedValue(),
                                                                    jsArray.GetTaggedValue(),
                                                                    thread->GlobalConstants()->GetUndefined(), 1);
    info->SetCallArg(thread->GlobalConstants()->GetUndefined());
    EcmaInterpreter::Execute(info);

    array = JSHandle<TaggedArray>(thread, jsArray->GetElements());
    Region *dstRegion = Region::ObjectAddressToRange(array.GetObject<TaggedArray>());
    std::set<uintptr_t> LocalToShareSlot;
    std::set<uintptr_t> LocalTaggedIntSlot;
    for (uint32_t i = 1; i < arrayLength + 1; i++) {
        if (i % 2 != 0) {
            LocalToShareSlot.insert(ToUintPtr(array->GetData() + i));
            continue;
        }
        LocalTaggedIntSlot.insert(ToUintPtr(array->GetData() + i));
    }
    dstRegion->IterateAllLocalToShareBits([&LocalToShareSlot, &LocalTaggedIntSlot](void *mem) {
        EXPECT_FALSE(LocalTaggedIntSlot.count(ToUintPtr(mem)));
        LocalToShareSlot.erase(ToUintPtr(mem));
        return true;
    });

    EXPECT_TRUE(LocalToShareSlot.empty());
#endif
}

HWTEST_F_L0(BarrierTest, UnshiftBarrierMoveForward1)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t arrayLength = 3;
    JSHandle<TaggedArray> array = factory->NewTaggedArray(arrayLength);
    for (uint32_t i = 0; i < arrayLength; i++) {
        if (i % 2 != 0) {
            JSHandle<EcmaString> str = factory->NewFromStdString(std::to_string(i) + "_" + std::to_string(i));
            // string longer than 1 will be in sweepable shared heap
            array->Set(thread, i, str);
            continue;
        }
        array->Set(thread, i, JSTaggedValue(1));
    }
    JSHandle<JSArray> jsArray = JSArray::CreateArrayFromList(thread, array);

    JSHandle<EcmaString> unshiftStr = factory->NewFromStdString("unshift");
    auto unshiftFunc = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>(jsArray),
                                                  JSHandle<JSTaggedValue>(unshiftStr)).GetValue();
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, unshiftFunc.GetTaggedValue(),
                                                                    jsArray.GetTaggedValue(),
                                                                    thread->GlobalConstants()->GetUndefined(), 3);
    info->SetCallArg(thread->GlobalConstants()->GetUndefined(), thread->GlobalConstants()->GetUndefined(),
                     thread->GlobalConstants()->GetUndefined());
    EcmaInterpreter::Execute(info);

    array = JSHandle<TaggedArray>(thread, jsArray->GetElements());
    Region *dstRegion = Region::ObjectAddressToRange(array.GetObject<TaggedArray>());
    std::set<uintptr_t> LocalToShareSlot;
    std::set<uintptr_t> LocalTaggedIntSlot;
    for (uint32_t i = 3; i < arrayLength + 3; i++) {
        if (i % 2 == 0) {
            LocalToShareSlot.insert(ToUintPtr(array->GetData() + i));
            continue;
        }
        LocalTaggedIntSlot.insert(ToUintPtr(array->GetData() + i));
    }
    dstRegion->IterateAllLocalToShareBits([&LocalToShareSlot, &LocalTaggedIntSlot](void *mem) {
        EXPECT_FALSE(LocalTaggedIntSlot.count(ToUintPtr(mem)));
        LocalToShareSlot.erase(ToUintPtr(mem));
        return true;
    });

    EXPECT_TRUE(LocalToShareSlot.empty());
}

HWTEST_F_L0(BarrierTest, SliceBarrierMove)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t arrayLength = 20;
    JSHandle<TaggedArray> array = factory->NewTaggedArray(arrayLength);
    for (uint32_t i = 0; i < arrayLength; i++) {
        if (i % 2 == 0) {
            JSHandle<EcmaString> str = factory->NewFromStdString(std::to_string(i) + "_" + std::to_string(i));
            // string longer than 1 will be in sweepable shared heap
            array->Set(thread, i, str);
            continue;
        }
        array->Set(thread, i, JSTaggedValue(1));
    }
    JSHandle<JSArray> jsArray = JSArray::CreateArrayFromList(thread, array);

    JSHandle<EcmaString> sliceStr = factory->NewFromStdString("slice");
    auto unshiftFunc = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>(jsArray),
                                                  JSHandle<JSTaggedValue>(sliceStr)).GetValue();
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, unshiftFunc.GetTaggedValue(),
                                                                    jsArray.GetTaggedValue(),
                                                                    thread->GlobalConstants()->GetUndefined(), 1);
    info->SetCallArg(JSTaggedValue(10));
    JSTaggedValue result = EcmaInterpreter::Execute(info);
    JSArray *resultArray = JSArray::Cast(result.GetTaggedObject());
    array = JSHandle<TaggedArray>(thread, resultArray->GetElements());
    Region *dstRegion = Region::ObjectAddressToRange(array.GetObject<TaggedArray>());
    std::set<uintptr_t> LocalToShareSlot;
    std::set<uintptr_t> LocalTaggedIntSlot;
    for (uint32_t i = 0; i < 10; i++) {
        if (i % 2 == 0) {
            LocalToShareSlot.insert(ToUintPtr(array->GetData() + i));
            continue;
        }
        LocalTaggedIntSlot.insert(ToUintPtr(array->GetData() + i));
    }
    dstRegion->IterateAllLocalToShareBits([&LocalToShareSlot, &LocalTaggedIntSlot](void *mem) {
        EXPECT_FALSE(LocalTaggedIntSlot.count(ToUintPtr(mem)));
        LocalToShareSlot.erase(ToUintPtr(mem));
        return true;
    });

    EXPECT_TRUE(LocalToShareSlot.empty());
}

HWTEST_F_L0(BarrierTest, LocalToShareReverse)
{
    if (!thread->IsAsmInterpreter()) {
        return;
    }
#ifdef ENABLE_NEXT_OPTIMIZATION
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t arrayLength = 40;
    uint32_t loopCount = 128;
    for (uint32_t loop = 0; loop < loopCount; loop++) {
        JSHandle<TaggedArray> array = factory->NewTaggedArray(arrayLength);
        for (uint32_t i = 0; i < arrayLength; i++) {
            if (i % 2 == 0) {
                JSHandle<EcmaString> str = factory->NewFromStdString(std::to_string(i) + "_" + std::to_string(i));
                array->Set(thread, i, str);
            } else {
                array->Set(thread, i, JSTaggedValue(i));
            }
        }
        JSHandle<JSArray> jsArray = JSArray::CreateArrayFromList(thread, array);

        JSHandle<EcmaString> reverseStr = factory->NewFromStdString("reverse");
        auto reverseFunc = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>(jsArray),
                                                    JSHandle<JSTaggedValue>(reverseStr)).GetValue();
        EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, reverseFunc.GetTaggedValue(),
                                                                        jsArray.GetTaggedValue(),
                                                                        thread->GlobalConstants()->GetUndefined(), 0);
        EcmaInterpreter::Execute(info);

        array = JSHandle<TaggedArray>(thread, jsArray->GetElements());
        Region *dstRegion = Region::ObjectAddressToRange(array.GetObject<TaggedArray>());
        std::set<uintptr_t> LocalToShareSlot1;
        std::set<uintptr_t> LocalToShareSlot2;
        for (uint32_t i = 0; i < arrayLength; i++) {
            if (i % 2 == 1) {
                LocalToShareSlot1.insert(ToUintPtr(array->GetData() + i));
            } else {
                LocalToShareSlot2.insert(ToUintPtr(array->GetData() + i));
            }
        }
        dstRegion->IterateAllLocalToShareBits([&LocalToShareSlot1, &LocalToShareSlot2](void *mem) {
            LocalToShareSlot1.erase(ToUintPtr(mem));
            LocalToShareSlot2.erase(ToUintPtr(mem));
            return true;
        });
        EXPECT_TRUE(LocalToShareSlot1.empty());
        EXPECT_EQ(LocalToShareSlot2.size(), arrayLength / 2);
    }
#endif
}

} // namespace panda::ecmascript
