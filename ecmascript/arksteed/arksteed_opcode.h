/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_ARKSTEED_OPCODE_H
#define ECMASCRIPT_ARKSTEED_OPCODE_H

#include "ecmascript/arksteed/arksteed_regalloc_types.h"
#include "ecmascript/arksteed/arksteed_vertex.h"
#include "ecmascript/arksteed/arksteed_vreg.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::arksteed {
using VirtualRegister = kungfu::VirtualRegister;

class BB;
class ArkSteedState;
class ArkSteedAssembler;
class MergePointFrameState;

/**
 * CRTP Mixin Classes for ArkSteed Opcodes
 *
 * This design uses the Curiously Recurring Template Pattern (CRTP) to provide
 * type-safe and efficient opcode implementations on top of the Vertex hierarchy.
 *
 * Key principles:
 * - Build on Vertex/ValueVertex/ControlVertex classes
 * - CRTP mixins provide compile-time type information and helpers
 * - No virtual function overhead
 * - Each opcode class implements its own operations (GenerateCode, Dump, etc.)
 *
 * Inheritance hierarchy example for CallVertex:
 *
 *          ValueVertex (base class)
 *                  ↑
 *                  | inherits
 *        VertexMixin<ValueVertex, CallVertex>
 *                  ↑
 *                  | inherits
 *                CallVertex
 *
 * Each concrete vertex class (excluding mixins) must implement:
 * - SetValueLocationConstraints(): Prepare register allocation constraints
 * - GenerateCode(): Generate assembly code
 * - Dump(): Print parameters for debugging
 */

/**
 * CRTP mixin for vertices with known class (opcode, properties)
 * The Base class should be Vertex, ValueVertex, or ControlVertex
 * The Derived class is the actual opcode
 *
 * Each derived class must define:
 * - PROPERTIES: VertexProperties for this vertex
 *
 * Note: INPUT_TYPES is only required when inheriting FixedInputVertexMixin
 */
template <typename Base, typename Derived>
class VertexMixin : public Base {
public:
    static constexpr VertexOpcode GetOpcodeValue()
    {
        return Vertex::opcode_of<Derived>();
    }

    static constexpr const VertexProperties &GetPropertiesValue()
    {
        return Derived::PROPERTIES;
    }

