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

#include <utility>
#include <vector>

#include "ecmascript/arksteed/arksteed_regalloc_types.h"
#include "ecmascript/arksteed/arksteed_vertex.h"
#include "ecmascript/arksteed/arksteed_vreg.h"
#include "ecmascript/compiler/deopt_type.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::arksteed {
using VirtualRegister = kungfu::VirtualRegister;

class BB;
class ArkSteedState;
class ArkSteedAssembler;

enum class BinaryOpKind : uint8_t {
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    EXP,
};

enum class Int32ConditionKind : uint8_t {
    EQUAL,
    NOT_EQUAL,
    LESS_THAN,
    LESS_THAN_OR_EQUAL,
    GREATER_THAN,
    GREATER_THAN_OR_EQUAL,
};

enum class Int32BitwiseKind : uint8_t {
    BITWISE_AND,
    BITWISE_OR,
    BITWISE_XOR,
    SHIFT_LEFT,
    SHIFT_RIGHT_LOGICAL,
    SHIFT_RIGHT_ARITHMETIC,
};

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

    constexpr uint32_t GetInputCount() const
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
            ASSERT(this->GetInputCount() == static_cast<uint32_t>(FIXED_INPUT_COUNT));

            // Verify input types if defined in derived class
            if constexpr (HasInputTypes<Derived>::value) {
                static_assert(FIXED_INPUT_COUNT == Derived::INPUT_TYPES.size());
                for (uint32_t i = 0; i < static_cast<uint32_t>(FIXED_INPUT_COUNT); ++i) {
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

    inline void CheckValueInput(uint32_t index, ValueRepresentation expectedRepr) const
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

class ThrowableMixin {
public:
    BB *CaughtBy() const
    {
        return caughtBy_;
    }

    void SetCaughtBy(BB *caughtBy)
    {
        caughtBy_ = caughtBy;
    }

    uint32_t GetCatchPredecessorIndex() const
    {
        return catchPredIndex_;
    }

    void SetCatchPredecessorIndex(uint32_t id)
    {
        catchPredIndex_ = id;
    }

private:
    BB *caughtBy_ = nullptr;
    uint32_t catchPredIndex_ = static_cast<uint32_t>(-1);
};

class DeoptimizableMixin {
public:
    DeoptimizableMixin(ChunkVector<VRegIDType> deoptVRegs, uint32_t bytecodeOffset)
        : DeoptimizableMixin(0, std::move(deoptVRegs), bytecodeOffset)
    {}

    DeoptimizableMixin(uint32_t firstDeoptInputIndex, ChunkVector<VRegIDType> deoptVRegs, uint32_t bytecodeOffset)
        : deoptVRegs_(std::move(deoptVRegs)),
          firstDeoptInputIndex_(firstDeoptInputIndex),
          bytecodeOffset_(bytecodeOffset)
    {}

    VRegIDType GetDeoptVReg(uint32_t index) const
    {
        ASSERT(index < deoptVRegs_.size());
        return deoptVRegs_[index];
    }

    const ChunkVector<VRegIDType> &GetDeoptVRegs() const
    {
        return deoptVRegs_;
    }

    uint32_t FirstDeoptInputIndex() const
    {
        return firstDeoptInputIndex_;
    }

    uint32_t DeoptInputCount() const
    {
        return static_cast<uint32_t>(deoptVRegs_.size());
    }

    int DeoptInputIndex(uint32_t index) const
    {
        ASSERT(index < DeoptInputCount());
        return static_cast<int>(firstDeoptInputIndex_ + index);
    }

    uint32_t GetBytecodeOffset() const
    {
        return bytecodeOffset_;
    }

private:
    ChunkVector<VRegIDType> deoptVRegs_;
    uint32_t firstDeoptInputIndex_ {0};
    uint32_t bytecodeOffset_;
};

//==============================================================================
// Constant Value Vertices
//==============================================================================

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

// Loads from raw pointer
class LoadFromAddressVertex : public FixedInputVertexMixin<1, ValueVertex, LoadFromAddressVertex> {
public:
    // Currently LoadFromAddressVertex can load TaggedValue only.
    static constexpr VertexProperties PROPERTIES = VertexProperties::TaggedValue();
    // 1 : Object
    static constexpr auto INPUT_TYPES = detail::InputTypes<1>(ValueRepresentation::INT_PTR);

    static constexpr size_t OBJECT_INDEX = 0;

    explicit LoadFromAddressVertex(uint64_t bitfield, int32_t offset)
        : FixedInputVertexMixin(bitfield), offset_(offset) {}

    int32_t GetOffset() const
    {
        return offset_;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

private:
    int32_t offset_;
};

// Loads exception from glue pointer (with fixed offset)
class LoadExceptionVertex : public FixedInputVertexMixin<1, ValueVertex, LoadExceptionVertex> {
public:
    // LoadExceptionVertex loads TaggedValue (the exception object) from glue pointer.
    static constexpr VertexProperties PROPERTIES = VertexProperties::TaggedValue();
    // 1 : Glue pointer
    static constexpr auto INPUT_TYPES = detail::InputTypes<1>(ValueRepresentation::INT_PTR);

    static constexpr size_t GLUE_INDEX = 0;

    explicit LoadExceptionVertex(uint64_t bitfield) : FixedInputVertexMixin(bitfield) {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
};

// Stores to raw pointer. 2 : Two inputs
class StoreToAddressVertex : public FixedInputVertexMixin<2, ValueVertex, StoreToAddressVertex> {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties(0);
    // 2: object and value to be stored (Currently StoreToAddressVertex can store TaggedValue only)
    static constexpr auto INPUT_TYPES =
        detail::InputTypes<2>(ValueRepresentation::INT_PTR, ValueRepresentation::TAGGED);

    static constexpr size_t OBJECT_INDEX = 0;
    static constexpr size_t VALUE_INDEX = 1;

    explicit StoreToAddressVertex(uint64_t bitfield, int32_t offset)
        : FixedInputVertexMixin(bitfield), offset_(offset) {}

    int32_t GetOffset() const
    {
        return offset_;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

private:
    int32_t offset_;
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
    static constexpr uint32_t UNKNOWN_PROPERTY_ID = UINT32_MAX;

    explicit StoreTaggedFieldVertex(uint64_t bitfield, int32_t offset, uint32_t propertyId = UNKNOWN_PROPERTY_ID)
        : FixedInputVertexMixin(bitfield), offset_(offset), propertyId_(propertyId)
    {
    }

    int32_t GetOffset() const
    {
        return offset_;
    }

    uint32_t GetPropertyId() const
    {
        return propertyId_;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

private:
    int32_t offset_;
    uint32_t propertyId_;
};

class StoreEnvSlotVertex : public FixedInputVertexMixin<2, ValueVertex, StoreEnvSlotVertex> {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::CanWriteProp();

    static constexpr auto INPUT_TYPES = detail::InputTypes<2>(ValueRepresentation::TAGGED, ValueRepresentation::TAGGED);

    static constexpr size_t ENV_INDEX = 0;
    static constexpr size_t VALUE_INDEX = 1;

    explicit StoreEnvSlotVertex(uint64_t bitfield, int32_t offset) : FixedInputVertexMixin(bitfield), offset_(offset) {}

    int32_t GetOffset() const
    {
        return offset_;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

private:
    int32_t offset_;
};

class SetValueWithBarrierVertex : public FixedInputVertexMixin<3, NonControlVertex, SetValueWithBarrierVertex> {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::Call();

    static constexpr auto INPUT_TYPES =
        detail::InputTypes<3>(ValueRepresentation::INT_PTR, ValueRepresentation::TAGGED,
                              ValueRepresentation::TAGGED);

    static constexpr size_t GLUE_INDEX = 0;
    static constexpr size_t OBJECT_INDEX = 1;
    static constexpr size_t VALUE_INDEX = 2;

    explicit SetValueWithBarrierVertex(uint64_t bitfield, int32_t offset)
        : FixedInputVertexMixin(bitfield), offset_(offset)
    {
    }

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
class CallRuntimeVertex : public VertexMixin<ValueVertex, CallRuntimeVertex>, public ThrowableMixin {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::JsCall();

    CallRuntimeVertex(uint64_t bitfield, kungfu::RuntimeStubCSigns::ID id,
                      SideEffectKind sideEffectKind = SideEffectKind::UNKNOWN_CALL)
        : VertexMixin(bitfield), runtimeId_(id), sideEffectKind_(sideEffectKind)
    {
        // Only UNKNOWN_CALL and SAFE_CALL are supported by now.
        ASSERT(sideEffectKind_ == SideEffectKind::UNKNOWN_CALL || sideEffectKind_ == SideEffectKind::SAFE_CALL);
    }

    kungfu::RuntimeStubCSigns::ID GetRuntimeId() const
    {
        return runtimeId_;
    }
    size_t GetArgCount() const
    {
        return GetInputCount();
    }
    SideEffectKind GetSideEffectKind() const
    {
        return sideEffectKind_;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

    // Runtime argv may be empty.
    void VerifyInputs() const {}

private:
    kungfu::RuntimeStubCSigns::ID runtimeId_;
    SideEffectKind sideEffectKind_;
};

class CallVertex : public VertexMixin<ValueVertex, CallVertex> {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::JsCall();

    static constexpr uint32_t TARGET_INDEX = 0;
    static constexpr uint32_t NEW_TARGET_INDEX = 1;
    static constexpr uint32_t THIS_INDEX = 2;
    static constexpr uint32_t FIRST_ARG_INDEX = 3;

    CallVertex(uint64_t bitfield, uint32_t actualArgc)
        : VertexMixin(bitfield), actualArgc_(actualArgc)
    {}

    uint32_t GetActualArgc() const
    {
        return actualArgc_;
    }

    size_t GetArgCount() const
    {
        return GetInputCount();
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

    void VerifyInputs() const
    {
        ASSERT(GetInputCount() == static_cast<uint32_t>(FIRST_ARG_INDEX + actualArgc_));
    }

private:
    uint32_t actualArgc_;
};

/**
 * CallCommonStub vertex - for common stub calls
 */
class CallCommonStubVertex : public VertexMixin<ValueVertex, CallCommonStubVertex>, public ThrowableMixin {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::JsCall();

    CallCommonStubVertex(uint64_t bitfield, uint32_t stubId,
                         SideEffectKind sideEffectKind = SideEffectKind::UNKNOWN_CALL)
        : VertexMixin(bitfield), stubId_(stubId), sideEffectKind_(sideEffectKind)
    {
        // Only UNKNOWN_CALL and SAFE_CALL are supported by now.
        ASSERT(sideEffectKind_ == SideEffectKind::UNKNOWN_CALL || sideEffectKind_ == SideEffectKind::SAFE_CALL);
    }

    uint32_t GetStubId() const
    {
        return stubId_;
    }
    size_t GetArgCount() const
    {
        return GetInputCount();
    }
    SideEffectKind GetSideEffectKind() const
    {
        return sideEffectKind_;
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
    SideEffectKind sideEffectKind_;
};

class TaggedIntToI32Vertex : public FixedInputVertexMixin<1, ValueVertex, TaggedIntToI32Vertex> {
public:
    static constexpr int INPUT_INDEX = 0;
    static constexpr detail::InputTypes<1> INPUT_TYPES {ValueRepresentation::TAGGED};
    static constexpr VertexProperties PROPERTIES = VertexProperties::Int32();

    explicit TaggedIntToI32Vertex(uint64_t bitfield) : FixedInputVertexMixin(bitfield) {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
};

class CheckedTaggedIntToI32Vertex : public VertexMixin<ValueVertex, CheckedTaggedIntToI32Vertex>,
                                    public DeoptimizableMixin {
public:
    static constexpr int INPUT_INDEX = 0;
    static constexpr int FIRST_DEOPT_INDEX = 1;
    static constexpr VertexProperties PROPERTIES = VertexProperties::Int32() | VertexProperties::NotIdempotent();

    explicit CheckedTaggedIntToI32Vertex(uint64_t bitfield, uint32_t firstDeoptInputIndex,
                                         ChunkVector<VRegIDType> deoptVRegs, uint32_t bytecodeOffset)
        : VertexMixin(bitfield), DeoptimizableMixin(firstDeoptInputIndex, std::move(deoptVRegs), bytecodeOffset)
    {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
    void VerifyInputs() const
    {
        ASSERT(FirstDeoptInputIndex() == FIRST_DEOPT_INDEX);
        ASSERT(GetInputCount() == static_cast<int>(FirstDeoptInputIndex() + DeoptInputCount()));
        ASSERT(GetInput(INPUT_INDEX)->GetValueRepresentation() == ValueRepresentation::TAGGED);
    }
};

class CheckedTaggedStringVertex : public VertexMixin<ValueVertex, CheckedTaggedStringVertex>,
                                  public DeoptimizableMixin {
public:
    static constexpr int INPUT_INDEX = 0;
    static constexpr int FIRST_DEOPT_INDEX = 1;
    static constexpr VertexProperties PROPERTIES = VertexProperties::TaggedValue() | VertexProperties::NotIdempotent();

    explicit CheckedTaggedStringVertex(uint64_t bitfield, uint32_t firstDeoptInputIndex,
                                       ChunkVector<VRegIDType> deoptVRegs, uint32_t bytecodeOffset)
        : VertexMixin(bitfield), DeoptimizableMixin(firstDeoptInputIndex, std::move(deoptVRegs), bytecodeOffset)
    {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
    void VerifyInputs() const
    {
        ASSERT(FirstDeoptInputIndex() == FIRST_DEOPT_INDEX);
        ASSERT(GetInputCount() == static_cast<int>(FirstDeoptInputIndex() + DeoptInputCount()));
        ASSERT(GetInput(INPUT_INDEX)->GetValueRepresentation() == ValueRepresentation::TAGGED);
    }
};

class I32AddWithOverflowVertex : public VertexMixin<ValueVertex, I32AddWithOverflowVertex>,
                                 public DeoptimizableMixin {
public:
    static constexpr int LEFT_INDEX = 0;
    static constexpr int RIGHT_INDEX = 1;
    static constexpr int FIRST_DEOPT_INDEX = 2;
    static constexpr VertexProperties PROPERTIES = VertexProperties::Int32() | VertexProperties::NotIdempotent();

    explicit I32AddWithOverflowVertex(uint64_t bitfield, uint32_t firstDeoptInputIndex,
                                       ChunkVector<VRegIDType> deoptVRegs, uint32_t bytecodeOffset)
        : VertexMixin(bitfield), DeoptimizableMixin(firstDeoptInputIndex, std::move(deoptVRegs), bytecodeOffset)
    {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
    void VerifyInputs() const
    {
        VerifyI32BinOpInputs();
    }

private:
    void VerifyI32BinOpInputs() const;
};

class I32SubWithOverflowVertex : public VertexMixin<ValueVertex, I32SubWithOverflowVertex>,
                                 public DeoptimizableMixin {
public:
    static constexpr int LEFT_INDEX = 0;
    static constexpr int RIGHT_INDEX = 1;
    static constexpr int FIRST_DEOPT_INDEX = 2;
    static constexpr VertexProperties PROPERTIES = VertexProperties::Int32() | VertexProperties::NotIdempotent();

    explicit I32SubWithOverflowVertex(uint64_t bitfield, uint32_t firstDeoptInputIndex,
                                       ChunkVector<VRegIDType> deoptVRegs, uint32_t bytecodeOffset)
        : VertexMixin(bitfield), DeoptimizableMixin(firstDeoptInputIndex, std::move(deoptVRegs), bytecodeOffset)
    {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
    void VerifyInputs() const
    {
        VerifyI32BinOpInputs();
    }

private:
    void VerifyI32BinOpInputs() const;
};

class I32MulWithOverflowVertex : public VertexMixin<ValueVertex, I32MulWithOverflowVertex>,
                                 public DeoptimizableMixin {
public:
    static constexpr int LEFT_INDEX = 0;
    static constexpr int RIGHT_INDEX = 1;
    static constexpr int FIRST_DEOPT_INDEX = 2;
    static constexpr VertexProperties PROPERTIES = VertexProperties::Int32() | VertexProperties::NotIdempotent();

    explicit I32MulWithOverflowVertex(uint64_t bitfield, uint32_t firstDeoptInputIndex,
                                      ChunkVector<VRegIDType> deoptVRegs, uint32_t bytecodeOffset)
        : VertexMixin(bitfield), DeoptimizableMixin(firstDeoptInputIndex, std::move(deoptVRegs), bytecodeOffset)
    {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
    void VerifyInputs() const
    {
        VerifyI32BinOpInputs();
    }

private:
    void VerifyI32BinOpInputs() const;
};

class I32DivWithOverflowVertex : public VertexMixin<ValueVertex, I32DivWithOverflowVertex>,
                                 public DeoptimizableMixin {
public:
    static constexpr int LEFT_INDEX = 0;
    static constexpr int RIGHT_INDEX = 1;
    static constexpr int FIRST_DEOPT_INDEX = 2;
    static constexpr VertexProperties PROPERTIES = VertexProperties::Int32() | VertexProperties::NotIdempotent();

    explicit I32DivWithOverflowVertex(uint64_t bitfield, uint32_t firstDeoptInputIndex,
                                      ChunkVector<VRegIDType> deoptVRegs, uint32_t bytecodeOffset)
        : VertexMixin(bitfield), DeoptimizableMixin(firstDeoptInputIndex, std::move(deoptVRegs), bytecodeOffset)
    {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
    void VerifyInputs() const
    {
        VerifyI32BinOpInputs();
    }

private:
    void VerifyI32BinOpInputs() const;
};

class I32DivByConstWithCheckVertex : public VertexMixin<ValueVertex, I32DivByConstWithCheckVertex>,
                                     public DeoptimizableMixin {
public:
    static constexpr int INPUT_INDEX = 0;
    static constexpr int FIRST_DEOPT_INDEX = 1;
    static constexpr VertexProperties PROPERTIES = VertexProperties::Int32() | VertexProperties::NotIdempotent();

    explicit I32DivByConstWithCheckVertex(uint64_t bitfield, uint32_t firstDeoptInputIndex,
                                          ChunkVector<VRegIDType> deoptVRegs, uint32_t bytecodeOffset,
                                          int32_t divisor, int32_t magic, uint32_t shift)
        : VertexMixin(bitfield),
          DeoptimizableMixin(firstDeoptInputIndex, std::move(deoptVRegs), bytecodeOffset),
          divisor_(divisor),
          magic_(magic),
          shift_(shift)
    {}

    int32_t GetDivisor() const
    {
        return divisor_;
    }

    int32_t GetMagic() const
    {
        return magic_;
    }

    uint32_t GetShift() const
    {
        return shift_;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

    void VerifyInputs() const
    {
        ASSERT(FirstDeoptInputIndex() == FIRST_DEOPT_INDEX);
        ASSERT(GetInputCount() == static_cast<int>(FirstDeoptInputIndex() + DeoptInputCount()));
        ASSERT(GetInput(INPUT_INDEX)->GetValueRepresentation() == ValueRepresentation::INT32);
        ASSERT(divisor_ <= -2 || divisor_ >= 2);
    }

private:
    int32_t divisor_;
    int32_t magic_;
    uint32_t shift_;
};

class I32AddVertex : public FixedInputVertexMixin<2, ValueVertex, I32AddVertex> {
public:
    static constexpr int LEFT_INDEX = 0;
    static constexpr int RIGHT_INDEX = 1;
    static constexpr detail::InputTypes<2> INPUT_TYPES {ValueRepresentation::INT32, ValueRepresentation::INT32};
    static constexpr VertexProperties PROPERTIES = VertexProperties::Int32();

    explicit I32AddVertex(uint64_t bitfield) : FixedInputVertexMixin(bitfield) {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
};

class I32SubVertex : public FixedInputVertexMixin<2, ValueVertex, I32SubVertex> {
public:
    static constexpr int LEFT_INDEX = 0;
    static constexpr int RIGHT_INDEX = 1;
    static constexpr detail::InputTypes<2> INPUT_TYPES {ValueRepresentation::INT32, ValueRepresentation::INT32};
    static constexpr VertexProperties PROPERTIES = VertexProperties::Int32();

    explicit I32SubVertex(uint64_t bitfield) : FixedInputVertexMixin(bitfield) {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
};

class I32MulVertex : public FixedInputVertexMixin<2, ValueVertex, I32MulVertex> {
public:
    static constexpr int LEFT_INDEX = 0;
    static constexpr int RIGHT_INDEX = 1;
    static constexpr detail::InputTypes<2> INPUT_TYPES {ValueRepresentation::INT32, ValueRepresentation::INT32};
    static constexpr VertexProperties PROPERTIES = VertexProperties::Int32();

    explicit I32MulVertex(uint64_t bitfield) : FixedInputVertexMixin(bitfield) {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
};

class I32DivVertex : public FixedInputVertexMixin<2, ValueVertex, I32DivVertex> {
public:
    static constexpr int LEFT_INDEX = 0;
    static constexpr int RIGHT_INDEX = 1;
    static constexpr detail::InputTypes<2> INPUT_TYPES {ValueRepresentation::INT32, ValueRepresentation::INT32};
    static constexpr VertexProperties PROPERTIES = VertexProperties::Int32();

    explicit I32DivVertex(uint64_t bitfield) : FixedInputVertexMixin(bitfield) {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
};

class PositiveI32ModVertex : public FixedInputVertexMixin<2, ValueVertex, PositiveI32ModVertex> {
public:
    static constexpr int LEFT_INDEX = 0;
    static constexpr int RIGHT_INDEX = 1;
    static constexpr detail::InputTypes<2> INPUT_TYPES {ValueRepresentation::INT32, ValueRepresentation::INT32};
    static constexpr VertexProperties PROPERTIES = VertexProperties::Int32();

    explicit PositiveI32ModVertex(uint64_t bitfield) : FixedInputVertexMixin(bitfield) {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
};

class CheckedPositiveI32ModVertex : public VertexMixin<ValueVertex, CheckedPositiveI32ModVertex>,
                                    public DeoptimizableMixin {
public:
    static constexpr int LEFT_INDEX = 0;
    static constexpr int RIGHT_INDEX = 1;
    static constexpr int FIRST_DEOPT_INDEX = 2;
    static constexpr VertexProperties PROPERTIES = VertexProperties::Int32() | VertexProperties::NotIdempotent();

    explicit CheckedPositiveI32ModVertex(uint64_t bitfield, uint32_t firstDeoptInputIndex,
                                         ChunkVector<VRegIDType> deoptVRegs, uint32_t bytecodeOffset)
        : VertexMixin(bitfield), DeoptimizableMixin(firstDeoptInputIndex, std::move(deoptVRegs), bytecodeOffset)
    {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
    void VerifyInputs() const
    {
        ASSERT(FirstDeoptInputIndex() == FIRST_DEOPT_INDEX);
        ASSERT(GetInputCount() == static_cast<int>(FirstDeoptInputIndex() + DeoptInputCount()));
        ASSERT(GetInput(LEFT_INDEX)->GetValueRepresentation() == ValueRepresentation::INT32);
        ASSERT(GetInput(RIGHT_INDEX)->GetValueRepresentation() == ValueRepresentation::INT32);
    }
};

class I32BitwiseBinaryVertex : public FixedInputVertexMixin<2, ValueVertex, I32BitwiseBinaryVertex> {
public:
    static constexpr int LEFT_INDEX = 0;
    static constexpr int RIGHT_INDEX = 1;
    static constexpr detail::InputTypes<2> INPUT_TYPES {ValueRepresentation::INT32, ValueRepresentation::INT32};
    static constexpr VertexProperties PROPERTIES = VertexProperties::Int32();

    explicit I32BitwiseBinaryVertex(uint64_t bitfield, Int32BitwiseKind kind)
        : FixedInputVertexMixin(bitfield), kind_(kind)
    {}

    Int32BitwiseKind GetKind() const
    {
        return kind_;
    }

    bool RightInputIsConstant() const
    {
        return Arg(RIGHT_INDEX).vertex()->TryCast<Int32ConstantVertex>() != nullptr;
    }

    bool IsShift() const
    {
        return kind_ == Int32BitwiseKind::SHIFT_LEFT ||
               kind_ == Int32BitwiseKind::SHIFT_RIGHT_LOGICAL ||
               kind_ == Int32BitwiseKind::SHIFT_RIGHT_ARITHMETIC;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

private:
    Int32BitwiseKind kind_;
};

class CheckedNonNegativeI32ToTaggedIntVertex
    : public VertexMixin<ValueVertex, CheckedNonNegativeI32ToTaggedIntVertex>,
      public DeoptimizableMixin {
public:
    static constexpr int INPUT_INDEX = 0;
    static constexpr int FIRST_DEOPT_INDEX = 1;
    static constexpr VertexProperties PROPERTIES = VertexProperties::TaggedValue() | VertexProperties::NotIdempotent();

    explicit CheckedNonNegativeI32ToTaggedIntVertex(uint64_t bitfield, uint32_t firstDeoptInputIndex,
                                                    ChunkVector<VRegIDType> deoptVRegs, uint32_t bytecodeOffset)
        : VertexMixin(bitfield), DeoptimizableMixin(firstDeoptInputIndex, std::move(deoptVRegs), bytecodeOffset)
    {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
    void VerifyInputs() const
    {
        ASSERT(FirstDeoptInputIndex() == FIRST_DEOPT_INDEX);
        ASSERT(GetInputCount() == static_cast<int>(FirstDeoptInputIndex() + DeoptInputCount()));
        ASSERT(GetInput(INPUT_INDEX)->GetValueRepresentation() == ValueRepresentation::INT32);
    }
};

class I32BNotVertex : public FixedInputVertexMixin<1, ValueVertex, I32BNotVertex> {
public:
    static constexpr int VALUE_INDEX = 0;
    static constexpr detail::InputTypes<1> INPUT_TYPES {ValueRepresentation::INT32};
    static constexpr VertexProperties PROPERTIES = VertexProperties::Int32();

    explicit I32BNotVertex(uint64_t bitfield) : FixedInputVertexMixin(bitfield) {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
};

class I32NegWithOverflowVertex : public VertexMixin<ValueVertex, I32NegWithOverflowVertex>,
                                 public DeoptimizableMixin {
public:
    static constexpr int VALUE_INDEX = 0;
    static constexpr int FIRST_DEOPT_INDEX = 1;
    static constexpr VertexProperties PROPERTIES = VertexProperties::Int32() | VertexProperties::NotIdempotent();

    explicit I32NegWithOverflowVertex(uint64_t bitfield, uint32_t firstDeoptInputIndex,
                                      ChunkVector<VRegIDType> deoptVRegs, uint32_t bytecodeOffset)
        : VertexMixin(bitfield), DeoptimizableMixin(firstDeoptInputIndex, std::move(deoptVRegs), bytecodeOffset)
    {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
    void VerifyInputs() const
    {
        VerifyI32UnaryOpInputs();
    }

private:
    void VerifyI32UnaryOpInputs() const;
};

class I32IncWithOverflowVertex : public VertexMixin<ValueVertex, I32IncWithOverflowVertex>,
                                 public DeoptimizableMixin {
public:
    static constexpr int VALUE_INDEX = 0;
    static constexpr int FIRST_DEOPT_INDEX = 1;
    static constexpr VertexProperties PROPERTIES = VertexProperties::Int32() | VertexProperties::NotIdempotent();

    explicit I32IncWithOverflowVertex(uint64_t bitfield, uint32_t firstDeoptInputIndex,
                                      ChunkVector<VRegIDType> deoptVRegs, uint32_t bytecodeOffset)
        : VertexMixin(bitfield), DeoptimizableMixin(firstDeoptInputIndex, std::move(deoptVRegs), bytecodeOffset)
    {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
    void VerifyInputs() const
    {
        VerifyI32UnaryOpInputs();
    }

private:
    void VerifyI32UnaryOpInputs() const;
};

class I32DecWithOverflowVertex : public VertexMixin<ValueVertex, I32DecWithOverflowVertex>,
                                 public DeoptimizableMixin {
public:
    static constexpr int VALUE_INDEX = 0;
    static constexpr int FIRST_DEOPT_INDEX = 1;
    static constexpr VertexProperties PROPERTIES = VertexProperties::Int32() | VertexProperties::NotIdempotent();

    explicit I32DecWithOverflowVertex(uint64_t bitfield, uint32_t firstDeoptInputIndex,
                                      ChunkVector<VRegIDType> deoptVRegs, uint32_t bytecodeOffset)
        : VertexMixin(bitfield), DeoptimizableMixin(firstDeoptInputIndex, std::move(deoptVRegs), bytecodeOffset)
    {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
    void VerifyInputs() const
    {
        VerifyI32UnaryOpInputs();
    }

private:
    void VerifyI32UnaryOpInputs() const;
};

class I32ToF64Vertex : public FixedInputVertexMixin<1, ValueVertex, I32ToF64Vertex> {
public:
    static constexpr int INPUT_INDEX = 0;
    static constexpr detail::InputTypes<1> INPUT_TYPES {ValueRepresentation::INT32};
    static constexpr VertexProperties PROPERTIES = VertexProperties::Float64();

    explicit I32ToF64Vertex(uint64_t bitfield) : FixedInputVertexMixin(bitfield) {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
};

class CheckedNumberToF64Vertex : public VertexMixin<ValueVertex, CheckedNumberToF64Vertex>,
                                public DeoptimizableMixin {
public:
    static constexpr int INPUT_INDEX = 0;
    static constexpr int FIRST_DEOPT_INDEX = 1;
    static constexpr VertexProperties PROPERTIES = VertexProperties::Float64() | VertexProperties::NotIdempotent();

    explicit CheckedNumberToF64Vertex(uint64_t bitfield, uint32_t firstDeoptInputIndex,
                                      ChunkVector<VRegIDType> deoptVRegs, uint32_t bytecodeOffset)
        : VertexMixin(bitfield), DeoptimizableMixin(firstDeoptInputIndex, std::move(deoptVRegs), bytecodeOffset)
    {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
    void VerifyInputs() const
    {
        ASSERT(FirstDeoptInputIndex() == FIRST_DEOPT_INDEX);
        ASSERT(GetInputCount() == static_cast<int>(FirstDeoptInputIndex() + DeoptInputCount()));
        ASSERT(GetInput(INPUT_INDEX)->GetValueRepresentation() == ValueRepresentation::TAGGED);
    }
};

class F64ToI32TruncVertex : public FixedInputVertexMixin<1, ValueVertex, F64ToI32TruncVertex> {
public:
    static constexpr int INPUT_INDEX = 0;
    static constexpr detail::InputTypes<1> INPUT_TYPES {ValueRepresentation::FLOAT64};
    static constexpr VertexProperties PROPERTIES = VertexProperties::Int32();

    explicit F64ToI32TruncVertex(uint64_t bitfield) : FixedInputVertexMixin(bitfield) {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
};

class F64ToTaggedDoubleVertex : public FixedInputVertexMixin<1, ValueVertex, F64ToTaggedDoubleVertex> {
public:
    static constexpr int INPUT_INDEX = 0;
    static constexpr detail::InputTypes<1> INPUT_TYPES {ValueRepresentation::FLOAT64};
    static constexpr VertexProperties PROPERTIES = VertexProperties::TaggedValue();

    explicit F64ToTaggedDoubleVertex(uint64_t bitfield) : FixedInputVertexMixin(bitfield) {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
};

class F64NegVertex : public FixedInputVertexMixin<1, ValueVertex, F64NegVertex> {
public:
    static constexpr int VALUE_INDEX = 0;
    static constexpr detail::InputTypes<1> INPUT_TYPES {ValueRepresentation::FLOAT64};
    static constexpr VertexProperties PROPERTIES = VertexProperties::Float64();

    explicit F64NegVertex(uint64_t bitfield) : FixedInputVertexMixin(bitfield) {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
};

class F64AddVertex : public FixedInputVertexMixin<2, ValueVertex, F64AddVertex> {
public:
    static constexpr int LEFT_INDEX = 0;
    static constexpr int RIGHT_INDEX = 1;
    static constexpr detail::InputTypes<2> INPUT_TYPES {ValueRepresentation::FLOAT64, ValueRepresentation::FLOAT64};
    static constexpr VertexProperties PROPERTIES = VertexProperties::Float64();

    explicit F64AddVertex(uint64_t bitfield) : FixedInputVertexMixin(bitfield) {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
};

class F64SubVertex : public FixedInputVertexMixin<2, ValueVertex, F64SubVertex> {
public:
    static constexpr int LEFT_INDEX = 0;
    static constexpr int RIGHT_INDEX = 1;
    static constexpr detail::InputTypes<2> INPUT_TYPES {ValueRepresentation::FLOAT64, ValueRepresentation::FLOAT64};
    static constexpr VertexProperties PROPERTIES = VertexProperties::Float64();

    explicit F64SubVertex(uint64_t bitfield) : FixedInputVertexMixin(bitfield) {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
};

class F64MulVertex : public FixedInputVertexMixin<2, ValueVertex, F64MulVertex> {
public:
    static constexpr int LEFT_INDEX = 0;
    static constexpr int RIGHT_INDEX = 1;
    static constexpr detail::InputTypes<2> INPUT_TYPES {ValueRepresentation::FLOAT64, ValueRepresentation::FLOAT64};
    static constexpr VertexProperties PROPERTIES = VertexProperties::Float64();

    explicit F64MulVertex(uint64_t bitfield) : FixedInputVertexMixin(bitfield) {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
};

class F64DivVertex : public FixedInputVertexMixin<2, ValueVertex, F64DivVertex> {
public:
    static constexpr int LEFT_INDEX = 0;
    static constexpr int RIGHT_INDEX = 1;
    static constexpr detail::InputTypes<2> INPUT_TYPES {ValueRepresentation::FLOAT64, ValueRepresentation::FLOAT64};
    static constexpr VertexProperties PROPERTIES = VertexProperties::Float64();

    explicit F64DivVertex(uint64_t bitfield) : FixedInputVertexMixin(bitfield) {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
};

//==============================================================================
// Load/Store Vertices
//==============================================================================

class DeoptIfHClassMismatchVertex : public VertexMixin<NonControlVertex, DeoptIfHClassMismatchVertex>,
                                    public DeoptimizableMixin {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::EagerDeopt() | VertexProperties::CanReadProp();

    static constexpr size_t RECEIVER_INDEX = 0;

    explicit DeoptIfHClassMismatchVertex(uint64_t bitfield,
                                         JSHClass *expectedHClass,
                                         ChunkVector<VRegIDType> deoptVRegs,
                                         uint32_t bytecodeOffset)
        : VertexMixin(bitfield),
          DeoptimizableMixin(RECEIVER_INDEX + 1, std::move(deoptVRegs), bytecodeOffset),
          expectedHClass_(expectedHClass)
    {}

    JSHClass *GetExpectedHClass() const
    {
        return expectedHClass_;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

    void VerifyInputs() const
    {
        ASSERT(expectedHClass_ != nullptr);
        ASSERT(GetInputCount() == static_cast<uint32_t>(GetDeoptVRegs().size() + 1));
    }

private:
    JSHClass *expectedHClass_;
};

class DeoptIfInt32ConditionVertex : public VertexMixin<NonControlVertex, DeoptIfInt32ConditionVertex>,
                                    public DeoptimizableMixin {
public:
    static constexpr int LEFT_INDEX = 0;
    static constexpr int RIGHT_INDEX = 1;
    static constexpr int FIRST_DEOPT_INDEX = 2;
    static constexpr VertexProperties PROPERTIES = VertexProperties::EagerDeopt();

    explicit DeoptIfInt32ConditionVertex(uint64_t bitfield, uint32_t firstDeoptInputIndex,
                                         ChunkVector<VRegIDType> deoptVRegs, uint32_t bytecodeOffset,
                                         Int32ConditionKind condition, kungfu::DeoptType deoptType)
        : VertexMixin(bitfield),
          DeoptimizableMixin(firstDeoptInputIndex, std::move(deoptVRegs), bytecodeOffset),
          condition_(condition),
          deoptType_(deoptType)
    {}

    Int32ConditionKind GetCondition() const
    {
        return condition_;
    }

    kungfu::DeoptType GetDeoptType() const
    {
        return deoptType_;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

    void VerifyInputs() const
    {
        ASSERT(FirstDeoptInputIndex() == FIRST_DEOPT_INDEX);
        ASSERT(GetInputCount() == static_cast<int>(FirstDeoptInputIndex() + DeoptInputCount()));
        ASSERT(GetInput(LEFT_INDEX)->GetValueRepresentation() == ValueRepresentation::INT32);
        ASSERT(GetInput(RIGHT_INDEX)->GetValueRepresentation() == ValueRepresentation::INT32);
    }

private:
    Int32ConditionKind condition_;
    kungfu::DeoptType deoptType_;
};

class DeoptVertex : public VertexMixin<NonControlVertex, DeoptVertex>, public DeoptimizableMixin {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::EagerDeopt();

    explicit DeoptVertex(uint64_t bitfield,
                         kungfu::DeoptType type,
                         ChunkVector<VRegIDType> deoptVRegs,
                         uint32_t bytecodeOffset)
        : VertexMixin(bitfield),
          DeoptimizableMixin(std::move(deoptVRegs), bytecodeOffset),
          deoptType_(type)
    {}

    kungfu::DeoptType GetDeoptType() const
    {
        return deoptType_;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

    void VerifyInputs() const
    {
        ASSERT(GetInputCount() == static_cast<uint32_t>(GetDeoptVRegs().size()));
    }

private:
    kungfu::DeoptType deoptType_;
};

//==============================================================================
// Control Flow Vertices
//==============================================================================

class UnconditionalControlVertex : public ControlVertex {
public:
    BB *Target() const
    {
        return target_;
    }
    void SetTarget(BB *block)
    {
        target_ = block;
    }

    uint32_t GetPredecessorId() const
    {
        return predecessorId_;
    }

    void SetPredecessorId(uint32_t id)
    {
        predecessorId_ = id;
    }

protected:
    explicit UnconditionalControlVertex(uint64_t bitfield, BB *target)
        : ControlVertex(bitfield), target_(target), predecessorId_(0)
    {}

private:
    BB *target_;
    uint32_t predecessorId_;
};

/**
 * UnconditionalControlVertexT - CRTP mixin for unconditional control vertices
 * Provides compile-time type checking and target management for unconditional jumps
 * (Jump, JumpLoop, etc.)
 */
template <typename Derived>
class UnconditionalControlVertexT : public FixedInputVertexMixin<0, UnconditionalControlVertex, Derived> {
protected:
    UnconditionalControlVertexT(uint64_t bitfield, BB *target)
        : FixedInputVertexMixin<0, UnconditionalControlVertex, Derived>(bitfield, target)
    {}
};

class BranchControlVertex : public ControlVertex {
public:
    BB *IfTrue() const
    {
        return ifTrue_;
    }

    BB *IfFalse() const
    {
        return ifFalse_;
    }

    void SetIfTrue(BB *block)
    {
        ifTrue_ = block;
    }

    void SetIfFalse(BB *block)
    {
        ifFalse_ = block;
    }

protected:
    BranchControlVertex(uint64_t bitfield, BB *ifTrue, BB *ifFalse)
        : ControlVertex(bitfield), ifTrue_(ifTrue), ifFalse_(ifFalse)
    {}

private:
    BB *ifTrue_;
    BB *ifFalse_;
};

/**
 * BranchControlVertexT - CRTP mixin for conditional control vertices
 * Provides compile-time type checking and branch target management.
 */
template <size_t InputCount, typename Derived>
class BranchControlVertexT : public FixedInputVertexMixin<InputCount, BranchControlVertex, Derived> {
protected:
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

    JumpVertex(uint64_t bitfield, BB *target) : UnconditionalControlVertexT(bitfield, target) {}

    void SetValueLocationConstraints();
};

/**
 * JumpLoop vertex - unconditional branch to loop header (back edge)
 */
class JumpLoopVertex : public UnconditionalControlVertexT<JumpLoopVertex> {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::Pure();

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
class ThrowVertex : public VertexMixin<ControlVertex, ThrowVertex>, public ThrowableMixin {
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
    : public FixedInputVertexMixin<2, NonControlVertex, ThrowIfSuperNotCorrectCallVertex>, public ThrowableMixin {
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

    explicit PhiVertex(uint64_t bitfield, VirtualRegister owner)
        : VertexMixin(bitfield), owner_(owner)
    {}

    uint32_t GetPredecessorCount() const
    {
        return static_cast<uint32_t>(GetInputCount());
    }
    const ValueVertex *GetPredecessor(int index) const
    {
        return GetInput(index);
    }
    void SetPredecessor(int index, ValueVertex *value)
    {
        SetInput(index, value);
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
        for (uint32_t i = 1, n = GetInputCount(); i < n; ++i) {
            ASSERT(GetInput(i)->GetValueRepresentation() == repr);
        }
    }

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

inline BB *CatchBlockOf(Vertex *vertex)
{
    if (auto *derived = vertex->TryCast<CallCommonStubVertex>()) {
        return derived->CaughtBy();
    }
    if (auto *derived = vertex->TryCast<CallRuntimeVertex>()) {
        return derived->CaughtBy();
    }
    if (auto *derived = vertex->TryCast<ThrowVertex>()) {
        return derived->CaughtBy();
    }
    return nullptr;
}

inline uint32_t CatchPredecessorIndexOf(Vertex *vertex)
{
    if (auto *derived = vertex->TryCast<CallCommonStubVertex>()) {
        return derived->GetCatchPredecessorIndex();
    }
    if (auto *derived = vertex->TryCast<CallRuntimeVertex>()) {
        return derived->GetCatchPredecessorIndex();
    }
    if (auto *derived = vertex->TryCast<ThrowVertex>()) {
        return derived->GetCatchPredecessorIndex();
    }
    return static_cast<uint32_t>(-1);  // NULL_INDEX
}

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

inline void UseSlot(Input input)
{
    input.GetLocation()->GetOperand() = UnallocatedState(UnallocatedState::ExtendedPolicy::MUST_HAVE_SLOT,
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
