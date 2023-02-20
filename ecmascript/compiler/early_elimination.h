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

#ifndef ECMASCRIPT_COMPILER_EARLY_ELIMINATION_H
#define ECMASCRIPT_COMPILER_EARLY_ELIMINATION_H

#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/graph_visitor.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::kungfu {
class ElementInfo {
public:
    ElementInfo(TypedLoadOp loadOp, GateRef receiver, GateRef index)
        : loadOp_(loadOp), receiver_(receiver), index_(index) {}
    ~ElementInfo() = default;
    bool operator < (const ElementInfo& rhs) const
    {
        if (loadOp_ != rhs.loadOp_) {
            return loadOp_ < rhs.loadOp_;
        } else if (receiver_ != rhs.receiver_) {
            return receiver_ < rhs.receiver_;
        } else {
            return index_ < rhs.index_;
        }
    }

    bool operator == (const ElementInfo& rhs) const
    {
        if (loadOp_ != rhs.loadOp_) {
            return false;
        } else if (receiver_ != rhs.receiver_) {
            return false;
        } else {
            return index_ == rhs.index_;
        }
    }

private:
    TypedLoadOp loadOp_ {0};
    GateRef receiver_ {Circuit::NullGate()};
    GateRef index_ {Circuit::NullGate()};
};

class PropertyInfo {
public:
    PropertyInfo(GateRef receiver, GateRef offset) : receiver_(receiver), offset_(offset) {}
    ~PropertyInfo() = default;
    bool operator < (const PropertyInfo& rhs) const
    {
        if (receiver_ != rhs.receiver_) {
            return receiver_ < rhs.receiver_;
        } else {
            return offset_ < rhs.offset_;
        }
    }

    bool operator == (const PropertyInfo& rhs) const
    {
        if (receiver_ != rhs.receiver_) {
            return false;
        } else {
            return offset_ == rhs.offset_;
        }
    }

private:
    GateRef receiver_ {Circuit::NullGate()};
    GateRef offset_ {Circuit::NullGate()};
};

class ArrayLengthInfo {
public:
    ArrayLengthInfo(GateRef receiver) : receiver_(receiver) {}
    ~ArrayLengthInfo() = default;
    bool operator < (const ArrayLengthInfo& rhs) const
    {
        return receiver_ < rhs.receiver_;
    }

    bool operator == (const ArrayLengthInfo& rhs) const
    {
        return receiver_ == rhs.receiver_;
    }

private:
    GateRef receiver_ {Circuit::NullGate()};
};

class GateTypeCheckInfo {
public:
    GateTypeCheckInfo(GateRef value, GateType type) : value_(value), type_(type) {}
    GateTypeCheckInfo() : GateTypeCheckInfo(Circuit::NullGate(), GateType::Empty()) {}
    ~GateTypeCheckInfo() = default;
    bool operator < (const GateTypeCheckInfo& rhs) const
    {
        if (value_ != rhs.value_) {
            return value_ < rhs.value_;
        } else {
            return type_ < rhs.type_;
        }
    }

    bool operator == (const GateTypeCheckInfo& rhs) const
    {
        if (value_ != rhs.value_) {
            return false;
        } else {
            return type_ == rhs.type_;
        }
    }

    GateType GetType() const
    {
        return type_;
    }

    GateRef GetValue() const
    {
        return value_;
    }

    bool IsEmpty() const
    {
        return value_ == Circuit::NullGate();
    }

private:
    GateRef value_ {Circuit::NullGate()};
    GateType type_ {GateType::Empty()};
};

class Int32OverflowCheckInfo {
public:
    Int32OverflowCheckInfo(TypedUnOp unOp, GateRef value) : unOp_(unOp), value_(value) {}
    ~Int32OverflowCheckInfo() = default;
    bool operator < (const Int32OverflowCheckInfo& rhs) const
    {
        if (unOp_ != rhs.unOp_) {
            return unOp_ < rhs.unOp_;
        } else {
            return value_ < rhs.value_;
        }
    }

    bool operator == (const Int32OverflowCheckInfo& rhs) const
    {
        if (unOp_ != rhs.unOp_) {
            return false;
        } else {
            return value_ == rhs.value_;
        }
    }

private:
    TypedUnOp unOp_ {0};
    GateRef value_ {Circuit::NullGate()};
};

class StableArrayCheckInfo {
public:
    StableArrayCheckInfo(GateRef receiver) : receiver_(receiver) {}
    ~StableArrayCheckInfo() = default;
    bool operator < (const StableArrayCheckInfo& rhs) const
    {
        return receiver_ < rhs.receiver_;
    }

