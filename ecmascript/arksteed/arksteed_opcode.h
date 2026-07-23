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

#include <algorithm>
#include <utility>
#include <vector>

#include "ecmascript/arksteed/arksteed_condition_code.h"
#include "ecmascript/arksteed/arksteed_deopt_helper.h"
#include "ecmascript/arksteed/arksteed_regalloc_types.h"
#include "ecmascript/arksteed/arksteed_vertex.h"
#include "ecmascript/arksteed/arksteed_vreg.h"
#include "ecmascript/compiler/deopt_type.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::arksteed {
using CommonStubID = kungfu::CommonStubCSigns::ID;
using RuntimeStubID = kungfu::RuntimeStubCSigns::ID;

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

enum class IntConditionKind : uint8_t {
    EQUAL,
    NOT_EQUAL,
    LESS_THAN,
    LESS_THAN_OR_EQUAL,
    GREATER_THAN,
    GREATER_THAN_OR_EQUAL,
};

enum class CompareOpKind : uint8_t {
    EQUAL,
    NOT_EQUAL,
    LESS_THAN,
    LESS_THAN_OR_EQUAL,
    GREATER_THAN,
    GREATER_THAN_OR_EQUAL,
    STRICT_EQUAL,
    STRICT_NOT_EQUAL,
};

enum class IntBitwiseKind : uint8_t {
    BITWISE_AND,
    BITWISE_OR,
    BITWISE_XOR,
    SHIFT_LEFT,
    SHIFT_RIGHT_LOGICAL,
    SHIFT_RIGHT_ARITHMETIC,
};

struct DeoptMetadata {
    explicit DeoptMetadata(Chunk *chunk, uint32_t offset)
        : indices(chunk), sources(chunk), locations(chunk), bcOffset(offset)
    {}

    ChunkVector<VRegIDType> indices;
    ChunkVector<ValueVertex *> sources;
    // Set during register allocation.
    ChunkVector<InputLocation> locations;
    // bcOffset = starting position of its owner.
    // For lazy-deopt, PC advancing is done by deopt trampoline in runtime.
    uint32_t bcOffset;
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

class CommonStubIDMixin {
public:
    explicit CommonStubIDMixin(CommonStubID id) : id_(id) {}

    CommonStubID GetCommonStubID() const
    {
        return id_;
    }
    void SetCommonStubID(CommonStubID id)
    {
        id_ = id;
    }

private:
    CommonStubID id_;
};

class RuntimeStubIDMixin {
public:
    explicit RuntimeStubIDMixin(RuntimeStubID id) : id_(id) {}

    RuntimeStubID GetRuntimeStubID() const
    {
        return id_;
    }
    void SetRuntimeStubID(RuntimeStubID id)
    {
        id_ = id;
    }

private:
    RuntimeStubID id_;
};

class ConditionCodeMixin {
public:
    explicit ConditionCodeMixin(Condition cc) : cc_(cc) {}

    Condition GetConditionCode() const
    {
        return cc_;
    }
    void SetConditionCode(Condition cc)
    {
        cc_ = cc;
    }

private:
    Condition cc_;
};

class ThrowableMixin {
public:
    ThrowableMixin() = default;

    ThrowableMixin(BB *caughtBy, uint32_t catchPredIndex)
        : catchPredIndex_(catchPredIndex),
          caughtBy_(caughtBy)
    {
        ASSERT(caughtBy_ != nullptr);
        ASSERT(catchPredIndex_ != NULL_INDEX);
    }

    bool HasCatchBlock() const
    {
        return caughtBy_ != nullptr;
    }

    BB *GetCatchBlock()
    {
        return caughtBy_;
    }

    const BB *GetCatchBlock() const
    {
        return caughtBy_;
    }

    void LoadCatchBlock(BB *block, uint32_t catchPredIndex)
    {
        ASSERT(block != nullptr);
        ASSERT(catchPredIndex != NULL_INDEX);
        caughtBy_ = block;
        catchPredIndex_ = catchPredIndex;
    }

    uint32_t GetCatchPredecessorIndex() const
    {
        ASSERT(HasCatchBlock() && "Check required.");
        ASSERT(catchPredIndex_ != NULL_INDEX);
        return catchPredIndex_;
    }

    void LoadExceptionLazyDeoptMetadata(DeoptMetadata *metadata)
    {
        ASSERT(metadata != nullptr);
        deoptMetadata_ = metadata;
    }

    bool HasExceptionLazyDeoptMetadata() const
    {
        return deoptMetadata_ != nullptr;
    }

    DeoptMetadata *GetExceptionLazyDeoptMetadata()
    {
        return deoptMetadata_;
    }

    const DeoptMetadata *GetExceptionLazyDeoptMetadata() const
    {
        return deoptMetadata_;
    }

private:
    static constexpr uint32_t NULL_INDEX = static_cast<uint32_t>(-1);
    uint32_t catchPredIndex_ = NULL_INDEX;
    BB *caughtBy_ {nullptr};
    DeoptMetadata *deoptMetadata_ {nullptr};
};

class LazyDeoptimizableMixin {
public:
    LazyDeoptimizableMixin() = default;

    explicit LazyDeoptimizableMixin(DeoptMetadata *metadata) : metadata_(metadata) {}

    void LoadLazyDeoptMetadata(DeoptMetadata *metadata)
    {
        ASSERT(metadata != nullptr);
        metadata_ = metadata;
    }

    bool HasLazyDeoptMetadata() const
    {
        return metadata_ != nullptr;
    }

    DeoptMetadata *GetLazyDeoptMetadata()
    {
        return metadata_;
    }

    const DeoptMetadata *GetLazyDeoptMetadata() const
    {
        return metadata_;
    }

    uint32_t DeoptInputCount() const
    {
        return metadata_ == nullptr ? 0 : static_cast<uint32_t>(metadata_->sources.size());
    }

    VRegIDType GetDeoptVReg(uint32_t index) const
    {
        ASSERT(metadata_ != nullptr);
        ASSERT(index < metadata_->indices.size());
        return metadata_->indices[index];
    }

    const ChunkVector<VRegIDType> &GetDeoptVRegs() const
    {
        ASSERT(metadata_ != nullptr);
        return metadata_->indices;
    }

    ValueVertex *GetDeoptSource(uint32_t index)
    {
        ASSERT(metadata_ != nullptr);
        ASSERT(index < metadata_->sources.size());
        return metadata_->sources[index];
    }

    const ValueVertex *GetDeoptSource(uint32_t index) const
    {
        ASSERT(metadata_ != nullptr);
        ASSERT(index < metadata_->sources.size());
        return metadata_->sources[index];
    }

    InputLocation *GetDeoptLocation(uint32_t index)
    {
        ASSERT(metadata_ != nullptr);
        ASSERT(index < metadata_->locations.size());
        return &metadata_->locations[index];
    }

    const InputLocation *GetDeoptLocation(uint32_t index) const
    {
        ASSERT(metadata_ != nullptr);
        ASSERT(index < metadata_->locations.size());
        return &metadata_->locations[index];
    }

    uint32_t GetBytecodeOffset() const
    {
        ASSERT(metadata_ != nullptr);
        return metadata_->bcOffset;
    }

private:
    DeoptMetadata *metadata_ {nullptr};
};

class EagerDeoptimizableMixin {
public:
    struct EagerDeoptFrameValue {
        VRegIDType vreg;
        ValueVertex *value;
        DeoptTranslationKind valueKind;
        InputLocation sourceLocation;

        EagerDeoptFrameValue(VRegIDType vregId, ValueVertex *frameValue, DeoptTranslationKind kind)
            : vreg(vregId), value(frameValue), valueKind(kind)
        {}
    };

    using EagerDeoptFrameState = ChunkVector<EagerDeoptFrameValue>;

    EagerDeoptimizableMixin(Chunk *chunk, uint32_t bytecodeOffset)
        : eagerDeoptFrameState_(chunk),
          bytecodeOffset_(bytecodeOffset)
    {}

    void SetEagerDeoptFrameState(EagerDeoptFrameState frameState)
    {
        eagerDeoptFrameState_ = std::move(frameState);
    }

    VRegIDType GetDeoptVReg(uint32_t index) const
    {
        ASSERT(index < eagerDeoptFrameState_.size());
        return eagerDeoptFrameState_[index].vreg;
    }

    uint32_t GetDeoptFrameValueCount() const
    {
        return static_cast<uint32_t>(eagerDeoptFrameState_.size());
    }

    ValueVertex *GetDeoptFrameValue(uint32_t index) const
    {
        ASSERT(index < GetDeoptFrameValueCount());
        return eagerDeoptFrameState_[index].value;
    }

    DeoptTranslationKind GetDeoptValueKind(uint32_t index) const
    {
        ASSERT(index < GetDeoptFrameValueCount());
        return eagerDeoptFrameState_[index].valueKind;
    }

