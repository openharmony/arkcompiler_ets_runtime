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
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::kungfu {
class ElementInfo : public ChunkObject {
public:
    ElementInfo(TypedLoadOp loadOp, GateRef receiver, GateRef index)
        : loadOp_(loadOp), receiver_(receiver), index_(index) {};
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

private:
    TypedLoadOp loadOp_ {0};
    GateRef receiver_ {Circuit::NullGate()};
    GateRef index_ {Circuit::NullGate()};
};

class PropertyInfo : public ChunkObject {
public:
    PropertyInfo(GateRef receiver, GateRef offset) : receiver_(receiver), offset_(offset) {};
    ~PropertyInfo() = default;
    bool operator < (const PropertyInfo& rhs) const
    {
        if (receiver_ != rhs.receiver_) {
            return receiver_ < rhs.receiver_;
        } else {
            return offset_ < rhs.offset_;
        }
    }

private:
    GateRef receiver_ {Circuit::NullGate()};
    GateRef offset_ {Circuit::NullGate()};
};

class ArrayLengthInfo : public ChunkObject {
public:
    ArrayLengthInfo(GateRef receiver) : receiver_(receiver){};
    ~ArrayLengthInfo() = default;
    bool operator < (const ArrayLengthInfo& rhs) const
    {
        return receiver_ < rhs.receiver_;
    }

private:
    GateRef receiver_ {Circuit::NullGate()};
};

class PrimitiveTypeCheckInfo : public ChunkObject {
public:
    PrimitiveTypeCheckInfo(GateType type, GateRef value) : type_(type), value_(value) {};
    ~PrimitiveTypeCheckInfo() = default;
    bool operator < (const PrimitiveTypeCheckInfo& rhs) const
    {
        if (type_ != rhs.type_) {
            return type_ < rhs.type_;
        } else {
            return value_ < rhs.value_;
        }
    }

private:
    GateType type_ {GateType::Empty()};
    GateRef value_ {Circuit::NullGate()};
};

class Int32OverflowCheckInfo : public ChunkObject {
public:
    Int32OverflowCheckInfo(TypedUnOp unOp, GateRef value) : unOp_(unOp), value_(value) {};
    ~Int32OverflowCheckInfo() = default;
    bool operator < (const Int32OverflowCheckInfo& rhs) const
    {
        if (unOp_ != rhs.unOp_) {
            return unOp_ < rhs.unOp_;
        } else {
            return value_ < rhs.value_;
        }
    }

private:
    TypedUnOp unOp_ {0};
    GateRef value_ {Circuit::NullGate()};
};

class ArrayCheckInfo : public ChunkObject {
public:
    ArrayCheckInfo(GateRef receiver) : receiver_(receiver){};
    ~ArrayCheckInfo() = default;
    bool operator < (const ArrayCheckInfo& rhs) const
    {
        return receiver_ < rhs.receiver_;
    }

private:
    GateRef receiver_ {Circuit::NullGate()};
};

class StableArrayCheckInfo : public ChunkObject {
public:
    StableArrayCheckInfo(GateRef receiver) : receiver_(receiver){};
    ~StableArrayCheckInfo() = default;
    bool operator < (const StableArrayCheckInfo& rhs) const
    {
        return receiver_ < rhs.receiver_;
    }

private:
    GateRef receiver_ {Circuit::NullGate()};
};

class TypedArrayCheckInfo : public ChunkObject {
public:
    TypedArrayCheckInfo(GateType type, GateRef receiver)
        : type_(type), receiver_(receiver) {};
    ~TypedArrayCheckInfo() = default;
    bool operator < (const TypedArrayCheckInfo& rhs) const
    {
        if (type_ != rhs.type_) {
            return type_ < rhs.type_;
        } else {
            return receiver_ < rhs.receiver_;
        }
    }

private:
    GateType type_ {GateType::Empty()};
    GateRef receiver_ {Circuit::NullGate()};
};

class ObjectTypeCheckInfo : public ChunkObject {
public:
    ObjectTypeCheckInfo(GateType type, GateRef receiver, GateRef offset)
        : type_(type), receiver_(receiver), offset_(offset) {};
    ~ObjectTypeCheckInfo() = default;
    bool operator < (const ObjectTypeCheckInfo& rhs) const
    {
        if (type_ != rhs.type_) {
            return type_ < rhs.type_;
        } else if (receiver_ != rhs.receiver_) {
            return receiver_ < rhs.receiver_;
        } else {
            return offset_ < rhs.offset_;
        }
    }

private:
    GateType type_ {GateType::Empty()};
    GateRef receiver_ {Circuit::NullGate()};
    GateRef offset_ {Circuit::NullGate()};
};

class IndexCheckInfo : public ChunkObject {
public:
    IndexCheckInfo(GateType type, GateRef receiver, GateRef index)
        : type_(type), receiver_(receiver), index_(index) {};
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

private:
    GateType type_ {GateType::Empty()};
    GateRef receiver_ {Circuit::NullGate()};
    GateRef index_ {Circuit::NullGate()};
};

class TypedCallCheckInfo : public ChunkObject {
public:
    TypedCallCheckInfo(GateRef func, GateRef id, GateRef para)
        : func_(func), id_(id), para_(para) {};
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

private:
    GateRef func_ {Circuit::NullGate()};
    GateRef id_ {Circuit::NullGate()};
    GateRef para_ {Circuit::NullGate()};
};

class DependChainInfo : public ChunkObject {
public:
    DependChainInfo(Chunk* chunk) : chunk_(chunk) {};
    ~DependChainInfo() = default;
    
    bool operator == (const DependChainInfo& rhs) const
    {
        return (elementMap_ == rhs.elementMap_) &&
               (propertyMap_ == rhs.propertyMap_) &&
               (arrayLengthMap_ == rhs.arrayLengthMap_) &&
               (primitiveTypeCheckSet_ == rhs.primitiveTypeCheckSet_) &&
               (int32OverflowCheckSet_ == rhs.int32OverflowCheckSet_) &&
               (arrayCheckSet_ == rhs.arrayCheckSet_) &&
               (stableArrayCheckSet_ == rhs.stableArrayCheckSet_) &&
               (typedArrayCheckSet_ == rhs.typedArrayCheckSet_) &&
               (objectTypeCheckSet_ == rhs.objectTypeCheckSet_) &&
               (indexCheckSet_ == rhs.indexCheckSet_) &&
               (typedCallCheckSet_ == rhs.typedCallCheckSet_) &&
               (frameState_ == rhs.frameState_);
    }

    GateRef LookUpElement(ElementInfo* info) const;
    GateRef LookUpProperty(PropertyInfo* info) const;
    GateRef LookUpArrayLength(ArrayLengthInfo* info) const;
    bool LookUpPrimitiveTypeCheck(PrimitiveTypeCheckInfo* info) const;
    bool LookUpInt32OverflowCheck(Int32OverflowCheckInfo* info) const;
    bool LookUpArrayCheck(ArrayCheckInfo* info) const;
    bool LookUpStableArrayCheck(StableArrayCheckInfo* info) const;
    bool LookUpTypedArrayCheck(TypedArrayCheckInfo* info) const;
    bool LookUpObjectTypeCheck(ObjectTypeCheckInfo* info) const;
    bool LookUpIndexCheck(IndexCheckInfo* info) const;
    bool LookUpTypedCallCheck(TypedCallCheckInfo* info) const;
    GateRef LookUpFrameState() const;

    DependChainInfo* UpdateElement(ElementInfo* info, GateRef gate);
    DependChainInfo* UpdateProperty(PropertyInfo* info, GateRef gate);
    DependChainInfo* UpdateArrayLength(ArrayLengthInfo* info, GateRef gate);
    DependChainInfo* UpdatePrimitiveTypeCheck(PrimitiveTypeCheckInfo* info);
    DependChainInfo* UpdateInt32OverflowCheck(Int32OverflowCheckInfo* info);
    DependChainInfo* UpdateArrayCheck(ArrayCheckInfo* info);
    DependChainInfo* UpdateStableArrayCheck(StableArrayCheckInfo* info);
    DependChainInfo* UpdateTypedArrayCheck(TypedArrayCheckInfo* info);
    DependChainInfo* UpdateObjectTypeCheck(ObjectTypeCheckInfo* info);
    DependChainInfo* UpdateIndexCheck(IndexCheckInfo* info);
    DependChainInfo* UpdateTypedCallCheck(TypedCallCheckInfo* info);
    DependChainInfo* UpdateFrameState(GateRef gate);
    DependChainInfo* UpdateWrite();

    bool Empty() const;
    bool Equal(DependChainInfo* that);
    template<typename K, typename V>
    ChunkMap<K, V>* MergeMap(ChunkMap<K, V>* thisMap, ChunkMap<K, V>* thatMap);
    template<typename K>
    ChunkSet<K>* MergeSet(ChunkSet<K>* thisSet, ChunkSet<K>* thatSet);
    DependChainInfo* Merge(DependChainInfo* that);

private:
    ChunkMap<ElementInfo, GateRef>* elementMap_ {nullptr};
    ChunkMap<PropertyInfo, GateRef>* propertyMap_ {nullptr};
    ChunkMap<ArrayLengthInfo, GateRef>* arrayLengthMap_ {nullptr};
    ChunkSet<PrimitiveTypeCheckInfo>* primitiveTypeCheckSet_ {nullptr};
    ChunkSet<Int32OverflowCheckInfo>* int32OverflowCheckSet_ {nullptr};
    ChunkSet<ArrayCheckInfo>* arrayCheckSet_ {nullptr};
    ChunkSet<StableArrayCheckInfo>* stableArrayCheckSet_ {nullptr};
    ChunkSet<TypedArrayCheckInfo>* typedArrayCheckSet_ {nullptr};
    ChunkSet<ObjectTypeCheckInfo>* objectTypeCheckSet_ {nullptr};
    ChunkSet<IndexCheckInfo>* indexCheckSet_ {nullptr};
    ChunkSet<TypedCallCheckInfo>* typedCallCheckSet_ {nullptr};
    GateRef frameState_ {Circuit::NullGate()};
    Chunk* chunk_;
};

class EarlyElimination {
public:
    EarlyElimination(Circuit *circuit, bool enableLog, const std::string& name, NativeAreaAllocator *allocator)
        : circuit_(circuit), acc_(circuit), enableLog_(enableLog),
        methodName_(name), chunk_(allocator), dependInfos_(&chunk_), stateSplits_(&chunk_) {}

    ~EarlyElimination() = default;

    void Run();

private:
    bool IsLogEnabled() const
    {
        return enableLog_;
    }

    const std::string& GetMethodName() const
    {
        return methodName_;
    }

    Chunk *GetChunk() const
    {
        return const_cast<Chunk *>(&chunk_);
    }

    ElementInfo* GetElementInfo(GateRef gate) const;
    PropertyInfo* GetPropertyInfo(GateRef gate) const;
    ArrayLengthInfo* GetArrayLengthInfo(GateRef gate) const;
    PrimitiveTypeCheckInfo* GetPrimitiveTypeCheckInfo(GateRef gate) const;
    Int32OverflowCheckInfo* GetInt32OverflowCheckInfo(GateRef gate) const;
    ArrayCheckInfo* GetArrayCheckInfo(GateRef gate) const;
    StableArrayCheckInfo* GetStableArrayCheckInfo(GateRef gate) const;
    TypedArrayCheckInfo* GetTypedArrayCheckInfo(GateRef gate) const;
    ObjectTypeCheckInfo* GetObjectTypeCheckInfo(GateRef gate) const;
    IndexCheckInfo* GetIndexCheckInfo(GateRef gate) const;
    TypedCallCheckInfo* GetTypedCallCheckInfo(GateRef gate) const;

    bool IsSideEffectLoop(GateRef gate);

    void TryEliminate(GateRef gate);
    void TryEliminateElement(GateRef gate);
    void TryEliminateProperty(GateRef gate);
    void TryEliminateArrayLength(GateRef gate);
    void TryEliminatePrimitiveTypeCheck(GateRef gate);
    void TryEliminateInt32OverflowCheck(GateRef gate);
    void TryEliminateArrayCheck(GateRef gate);
    void TryEliminateStableArrayCheck(GateRef gate);
    void TryEliminateTypedArrayCheck(GateRef gate);
    void TryEliminateObjectTypeCheck(GateRef gate);
    void TryEliminateIndexCheck(GateRef gate);
    void TryEliminateTypedCallCheck(GateRef gate);
    void TryEliminateStateSplitAndFrameState(GateRef gate);
    void TryEliminateDependSelector(GateRef gate);
    void TryEliminateDependAnd(GateRef gate);
    void TryEliminateOther(GateRef gate);

    bool IsTrustedType(GateRef gate) const;
    bool IsPrimitiveTypeCheck(GateRef gate) const;
    void TrustedTypePropagate(ChunkQueue<GateRef>& workList, const ChunkVector<GateRef>& checkList);
    void RemoveGate(GateRef gate, GateRef value);
    void RemoveRedundantGate();
    void RemoveTypeTrustedCheck();

    Circuit *circuit_ {nullptr};
    GateAccessor acc_;
    bool enableLog_ {false};
    std::string methodName_;
    Chunk chunk_;
    ChunkVector<DependChainInfo*> dependInfos_;
    ChunkVector<GateRef> stateSplits_;
    GateId maxId_ {0};
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_EARLY_ELIMINATION_H