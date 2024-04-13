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

#include "ecmascript/compiler/pgo_type/pgo_type_location.h"
#include "ecmascript/compiler/share_gate_meta_data.h"
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

class PGOObjectAccessInfo final {
public:
    explicit PGOObjectAccessInfo(
        ProfileTyper type, int hclassIndex = -1, PropertyLookupResult plr = PropertyLookupResult())
        : type_(type), hclassIndex_(hclassIndex), plr_(plr) {}
    ~PGOObjectAccessInfo() = default;

    void Set(int hclassIndex, PropertyLookupResult plr)
    {
        hclassIndex_ = hclassIndex;
        plr_ = plr;
    }

    ProfileTyper Type() const
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
    ProfileTyper type_ {ProfileTyper()};
    int hclassIndex_ {-1};
    PropertyLookupResult plr_ {};
};

// An auxiliary class serving TypedBytecodeLowering, used for named object property access,
// invoking TSManager and HClass.
class ObjectAccessHelper final {
public:
    static constexpr size_t POLYMORPHIC_MAX_SIZE = 4;

    enum AccessMode : uint8_t {
        LOAD = 0,
        STORE
    };

    explicit ObjectAccessHelper(CompilationEnv *env, AccessMode mode, GateRef receiver, GateType type,
                                JSTaggedValue key, GateRef value)
        : tsManager_(env->GetTSManager()),
          compilationEnv_(env),
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
    const CompilationEnv *compilationEnv_ {nullptr};
    AccessMode mode_ {};
    GateRef receiver_ {Circuit::NullGate()};
    GateType type_ {GateType::AnyType()};
    JSTaggedValue key_ {JSTaggedValue::Hole()};
    GateRef value_ {Circuit::NullGate()};
};

class PGOObjectAccessHelper final {
public:
    static constexpr size_t POLYMORPHIC_MAX_SIZE = 4;

    enum AccessMode : uint8_t {
        LOAD = 0,
        STORE
    };

    explicit PGOObjectAccessHelper(CompilationEnv *env, AccessMode mode, GateRef receiver, ProfileTyper type,
                                   JSTaggedValue key, GateRef value)
        : tsManager_(env->GetTSManager()),
          compilationEnv_(env),
          mode_(mode),
          receiver_(receiver),
          holder_(receiver),
          type_(type),
          key_(key),
          value_(value) {}

    ~PGOObjectAccessHelper() = default;

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

    GateRef GetHolder() const
    {
        return holder_;
    }

    void SetHolder(GateRef holder)
    {
        holder_ = holder;
    }

    GateRef GetValue() const
    {
        return value_;
    }
    bool ComputeForClassInstance(PGOObjectAccessInfo &info);
    bool ClassInstanceIsCallable(PGOObjectAccessInfo &info);

private:

    TSManager *tsManager_ {nullptr};
    const CompilationEnv *compilationEnv_ {nullptr};
    AccessMode mode_ {};
    GateRef receiver_ {Circuit::NullGate()};
    GateRef holder_ {Circuit::NullGate()};
    ProfileTyper type_ {ProfileTyper()};
    JSTaggedValue key_ {JSTaggedValue::Hole()};
    GateRef value_ {Circuit::NullGate()};
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_OBJECT_ACCESS_HELPER_H