    bool operator == (const StableArrayCheckInfo& rhs) const
    {
        return receiver_ == rhs.receiver_;
    }

private:
    GateRef receiver_ {Circuit::NullGate()};
};

class ObjectTypeCheckInfo : public ChunkObject {
public:
    ObjectTypeCheckInfo(GateType type, GateRef receiver)
        : type_(type), receiver_(receiver) {};
    ~ObjectTypeCheckInfo() = default;
    bool operator < (const ObjectTypeCheckInfo& rhs) const
    {
        if (type_ != rhs.type_) {
            return type_ < rhs.type_;
        } else {
            return receiver_ < rhs.receiver_;
        }
    }

    bool operator == (const ObjectTypeCheckInfo& rhs) const
    {
        if (type_ != rhs.type_) {
            return false;
        } else {
            return receiver_ == rhs.receiver_;
        }
    }

private:
    GateType type_ {GateType::Empty()};
    GateRef receiver_ {Circuit::NullGate()};
};

class IndexCheckInfo {
public:
    IndexCheckInfo(GateType type, GateRef receiver, GateRef index)
        : type_(type), receiver_(receiver), index_(index) {}
    ~IndexCheckInfo() = default;
    bool operator < (const IndexCheckInfo& rhs) const
    {
        if (type_ != rhs.type_) {
            return type_ < rhs.type_;
        } else if (receiver_ != rhs.receiver_) {
            return receiver_ < rhs.receiver_;
        } else {
            return index_ < rhs.index_;
        }
    }

    bool operator == (const IndexCheckInfo& rhs) const
    {
        if (type_ != rhs.type_) {
            return false;
        } else if (receiver_ != rhs.receiver_) {
            return false;
        } else {
            return index_ == rhs.index_;
        }
    }

private:
    GateType type_ {GateType::Empty()};
    GateRef receiver_ {Circuit::NullGate()};
    GateRef index_ {Circuit::NullGate()};
};

class TypedCallCheckInfo {
public:
    TypedCallCheckInfo(GateRef func, GateRef id, GateRef para)
        : func_(func), id_(id), para_(para) {}
    ~TypedCallCheckInfo() = default;
    bool operator < (const TypedCallCheckInfo& rhs) const
    {
        if (func_ != rhs.func_) {
            return func_ < rhs.func_;
        } else if (id_ != rhs.id_) {
            return id_ < rhs.id_;
        } else {
            return para_ < rhs.para_;
        }
    }

    bool operator == (const TypedCallCheckInfo& rhs) const
    {
        if (func_ != rhs.func_) {
            return false;
        } else if (id_ != rhs.id_) {
            return false;
        } else {
            return para_ == rhs.para_;
        }
    }

private:
    GateRef func_ {Circuit::NullGate()};
    GateRef id_ {Circuit::NullGate()};
    GateRef para_ {Circuit::NullGate()};
};

class DependChainInfo : public ChunkObject {
public:
    DependChainInfo(Chunk* chunk) : chunk_(chunk) {}
    ~DependChainInfo() = default;

    GateRef LookUpElement(ElementInfo info) const;
    GateRef LookUpProperty(PropertyInfo info) const;
    GateRef LookUpArrayLength(ArrayLengthInfo info) const;
    bool LookUpGateTypeCheck(GateTypeCheckInfo info) const;
    GateTypeCheckInfo LookUpGateTypeCheck(GateRef gate) const;
    bool LookUpInt32OverflowCheck(Int32OverflowCheckInfo info) const;
    bool LookUpStableArrayCheck(StableArrayCheckInfo info) const;
    bool LookUpObjectTypeCheck(ObjectTypeCheckInfo info) const;
    bool LookUpIndexCheck(IndexCheckInfo info) const;
    bool LookUpTypedCallCheck(TypedCallCheckInfo info) const;
    GateRef LookUpFrameState() const;

    DependChainInfo* UpdateElement(ElementInfo info, GateRef gate);
    DependChainInfo* UpdateProperty(PropertyInfo info, GateRef gate);
    DependChainInfo* UpdateArrayLength(ArrayLengthInfo info, GateRef gate);
    DependChainInfo* UpdateGateTypeCheck(GateTypeCheckInfo info);
    DependChainInfo* UpdateInt32OverflowCheck(Int32OverflowCheckInfo info);
    DependChainInfo* UpdateStableArrayCheck(StableArrayCheckInfo info);
    DependChainInfo* UpdateObjectTypeCheck(ObjectTypeCheckInfo info);
    DependChainInfo* UpdateIndexCheck(IndexCheckInfo info);
    DependChainInfo* UpdateTypedCallCheck(TypedCallCheckInfo info);
    DependChainInfo* UpdateFrameState(GateRef gate);
    DependChainInfo* UpdateWrite();

