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

#include <vector>

#include "common_components/heap/barrier/barrier.h"
#include "common_components/heap/collector/collector.h"
#include "common_interfaces/objects/ref_field.h"
#include "common_interfaces/objects/base_object.h"

#include "common_components/tests/test_helper.h"

using namespace common;


class DummyObject : public BaseObject {
public:
    const common::TypeInfo* GetTypeInfo() const
    {
        return nullptr;
    }
    size_t GetSize() const
    {
        return sizeof(DummyObject);
    }
};


class MockCollector : public Collector {
public:
    void Init(const RuntimeParam& param) override {}
    void RunGarbageCollection(uint64_t, GCReason) override {}
    BaseObject* ForwardObject(BaseObject*) override
    {
        return nullptr;
    }
    bool ShouldIgnoreRequest(GCRequest&) override
    {
        return false;
    }
    bool IsFromObject(BaseObject*) const override
    {
        return false;
    }
    bool IsUnmovableFromObject(BaseObject*) const override
    {
        return false;
    }
    BaseObject* FindToVersion(BaseObject*) const override
    {
        return nullptr;
    }

    bool TryUpdateRefField(BaseObject* Obj, RefField<>& field, BaseObject*& toVersion) const override
    {
        toVersion = reinterpret_cast<BaseObject*>(field.GetFieldValue());
        return true;
    }

    bool TryForwardRefField(BaseObject*, RefField<>&, BaseObject*&) const override
    {
        return false;
    }
    bool TryUntagRefField(BaseObject*, RefField<>&, BaseObject*&) const override
    {
        return false;
    }
    RefField<> GetAndTryTagRefField(BaseObject*) const override
    {
        return RefField<>(nullptr);
    }
    bool IsOldPointer(RefField<>&) const override
    {
        return false;
    }
    bool IsCurrentPointer(RefField<>&) const override
    {
        return false;
    }
    void AddRawPointerObject(BaseObject*) override {}
    void RemoveRawPointerObject(BaseObject*) override {}
};


class BarrierTest : public common::test::BaseTestWithScope {
protected:
    MockCollector collector;
    Barrier barrier{collector};
    std::unique_ptr<DummyObject> dummyObj;

    void SetUp() override
    {
        dummyObj = std::make_unique<DummyObject>();
    }
};


HWTEST_F_L0(BarrierTest, ReadRefField_ReturnsExpectedValue) {
    uint64_t value = reinterpret_cast<uint64_t>(dummyObj.get());
    RefField<false> field(value);

    BaseObject* result = barrier.ReadRefField(nullptr, field);
    EXPECT_EQ(result, dummyObj.get());
}


HWTEST_F_L0(BarrierTest, WriteRefField_SetsTargetObject) {
    uint64_t initValue = 0;
    RefField<false> field(initValue);
    BaseObject* newRef = dummyObj.get();

    barrier.WriteRefField(nullptr, field, newRef);
    EXPECT_EQ(field.GetTargetObject(), newRef);
}


HWTEST_F_L0(BarrierTest, WriteStaticRef_SetsTargetObject) {
    uint64_t initValue = 0;
    RefField<false> field(initValue);
    BaseObject* newRef = dummyObj.get();

    barrier.WriteStaticRef(field, newRef);
    EXPECT_EQ(field.GetTargetObject(), newRef);
}


HWTEST_F_L0(BarrierTest, AtomicWriteRefField_UpdatesWithMemoryOrder) {
    uint64_t initValue = 0;
    RefField<true> field(initValue);
    BaseObject* newRef = dummyObj.get();

    barrier.AtomicWriteRefField(nullptr, field, newRef, std::memory_order_relaxed);
    EXPECT_EQ(field.GetTargetObject(std::memory_order_relaxed), newRef);
}


HWTEST_F_L0(BarrierTest, CompareAndSwapRefField_WorksWithSuccessAndFailure) {
    uint64_t initValue = 0;
    RefField<true> field(initValue);
    BaseObject* oldRef = nullptr;
    BaseObject* newRef = dummyObj.get();

    bool result = barrier.CompareAndSwapRefField(nullptr, field, oldRef, newRef,
                                                 std::memory_order_seq_cst, std::memory_order_relaxed);
    EXPECT_TRUE(result);
    EXPECT_EQ(field.GetTargetObject(std::memory_order_relaxed), newRef);
}