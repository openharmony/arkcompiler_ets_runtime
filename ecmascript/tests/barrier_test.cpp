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
    std::set<uintptr_t> NewToEdenBeforeCopy;
    std::set<uintptr_t> LocalToShareBeforeCopy;

    Region* dstRegion = Region::ObjectAddressToRange(dstArray.GetObject<TaggedArray>());
    dstRegion->IterateAllNewToEdenBits([&NewToEdenBeforeCopy](void* mem) {
        NewToEdenBeforeCopy.emplace(ToUintPtr(mem));
        return true;
    });
    dstRegion->IterateAllLocalToShareBits([&LocalToShareBeforeCopy](void* mem) {
        LocalToShareBeforeCopy.emplace(ToUintPtr(mem));
        return true;
    });

    JSTaggedValue* to = reinterpret_cast<JSTaggedValue*>(ToUintPtr(dstArray->GetData()));
    JSTaggedValue* from = reinterpret_cast<JSTaggedValue*>(ToUintPtr(srcArray->GetData()));
    Barriers::CopyObject<true, false>(thread, to, from, arrayLength);

    // young to young, all the bitset should not be changed.
    dstRegion->IterateAllNewToEdenBits([&NewToEdenBeforeCopy](void* mem) {
        EXPECT_TRUE(NewToEdenBeforeCopy.count(ToUintPtr(mem)));
        return true;
    });
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
    std::set<uintptr_t> NewToEdenBeforeCopy;
    std::set<uintptr_t> LocalToShareBeforeCopy;

    Region* dstRegion = Region::ObjectAddressToRange(dstArray.GetObject<TaggedArray>());
    dstRegion->IterateAllNewToEdenBits([&NewToEdenBeforeCopy](void* mem) {
        NewToEdenBeforeCopy.emplace(ToUintPtr(mem));
        return true;
    });
    dstRegion->IterateAllLocalToShareBits([&LocalToShareBeforeCopy](void* mem) {
        LocalToShareBeforeCopy.emplace(ToUintPtr(mem));
        return true;
    });

    JSTaggedValue* to = reinterpret_cast<JSTaggedValue*>(ToUintPtr(dstArray->GetData()));
    JSTaggedValue* from = reinterpret_cast<JSTaggedValue*>(ToUintPtr(srcArray->GetData()));
    Barriers::CopyObjectPrimitive<false>(to, from, arrayLength);

    dstRegion->IterateAllNewToEdenBits([&NewToEdenBeforeCopy](void* mem) {
        EXPECT_TRUE(NewToEdenBeforeCopy.count(ToUintPtr(mem)));
        return true;
    });
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
    Barriers::CopyObject<true, false>(thread, to2, from2, arrayLength);
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
    std::set<uintptr_t> NewToEdenBeforeCopy;
    std::set<uintptr_t> LocalToShareBeforeCopy;

    Region* dstRegion = Region::ObjectAddressToRange(dstArray.GetObject<TaggedArray>());
    dstRegion->IterateAllNewToEdenBits([&NewToEdenBeforeCopy](void* mem) {
        NewToEdenBeforeCopy.emplace(ToUintPtr(mem));
        return true;
    });
    dstRegion->IterateAllLocalToShareBits([&LocalToShareBeforeCopy](void* mem) {
        LocalToShareBeforeCopy.emplace(ToUintPtr(mem));
        return true;
    });

    JSTaggedValue* to = reinterpret_cast<JSTaggedValue*>(ToUintPtr(dstArray->GetData()));
    JSTaggedValue* from = reinterpret_cast<JSTaggedValue*>(ToUintPtr(srcArray->GetData()));
    Barriers::CopyObject<true, false>(thread, to, from, arrayLength);

    std::set<uintptr_t> LocalToShareSlot;
    for (uint32_t i = 0; i < arrayLength; i++) {
        LocalToShareSlot.insert(ToUintPtr(dstArray->GetData() + i));
    }
    // young to young, all the bitset should not be changed.
    dstRegion->IterateAllNewToEdenBits([&NewToEdenBeforeCopy](void* mem) {
        EXPECT_TRUE(NewToEdenBeforeCopy.count(ToUintPtr(mem)));
        return true;
    });
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
    std::set<uintptr_t> NewToEdenBeforeCopy;
    std::set<uintptr_t> LocalToShareBeforeCopy;

    Region* dstRegion = Region::ObjectAddressToRange(dstArray.GetObject<TaggedArray>());
    dstRegion->IterateAllNewToEdenBits([&NewToEdenBeforeCopy](void* mem) {
        NewToEdenBeforeCopy.emplace(ToUintPtr(mem));
        return true;
    });
    dstRegion->IterateAllLocalToShareBits([&LocalToShareBeforeCopy](void* mem) {
        LocalToShareBeforeCopy.emplace(ToUintPtr(mem));
        return true;
    });

    JSTaggedValue* to = reinterpret_cast<JSTaggedValue*>(ToUintPtr(dstArray->GetData()));
    JSTaggedValue* from = reinterpret_cast<JSTaggedValue*>(ToUintPtr(srcArray->GetData()));
    Barriers::CopyObject<true, false>(thread, to, from, arrayLength);

    std::set<uintptr_t> LocalToShareSlot;
    for (uint32_t i = 0; i < arrayLength; i++) {
        LocalToShareSlot.insert(ToUintPtr(dstArray->GetData() + i));
    }
    // young to young, all the bitset should not be changed.
    dstRegion->IterateAllNewToEdenBits([&NewToEdenBeforeCopy](void* mem) {
        EXPECT_TRUE(NewToEdenBeforeCopy.count(ToUintPtr(mem)));
        return true;
    });
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
    std::set<uintptr_t> NewToEdenBeforeCopy;
    std::set<uintptr_t> LocalToShareBeforeCopy;

    Region* dstRegion = Region::ObjectAddressToRange(dstArray.GetObject<TaggedArray>());
    dstRegion->IterateAllNewToEdenBits([&NewToEdenBeforeCopy](void* mem) {
        NewToEdenBeforeCopy.emplace(ToUintPtr(mem));
        return true;
    });
    dstRegion->IterateAllLocalToShareBits([&LocalToShareBeforeCopy](void* mem) {
        LocalToShareBeforeCopy.emplace(ToUintPtr(mem));
        return true;
    });

    JSTaggedValue* to = reinterpret_cast<JSTaggedValue*>(ToUintPtr(dstArray->GetData()));
    JSTaggedValue* from = reinterpret_cast<JSTaggedValue*>(ToUintPtr(srcArray->GetData()));
    Barriers::CopyObject<true, false>(thread, to, from, arrayLength);

    std::set<uintptr_t> LocalToShareSlot;
    for (uint32_t i = 0; i < arrayLength; i++) {
        if (i % 2 == 0) {
            LocalToShareSlot.insert(ToUintPtr(dstArray->GetData() + i));
        }
    }
    // young to young, all the bitset should not be changed.
    dstRegion->IterateAllNewToEdenBits([&NewToEdenBeforeCopy](void* mem) {
        EXPECT_TRUE(NewToEdenBeforeCopy.count(ToUintPtr(mem)));
        return true;
    });
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
    Barriers::CopyObject<true, false>(thread, to, from, arrayLength);

    // young to young, all the bitset should not be changed.
    dstRegion->IterateAllNewToEdenBits([&OldToNewSlot, &OldToNewBeforeCopy, &dstArray, arrayLength](void* mem) {
        if (!OldToNewBeforeCopy.count(ToUintPtr(mem))) {
            EXPECT_GE(ToUintPtr(mem), ToUintPtr(dstArray->GetData()));
            EXPECT_LT(ToUintPtr(mem), ToUintPtr(dstArray->GetData()+arrayLength));
            OldToNewSlot.erase(ToUintPtr(mem));
        } else {
            EXPECT_TRUE(OldToNewBeforeCopy.count(ToUintPtr(mem)));
        }
        return true;
    });
    dstRegion->IterateAllLocalToShareBits([&LocalToShareBeforeCopy](void* mem) {
        EXPECT_TRUE(LocalToShareBeforeCopy.count(ToUintPtr(mem)));
        return true;
    });
    EXPECT_TRUE(OldToNewSlot.empty());
    // check
    for (uint32_t i = 0; i < arrayLength; i++) {
        EXPECT_EQ(dstArray->Get(thread, i), srcArray->Get(thread, i));
    }
}
} // namespace panda::ecmascript