    InputLocation *GetDeoptSourceLocation(uint32_t index)
    {
        ASSERT(index < GetDeoptFrameValueCount());
        return &eagerDeoptFrameState_[index].sourceLocation;
    }

    const InputLocation *GetDeoptSourceLocation(uint32_t index) const
    {
        ASSERT(index < GetDeoptFrameValueCount());
        return &eagerDeoptFrameState_[index].sourceLocation;
    }

    const EagerDeoptFrameState &GetEagerDeoptFrameState() const
    {
        return eagerDeoptFrameState_;
    }

    uint32_t GetBytecodeOffset() const
    {
        return bytecodeOffset_;
    }

private:
    EagerDeoptFrameState eagerDeoptFrameState_;
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

class Int64ConstantVertex : public FixedInputVertexMixin<0, ValueVertex, Int64ConstantVertex> {
public:
    using OutputRegister = ArkSteedRegister;
    static constexpr VertexProperties PROPERTIES = VertexProperties::Int64();

    Int64ConstantVertex(uint64_t bitfield, int64_t value) : FixedInputVertexMixin(bitfield), value_(value) {}

    int64_t GetValue() const
    {
        return value_;
    }

    void DoLoadToRegister(ArkSteedAssembler *, OutputRegister) const;
    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

private:
    int64_t value_;
};

// TODO: Adaptation to 32-bit platform
using IntPtrConstantVertex = Int64ConstantVertex;

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

// Loads TaggedValue from raw pointer
class LoadTaggedFromAddressVertex : public FixedInputVertexMixin<1, ValueVertex, LoadTaggedFromAddressVertex> {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::TaggedValue();
    // 1 : Object
    static constexpr auto INPUT_TYPES = detail::InputTypes<1>(ValueRepresentation::INT_PTR);

    static constexpr size_t OBJECT_INDEX = 0;

    explicit LoadTaggedFromAddressVertex(uint64_t bitfield, int32_t offset)
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

// Stores TaggedValue to raw pointer. 2 : Two inputs
class StoreTaggedToAddressVertex : public FixedInputVertexMixin<2, NonControlVertex, StoreTaggedToAddressVertex> {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties(0);
    // 2: object and value to be stored
    static constexpr auto INPUT_TYPES =
        detail::InputTypes<2>(ValueRepresentation::INT_PTR, ValueRepresentation::TAGGED);

    static constexpr size_t OBJECT_INDEX = 0;
    static constexpr size_t VALUE_INDEX = 1;

    explicit StoreTaggedToAddressVertex(uint64_t bitfield, int32_t offset)
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

// Loads Int32 from raw pointer
class LoadI32FromAddressVertex : public FixedInputVertexMixin<1, ValueVertex, LoadI32FromAddressVertex> {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::Int32();
    static constexpr auto INPUT_TYPES = detail::InputTypes<1>(ValueRepresentation::INT_PTR);

    static constexpr size_t OBJECT_INDEX = 0;

    explicit LoadI32FromAddressVertex(uint64_t bitfield, int32_t offset)
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

// Stores Int32 to raw pointer. 2 : Two inputs
class StoreI32ToAddressVertex : public FixedInputVertexMixin<2, NonControlVertex, StoreI32ToAddressVertex> {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties(0);
    static constexpr auto INPUT_TYPES =
        detail::InputTypes<2>(ValueRepresentation::INT_PTR, ValueRepresentation::INT32);

    static constexpr size_t OBJECT_INDEX = 0;
    static constexpr size_t VALUE_INDEX = 1;

    explicit StoreI32ToAddressVertex(uint64_t bitfield, int32_t offset)
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

// Loads Int64 from raw pointer
class LoadI64FromAddressVertex : public FixedInputVertexMixin<1, ValueVertex, LoadI64FromAddressVertex> {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::Int64();
    static constexpr auto INPUT_TYPES = detail::InputTypes<1>(ValueRepresentation::INT_PTR);

    static constexpr size_t OBJECT_INDEX = 0;

    explicit LoadI64FromAddressVertex(uint64_t bitfield, int32_t offset)
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

// Stores Int64 to raw pointer. 2 : Two inputs
class StoreI64ToAddressVertex : public FixedInputVertexMixin<2, NonControlVertex, StoreI64ToAddressVertex> {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties(0);
    static constexpr auto INPUT_TYPES =
        detail::InputTypes<2>(ValueRepresentation::INT_PTR, ValueRepresentation::INT64);

    static constexpr size_t OBJECT_INDEX = 0;
    static constexpr size_t VALUE_INDEX = 1;

    explicit StoreI64ToAddressVertex(uint64_t bitfield, int32_t offset)
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

// Loads Float64 from raw pointer
class LoadF64FromAddressVertex : public FixedInputVertexMixin<1, ValueVertex, LoadF64FromAddressVertex> {
public:
    using OutputRegister = ArkSteedDoubleRegister;
    static constexpr VertexProperties PROPERTIES = VertexProperties::Float64();
    static constexpr auto INPUT_TYPES = detail::InputTypes<1>(ValueRepresentation::INT_PTR);

    static constexpr size_t OBJECT_INDEX = 0;

    explicit LoadF64FromAddressVertex(uint64_t bitfield, int32_t offset)
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

// Stores Float64 to raw pointer. 2 : Two inputs
class StoreF64ToAddressVertex : public FixedInputVertexMixin<2, NonControlVertex, StoreF64ToAddressVertex> {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties(0);
    static constexpr auto INPUT_TYPES =
        detail::InputTypes<2>(ValueRepresentation::INT_PTR, ValueRepresentation::FLOAT64);

    static constexpr size_t OBJECT_INDEX = 0;
    static constexpr size_t VALUE_INDEX = 1;

    explicit StoreF64ToAddressVertex(uint64_t bitfield, int32_t offset)
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

class LoadPrototypeFromObjectVertex : public FixedInputVertexMixin<1, ValueVertex, LoadPrototypeFromObjectVertex> {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::TaggedValue() | VertexProperties::CanReadProp();

    static constexpr auto INPUT_TYPES = detail::InputTypes<1>(ValueRepresentation::TAGGED);

    static constexpr size_t OBJECT_INDEX = 0;

    explicit LoadPrototypeFromObjectVertex(uint64_t bitfield) : FixedInputVertexMixin(bitfield) {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
};

class LoadHClassAddressVertex : public FixedInputVertexMixin<1, ValueVertex, LoadHClassAddressVertex> {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::IntPtr() | VertexProperties::CanReadProp();
    static constexpr auto INPUT_TYPES = detail::InputTypes<1>(ValueRepresentation::TAGGED);
    static constexpr size_t OBJECT_INDEX = 0;