    template <typename... Args>
    static Derived *New(Chunk *chunk, std::initializer_list<ValueVertex *> inputs, Args &&...args)
    {
        return Vertex::New<Derived>(chunk, inputs, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static Derived *New(Chunk *chunk, const ChunkVector<ValueVertex *> &inputs, Args &&...args)
    {
        return Vertex::New<Derived>(chunk, inputs, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static Derived *New(Chunk *chunk, size_t inputCount, Args &&...args)
    {
        return Vertex::New<Derived>(chunk, inputCount, std::forward<Args>(args)...);
    }

protected:
    template <typename... Args>
    explicit VertexMixin(uint64_t bitfield, Args &&...args) : Base(bitfield, std::forward<Args>(args)...)
    {
        ASSERT(this->GetOpcode() == Derived::GetOpcodeValue());
        ASSERT(this->GetProperties() == Derived::GetPropertiesValue());
    }
};

/**
 * CRTP mixin for vertices with fixed number of inputs
 * Provides compile-time input count checking and verification
 */
template <size_t InputCount, typename Base, typename Derived>
class FixedInputVertexMixin : public VertexMixin<Base, Derived> {
public:
    static constexpr size_t FIXED_INPUT_COUNT = InputCount;

    // Shadow methods that use compile-time knowledge
    constexpr bool HasInputs() const
    {
        return FIXED_INPUT_COUNT > 0;
    }

    constexpr int GetInputCount() const
    {
        return FIXED_INPUT_COUNT;
    }

    constexpr auto GetInputEnd()
    {
        return std::make_reverse_iterator(&this->GetInput(GetInputCount() - 1));
    }

    void VerifyInputs() const
    {
        if constexpr (FIXED_INPUT_COUNT != 0) {
            // Verify runtime input count matches compile-time count
            ASSERT(this->GetInputCount() == static_cast<int>(FIXED_INPUT_COUNT));

            // Verify input types if defined in derived class
            if constexpr (HasInputTypes<Derived>::value) {
                static_assert(FIXED_INPUT_COUNT == Derived::INPUT_TYPES.size());
                for (int i = 0; i < static_cast<int>(FIXED_INPUT_COUNT); ++i) {
                    CheckValueInput(i, Derived::INPUT_TYPES[i]);
                }
            }
        }
    }

protected:
    template <typename... Args>
    explicit FixedInputVertexMixin(uint64_t bitfield, Args &&...args)
        : VertexMixin<Base, Derived>(bitfield, std::forward<Args>(args)...)
    {}

    inline void CheckValueInput(int index, ValueRepresentation expectedRepr) const
    {
        const ValueVertex *input = this->GetInput(index);
        ASSERT(input != nullptr);
        ASSERT(input->GetValueRepresentation() == expectedRepr);
    }

private:
    // Helper to detect if derived class has INPUT_TYPES
    template <typename T, typename = void>
    struct HasInputTypes : std::false_type {};

    template <typename T>
    struct HasInputTypes<T, decltype(void(T::INPUT_TYPES))> : std::true_type {};
};

//==============================================================================
// Constant Value Vertices
//==============================================================================

class ConstantVertex : public FixedInputVertexMixin<0, ValueVertex, ConstantVertex> {
public:
    using OutputRegister = ArkSteedRegister;
    static constexpr VertexProperties PROPERTIES = VertexProperties::TaggedValue();

    ConstantVertex(uint64_t bitfield, JSTaggedValue value) : FixedInputVertexMixin(bitfield), value_(value) {}

    JSTaggedValue GetValue() const
    {
        return value_;
    }

    void DoLoadToRegister(ArkSteedAssembler *, OutputRegister) const;
    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

private:
    JSTaggedValue value_;
};

class RootConstantVertex : public FixedInputVertexMixin<0, ValueVertex, RootConstantVertex> {
public:
    using OutputRegister = ArkSteedRegister;
    static constexpr VertexProperties PROPERTIES = VertexProperties::TaggedValue();

    enum class RootIndex : uint8_t {
        UNDEFINED = 0,
        NULL_VALUE,
        TRUE_VALUE,
        FALSE_VALUE,
    };

    RootConstantVertex(uint64_t bitfield, RootIndex index) : FixedInputVertexMixin(bitfield), index_(index) {}

    RootIndex GetIndex() const
    {
        return index_;
    }

    void DoLoadToRegister(ArkSteedAssembler *, OutputRegister) const;
    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

private:
    RootIndex index_;
};

class BooleanConstantVertex : public FixedInputVertexMixin<0, ValueVertex, BooleanConstantVertex> {
public:
    using OutputRegister = ArkSteedRegister;
    static constexpr VertexProperties PROPERTIES = VertexProperties::Int32();

    BooleanConstantVertex(uint64_t bitfield, bool value) : FixedInputVertexMixin(bitfield), value_(value) {}

    bool GetValue() const
    {
        return value_;
    }

    void DoLoadToRegister(ArkSteedAssembler *, OutputRegister) const;
    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

private:
    bool value_;
};

class Int32ConstantVertex : public FixedInputVertexMixin<0, ValueVertex, Int32ConstantVertex> {
public:
    using OutputRegister = ArkSteedRegister;
    static constexpr VertexProperties PROPERTIES = VertexProperties::Int32();

    Int32ConstantVertex(uint64_t bitfield, int32_t value) : FixedInputVertexMixin(bitfield), value_(value) {}

    int32_t GetValue() const
    {
        return value_;
    }

    void DoLoadToRegister(ArkSteedAssembler *, OutputRegister) const;
    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

private:
    int32_t value_;
};

class IntPtrConstantVertex : public FixedInputVertexMixin<0, ValueVertex, IntPtrConstantVertex> {
public:
    using OutputRegister = ArkSteedRegister;
    static constexpr VertexProperties PROPERTIES = VertexProperties::IntPtr();

    IntPtrConstantVertex(uint64_t bitfield, intptr_t value) : FixedInputVertexMixin(bitfield), value_(value) {}

    intptr_t GetValue() const
    {
        return value_;
    }

    void DoLoadToRegister(ArkSteedAssembler *, OutputRegister) const;
    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

private:
    intptr_t value_;
};

class Float64ConstantVertex : public FixedInputVertexMixin<0, ValueVertex, Float64ConstantVertex> {
public:
    using OutputRegister = ArkSteedDoubleRegister;
    static constexpr VertexProperties PROPERTIES = VertexProperties::Float64();

    Float64ConstantVertex(uint64_t bitfield, double value) : FixedInputVertexMixin(bitfield), value_(value) {}

    double GetValue() const
    {
        return value_;
    }

    void DoLoadToRegister(ArkSteedAssembler *, OutputRegister) const;
    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

private:
    double value_;
};

class TaggedConstantVertex : public FixedInputVertexMixin<0, ValueVertex, TaggedConstantVertex> {
public:
    using OutputRegister = ArkSteedRegister;
    static constexpr VertexProperties PROPERTIES = VertexProperties::TaggedValue();

    TaggedConstantVertex(uint64_t bitfield, uint64_t value) : FixedInputVertexMixin(bitfield), value_(value) {}

    uint64_t GetValue() const
    {
        return value_;
    }

    void DoLoadToRegister(ArkSteedAssembler *, OutputRegister) const;
    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

private:
    uint64_t value_;
};

class InitialValueVertex : public FixedInputVertexMixin<0, ValueVertex, InitialValueVertex> {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::TaggedValue();

    InitialValueVertex(uint64_t bitfield, int32_t frameSlotIndex)
        : FixedInputVertexMixin(bitfield), frameSlotIndex_(frameSlotIndex)
    {}

    int32_t GetFrameSlotIndex() const
    {
        return frameSlotIndex_;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

private:
    int32_t frameSlotIndex_;
};

class ActualArgcVertex : public FixedInputVertexMixin<0, ValueVertex, ActualArgcVertex> {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::IntPtr();

    explicit ActualArgcVertex(uint64_t bitfield) : FixedInputVertexMixin(bitfield) {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
};

class LoadTaggedFieldVertex : public FixedInputVertexMixin<1, ValueVertex, LoadTaggedFieldVertex> {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::TaggedValue() | VertexProperties::CanReadProp();

    static constexpr auto INPUT_TYPES = detail::InputTypes<1>(ValueRepresentation::TAGGED);

    static constexpr size_t OBJECT_INDEX = 0;

    explicit LoadTaggedFieldVertex(uint64_t bitfield, int32_t offset) : FixedInputVertexMixin(bitfield), offset_(offset)
    {}

    int32_t GetOffset() const
    {
        return offset_;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

private:
    int32_t offset_;
};

// 2: object and value inputs
class StoreTaggedFieldVertex : public FixedInputVertexMixin<2, ValueVertex, StoreTaggedFieldVertex> {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::CanWriteProp() | VertexProperties::DeferredCall();

    // 2: object and value input types
    static constexpr auto INPUT_TYPES = detail::InputTypes<2>(ValueRepresentation::TAGGED, ValueRepresentation::TAGGED);

    static constexpr size_t OBJECT_INDEX = 0;
    static constexpr size_t VALUE_INDEX = 1;

    explicit StoreTaggedFieldVertex(uint64_t bitfield, int32_t offset)
        : FixedInputVertexMixin(bitfield), offset_(offset)
    {}

    int32_t GetOffset() const
    {
        return offset_;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

private:
    int32_t offset_;
};

/**
 * CallRuntime vertex - for runtime function calls
 */
class CallRuntimeVertex : public VertexMixin<ValueVertex, CallRuntimeVertex> {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::JsCall();

    CallRuntimeVertex(uint64_t bitfield, kungfu::RuntimeStubCSigns::ID id) : VertexMixin(bitfield), runtimeId_(id) {}

    kungfu::RuntimeStubCSigns::ID GetRuntimeId() const
    {
        return runtimeId_;
    }
    size_t GetArgCount() const
    {
        return GetInputCount();
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

    // Runtime argv may be empty.
    void VerifyInputs() const {}

private:
    kungfu::RuntimeStubCSigns::ID runtimeId_;
};

/**
 * CallCommonStub vertex - for common stub calls
 */
class CallCommonStubVertex : public VertexMixin<ValueVertex, CallCommonStubVertex> {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::JsCall();

    CallCommonStubVertex(uint64_t bitfield, uint32_t stubId) : VertexMixin(bitfield), stubId_(stubId) {}

    uint32_t GetStubId() const
    {
        return stubId_;
    }
    size_t GetArgCount() const
    {
        return GetInputCount();
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

    // VerifyInputs: Variable-input vertex must validate minimum input requirements
    void VerifyInputs() const
    {
        // Stub calls require at least glue parameter
        ASSERT(GetInputCount() >= 1);
    }

private:
    uint32_t stubId_;
};

//==============================================================================
// Load/Store Vertices
//==============================================================================

/**
 * Deoptimize vertex
 */
class DeoptVertex : public VertexMixin<ValueVertex, DeoptVertex> {
public:
    enum class Reason {
        DIVISION_BY_ZERO,
        NEGATIVE_ZERO,
        STACK_OVERFLOW,
        HEAP_MISMATCH,
        TYPE_MISMATCH,
        OUT_OF_BOUNDS,
        NULL_POINTER,
        UNDEFINED_VALUE
    };

    static constexpr VertexProperties PROPERTIES = VertexProperties::EagerDeopt();

    explicit DeoptVertex(uint64_t bitfield, Reason reason) : VertexMixin(bitfield), reason_(reason) {}

    Reason GetReason() const
    {
        return reason_;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

    // VerifyInputs: Variable-input vertex must validate frame state completeness
    void VerifyInputs() const
    {
        // Deopt saves execution state, input count should match frame state requirements
        // Frame state: at minimum needs function, context, and accumulator
        ASSERT(GetInputCount() >= 3);  // 3: minimum inputs for frame state
    }

private:
    Reason reason_;
};

//==============================================================================
// Control Flow Vertices
//==============================================================================

class UnconditionalControlVertex : public ControlVertex {
public:
    BB *Target() const
    {
        return target_.BlockRef();
    }
    void SetTarget(BB *block)
    {
        target_.SetBlockRef(block);
    }

    int GetPredecessorId() const
    {
        return predecessorId_;
    }

    void SetPredecessorId(int id)
    {
        predecessorId_ = id;
    }

protected:
    explicit UnconditionalControlVertex(uint64_t bitfield, BBRef *targetRefs)
        : ControlVertex(bitfield), target_(targetRefs), predecessorId_(0)
    {}
    explicit UnconditionalControlVertex(uint64_t bitfield, BB *target)
        : ControlVertex(bitfield), target_(target), predecessorId_(0)
    {}

private:
    BBRef target_;
    int predecessorId_;
};

/**
 * UnconditionalControlVertexT - CRTP mixin for unconditional control vertices
 * Provides compile-time type checking and target management for unconditional jumps
 * (Jump, JumpLoop, etc.)
 */
template <typename Derived>
class UnconditionalControlVertexT : public FixedInputVertexMixin<0, UnconditionalControlVertex, Derived> {
protected:
    explicit UnconditionalControlVertexT(uint64_t bitfield, BBRef *targetRefs)
        : FixedInputVertexMixin<0, UnconditionalControlVertex, Derived>(bitfield, targetRefs)
    {}
    explicit UnconditionalControlVertexT(uint64_t bitfield, BB *target)
        : FixedInputVertexMixin<0, UnconditionalControlVertex, Derived>(bitfield, target)
    {}
};

class BranchControlVertex : public ControlVertex {
public:
    BB *IfTrue() const
    {
        return ifTrue_.BlockRef();
    }

    BB *IfFalse() const
    {
        return ifFalse_.BlockRef();
    }

    void SetIfTrue(BB *block)
    {
        ifTrue_.SetBlockRef(block);
    }

    void SetIfFalse(BB *block)
    {
        ifFalse_.SetBlockRef(block);
    }

protected:
    BranchControlVertex(uint64_t bitfield, BBRef *ifTrue, BBRef *ifFalse)
        : ControlVertex(bitfield), ifTrue_(ifTrue), ifFalse_(ifFalse)
    {}
    BranchControlVertex(uint64_t bitfield, BB *ifTrue, BB *ifFalse)
        : ControlVertex(bitfield), ifTrue_(ifTrue), ifFalse_(ifFalse)
    {}

private:
    BBRef ifTrue_;
    BBRef ifFalse_;
};

/**
 * BranchControlVertexT - CRTP mixin for conditional control vertices
 * Provides compile-time type checking and branch target management.
 */
template <size_t InputCount, typename Derived>
class BranchControlVertexT : public FixedInputVertexMixin<InputCount, BranchControlVertex, Derived> {
protected:
    BranchControlVertexT(uint64_t bitfield, BBRef *ifTrue, BBRef *ifFalse)
        : FixedInputVertexMixin<InputCount, BranchControlVertex, Derived>(bitfield, ifTrue, ifFalse)
    {}
    BranchControlVertexT(uint64_t bitfield, BB *ifTrue, BB *ifFalse)
        : FixedInputVertexMixin<InputCount, BranchControlVertex, Derived>(bitfield, ifTrue, ifFalse)
    {}
};

/**
 * BranchIfTrue vertex - conditional branch based on boolean true value
 * Jumps to if_true if the condition is exactly the boolean true value,
 * otherwise jumps to if_false
 */
class BranchIfTrueVertex : public BranchControlVertexT<1, BranchIfTrueVertex> {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::Pure();

    BranchIfTrueVertex(uint64_t bitfield, BBRef *ifTrue, BBRef *ifFalse)
        : BranchControlVertexT(bitfield, ifTrue, ifFalse)
    {}

    BranchIfTrueVertex(uint64_t bitfield, BB *ifTrue, BB *ifFalse)
        : BranchControlVertexT(bitfield, ifTrue, ifFalse)
    {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
};

/**
 * Jump vertex - unconditional branch
 */
class JumpVertex : public UnconditionalControlVertexT<JumpVertex> {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::Pure();

    JumpVertex(uint64_t bitfield, BBRef *targetRefs) : UnconditionalControlVertexT(bitfield, targetRefs) {}
    JumpVertex(uint64_t bitfield, BB *target) : UnconditionalControlVertexT(bitfield, target) {}

    void SetValueLocationConstraints();
};

/**
 * JumpLoop vertex - unconditional branch to loop header (back edge)
 */
class JumpLoopVertex : public UnconditionalControlVertexT<JumpLoopVertex> {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::Pure();

    explicit JumpLoopVertex(uint64_t bitfield, BBRef *targetRefs) : UnconditionalControlVertexT(bitfield, targetRefs) {}
    explicit JumpLoopVertex(uint64_t bitfield, BB *target) : UnconditionalControlVertexT(bitfield, target) {}

    void SetValueLocationConstraints();

    using UsedVerticesType = std::vector<std::pair<ValueVertex *, InputLocation>>;

    void SetUsedVertices(UsedVerticesType usedVertices)
    {
        usedVertices_ = std::move(usedVertices);
    }

    const UsedVerticesType &GetUsedVertices() const
    {
        return usedVertices_;
    }

    UsedVerticesType &GetUsedVertices()
    {
        return usedVertices_;
    }

private:
    UsedVerticesType usedVertices_;
};

/**
 * Return vertex - function return
 */
class ReturnVertex : public FixedInputVertexMixin<1, ControlVertex, ReturnVertex> {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::Pure();

    explicit ReturnVertex(uint64_t bitfield) : FixedInputVertexMixin(bitfield) {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
};

/**
 * Throw vertex - exception throw
 */
class ThrowVertex : public VertexMixin<ControlVertex, ThrowVertex> {
public:
    static constexpr int EXCEPTION_INDEX = 0;

    static constexpr VertexProperties PROPERTIES =
        VertexProperties::CanThrowProp() | VertexProperties::Call() | VertexProperties::NotIdempotent();

    explicit ThrowVertex(uint64_t bitfield, kungfu::RuntimeStubCSigns::ID id, bool hasInput)
        : VertexMixin(bitfield), id_(id), hasInput_(hasInput)
    {}
    kungfu::RuntimeStubCSigns::ID GetRuntimeId() const
    {
        return id_;
    }
    bool HasInput() const
    {
        return hasInput_;
    }
    ValueVertex *GetException()
    {
        ASSERT(HasInput());
        return GetInput(EXCEPTION_INDEX);
    }
    size_t GetArgCount() const
    {
        return GetInputCount();
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

    // VerifyInputs: Variable-input vertex must validate input matches hasInput flag
    void VerifyInputs() const {}

private:
    kungfu::RuntimeStubCSigns::ID id_;
    bool hasInput_;
};

class ThrowIfSuperNotCorrectCallVertex
    // 2: index and thisValue inputs
    : public FixedInputVertexMixin<2, NonControlVertex, ThrowIfSuperNotCorrectCallVertex> {
public:
    static constexpr int INDEX_INDEX = 0;
    static constexpr int THIS_VALUE_INDEX = 1;

    static constexpr VertexProperties PROPERTIES =
        VertexProperties::CanThrowProp() | VertexProperties::Call() | VertexProperties::DeferredCall();

    explicit ThrowIfSuperNotCorrectCallVertex(uint64_t bitfield, kungfu::RuntimeStubCSigns::ID id)
        : FixedInputVertexMixin(bitfield), id_(id)
    {}

    kungfu::RuntimeStubCSigns::ID GetRuntimeId() const
    {
        return id_;
    }
    ValueVertex *GetIndex()
    {
        return GetInput(INDEX_INDEX);
    }
    ValueVertex *GetThisValue()
    {
        return GetInput(THIS_VALUE_INDEX);
    }
    size_t GetArgCount() const
    {
        return GetInputCount();
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

private:
    kungfu::RuntimeStubCSigns::ID id_;
};

class ThrowIfNotObjectVertex : public FixedInputVertexMixin<1, NonControlVertex, ThrowIfNotObjectVertex> {
public:
    static constexpr int VALUE_INDEX = 0;

    static constexpr VertexProperties PROPERTIES =
        VertexProperties::CanThrowProp() | VertexProperties::Call() | VertexProperties::DeferredCall();

    explicit ThrowIfNotObjectVertex(uint64_t bitfield, kungfu::RuntimeStubCSigns::ID id)
        : FixedInputVertexMixin(bitfield), runtimeId_(id)
    {}

    kungfu::RuntimeStubCSigns::ID GetRuntimeId() const
    {
        return runtimeId_;
    }
    ValueVertex *GetValue()
    {
        return GetInput(VALUE_INDEX);
    }
    size_t GetArgCount() const
    {
        return GetInputCount();
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

private:
    kungfu::RuntimeStubCSigns::ID runtimeId_;
};

// 2: hole and obj inputs
class ThrowUndefinedIfHoleVertex : public FixedInputVertexMixin<2, NonControlVertex, ThrowUndefinedIfHoleVertex> {
public:
    static constexpr int HOLE_INDEX = 0;
    static constexpr int OBJ_INDEX = 1;

    static constexpr VertexProperties PROPERTIES =
        VertexProperties::CanThrowProp() | VertexProperties::Call() | VertexProperties::DeferredCall();

    explicit ThrowUndefinedIfHoleVertex(uint64_t bitfield, kungfu::RuntimeStubCSigns::ID id)
        : FixedInputVertexMixin(bitfield), runtimeId_(id)
    {}

    kungfu::RuntimeStubCSigns::ID GetRuntimeId() const
    {
        return runtimeId_;
    }
    ValueVertex *GetHole()
    {
        return GetInput(HOLE_INDEX);
    }
    ValueVertex *GetObj()
    {
        return GetInput(OBJ_INDEX);
    }
    size_t GetArgCount() const
    {
        return GetInputCount();
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

private:
    kungfu::RuntimeStubCSigns::ID runtimeId_;
};

class ThrowUndefinedIfHoleWithNameVertex
    // 2: stringId and hole inputs
    : public FixedInputVertexMixin<2, NonControlVertex, ThrowUndefinedIfHoleWithNameVertex> {
public:
    static constexpr int STRING_ID_INDEX = 0;
    static constexpr int HOLE_INDEX = 1;

    static constexpr VertexProperties PROPERTIES =
        VertexProperties::CanThrowProp() | VertexProperties::Call() | VertexProperties::DeferredCall();

    explicit ThrowUndefinedIfHoleWithNameVertex(uint64_t bitfield, kungfu::RuntimeStubCSigns::ID id)
        : FixedInputVertexMixin(bitfield), runtimeId_(id)
    {}

    kungfu::RuntimeStubCSigns::ID GetRuntimeId() const
    {
        return runtimeId_;
    }
    ValueVertex *GetStringIdInput()
    {
        return GetInput(STRING_ID_INDEX);
    }
    ValueVertex *GetHole()
    {
        return GetInput(HOLE_INDEX);
    }
    size_t GetArgCount() const
    {
        return GetInputCount();
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

private:
    kungfu::RuntimeStubCSigns::ID runtimeId_;
};

class GapMoveVertex : public FixedInputVertexMixin<0, NonControlVertex, GapMoveVertex> {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::Pure();

    explicit GapMoveVertex(uint64_t bitfield, AllocatedState source, AllocatedState target)
        : FixedInputVertexMixin(bitfield), source_(source), target_(target)
    {}

    const AllocatedState &GetSource() const
    {
        return source_;
    }

    const AllocatedState &GetTarget() const
    {
        return target_;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

private:
    AllocatedState source_;
    AllocatedState target_;
};

class ConstantGapMoveVertex : public FixedInputVertexMixin<0, NonControlVertex, ConstantGapMoveVertex> {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::Pure();

    explicit ConstantGapMoveVertex(uint64_t bitfield, ValueVertex *vertex, AllocatedState target)
        : FixedInputVertexMixin(bitfield), vertex_(vertex), target_(target)
    {}

    ValueVertex *GetVertex() const
    {
        return vertex_;
    }

    const AllocatedState &GetTarget() const
    {
        return target_;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

private:
    ValueVertex *vertex_;
    AllocatedState target_;
};

/**
 * Phi vertex - SSA merge point for values from different predecessor blocks
 */
class PhiVertex : public VertexMixin<ValueVertex, PhiVertex> {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::TaggedValue();

    explicit PhiVertex(uint64_t bitfield, MergePointFrameState *mergeState, VirtualRegister owner)
        : VertexMixin(bitfield), mergeState_(mergeState), owner_(owner)
    {}

    int GetPredecessorCount() const
    {
        return GetInputCount();
    }
    const ValueVertex *GetPredecessor(int index) const
    {
        return GetInput(index);
    }
    void SetPredecessor(int index, ValueVertex *value)
    {
        SetInput(index, value);
    }
    // Note: Remove this field after refactoring (from ArkSteed* to *New) is fully done.
    //       It's no more used in the new implementation.
    MergePointFrameState *GetMergePointState() const
    {
        return mergeState_;
    }
    VirtualRegister GetOwner() const
    {
        return owner_;
    }

    void SetValueLocationConstraints();

    // VerifyInputs: Variable-input vertex must validate input count and type consistency
    void VerifyInputs() const
    {
        ASSERT(GetInputCount() > 0);
        ValueRepresentation repr = GetInput(0)->GetValueRepresentation();
        for (int i = 1; i < GetInputCount(); ++i) {
            ASSERT(GetInput(i)->GetValueRepresentation() == repr);
        }
    }

    // Note: Remove this field after refactoring (from ArkSteed* to *New) is fully done.
    MergePointFrameState *const mergeState_;
    const VirtualRegister owner_;
};

class ToTaggedIntVertex : public FixedInputVertexMixin<1, ValueVertex, ToTaggedIntVertex> {
public:
    static constexpr int INPUT_INDEX = 0;
    static constexpr VertexProperties PROPERTIES = VertexProperties::TaggedValue();

    explicit ToTaggedIntVertex(uint64_t bitfield) : FixedInputVertexMixin(bitfield) {}

    const ValueVertex *GetInputValue() const
    {
        return GetInput(INPUT_INDEX);
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
};

// =============================================================================
// Helper functions for setting location constraints
// =============================================================================

constexpr int NO_VREG = -1;

inline void DefineAsRegister(ValueVertex *vertex)
{
    vertex->Result().SetUnallocated(UnallocatedState::ExtendedPolicy::MUST_HAVE_REGISTER, NO_VREG);
}

inline void DefineAsConstant(ValueVertex *vertex)
{
    vertex->Result().SetUnallocated(UnallocatedState::ExtendedPolicy::NONE, NO_VREG);
}

inline void DefineAsFixed(ValueVertex *vertex, uint32_t regCode)
{
    vertex->Result().SetUnallocated(UnallocatedState::ExtendedPolicy::FIXED_REGISTER, regCode, NO_VREG);
}

inline void DefineSameAsFirst(ValueVertex *vertex)
{
    vertex->Result().SetUnallocated(NO_VREG, 0);
}

inline void UseRegister(Input input)
{
    input.GetLocation()->GetOperand() = UnallocatedState(UnallocatedState::ExtendedPolicy::MUST_HAVE_REGISTER,
                                                         UnallocatedState::LifetimeFlag::USED_AT_END,
                                                         NO_VREG);
}

inline void UseAndClobberRegister(Input input)
{
    input.GetLocation()->GetOperand() = UnallocatedState(UnallocatedState::ExtendedPolicy::MUST_HAVE_REGISTER,
                                                         UnallocatedState::LifetimeFlag::USED_AT_START,
                                                         NO_VREG);
}

inline void UseAny(Input input)
{
    input.GetLocation()->GetOperand() = UnallocatedState(UnallocatedState::ExtendedPolicy::REGISTER_OR_SLOT_OR_CONSTANT,
                                                         UnallocatedState::LifetimeFlag::USED_AT_END,
                                                         NO_VREG);
}

inline void UseFixed(Input input, uint32_t regCode)
{
    input.GetLocation()->GetOperand() =
        UnallocatedState(UnallocatedState::ExtendedPolicy::FIXED_REGISTER, regCode, NO_VREG);
}

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_OPCODE_H