    bool Empty() const;
    bool Equals(DependChainInfo* that);
    template<typename K, typename V>
    ChunkMap<K, V>* MergeMap(ChunkMap<K, V>* thisMap, ChunkMap<K, V>* thatMap);
    template<typename K>
    ChunkSet<K>* MergeSet(ChunkSet<K>* thisSet, ChunkSet<K>* thatSet);
    template<typename K, typename V>
    bool EqualsMap(ChunkMap<K, V>* thisMap, ChunkMap<K, V>* thatMap);
    template<typename K>
    bool EqualsSet(ChunkSet<K>* thisSet, ChunkSet<K>* thatSet);
    DependChainInfo* Merge(DependChainInfo* that);

private:
    ChunkMap<ElementInfo, GateRef>* elementMap_ {nullptr};
    ChunkMap<PropertyInfo, GateRef>* propertyMap_ {nullptr};
    ChunkMap<ArrayLengthInfo, GateRef>* arrayLengthMap_ {nullptr};
    ChunkSet<GateTypeCheckInfo>* gateTypeCheckSet_ {nullptr};
    ChunkSet<Int32OverflowCheckInfo>* int32OverflowCheckSet_ {nullptr};
    ChunkSet<StableArrayCheckInfo>* stableArrayCheckSet_ {nullptr};
    ChunkSet<ObjectTypeCheckInfo>* objectTypeCheckSet_ {nullptr};
    ChunkSet<IndexCheckInfo>* indexCheckSet_ {nullptr};
    ChunkSet<TypedCallCheckInfo>* typedCallCheckSet_ {nullptr};
    GateRef frameState_ {Circuit::NullGate()};
    Chunk* chunk_;
};

class EarlyElimination : public GraphVisitor {
public:
    EarlyElimination(Circuit *circuit, bool enableLog, const std::string& name, Chunk* chunk)
        : GraphVisitor(circuit, chunk), enableLog_(enableLog),
        methodName_(name), dependInfos_(chunk), stateSplits_(chunk) {}

    ~EarlyElimination() = default;

    void Run();

    GateRef VisitGate(GateRef gate) override;
private:
    bool IsLogEnabled() const
    {
        return enableLog_;
    }

    const std::string& GetMethodName() const
    {
        return methodName_;
    }

    ElementInfo GetElementInfo(GateRef gate) const;
    PropertyInfo GetPropertyInfo(GateRef gate) const;
    ArrayLengthInfo GetArrayLengthInfo(GateRef gate) const;
    Int32OverflowCheckInfo GetInt32OverflowCheckInfo(GateRef gate) const;
    StableArrayCheckInfo GetStableArrayCheckInfo(GateRef gate) const;
    IndexCheckInfo GetIndexCheckInfo(GateRef gate) const;
    TypedCallCheckInfo GetTypedCallCheckInfo(GateRef gate) const;

    bool IsSideEffectLoop(GateRef gate);
    GateRef TryEliminateElement(GateRef gate);
    GateRef TryEliminateProperty(GateRef gate);
    GateRef TryEliminateArrayLength(GateRef gate);
    GateRef TryEliminateGateTypeCheck(GateRef gate);
    GateRef TryEliminateInt32OverflowCheck(GateRef gate);
    GateRef TryEliminateStableArrayCheck(GateRef gate);
    GateRef TryEliminateObjectTypeCheck(GateRef gate);
    GateRef TryEliminateIndexCheck(GateRef gate);
    GateRef TryEliminateTypedCallCheck(GateRef gate);
    GateRef TryEliminateStateSplitAndFrameState(GateRef gate);
    GateRef TryEliminateDependSelector(GateRef gate);
    GateRef TryEliminateOther(GateRef gate);
    GateRef VisitDependEntry(GateRef gate);
    GateRef VisitTypedUnaryOp(GateRef gate);
    GateRef VisitTypedBinaryOp(GateRef gate);

    DependChainInfo *UpadateDependInfoForPhi(
        DependChainInfo* outDependInfo, GateRef dependPhi, GateRef phi);
    GateRef UpdateDependInfo(GateRef gate, DependChainInfo* dependInfo);

    bool enableLog_ {false};
    std::string methodName_;
    ChunkVector<DependChainInfo*> dependInfos_;
    ChunkSet<GateRef> stateSplits_;
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_EARLY_ELIMINATION_H