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

#ifndef ECMASCRIPT_COMPILER_OBJECT_ACCESS_HELPER_H
#define ECMASCRIPT_COMPILER_OBJECT_ACCESS_HELPER_H

#include "ecmascript/ts_types/ts_manager.h"

namespace panda::ecmascript::kungfu {
class ObjectAccessInfo final {
public:
    explicit ObjectAccessInfo(GateType type, int hclassIndex = -1, PropertyLookupResult plr = PropertyLookupResult())
        : type_(type), hclassIndex_(hclassIndex), plr_(plr) {}
    ~ObjectAccessInfo() = default;

    void Set(int hclassIndex, PropertyLookupResult plr)
    {
        hclassIndex_ = hclassIndex;
        plr_ = plr;
    }

    GateType Type() const
    {
        return type_;
    }

    int HClassIndex() const
    {
        return hclassIndex_;
    }

    PropertyLookupResult Plr() const
    {
        return plr_;
    }

private:
    GateType type_ {GateType::AnyType()};
    int hclassIndex_ {-1};
    PropertyLookupResult plr_ {};
};

// An auxiliary class serving TSHCRLowering, used for named object property access,
// invoking TSManager and HClass.
class ObjectAccessHelper final {
public:
    static constexpr size_t POLYMORPHIC_MAX_SIZE = 4;

    enum AccessMode : uint8_t {
        LOAD = 0,
        STORE
    };

    explicit ObjectAccessHelper(TSManager *tsManager, AccessMode mode, GateRef receiver, GateType type,
                                JSTaggedValue key, GateRef value)
        : tsManager_(tsManager),
          thread_(tsManager_->GetThread()),
          mode_(mode),
          receiver_(receiver),
          type_(type),
          key_(key),
          value_(value) {}

    ~ObjectAccessHelper() = default;

    bool Compute(ChunkVector<ObjectAccessInfo> &infos);

    AccessMode GetAccessMode() const
    {
        return mode_;
    }

    bool IsLoading() const
    {
        return mode_ == AccessMode::LOAD;
    }

    GateRef GetReceiver() const
    {
        return receiver_;
    }

    GateRef GetValue() const
    {
        return value_;
    }

private:
    bool ComputeForClassInstance(ObjectAccessInfo &info);
    bool ComputeForClassOrObject(ObjectAccessInfo &info);
    bool ComputePolymorphism(ChunkVector<ObjectAccessInfo> &infos);

    TSManager *tsManager_ {nullptr};
    const JSThread *thread_ {nullptr};
    AccessMode mode_ {};
    GateRef receiver_ {Circuit::NullGate()};
    GateType type_ {GateType::AnyType()};
    JSTaggedValue key_ {JSTaggedValue::Hole()};
    GateRef value_ {Circuit::NullGate()};
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_OBJECT_ACCESS_HELPER_H