    explicit LoadHClassAddressVertex(uint64_t bitfield) : FixedInputVertexMixin(bitfield) {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
};

class LoadPrototypeHolderByHClassVertex : public VertexMixin<ValueVertex, LoadPrototypeHolderByHClassVertex>,
                                          public EagerDeoptimizableMixin {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::TaggedValue() |
                                                   VertexProperties::CanReadProp() |
                                                   VertexProperties::EagerDeopt();

    static constexpr size_t RECEIVER_INDEX = 0;

    explicit LoadPrototypeHolderByHClassVertex(uint64_t bitfield,
                                               Chunk *chunk,
                                               JSHClass *holderHClass,
                                               std::vector<JSHClass *> expectedPrototypeHClasses,
                                               uint32_t holderDepth,
                                               uint32_t bytecodeOffset)
        : VertexMixin(bitfield),
          EagerDeoptimizableMixin(chunk, bytecodeOffset),
          holderHClass_(holderHClass),
          expectedPrototypeHClasses_(std::move(expectedPrototypeHClasses)),
          holderDepth_(holderDepth)
    {}

    JSHClass *GetHolderHClass() const
    {
        return holderHClass_;
    }

    uint32_t GetHolderDepth() const
    {
        return holderDepth_;
    }

    const std::vector<JSHClass *> &GetExpectedPrototypeHClasses() const
    {
        return expectedPrototypeHClasses_;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

    void VerifyInputs() const
    {
        ASSERT(holderHClass_ != nullptr);
        ASSERT(holderDepth_ != 0);
        ASSERT(!expectedPrototypeHClasses_.empty());
        ASSERT(expectedPrototypeHClasses_.size() == holderDepth_);
        ASSERT(expectedPrototypeHClasses_.back() == holderHClass_);
        ASSERT(GetInputCount() == 1);
        ASSERT(GetInput(RECEIVER_INDEX)->GetValueRepresentation() == ValueRepresentation::TAGGED);
    }

private:
    JSHClass *holderHClass_ {nullptr};
    std::vector<JSHClass *> expectedPrototypeHClasses_;
    uint32_t holderDepth_ {0};
};

class ConvertHoleToUndefinedVertex : public FixedInputVertexMixin<1, ValueVertex, ConvertHoleToUndefinedVertex> {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::TaggedValue();

    static constexpr auto INPUT_TYPES = detail::InputTypes<1>(ValueRepresentation::TAGGED);

    static constexpr size_t VALUE_INDEX = 0;

    explicit ConvertHoleToUndefinedVertex(uint64_t bitfield) : FixedInputVertexMixin(bitfield) {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
};

class FindPrototypeHolderVertex : public VertexMixin<ValueVertex, FindPrototypeHolderVertex>,
                                  public EagerDeoptimizableMixin {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::TaggedValue() |
        VertexProperties::EagerDeopt() | VertexProperties::CanReadProp();
    static constexpr int RECEIVER_INDEX = 0;
    FindPrototypeHolderVertex(uint64_t bitfield, Chunk *chunk, JSHClass *expectedHolderHClass,
                              uint32_t bytecodeOffset)
        : VertexMixin(bitfield),
          EagerDeoptimizableMixin(chunk, bytecodeOffset),
          expectedHolderHClass_(expectedHolderHClass)
    {}

    JSHClass *GetExpectedHolderHClass() const
    {
        return expectedHolderHClass_;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

    void VerifyInputs() const
    {
        ASSERT(expectedHolderHClass_ != nullptr);
        ASSERT(GetInputCount() == RECEIVER_INDEX + 1);
        ASSERT(GetInput(RECEIVER_INDEX)->GetValueRepresentation() == ValueRepresentation::TAGGED);
    }

private:
    JSHClass *expectedHolderHClass_ {nullptr};
};

enum class ArkSteedWriteBarrierKind : uint8_t {
    NO_BARRIER,
    GENERIC_BARRIER,
    SHARED_BARRIER,
};

enum class ArkSteedWriteBarrierValueKind : uint8_t {
    Unknown,
    NonHeap,
    HeapObject,
};

// 2: object and value inputs
class StoreTaggedFieldVertex : public FixedInputVertexMixin<2, NonControlVertex, StoreTaggedFieldVertex> {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::CanWriteProp();

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

class StoreTaggedFieldWithBarrierVertex
    : public FixedInputVertexMixin<3, NonControlVertex, StoreTaggedFieldWithBarrierVertex> {
public:
    static constexpr int GLUE_INDEX = 0;
    static constexpr int OBJECT_INDEX = 1;
    static constexpr int VALUE_INDEX = 2;
    static constexpr auto INPUT_TYPES = detail::InputTypes<3>(ValueRepresentation::INT_PTR,
                                                              ValueRepresentation::TAGGED,
                                                              ValueRepresentation::TAGGED);
    static constexpr VertexProperties PROPERTIES = VertexProperties::CanWriteProp() |
        VertexProperties::DeferredCall();

    explicit StoreTaggedFieldWithBarrierVertex(uint64_t bitfield, int32_t offset,
                                               ArkSteedWriteBarrierValueKind valueKind =
                                                   ArkSteedWriteBarrierValueKind::Unknown)
        : FixedInputVertexMixin(bitfield), offset_(offset), valueKind_(valueKind)
    {
    }

    int32_t GetOffset() const
    {
        return offset_;
    }

    ArkSteedWriteBarrierValueKind GetValueKind() const
    {
        return valueKind_;
    }

    void SetValueKind(ArkSteedWriteBarrierValueKind valueKind)
    {
        valueKind_ = valueKind;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

private:
    int32_t offset_;
    ArkSteedWriteBarrierValueKind valueKind_ {ArkSteedWriteBarrierValueKind::Unknown};
};

class StoreSharedFieldWithBarrierVertex
    : public FixedInputVertexMixin<3, NonControlVertex, StoreSharedFieldWithBarrierVertex> {
public:
    static constexpr int GLUE_INDEX = 0;
    static constexpr int OBJECT_INDEX = 1;
    static constexpr int VALUE_INDEX = 2;
    static constexpr auto INPUT_TYPES = detail::InputTypes<3>(ValueRepresentation::INT_PTR,
                                                              ValueRepresentation::TAGGED,
                                                              ValueRepresentation::TAGGED);
    static constexpr VertexProperties PROPERTIES = VertexProperties::CanWriteProp() | VertexProperties::DeferredCall();

    explicit StoreSharedFieldWithBarrierVertex(uint64_t bitfield, int32_t offset,
                                               ArkSteedWriteBarrierValueKind valueKind =
                                                   ArkSteedWriteBarrierValueKind::Unknown)
        : FixedInputVertexMixin(bitfield), offset_(offset), valueKind_(valueKind)
    {
    }

    int32_t GetOffset() const
    {
        return offset_;
    }

    ArkSteedWriteBarrierValueKind GetValueKind() const
    {
        return valueKind_;
    }

    void SetValueKind(ArkSteedWriteBarrierValueKind valueKind)
    {
        valueKind_ = valueKind;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

private:
    int32_t offset_;
    ArkSteedWriteBarrierValueKind valueKind_ {ArkSteedWriteBarrierValueKind::Unknown};
};

class TransitionHClassWithBarrierVertex
    : public FixedInputVertexMixin<3, NonControlVertex, TransitionHClassWithBarrierVertex> {
public:
    static constexpr int GLUE_INDEX = 0;
    static constexpr int OBJECT_INDEX = 1;
    static constexpr int HCLASS_INDEX = 2;
    static constexpr auto INPUT_TYPES = detail::InputTypes<3>(ValueRepresentation::INT_PTR,
                                                              ValueRepresentation::TAGGED,
                                                              ValueRepresentation::TAGGED);
    static constexpr VertexProperties PROPERTIES = VertexProperties::CanWriteProp() |
        VertexProperties::DeferredCall();

    explicit TransitionHClassWithBarrierVertex(uint64_t bitfield) : FixedInputVertexMixin(bitfield) {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
};

class PrepareSharedStoreFieldVertex
    : public VertexMixin<ValueVertex, PrepareSharedStoreFieldVertex>,
      public ThrowableMixin,
      public LazyDeoptimizableMixin {
public:
    static constexpr int VALUE_INDEX = 0;
    static constexpr VertexProperties PROPERTIES = VertexProperties::TaggedValue() | VertexProperties::JsCall();

    PrepareSharedStoreFieldVertex(uint64_t bitfield, uint64_t handlerInfo)
        : VertexMixin(bitfield), handlerInfo_(handlerInfo)
    {
    }

    size_t GetArgCount() const
    {
        return GetInputCount();
    }

    uint64_t GetHandlerInfo() const
    {
        return handlerInfo_;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

    void VerifyInputs() const
    {
        ASSERT(GetInputCount() == VALUE_INDEX + 1);
        ASSERT(GetInput(VALUE_INDEX)->GetValueRepresentation() == ValueRepresentation::TAGGED);
    }

private:
    uint64_t handlerInfo_;
};

class EnsurePropertiesCapacityVertex : public VertexMixin<ValueVertex, EnsurePropertiesCapacityVertex>,
                                       public ThrowableMixin,
                                       public LazyDeoptimizableMixin {
public:
    static constexpr int GLUE_INDEX = 0;
    static constexpr int OBJECT_INDEX = 1;
    static constexpr VertexProperties PROPERTIES = VertexProperties::TaggedValue() | VertexProperties::CanWriteProp() |
        VertexProperties::JsCall();

    EnsurePropertiesCapacityVertex(uint64_t bitfield, int32_t fieldIndex)
        : VertexMixin(bitfield), fieldIndex_(fieldIndex)
    {
    }

    int32_t GetFieldIndex() const
    {
        return fieldIndex_;
    }

    size_t GetArgCount() const
    {
        return GetInputCount();
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

    void VerifyInputs() const
    {
        ASSERT(GetInputCount() == OBJECT_INDEX + 1);
        ASSERT(GetInput(GLUE_INDEX)->GetValueRepresentation() == ValueRepresentation::INT_PTR);
        ASSERT(GetInput(OBJECT_INDEX)->GetValueRepresentation() == ValueRepresentation::TAGGED);
    }

private:
    int32_t fieldIndex_ {0};
};

class StoreInt32FieldVertex : public FixedInputVertexMixin<2, NonControlVertex, StoreInt32FieldVertex> {
public:
    static constexpr int STORE_TARGET_INDEX = 0;
    static constexpr int VALUE_INDEX = 1;
    static constexpr auto INPUT_TYPES = detail::InputTypes<2>(ValueRepresentation::TAGGED,
                                                              ValueRepresentation::INT32);
    static constexpr VertexProperties PROPERTIES = VertexProperties::CanWriteProp();

    explicit StoreInt32FieldVertex(uint64_t bitfield, int32_t offset)
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
    int32_t offset_ {0};
};

class StoreDoubleFieldVertex : public FixedInputVertexMixin<2, NonControlVertex, StoreDoubleFieldVertex> {
public:
    static constexpr int STORE_TARGET_INDEX = 0;
    static constexpr int VALUE_INDEX = 1;
    static constexpr auto INPUT_TYPES = detail::InputTypes<2>(ValueRepresentation::TAGGED,
                                                              ValueRepresentation::FLOAT64);
    static constexpr VertexProperties PROPERTIES = VertexProperties::CanWriteProp();

    explicit StoreDoubleFieldVertex(uint64_t bitfield, int32_t offset)
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
    int32_t offset_ {0};
};

class StoreInt32FieldWithRepVertex
    : public VertexMixin<NonControlVertex, StoreInt32FieldWithRepVertex>, public EagerDeoptimizableMixin {
public:
    static constexpr uint32_t STORE_TARGET_INDEX = 0;
    static constexpr uint32_t VALUE_INDEX = 1;
    static constexpr VertexProperties PROPERTIES = VertexProperties::CanWriteProp() | VertexProperties::EagerDeopt();

    StoreInt32FieldWithRepVertex(uint64_t bitfield, Chunk *chunk, int32_t offset, uint32_t bytecodeOffset)
        : VertexMixin(bitfield),
          EagerDeoptimizableMixin(chunk, bytecodeOffset),
          offset_(offset)
    {
    }

    int32_t GetOffset() const
    {
        return offset_;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

    void VerifyInputs() const
    {
        ASSERT(GetInputCount() == VALUE_INDEX + 1);
        ASSERT(GetInput(STORE_TARGET_INDEX)->GetValueRepresentation() == ValueRepresentation::TAGGED);
        ASSERT(GetInput(VALUE_INDEX)->GetValueRepresentation() == ValueRepresentation::TAGGED);
    }

private:
    int32_t offset_ {0};
};

class StoreDoubleFieldWithRepVertex
    : public VertexMixin<NonControlVertex, StoreDoubleFieldWithRepVertex>, public EagerDeoptimizableMixin {
public:
    static constexpr uint32_t STORE_TARGET_INDEX = 0;
    static constexpr uint32_t VALUE_INDEX = 1;
    static constexpr VertexProperties PROPERTIES = VertexProperties::CanWriteProp() | VertexProperties::EagerDeopt();

    StoreDoubleFieldWithRepVertex(uint64_t bitfield, Chunk *chunk, int32_t offset, uint32_t bytecodeOffset)
        : VertexMixin(bitfield),
          EagerDeoptimizableMixin(chunk, bytecodeOffset),
          offset_(offset)
    {
    }

    int32_t GetOffset() const
    {
        return offset_;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

    void VerifyInputs() const
    {
        ASSERT(GetInputCount() == VALUE_INDEX + 1);
        ASSERT(GetInput(STORE_TARGET_INDEX)->GetValueRepresentation() == ValueRepresentation::TAGGED);
        ASSERT(GetInput(VALUE_INDEX)->GetValueRepresentation() == ValueRepresentation::TAGGED);
    }

private:
    int32_t offset_ {0};
};

struct StoreTaggedFieldByHClassCase {
    JSHClass *expectedHClass {nullptr};
    int32_t fieldOffset {0};
    bool propertiesArray {false};
};

class StoreTaggedFieldByHClassVertex
    : public VertexMixin<NonControlVertex, StoreTaggedFieldByHClassVertex>, public EagerDeoptimizableMixin {
public:
    static constexpr int GLUE_INDEX = 0;
    static constexpr int OBJECT_INDEX = 1;
    static constexpr int VALUE_INDEX = 2;
    static constexpr VertexProperties PROPERTIES = VertexProperties::CanWriteProp() |
        VertexProperties::CanReadProp() | VertexProperties::DeferredCall() | VertexProperties::EagerDeopt();

    StoreTaggedFieldByHClassVertex(uint64_t bitfield, Chunk *chunk,
                                   const std::vector<StoreTaggedFieldByHClassCase> &cases,
                                   ArkSteedWriteBarrierValueKind valueKind, uint32_t bytecodeOffset)
        : VertexMixin(bitfield),
          EagerDeoptimizableMixin(chunk, bytecodeOffset),
          cases_(chunk),
          valueKind_(valueKind)
    {
        cases_.assign(cases.begin(), cases.end());
    }

    const ChunkVector<StoreTaggedFieldByHClassCase> &GetCases() const
    {
        return cases_;
    }

    ArkSteedWriteBarrierValueKind GetValueKind() const
    {
        return valueKind_;
    }

    void SetValueKind(ArkSteedWriteBarrierValueKind valueKind)
    {
        valueKind_ = valueKind;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
    void VerifyInputs() const
    {
        ASSERT(!cases_.empty());
        ASSERT(GetInputCount() == VALUE_INDEX + 1);
        ASSERT(GetInput(GLUE_INDEX)->GetValueRepresentation() == ValueRepresentation::INT_PTR);
        ASSERT(GetInput(OBJECT_INDEX)->GetValueRepresentation() == ValueRepresentation::TAGGED);
        ASSERT(GetInput(VALUE_INDEX)->GetValueRepresentation() == ValueRepresentation::TAGGED);
    }

private:
    ChunkVector<StoreTaggedFieldByHClassCase> cases_;
    ArkSteedWriteBarrierValueKind valueKind_ {ArkSteedWriteBarrierValueKind::Unknown};
};

class StoreEnvSlotVertex : public FixedInputVertexMixin<2, NonControlVertex, StoreEnvSlotVertex> {
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
class CallRuntimeVertex : public VertexMixin<ValueVertex, CallRuntimeVertex>,
                          public ThrowableMixin,
                          public LazyDeoptimizableMixin,
                          public RuntimeStubIDMixin {
public:
    static constexpr VertexProperties PROPERTIES =
        VertexProperties::Call() | VertexProperties::AnySideEffects() | VertexProperties::LazyDeopt();

    CallRuntimeVertex(uint64_t bitfield, RuntimeStubID id, SideEffectKind sideEffectKind = SideEffectKind::UNKNOWN_CALL)
        : VertexMixin(bitfield),
          RuntimeStubIDMixin(id),
          sideEffectKind_(sideEffectKind)
    {
        ASSERT(sideEffectKind_ == SideEffectKind::UNKNOWN_CALL || sideEffectKind_ == SideEffectKind::SAFE_CALL);
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

    void VerifyInputs() const
    {
        ASSERT(GetArgCount() == GetInputCount());
    }

private:
    SideEffectKind sideEffectKind_;
};

class CallVertex : public VertexMixin<ValueVertex, CallVertex>,
                   public ThrowableMixin,
                   public LazyDeoptimizableMixin {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::JsCall();

    static constexpr uint32_t TARGET_INDEX = 0;
    static constexpr uint32_t NEW_TARGET_INDEX = 1;
    static constexpr uint32_t THIS_INDEX = 2;
    static constexpr uint32_t FIRST_ARG_INDEX = 3;

    CallVertex(uint64_t bitfield, uint32_t actualArgc) : VertexMixin(bitfield), actualArgc_(actualArgc) {}

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
        ASSERT(GetInputCount() == FIRST_ARG_INDEX + actualArgc_);
    }

private:
    uint32_t actualArgc_;
};

/**
 * CallCommonStub vertex - for common stub calls
 */
class CallCommonStubVertex : public VertexMixin<ValueVertex, CallCommonStubVertex>,
                             public ThrowableMixin,
                             public LazyDeoptimizableMixin,
                             public CommonStubIDMixin {
public:
    static constexpr VertexProperties PROPERTIES =
        VertexProperties::Call() | VertexProperties::AnySideEffects() | VertexProperties::LazyDeopt();

    CallCommonStubVertex(uint64_t bitfield, CommonStubID stubId,
                         SideEffectKind sideEffectKind = SideEffectKind::UNKNOWN_CALL)
        : VertexMixin(bitfield),
          CommonStubIDMixin(stubId),
          sideEffectKind_(sideEffectKind)
    {
        ASSERT(sideEffectKind_ == SideEffectKind::UNKNOWN_CALL || sideEffectKind_ == SideEffectKind::SAFE_CALL);
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

    void VerifyInputs() const
    {
        ASSERT(GetInputCount() >= 1);
    }

private:
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
                                    public EagerDeoptimizableMixin {
public:
    static constexpr int INPUT_INDEX = 0;
    static constexpr int INPUT_COUNT = 1;
    static constexpr VertexProperties PROPERTIES =
        VertexProperties::Int32() | VertexProperties::NotIdempotent() | VertexProperties::EagerDeopt();
    explicit CheckedTaggedIntToI32Vertex(uint64_t bitfield, Chunk *chunk, uint32_t bytecodeOffset)
        : VertexMixin(bitfield), EagerDeoptimizableMixin(chunk, bytecodeOffset)
    {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
    void VerifyInputs() const
    {
        ASSERT(GetInputCount() == INPUT_COUNT);
        ASSERT(GetInput(INPUT_INDEX)->GetValueRepresentation() == ValueRepresentation::TAGGED);
    }
};

class CheckedTaggedStringVertex : public VertexMixin<ValueVertex, CheckedTaggedStringVertex>,
                                  public EagerDeoptimizableMixin {
public:
    static constexpr int INPUT_INDEX = 0;
    static constexpr int INPUT_COUNT = 1;
    static constexpr VertexProperties PROPERTIES =
        VertexProperties::TaggedValue() | VertexProperties::NotIdempotent() | VertexProperties::EagerDeopt();
    explicit CheckedTaggedStringVertex(uint64_t bitfield, Chunk *chunk, uint32_t bytecodeOffset)
        : VertexMixin(bitfield), EagerDeoptimizableMixin(chunk, bytecodeOffset)
    {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
    void VerifyInputs() const
    {
        ASSERT(GetInputCount() == INPUT_COUNT);
        ASSERT(GetInput(INPUT_INDEX)->GetValueRepresentation() == ValueRepresentation::TAGGED);
    }
};

class I32ConditionCheckVertex : public FixedInputVertexMixin<2, ValueVertex, I32ConditionCheckVertex> {
public:
    static constexpr int LEFT_INDEX = 0;
    static constexpr int RIGHT_INDEX = 1;
    static constexpr detail::InputTypes<2> INPUT_TYPES {ValueRepresentation::INT32, ValueRepresentation::INT32};
    static constexpr VertexProperties PROPERTIES = VertexProperties::TaggedValue();

    explicit I32ConditionCheckVertex(uint64_t bitfield, IntConditionKind condition)
        : FixedInputVertexMixin(bitfield), condition_(condition)
    {}

    IntConditionKind GetCondition() const
    {
        return condition_;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

private:
    IntConditionKind condition_;
};

class F64ConditionCheckVertex : public FixedInputVertexMixin<2, ValueVertex, F64ConditionCheckVertex> {
public:
    static constexpr int LEFT_INDEX = 0;
    static constexpr int RIGHT_INDEX = 1;
    static constexpr detail::InputTypes<2> INPUT_TYPES {ValueRepresentation::FLOAT64, ValueRepresentation::FLOAT64};
    static constexpr VertexProperties PROPERTIES = VertexProperties::TaggedValue();

    explicit F64ConditionCheckVertex(uint64_t bitfield, IntConditionKind condition)
        : FixedInputVertexMixin(bitfield), condition_(condition)
    {}

    IntConditionKind GetCondition() const
    {
        return condition_;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

private:
    IntConditionKind condition_;
};

class TaggedEqualVertex : public FixedInputVertexMixin<2, ValueVertex, TaggedEqualVertex> {
public:
    static constexpr int LEFT_INDEX = 0;
    static constexpr int RIGHT_INDEX = 1;
    static constexpr detail::InputTypes<2> INPUT_TYPES {ValueRepresentation::TAGGED, ValueRepresentation::TAGGED};
    static constexpr VertexProperties PROPERTIES = VertexProperties::TaggedValue();

    explicit TaggedEqualVertex(uint64_t bitfield) : FixedInputVertexMixin(bitfield) {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
};

class TaggedNotEqualVertex : public FixedInputVertexMixin<2, ValueVertex, TaggedNotEqualVertex> {
public:
    static constexpr int LEFT_INDEX = 0;
    static constexpr int RIGHT_INDEX = 1;
    static constexpr detail::InputTypes<2> INPUT_TYPES {ValueRepresentation::TAGGED, ValueRepresentation::TAGGED};
    static constexpr VertexProperties PROPERTIES = VertexProperties::TaggedValue();

    explicit TaggedNotEqualVertex(uint64_t bitfield) : FixedInputVertexMixin(bitfield) {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
};

class StringEqualVertex : public FixedInputVertexMixin<4, ValueVertex, StringEqualVertex> {
public:
    static constexpr int GLUE_INDEX = 0;
    static constexpr int LEFT_INDEX = 1;
    static constexpr int RIGHT_INDEX = 2;
    static constexpr int GLOBAL_ENV_INDEX = 3;
    static constexpr detail::InputTypes<4> INPUT_TYPES {ValueRepresentation::INT_PTR, ValueRepresentation::TAGGED,
                                                        ValueRepresentation::TAGGED, ValueRepresentation::TAGGED};
    static constexpr VertexProperties PROPERTIES =
        VertexProperties::TaggedValue() | VertexProperties::Call() | VertexProperties::AnySideEffects();

    explicit StringEqualVertex(uint64_t bitfield) : FixedInputVertexMixin(bitfield) {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
};

class I32AddWithOverflowVertex : public VertexMixin<ValueVertex, I32AddWithOverflowVertex>,
                                 public EagerDeoptimizableMixin {
public:
    static constexpr int LEFT_INDEX = 0;
    static constexpr int RIGHT_INDEX = 1;
    static constexpr int INPUT_COUNT = 2;
    static constexpr VertexProperties PROPERTIES =
        VertexProperties::Int32() | VertexProperties::NotIdempotent() | VertexProperties::EagerDeopt();
    explicit I32AddWithOverflowVertex(uint64_t bitfield, Chunk *chunk, uint32_t bytecodeOffset)
        : VertexMixin(bitfield), EagerDeoptimizableMixin(chunk, bytecodeOffset)
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
                                 public EagerDeoptimizableMixin {
public:
    static constexpr int LEFT_INDEX = 0;
    static constexpr int RIGHT_INDEX = 1;
    static constexpr int INPUT_COUNT = 2;
    static constexpr VertexProperties PROPERTIES =
        VertexProperties::Int32() | VertexProperties::NotIdempotent() | VertexProperties::EagerDeopt();
    explicit I32SubWithOverflowVertex(uint64_t bitfield, Chunk *chunk, uint32_t bytecodeOffset)
        : VertexMixin(bitfield), EagerDeoptimizableMixin(chunk, bytecodeOffset)
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
                                 public EagerDeoptimizableMixin {
public:
    static constexpr int LEFT_INDEX = 0;
    static constexpr int RIGHT_INDEX = 1;
    static constexpr int INPUT_COUNT = 2;
    static constexpr VertexProperties PROPERTIES =
        VertexProperties::Int32() | VertexProperties::NotIdempotent() | VertexProperties::EagerDeopt();
    explicit I32MulWithOverflowVertex(uint64_t bitfield, Chunk *chunk, uint32_t bytecodeOffset)
        : VertexMixin(bitfield), EagerDeoptimizableMixin(chunk, bytecodeOffset)
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
                                 public EagerDeoptimizableMixin {
public:
    static constexpr int LEFT_INDEX = 0;
    static constexpr int RIGHT_INDEX = 1;
    static constexpr int INPUT_COUNT = 2;
    static constexpr VertexProperties PROPERTIES =
        VertexProperties::Int32() | VertexProperties::NotIdempotent() | VertexProperties::EagerDeopt();
    explicit I32DivWithOverflowVertex(uint64_t bitfield, Chunk *chunk, uint32_t bytecodeOffset)
        : VertexMixin(bitfield), EagerDeoptimizableMixin(chunk, bytecodeOffset)
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
                                     public EagerDeoptimizableMixin {
public:
    static constexpr int INPUT_INDEX = 0;
    static constexpr int INPUT_COUNT = 1;
    static constexpr VertexProperties PROPERTIES =
        VertexProperties::Int32() | VertexProperties::NotIdempotent() | VertexProperties::EagerDeopt();
    explicit I32DivByConstWithCheckVertex(uint64_t bitfield, Chunk *chunk, uint32_t bytecodeOffset,
                                          int32_t divisor, int32_t magic, uint32_t shift)
        : VertexMixin(bitfield),
          EagerDeoptimizableMixin(chunk, bytecodeOffset),
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
        ASSERT(GetInputCount() == INPUT_COUNT);
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

class CheckedI32ModVertex : public VertexMixin<ValueVertex, CheckedI32ModVertex>,
                            public EagerDeoptimizableMixin {
public:
    static constexpr int LEFT_INDEX = 0;
    static constexpr int RIGHT_INDEX = 1;
    static constexpr int INPUT_COUNT = 2;
    static constexpr VertexProperties PROPERTIES =
        VertexProperties::Int32() | VertexProperties::NotIdempotent() | VertexProperties::EagerDeopt();
    explicit CheckedI32ModVertex(uint64_t bitfield, Chunk *chunk, uint32_t bytecodeOffset)
        : VertexMixin(bitfield), EagerDeoptimizableMixin(chunk, bytecodeOffset)
    {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
    void VerifyInputs() const
    {
        ASSERT(GetInputCount() == INPUT_COUNT);
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

    explicit I32BitwiseBinaryVertex(uint64_t bitfield, IntBitwiseKind kind)
        : FixedInputVertexMixin(bitfield), kind_(kind)
    {}

    IntBitwiseKind GetKind() const
    {
        return kind_;
    }

    bool RightInputIsConstant() const
    {
        return Arg(RIGHT_INDEX).vertex()->TryCast<Int32ConstantVertex>() != nullptr;
    }

    bool IsShift() const
    {
        return kind_ == IntBitwiseKind::SHIFT_LEFT ||
               kind_ == IntBitwiseKind::SHIFT_RIGHT_LOGICAL ||
               kind_ == IntBitwiseKind::SHIFT_RIGHT_ARITHMETIC;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

private:
    IntBitwiseKind kind_;
};

class CheckedNonNegativeI32ToTaggedIntVertex
    : public VertexMixin<ValueVertex, CheckedNonNegativeI32ToTaggedIntVertex>,
      public EagerDeoptimizableMixin {
public:
    static constexpr int INPUT_INDEX = 0;
    static constexpr int INPUT_COUNT = 1;
    static constexpr VertexProperties PROPERTIES =
        VertexProperties::TaggedValue() | VertexProperties::NotIdempotent() | VertexProperties::EagerDeopt();
    explicit CheckedNonNegativeI32ToTaggedIntVertex(uint64_t bitfield, Chunk *chunk, uint32_t bytecodeOffset)
        : VertexMixin(bitfield), EagerDeoptimizableMixin(chunk, bytecodeOffset)
    {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
    void VerifyInputs() const
    {
        ASSERT(GetInputCount() == INPUT_COUNT);
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
                                 public EagerDeoptimizableMixin {
public:
    static constexpr int VALUE_INDEX = 0;
    static constexpr int INPUT_COUNT = 1;
    static constexpr VertexProperties PROPERTIES =
        VertexProperties::Int32() | VertexProperties::NotIdempotent() | VertexProperties::EagerDeopt();
    explicit I32NegWithOverflowVertex(uint64_t bitfield, Chunk *chunk, uint32_t bytecodeOffset)
        : VertexMixin(bitfield), EagerDeoptimizableMixin(chunk, bytecodeOffset)
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
                                 public EagerDeoptimizableMixin {
public:
    static constexpr int VALUE_INDEX = 0;
    static constexpr int INPUT_COUNT = 1;
    static constexpr VertexProperties PROPERTIES =
        VertexProperties::Int32() | VertexProperties::NotIdempotent() | VertexProperties::EagerDeopt();
    explicit I32IncWithOverflowVertex(uint64_t bitfield, Chunk *chunk, uint32_t bytecodeOffset)
        : VertexMixin(bitfield), EagerDeoptimizableMixin(chunk, bytecodeOffset)
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
                                 public EagerDeoptimizableMixin {
public:
    static constexpr int VALUE_INDEX = 0;
    static constexpr int INPUT_COUNT = 1;
    static constexpr VertexProperties PROPERTIES =
        VertexProperties::Int32() | VertexProperties::NotIdempotent() | VertexProperties::EagerDeopt();
    explicit I32DecWithOverflowVertex(uint64_t bitfield, Chunk *chunk, uint32_t bytecodeOffset)
        : VertexMixin(bitfield), EagerDeoptimizableMixin(chunk, bytecodeOffset)
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
                                public EagerDeoptimizableMixin {
public:
    static constexpr int INPUT_INDEX = 0;
    static constexpr int INPUT_COUNT = 1;
    static constexpr VertexProperties PROPERTIES =
        VertexProperties::Float64() | VertexProperties::NotIdempotent() | VertexProperties::EagerDeopt();
    explicit CheckedNumberToF64Vertex(uint64_t bitfield, Chunk *chunk, uint32_t bytecodeOffset)
        : VertexMixin(bitfield), EagerDeoptimizableMixin(chunk, bytecodeOffset)
    {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
    void VerifyInputs() const
    {
        ASSERT(GetInputCount() == INPUT_COUNT);
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
                                    public EagerDeoptimizableMixin {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::EagerDeopt() | VertexProperties::CanReadProp();
    static constexpr size_t RECEIVER_INDEX = 0;
    static constexpr size_t INPUT_COUNT = 1;

    explicit DeoptIfHClassMismatchVertex(uint64_t bitfield,
                                         Chunk *chunk,
                                         JSHClass *expectedHClass,
                                         uint32_t bytecodeOffset)
        : VertexMixin(bitfield),
          EagerDeoptimizableMixin(chunk, bytecodeOffset),
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
        ASSERT(GetInputCount() == INPUT_COUNT);
    }

private:
    JSHClass *expectedHClass_;
};

class DeoptIfHClassNotInVertex : public VertexMixin<NonControlVertex, DeoptIfHClassNotInVertex>,
                                 public EagerDeoptimizableMixin {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::EagerDeopt() | VertexProperties::CanReadProp();

    static constexpr size_t RECEIVER_INDEX = 0;

    explicit DeoptIfHClassNotInVertex(uint64_t bitfield,
                                      Chunk *chunk,
                                      std::vector<JSHClass *> expectedHClasses,
                                      uint32_t bytecodeOffset)
        : VertexMixin(bitfield),
          EagerDeoptimizableMixin(chunk, bytecodeOffset),
          expectedHClasses_(std::move(expectedHClasses))
    {}

    const std::vector<JSHClass *> &GetExpectedHClasses() const
    {
        return expectedHClasses_;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

    void VerifyInputs() const
    {
        ASSERT(!expectedHClasses_.empty());
        ASSERT(std::all_of(expectedHClasses_.begin(), expectedHClasses_.end(), [](JSHClass *hclass) {
            return hclass != nullptr;
        }));
        ASSERT(GetInputCount() == 1);
    }

private:
    std::vector<JSHClass *> expectedHClasses_;
};

class DeoptIfPrototypeChangedVertex : public VertexMixin<NonControlVertex, DeoptIfPrototypeChangedVertex>,
                                      public EagerDeoptimizableMixin {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::EagerDeopt() | VertexProperties::CanReadProp();
    static constexpr size_t RECEIVER_INDEX = 0;
    DeoptIfPrototypeChangedVertex(uint64_t bitfield, Chunk *chunk, bool checkProtoChangeMarker,
                                  bool checkNotPrototype, uint32_t bytecodeOffset)
        : VertexMixin(bitfield),
          EagerDeoptimizableMixin(chunk, bytecodeOffset),
          checkProtoChangeMarker_(checkProtoChangeMarker),
          checkNotPrototype_(checkNotPrototype)
    {}

    bool ShouldCheckProtoChangeMarker() const
    {
        return checkProtoChangeMarker_;
    }

    bool ShouldCheckNotPrototype() const
    {
        return checkNotPrototype_;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

    void VerifyInputs() const
    {
        ASSERT(checkProtoChangeMarker_ || checkNotPrototype_);
        ASSERT(GetInputCount() == RECEIVER_INDEX + 1);
    }

private:
    bool checkProtoChangeMarker_ {false};
    bool checkNotPrototype_ {false};
};

class DeoptIfInt32ConditionVertex : public VertexMixin<NonControlVertex, DeoptIfInt32ConditionVertex>,
                                    public EagerDeoptimizableMixin {
public:
    static constexpr int LEFT_INDEX = 0;
    static constexpr int RIGHT_INDEX = 1;
    static constexpr int INPUT_COUNT = 2;
    static constexpr VertexProperties PROPERTIES = VertexProperties::EagerDeopt();
    explicit DeoptIfInt32ConditionVertex(uint64_t bitfield, Chunk *chunk, uint32_t bytecodeOffset,
                                         IntConditionKind condition, kungfu::DeoptType deoptType)
        : VertexMixin(bitfield),
          EagerDeoptimizableMixin(chunk, bytecodeOffset),
          condition_(condition),
          deoptType_(deoptType)
    {}

    IntConditionKind GetCondition() const
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
        ASSERT(GetInputCount() == INPUT_COUNT);
        ASSERT(GetInput(LEFT_INDEX)->GetValueRepresentation() == ValueRepresentation::INT32);
        ASSERT(GetInput(RIGHT_INDEX)->GetValueRepresentation() == ValueRepresentation::INT32);
    }

private:
    IntConditionKind condition_;
    kungfu::DeoptType deoptType_;
};

class DeoptIfNotNumberVertex : public VertexMixin<NonControlVertex, DeoptIfNotNumberVertex>,
                               public EagerDeoptimizableMixin {
public:
    static constexpr int VALUE_INDEX = 0;
    static constexpr int INPUT_COUNT = 1;
    static constexpr VertexProperties PROPERTIES = VertexProperties::EagerDeopt();
    explicit DeoptIfNotNumberVertex(uint64_t bitfield, Chunk *chunk, uint32_t bytecodeOffset)
        : VertexMixin(bitfield),
          EagerDeoptimizableMixin(chunk, bytecodeOffset)
    {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

    void VerifyInputs() const
    {
        ASSERT(GetInputCount() == INPUT_COUNT);
        ASSERT(GetInput(VALUE_INDEX)->GetValueRepresentation() == ValueRepresentation::TAGGED);
    }
};

class DeoptVertex : public FixedInputVertexMixin<0, ControlVertex, DeoptVertex>, public EagerDeoptimizableMixin {
public:
    static constexpr VertexProperties PROPERTIES = VertexProperties::EagerDeopt();
    explicit DeoptVertex(uint64_t bitfield,
                         Chunk *chunk,
                         kungfu::DeoptType type,
                         uint32_t bytecodeOffset)
        : FixedInputVertexMixin(bitfield),
          EagerDeoptimizableMixin(chunk, bytecodeOffset),
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
        ASSERT(GetInputCount() == 0);
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

class BranchIfTaggedStringVertex : public BranchControlVertexT<1, BranchIfTaggedStringVertex> {
public:
    static constexpr int VALUE_INDEX = 0;
    static constexpr detail::InputTypes<1> INPUT_TYPES {ValueRepresentation::TAGGED};
    static constexpr VertexProperties PROPERTIES = VertexProperties::Pure();

    BranchIfTaggedStringVertex(uint64_t bitfield, BB *ifTrue, BB *ifFalse)
        : BranchControlVertexT(bitfield, ifTrue, ifFalse)
    {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
};

class BranchIfHClassInVertex : public BranchControlVertexT<1, BranchIfHClassInVertex> {
public:
    static constexpr int RECEIVER_INDEX = 0;
    static constexpr detail::InputTypes<1> INPUT_TYPES {ValueRepresentation::TAGGED};
    static constexpr VertexProperties PROPERTIES = VertexProperties::CanReadProp();

    BranchIfHClassInVertex(uint64_t bitfield, BB *ifTrue, BB *ifFalse,
                           std::vector<JSHClass *> expectedHClasses)
        : BranchControlVertexT(bitfield, ifTrue, ifFalse), expectedHClasses_(std::move(expectedHClasses))
    {}

    const std::vector<JSHClass *> &GetExpectedHClasses() const
    {
        return expectedHClasses_;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

    void VerifyInputs() const
    {
        ASSERT(!expectedHClasses_.empty());
        ASSERT(std::all_of(expectedHClasses_.begin(), expectedHClasses_.end(), [](JSHClass *hclass) {
            return hclass != nullptr;
        }));
    }

private:
    std::vector<JSHClass *> expectedHClasses_;
};

class BranchIfInt32CompareVertex : public BranchControlVertexT<2, BranchIfInt32CompareVertex> {
public:
    static constexpr int LEFT_INDEX = 0;
    static constexpr int RIGHT_INDEX = 1;
    static constexpr detail::InputTypes<2> INPUT_TYPES {ValueRepresentation::INT32, ValueRepresentation::INT32};
    static constexpr VertexProperties PROPERTIES = VertexProperties::Pure();

    BranchIfInt32CompareVertex(uint64_t bitfield, BB *ifTrue, BB *ifFalse, IntConditionKind condition)
        : BranchControlVertexT(bitfield, ifTrue, ifFalse), condition_(condition)
    {}

    IntConditionKind GetCondition() const
    {
        return condition_;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

private:
    IntConditionKind condition_;
};

class BranchIfInt64CompareVertex : public BranchControlVertexT<2, BranchIfInt64CompareVertex> {
public:
    static constexpr int LEFT_INDEX = 0;
    static constexpr int RIGHT_INDEX = 1;
    static constexpr detail::InputTypes<2> INPUT_TYPES {ValueRepresentation::INT64, ValueRepresentation::INT64};
    static constexpr VertexProperties PROPERTIES = VertexProperties::Pure();

    BranchIfInt64CompareVertex(uint64_t bitfield, BB *ifTrue, BB *ifFalse, IntConditionKind condition)
        : BranchControlVertexT(bitfield, ifTrue, ifFalse), condition_(condition)
    {}

    IntConditionKind GetCondition() const
    {
        return condition_;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

private:
    IntConditionKind condition_;
};

class BranchIfFloat64CompareVertex : public BranchControlVertexT<2, BranchIfFloat64CompareVertex> {
public:
    static constexpr int LEFT_INDEX = 0;
    static constexpr int RIGHT_INDEX = 1;
    static constexpr detail::InputTypes<2> INPUT_TYPES {ValueRepresentation::FLOAT64, ValueRepresentation::FLOAT64};
    static constexpr VertexProperties PROPERTIES = VertexProperties::Pure();

    BranchIfFloat64CompareVertex(uint64_t bitfield, BB *ifTrue, BB *ifFalse, IntConditionKind condition)
        : BranchControlVertexT(bitfield, ifTrue, ifFalse), condition_(condition)
    {}

    IntConditionKind GetCondition() const
    {
        return condition_;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

private:
    IntConditionKind condition_;
};

class BranchIfReferenceEqualVertex : public BranchControlVertexT<2, BranchIfReferenceEqualVertex> {
public:
    static constexpr int LEFT_INDEX = 0;
    static constexpr int RIGHT_INDEX = 1;
    static constexpr detail::InputTypes<2> INPUT_TYPES {ValueRepresentation::TAGGED, ValueRepresentation::TAGGED};
    static constexpr VertexProperties PROPERTIES = VertexProperties::Pure();

    BranchIfReferenceEqualVertex(uint64_t bitfield, BB *ifTrue, BB *ifFalse)
        : BranchControlVertexT(bitfield, ifTrue, ifFalse)
    {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
};

class BranchIfObjectTypeVertex : public BranchControlVertexT<1, BranchIfObjectTypeVertex> {
public:
    static constexpr int VALUE_INDEX = 0;
    static constexpr detail::InputTypes<1> INPUT_TYPES {ValueRepresentation::TAGGED};
    static constexpr VertexProperties PROPERTIES = VertexProperties::Pure();

    BranchIfObjectTypeVertex(uint64_t bitfield, BB *ifTrue, BB *ifFalse, JSType expectedType)
        : BranchControlVertexT(bitfield, ifTrue, ifFalse), expectedType_(expectedType)
    {}

    JSType GetExpectedType() const
    {
        return expectedType_;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

private:
    JSType expectedType_;
};

/**
 * BranchIfTaggedHeapObject vertex - branch based on whether a tagged value is a heap object.
 * Jumps to ifTrue if the value is a tagged heap object (tag bits == 0),
 * otherwise jumps to ifFalse.
 */
class BranchIfTaggedHeapObjectVertex : public BranchControlVertexT<1, BranchIfTaggedHeapObjectVertex> {
public:
    static constexpr int VALUE_INDEX = 0;
    static constexpr detail::InputTypes<1> INPUT_TYPES {ValueRepresentation::TAGGED};
    static constexpr VertexProperties PROPERTIES = VertexProperties::Pure();

    BranchIfTaggedHeapObjectVertex(uint64_t bitfield, BB *ifTrue, BB *ifFalse)
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

    explicit JumpLoopVertex(uint64_t bitfield, Chunk *chunk, BB *target)
        : UnconditionalControlVertexT(bitfield, target), usedVertices_(chunk) {}

    void SetValueLocationConstraints();

    using UsedVerticesType = ChunkVector<std::pair<ValueVertex *, InputLocation>>;

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
class ThrowVertex : public VertexMixin<ControlVertex, ThrowVertex>, public ThrowableMixin, public RuntimeStubIDMixin {
public:
    static constexpr VertexProperties PROPERTIES =
        VertexProperties::CanThrowProp() | VertexProperties::Call() | VertexProperties::NotIdempotent();

    explicit ThrowVertex(uint64_t bitfield, kungfu::RuntimeStubCSigns::ID id)
        : VertexMixin(bitfield), RuntimeStubIDMixin(id)
    {}

    size_t GetArgCount() const
    {
        return GetInputCount();
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

    // VerifyInputs: Variable-input vertex
    void VerifyInputs() const {}
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

    ValueVertex *GetPredecessor(int index)
    {
        return GetInput(index);
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

class I32ToTaggedIntVertex : public FixedInputVertexMixin<1, ValueVertex, I32ToTaggedIntVertex> {
public:
    static constexpr int INPUT_INDEX = 0;
    static constexpr VertexProperties PROPERTIES = VertexProperties::TaggedValue();

    explicit I32ToTaggedIntVertex(uint64_t bitfield) : FixedInputVertexMixin(bitfield) {}

    const ValueVertex *GetInputValue() const
    {
        return GetInput(INPUT_INDEX);
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
};

class RawI64ToTaggedVertex : public FixedInputVertexMixin<1, ValueVertex, RawI64ToTaggedVertex> {
public:
    static constexpr int INPUT_INDEX = 0;
    static constexpr VertexProperties PROPERTIES = VertexProperties::TaggedValue();

    explicit RawI64ToTaggedVertex(uint64_t bitfield) : FixedInputVertexMixin(bitfield) {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
};

class TaggedToRawI64Vertex : public FixedInputVertexMixin<1, ValueVertex, TaggedToRawI64Vertex> {
public:
    static constexpr int INPUT_INDEX = 0;
    static constexpr VertexProperties PROPERTIES = VertexProperties::Int64();

    explicit TaggedToRawI64Vertex(uint64_t bitfield) : FixedInputVertexMixin(bitfield) {}

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;
};

class I64BitwiseBinaryVertex : public FixedInputVertexMixin<2, ValueVertex, I64BitwiseBinaryVertex> {
public:
    static constexpr int LEFT_INDEX = 0;
    static constexpr int RIGHT_INDEX = 1;
    static constexpr detail::InputTypes<2> INPUT_TYPES {ValueRepresentation::INT64, ValueRepresentation::INT64};
    static constexpr VertexProperties PROPERTIES = VertexProperties::Int64();

    explicit I64BitwiseBinaryVertex(uint64_t bitfield, IntBitwiseKind kind)
        : FixedInputVertexMixin(bitfield), kind_(kind)
    {}

    IntBitwiseKind GetKind() const
    {
        return kind_;
    }

    void SetValueLocationConstraints();
    void Dump(std::ostream &output) const;

private:
    IntBitwiseKind kind_;
};

inline BB *CatchBlockOf(Vertex *vertex)
{
    if (auto *derived = vertex->TryCast<CallVertex>()) {
        return derived->GetCatchBlock();
    }
    if (auto *derived = vertex->TryCast<CallCommonStubVertex>()) {
        return derived->GetCatchBlock();
    }
    if (auto *derived = vertex->TryCast<CallRuntimeVertex>()) {
        return derived->GetCatchBlock();
    }
    if (auto *derived = vertex->TryCast<PrepareSharedStoreFieldVertex>()) {
        return derived->GetCatchBlock();
    }
    if (auto *derived = vertex->TryCast<EnsurePropertiesCapacityVertex>()) {
        return derived->GetCatchBlock();
    }
    if (auto *derived = vertex->TryCast<ThrowVertex>()) {
        return derived->GetCatchBlock();
    }
    return nullptr;
}

inline uint32_t CatchPredecessorIndexOf(Vertex *vertex)
{
    if (auto *derived = vertex->TryCast<CallVertex>()) {
        return derived->GetCatchPredecessorIndex();
    }
    if (auto *derived = vertex->TryCast<CallCommonStubVertex>()) {
        return derived->GetCatchPredecessorIndex();
    }
    if (auto *derived = vertex->TryCast<CallRuntimeVertex>()) {
        return derived->GetCatchPredecessorIndex();
    }
    if (auto *derived = vertex->TryCast<PrepareSharedStoreFieldVertex>()) {
        return derived->GetCatchPredecessorIndex();
    }
    if (auto *derived = vertex->TryCast<EnsurePropertiesCapacityVertex>()) {
        return derived->GetCatchPredecessorIndex();
    }
    if (auto *derived = vertex->TryCast<ThrowVertex>()) {
        return derived->GetCatchPredecessorIndex();
    }
    return static_cast<uint32_t>(-1);
}

inline const LazyDeoptimizableMixin *LazyDeoptMixinOf(const Vertex *vertex)
{
    if (auto *derived = vertex->TryCast<CallVertex>()) {
        return derived;
    }
    if (auto *derived = vertex->TryCast<CallCommonStubVertex>()) {
        return derived;
    }
    if (auto *derived = vertex->TryCast<CallRuntimeVertex>()) {
        return derived;
    }
    if (auto *derived = vertex->TryCast<PrepareSharedStoreFieldVertex>()) {
        return derived;
    }
    if (auto *derived = vertex->TryCast<EnsurePropertiesCapacityVertex>()) {
        return derived;
    }
    return nullptr;
}

inline LazyDeoptimizableMixin *LazyDeoptMixinOf(Vertex *vertex)
{
    return const_cast<LazyDeoptimizableMixin *>(LazyDeoptMixinOf(static_cast<const Vertex *>(vertex)));
}

inline ThrowableMixin *ThrowableMixinOf(Vertex *vertex)
{
    if (auto *derived = vertex->TryCast<CallVertex>()) {
        return derived;
    }
    if (auto *derived = vertex->TryCast<CallCommonStubVertex>()) {
        return derived;
    }
    if (auto *derived = vertex->TryCast<CallRuntimeVertex>()) {
        return derived;
    }
    if (auto *derived = vertex->TryCast<PrepareSharedStoreFieldVertex>()) {
        return derived;
    }
    if (auto *derived = vertex->TryCast<EnsurePropertiesCapacityVertex>()) {
        return derived;
    }
    return nullptr;
}

inline bool HasExceptionLazyDeoptMetadata(Vertex *vertex)
{
    auto hasExceptionLazyDeopt = [](auto *derived) {
        return !derived->HasCatchBlock() && derived->HasExceptionLazyDeoptMetadata();
    };
    if (auto *derived = vertex->TryCast<CallVertex>()) {
        return hasExceptionLazyDeopt(derived);
    }
    if (auto *derived = vertex->TryCast<CallCommonStubVertex>()) {
        return hasExceptionLazyDeopt(derived);
    }
    if (auto *derived = vertex->TryCast<CallRuntimeVertex>()) {
        return hasExceptionLazyDeopt(derived);
    }
    if (auto *derived = vertex->TryCast<PrepareSharedStoreFieldVertex>()) {
        return hasExceptionLazyDeopt(derived);
    }
    if (auto *derived = vertex->TryCast<EnsurePropertiesCapacityVertex>()) {
        return hasExceptionLazyDeopt(derived);
    }
    return false;
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

inline void DefineAsFixed(ValueVertex *vertex, ArkSteedRegister reg)
{
    vertex->Result().SetUnallocated(UnallocatedState::ExtendedPolicy::FIXED_REGISTER,
                                    static_cast<uint32_t>(reg.Code()),
                                    NO_VREG);
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
    // Hint the input's vertex towards this register to avoid a later move.
    input.vertex()->SetHint(input.GetOperand());
}

inline void UseFixed(Input input, ArkSteedRegister reg)
{
    input.GetLocation()->GetOperand() =
        UnallocatedState(UnallocatedState::ExtendedPolicy::FIXED_REGISTER, static_cast<uint32_t>(reg.Code()), NO_VREG);
    input.vertex()->SetHint(input.GetOperand());
}

inline void UseFixed(Input input, ArkSteedDoubleRegister reg)
{
    input.GetLocation()->GetOperand() = UnallocatedState(UnallocatedState::ExtendedPolicy::FIXED_FP_REGISTER,
                                                         static_cast<uint32_t>(reg.Code()), NO_VREG);
    input.vertex()->SetHint(input.GetOperand());
}

inline void UseAndClobberFixed(Input input, uint32_t regCode)
{
    input.GetLocation()->GetOperand() =
        UnallocatedState(UnallocatedState::ExtendedPolicy::FIXED_REGISTER,
                         UnallocatedState::LifetimeFlag::USED_AT_START,
                         regCode,
                         NO_VREG);
    input.vertex()->SetHint(input.GetOperand());
}

inline void UseAndClobberFixed(Input input, ArkSteedRegister reg)
{
    // Use this for fixed GPR inputs that are consumed at the start of a node and
    // then clobbered by the node's ABI sequence. Machine-instruction clobbers
    // that are not inputs should be modeled with RequireSpecificTemporary().
    input.GetLocation()->GetOperand() =
        UnallocatedState(UnallocatedState::ExtendedPolicy::FIXED_REGISTER,
                         UnallocatedState::LifetimeFlag::USED_AT_START,
                         static_cast<uint32_t>(reg.Code()),
                         NO_VREG);
    input.vertex()->SetHint(input.GetOperand());
}

inline void RequireSpecificTemporary(Vertex *vertex, ArkSteedRegister reg)
{
    vertex->GetRegallocInfo()->RequireSpecificTemporary(reg);
}

inline void RequireSpecificDoubleTemporary(Vertex *vertex, ArkSteedDoubleRegister reg)
{
    vertex->GetRegallocInfo()->RequireSpecificDoubleTemporary(reg);
}

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_OPCODE_H
