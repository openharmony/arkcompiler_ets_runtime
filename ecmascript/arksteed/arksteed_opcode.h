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
#include <cstdint>
#include <type_traits>
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
#include "ecmascript/on_heap.h"

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

enum class IntBitwiseKind : uint8_t {
    BITWISE_AND,
    BITWISE_OR,
    BITWISE_XOR,
    SHIFT_LEFT,
    SHIFT_RIGHT_LOGICAL,
    SHIFT_RIGHT_ARITHMETIC,
};

constexpr const char *BinaryOpKindName(BinaryOpKind kind)
{
    switch (kind) {
        case BinaryOpKind::ADD:
            return "ADD";
        case BinaryOpKind::SUB:
            return "SUB";
        case BinaryOpKind::MUL:
            return "MUL";
        case BinaryOpKind::DIV:
            return "DIV";
        case BinaryOpKind::MOD:
            return "MOD";
        case BinaryOpKind::EXP:
            return "EXP";
    }
    return "UNKNOWN";
}

constexpr const char *IntBitwiseKindName(IntBitwiseKind kind)
{
    switch (kind) {
        case IntBitwiseKind::BITWISE_AND:
            return "BITWISE_AND";
        case IntBitwiseKind::BITWISE_OR:
            return "BITWISE_OR";
        case IntBitwiseKind::BITWISE_XOR:
            return "BITWISE_XOR";
        case IntBitwiseKind::SHIFT_LEFT:
            return "SHIFT_LEFT";
        case IntBitwiseKind::SHIFT_RIGHT_LOGICAL:
            return "SHIFT_RIGHT_LOGICAL";
        case IntBitwiseKind::SHIFT_RIGHT_ARITHMETIC:
            return "SHIFT_RIGHT_ARITHMETIC";
    }
    return "UNKNOWN";
}

namespace details {
// Helper to detect if derived class has INPUT_TYPES
template <typename T, typename = void>
struct HasInputTypes : std::false_type {};

template <typename T>
struct HasInputTypes<T, decltype(void(T::INPUT_TYPES))> : std::true_type {};
}

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
 * - PROPERTIES: VertexPropertyFlag (bitmask) for this vertex
 *
 * Note: INPUT_TYPES is only required when inheriting FixedInputVertexMixin
 */
template <typename Base, typename Derived>
class VertexMixin : public Base {
public:
    template <typename... Args>
    static Derived *New(Chunk *chunk, std::initializer_list<ValueVertex *> inputs, Args &&...args)
    {
        return Vertex::New<Derived>(chunk, inputs, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static Derived *New(Chunk *chunk, Span<ValueVertex * const> inputs, Args &&...args)
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
    explicit VertexMixin(Args &&...args) : Base(std::forward<Args>(args)...) {}
};

/**
 * CRTP mixin for vertices with fixed number of inputs
 * Provides compile-time input count checking and verification
 */
template <typename Base, typename Derived>
class FixedInputVertexMixin : public VertexMixin<Base, Derived> {
public:
    static constexpr uint32_t NUM_INPUTS = Derived::NUM_INPUTS;

    static constexpr bool HasInputs()
    {
        return NUM_INPUTS > 0;
    }

    static constexpr uint32_t GetInputCount()
    {
        return NUM_INPUTS;
    }

    void VerifyInputs() const
    {
        if constexpr (NUM_INPUTS != 0) {
            // Verify runtime input count matches compile-time count
            ASSERT(this->GetInputCount() == NUM_INPUTS);

            // Verify input types if defined in derived class
            if constexpr (details::HasInputTypes<Derived>::value) {
                static_assert(NUM_INPUTS == Derived::INPUT_TYPES.size());
                for (uint32_t i = 0; i < NUM_INPUTS; ++i) {
                    CheckValueInput(i, Derived::INPUT_TYPES[i]);
                }
            }
        }
    }

protected:
    template <typename... Args>
    explicit FixedInputVertexMixin(Args &&...args)
        : VertexMixin<Base, Derived>(std::forward<Args>(args)...)
    {}

    inline void CheckValueInput(uint32_t index, ValueRepresentation expectedRepr) const
    {
        const ValueVertex *input = this->GetInput(index);
        ASSERT(input != nullptr);
        ASSERT(input->GetValueRepresentation() == expectedRepr);
    }
};

#define COMMON_MIXINS_LIST(V)       \
    V(CommonStubID, CommonStubID)   \
    V(RuntimeStubID, RuntimeStubID) \
    V(Condition, Condition)         \
    V(Offset, int32_t)

#define DEFINE_MIXIN_TYPE(Field, Type)                          \
    class Field##Mixin {                                        \
    public:                                                     \
        explicit Field##Mixin(Type value) : value_(value) {}    \
                                                                \
        Type Get##Field() const                                 \
        {                                                       \
            return value_;                                      \
        }                                                       \
        void Set##Field(Type value)                             \
        {                                                       \
            value_ = value;                                     \
        }                                                       \
    private:                                                    \
        Type value_;                                            \
    };
COMMON_MIXINS_LIST(DEFINE_MIXIN_TYPE)
#undef DEFINE_MIXIN_TYPE

class LazyDeoptimizableMixin {
public:
    struct LazyDeoptFrameValue {
        VRegIDType vreg;
        ValueVertex *value;
        DeoptTranslationKind valueKind;
        InputLocation sourceLocation;

        LazyDeoptFrameValue(VRegIDType vregId, ValueVertex *frameValue, DeoptTranslationKind kind)
            : vreg(vregId), value(frameValue), valueKind(kind)
        {}
    };

    using LazyDeoptFrameState = ChunkVector<LazyDeoptFrameValue>;

    LazyDeoptimizableMixin() = default;

    void SetLazyDeoptFrameState(LazyDeoptFrameState *frameState, uint32_t bytecodeOffset)
    {
        ASSERT(frameState != nullptr);
        lazyDeoptFrameState_ = frameState;
        bytecodeOffset_ = bytecodeOffset;
    }

    bool HasLazyDeoptFrameState() const
    {
        return lazyDeoptFrameState_ != nullptr;
    }

    uint32_t GetDeoptFrameValueCount() const
    {
        return lazyDeoptFrameState_ == nullptr ? 0 : static_cast<uint32_t>(lazyDeoptFrameState_->size());
    }

    VRegIDType GetDeoptVReg(uint32_t index) const
    {
        ASSERT(lazyDeoptFrameState_ != nullptr);
        ASSERT(index < lazyDeoptFrameState_->size());
        return (*lazyDeoptFrameState_)[index].vreg;
    }

    ValueVertex *GetDeoptFrameValue(uint32_t index) const
    {
        ASSERT(lazyDeoptFrameState_ != nullptr);
        ASSERT(index < lazyDeoptFrameState_->size());
        return (*lazyDeoptFrameState_)[index].value;
    }

    DeoptTranslationKind GetDeoptValueKind(uint32_t index) const
    {
        ASSERT(lazyDeoptFrameState_ != nullptr);
        ASSERT(index < lazyDeoptFrameState_->size());
        return (*lazyDeoptFrameState_)[index].valueKind;
    }

    InputLocation *GetDeoptSourceLocation(uint32_t index)
    {
        ASSERT(lazyDeoptFrameState_ != nullptr);
        ASSERT(index < lazyDeoptFrameState_->size());
        return &(*lazyDeoptFrameState_)[index].sourceLocation;
    }

    const InputLocation *GetDeoptSourceLocation(uint32_t index) const
    {
        ASSERT(lazyDeoptFrameState_ != nullptr);
        ASSERT(index < lazyDeoptFrameState_->size());
        return &(*lazyDeoptFrameState_)[index].sourceLocation;
    }

    const LazyDeoptFrameState &GetLazyDeoptFrameState() const
    {
        ASSERT(lazyDeoptFrameState_ != nullptr);
        return *lazyDeoptFrameState_;
    }

    uint32_t GetBytecodeOffset() const
    {
        ASSERT(lazyDeoptFrameState_ != nullptr);
        return bytecodeOffset_;
    }

private:
    LazyDeoptFrameState *lazyDeoptFrameState_ {nullptr};
    uint32_t bytecodeOffset_ {0};
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

    void MarkExceptionLazyDeopt()
    {
        exceptionLazyDeopt_ = true;
    }

    bool HasExceptionLazyDeopt() const
    {
        return exceptionLazyDeopt_;
    }

private:
    static constexpr uint32_t NULL_INDEX = static_cast<uint32_t>(-1);

    uint32_t catchPredIndex_ = NULL_INDEX;
    // If exceptionLazyDeopt_ == true, then LazyDeoptimizableMixin is used instead.
    bool exceptionLazyDeopt_ = false;
    BB *caughtBy_ = nullptr;
};

//==============================================================================
// Constant Value Vertices
//==============================================================================

class Int32ConstantVertex : public FixedInputVertexMixin<ValueVertex, Int32ConstantVertex> {
public:
    using OutputRegister = ArkSteedRegister;

    static constexpr uint32_t NUM_INPUTS = 0;
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::INT32;
    static constexpr VertexPropertyFlag PROPERTIES = {};

    Int32ConstantVertex(int32_t value) : FixedInputVertexMixin(), value_(value) {}

    int32_t GetValue() const
    {
        return value_;
    }

    void DoLoadToRegister(ArkSteedAssembler *, OutputRegister) const;
    void SetValueLocationConstraints();

private:
    int32_t value_;
};

class Int64ConstantVertex : public FixedInputVertexMixin<ValueVertex, Int64ConstantVertex> {
public:
    using OutputRegister = ArkSteedRegister;

    static constexpr uint32_t NUM_INPUTS = 0;
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::INT64;
    static constexpr VertexPropertyFlag PROPERTIES = {};

    Int64ConstantVertex(int64_t value) : FixedInputVertexMixin(), value_(value) {}

    int64_t GetValue() const
    {
        return value_;
    }

    void DoLoadToRegister(ArkSteedAssembler *, OutputRegister) const;
    void SetValueLocationConstraints();

private:
    int64_t value_;
};

// TODO: Adaptation to 32-bit platform
using IntPtrConstantVertex = Int64ConstantVertex;

class Float64ConstantVertex : public FixedInputVertexMixin<ValueVertex, Float64ConstantVertex> {
public:
    using OutputRegister = ArkSteedDoubleRegister;

    static constexpr uint32_t NUM_INPUTS = 0;
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::FLOAT64;
    static constexpr VertexPropertyFlag PROPERTIES = {};

    Float64ConstantVertex(double value) : FixedInputVertexMixin(), value_(value) {}

    double GetValue() const
    {
        return value_;
    }

    void DoLoadToRegister(ArkSteedAssembler *, OutputRegister) const;
    void SetValueLocationConstraints();

private:
    double value_;
};

class TaggedConstantVertex : public FixedInputVertexMixin<ValueVertex, TaggedConstantVertex> {
public:
    using OutputRegister = ArkSteedRegister;

    static constexpr uint32_t NUM_INPUTS = 0;
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::TAGGED;
    static constexpr VertexPropertyFlag PROPERTIES = {};

    TaggedConstantVertex(uint64_t value) : FixedInputVertexMixin(), value_(value) {}

    uint64_t GetValue() const
    {
        return value_;
    }

    void DoLoadToRegister(ArkSteedAssembler *, OutputRegister) const;
    void SetValueLocationConstraints();

private:
    uint64_t value_;
};

class HeapConstantVertex : public FixedInputVertexMixin<ValueVertex, HeapConstantVertex> {
public:
    using OutputRegister = ArkSteedRegister;
    static constexpr uint32_t NUM_INPUTS = 0;
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::TAGGED;
    static constexpr VertexPropertyFlag PROPERTIES = {};

    HeapConstantVertex(uint32_t handleIndex, uint16_t staticNodeType)
        : FixedInputVertexMixin(), handleIndex_(handleIndex), staticNodeType_(staticNodeType)
    {
    }

    uint32_t GetHandleIndex() const
    {
        return handleIndex_;
    }

    uint16_t GetStaticNodeType() const
    {
        return staticNodeType_;
    }

    void DoLoadToRegister(ArkSteedAssembler *, OutputRegister) const;
    void SetValueLocationConstraints();

private:
    uint32_t handleIndex_;
    uint16_t staticNodeType_;
};

class InitialValueVertex : public FixedInputVertexMixin<ValueVertex, InitialValueVertex> {
public:
    static constexpr uint32_t NUM_INPUTS = 0;
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::TAGGED;
    static constexpr VertexPropertyFlag PROPERTIES = {};

    InitialValueVertex(int32_t frameSlotIndex)
        : FixedInputVertexMixin(), frameSlotIndex_(frameSlotIndex)
    {}

    int32_t GetFrameSlotIndex() const
    {
        return frameSlotIndex_;
    }

    void SetValueLocationConstraints();

private:
    int32_t frameSlotIndex_;
};

class ActualArgcVertex : public FixedInputVertexMixin<ValueVertex, ActualArgcVertex> {
public:
    static constexpr uint32_t NUM_INPUTS = 0;
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::INT64;
    static constexpr VertexPropertyFlag PROPERTIES = {};

    explicit ActualArgcVertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

// Loads TaggedValue from raw pointer
class LoadTaggedFromAddressVertex : public FixedInputVertexMixin<ValueVertex, LoadTaggedFromAddressVertex>,
                                    public OffsetMixin {
public:
    enum Indices : uint32_t {
        OBJECT_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::TAGGED;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::INT_PTR,
    };
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_READ;

    explicit LoadTaggedFromAddressVertex(int32_t offset)
        : FixedInputVertexMixin(), OffsetMixin(offset) {}

    void SetValueLocationConstraints();
};

// Loads exception from glue pointer (with fixed offset)
class LoadExceptionVertex : public FixedInputVertexMixin<ValueVertex, LoadExceptionVertex> {
public:
    enum Indices : uint32_t {
        GLUE_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::TAGGED;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::INT_PTR,
    };
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_READ;

    explicit LoadExceptionVertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

// Stores TaggedValue to raw pointer.
class StoreTaggedToAddressVertex : public FixedInputVertexMixin<NonControlVertex, StoreTaggedToAddressVertex>,
                                   public OffsetMixin {
public:
    enum Indices : uint32_t {
        OBJECT_INDEX = 0,
        VALUE_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::INT_PTR,
        ValueRepresentation::TAGGED,
    };
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_WRITE;
    // 2: object and value to be stored

    explicit StoreTaggedToAddressVertex(int32_t offset)
        : FixedInputVertexMixin(), OffsetMixin(offset) {}

    void SetValueLocationConstraints();
};

// Loads Int32 from raw pointer
class LoadI32FromAddressVertex : public FixedInputVertexMixin<ValueVertex, LoadI32FromAddressVertex>,
                                 public OffsetMixin {
public:
    enum Indices : uint32_t {
        OBJECT_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::INT32;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::INT_PTR,
    };
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_READ;

    explicit LoadI32FromAddressVertex(int32_t offset)
        : FixedInputVertexMixin(), OffsetMixin(offset) {}

    void SetValueLocationConstraints();
};

// Stores Int32 to raw pointer.
class StoreI32ToAddressVertex : public FixedInputVertexMixin<NonControlVertex, StoreI32ToAddressVertex>,
                                public OffsetMixin {
public:
    enum Indices : uint32_t {
        OBJECT_INDEX = 0,
        VALUE_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::INT_PTR,
        ValueRepresentation::INT32,
    };
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_WRITE;

    explicit StoreI32ToAddressVertex(int32_t offset)
        : FixedInputVertexMixin(), OffsetMixin(offset) {}

    void SetValueLocationConstraints();
};

// Loads Int64 from raw pointer
class LoadI64FromAddressVertex : public FixedInputVertexMixin<ValueVertex, LoadI64FromAddressVertex>,
                                 public OffsetMixin {
public:
    enum Indices : uint32_t {
        OBJECT_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::INT64;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::INT_PTR,
    };
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_READ;

    explicit LoadI64FromAddressVertex(int32_t offset)
        : FixedInputVertexMixin(), OffsetMixin(offset) {}

    void SetValueLocationConstraints();
};

// Stores Int64 to raw pointer.
class StoreI64ToAddressVertex : public FixedInputVertexMixin<NonControlVertex, StoreI64ToAddressVertex>,
                                public OffsetMixin {
public:
    enum Indices : uint32_t {
        OBJECT_INDEX = 0,
        VALUE_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::INT_PTR,
        ValueRepresentation::INT64,
    };
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_WRITE;

    explicit StoreI64ToAddressVertex(int32_t offset)
        : FixedInputVertexMixin(), OffsetMixin(offset) {}

    void SetValueLocationConstraints();
};

// Loads Float64 from raw pointer
class LoadF64FromAddressVertex : public FixedInputVertexMixin<ValueVertex, LoadF64FromAddressVertex>,
                                 public OffsetMixin {
public:
    using OutputRegister = ArkSteedDoubleRegister;

    enum Indices : uint32_t {
        OBJECT_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::FLOAT64;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::INT_PTR,
    };
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_READ;

    explicit LoadF64FromAddressVertex(int32_t offset)
        : FixedInputVertexMixin(), OffsetMixin(offset) {}

    void SetValueLocationConstraints();
};

// Stores Float64 to raw pointer.
class StoreF64ToAddressVertex : public FixedInputVertexMixin<NonControlVertex, StoreF64ToAddressVertex>,
                                public OffsetMixin {
public:
    enum Indices : uint32_t {
        OBJECT_INDEX = 0,
        VALUE_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::INT_PTR,
        ValueRepresentation::FLOAT64,
    };
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_WRITE;

    explicit StoreF64ToAddressVertex(int32_t offset)
        : FixedInputVertexMixin(), OffsetMixin(offset) {}

    void SetValueLocationConstraints();
};

class LoadTaggedFieldVertex : public FixedInputVertexMixin<ValueVertex, LoadTaggedFieldVertex>,
                              public OffsetMixin {
public:
    enum Indices : uint32_t {
        OBJECT_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::TAGGED;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::TAGGED,
    };
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_READ;

    explicit LoadTaggedFieldVertex(int32_t offset) : FixedInputVertexMixin(), OffsetMixin(offset)
    {}

    void SetValueLocationConstraints();
};

class LoadInt32FieldVertex : public FixedInputVertexMixin<ValueVertex, LoadInt32FieldVertex>,
                             public OffsetMixin {
public:
    enum Indices : uint32_t {
        OBJECT_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::INT32;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::TAGGED,
    };
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_READ;

    explicit LoadInt32FieldVertex(int32_t offset) : FixedInputVertexMixin(), OffsetMixin(offset)
    {}

    void SetValueLocationConstraints();
};

class LoadTaggedElementVertex : public FixedInputVertexMixin<ValueVertex, LoadTaggedElementVertex> {
public:
    enum Indices : uint32_t {
        ELEMENTS_INDEX = 0,
        INDEX_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::TAGGED;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::TAGGED,
        ValueRepresentation::INT32,
    };
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_READ;

    explicit LoadTaggedElementVertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

class LoadSingleCharTableElementVertex
    : public FixedInputVertexMixin<ValueVertex, LoadSingleCharTableElementVertex> {
public:
    enum Indices : uint32_t {
        GLUE_INDEX = 0,
        CHAR_CODE_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::TAGGED;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::INT_PTR,
        ValueRepresentation::INT32,
    };
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_READ;
    static constexpr int32_t MIN_CHAR_CODE = 1;
    static constexpr int32_t MAX_CHAR_CODE = 0x7F;

    explicit LoadSingleCharTableElementVertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

class TypedArrayIntLoadElementVertex : public FixedInputVertexMixin<ValueVertex, TypedArrayIntLoadElementVertex> {
public:
    enum Indices : uint32_t {
        RECEIVER_INDEX = 0,
        ELEMENT_INDEX = 1,
        STORAGE_INDEX = 2,
        NUM_INPUTS = 3,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::INT32;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::TAGGED,
        ValueRepresentation::INT32,
        ValueRepresentation::TAGGED,
    };
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_READ;

    TypedArrayIntLoadElementVertex(JSType elementType, bool isOnHeap)
        : FixedInputVertexMixin(), elementType_(elementType), isOnHeap_(isOnHeap)
    {
    }

    JSType GetElementType() const
    {
        return elementType_;
    }

    bool IsOnHeap() const
    {
        return isOnHeap_;
    }

    void SetValueLocationConstraints();

private:
    JSType elementType_;
    bool isOnHeap_;
};

class TypedArrayDoubleLoadElementVertex
    : public FixedInputVertexMixin<ValueVertex, TypedArrayDoubleLoadElementVertex> {
public:
    enum Indices : uint32_t {
        RECEIVER_INDEX = 0,
        ELEMENT_INDEX = 1,
        STORAGE_INDEX = 2,
        NUM_INPUTS = 3,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::FLOAT64;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::TAGGED,
        ValueRepresentation::INT32,
        ValueRepresentation::TAGGED,
    };
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_READ;

    TypedArrayDoubleLoadElementVertex(JSType elementType, bool isOnHeap)
        : FixedInputVertexMixin(), elementType_(elementType), isOnHeap_(isOnHeap)
    {
    }

    JSType GetElementType() const
    {
        return elementType_;
    }

    bool IsOnHeap() const
    {
        return isOnHeap_;
    }

    void SetValueLocationConstraints();

private:
    JSType elementType_;
    bool isOnHeap_;
};

class LoadPrototypeFromObjectVertex : public FixedInputVertexMixin<ValueVertex, LoadPrototypeFromObjectVertex> {
public:
    enum Indices : uint32_t {
        OBJECT_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::TAGGED;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::TAGGED,
    };
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_READ;

    explicit LoadPrototypeFromObjectVertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

class LoadHClassAddressVertex : public FixedInputVertexMixin<ValueVertex, LoadHClassAddressVertex> {
public:
    enum Indices : uint32_t {
        OBJECT_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::INT64;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::TAGGED,
    };
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_READ;

    explicit LoadHClassAddressVertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

class LoadPrototypeHolderByHClassVertex : public VertexMixin<ValueVertex, LoadPrototypeHolderByHClassVertex>,
                                          public EagerDeoptimizableMixin {
public:
    enum Indices : uint32_t {
        RECEIVER_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::TAGGED;
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::CAN_READ |
        VertexPropertyFlag::CAN_EAGER_DEOPT;

    explicit LoadPrototypeHolderByHClassVertex(Chunk *chunk,
                                               JSHClass *holderHClass,
                                               std::vector<JSHClass *> expectedPrototypeHClasses,
                                               uint32_t holderDepth,
                                               uint32_t bytecodeOffset)
        : VertexMixin(),
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

class ConvertHoleToUndefinedVertex : public FixedInputVertexMixin<ValueVertex, ConvertHoleToUndefinedVertex> {
public:
    enum Indices : uint32_t {
        VALUE_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::TAGGED;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::TAGGED,
    };
    static constexpr VertexPropertyFlag PROPERTIES = {};

    explicit ConvertHoleToUndefinedVertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

class FindPrototypeHolderVertex : public VertexMixin<ValueVertex, FindPrototypeHolderVertex>,
                                  public EagerDeoptimizableMixin {
public:
    enum Indices : uint32_t {
        RECEIVER_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::TAGGED;
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::CAN_EAGER_DEOPT |
        VertexPropertyFlag::CAN_READ;

    FindPrototypeHolderVertex(Chunk *chunk, JSHClass *expectedHolderHClass,
                              uint32_t bytecodeOffset)
        : VertexMixin(),
          EagerDeoptimizableMixin(chunk, bytecodeOffset),
          expectedHolderHClass_(expectedHolderHClass)
    {}

    JSHClass *GetExpectedHolderHClass() const
    {
        return expectedHolderHClass_;
    }

    void SetValueLocationConstraints();

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

constexpr const char *WriteBarrierValueKindName(ArkSteedWriteBarrierValueKind kind)
{
    switch (kind) {
        case ArkSteedWriteBarrierValueKind::NonHeap:
            return "NON_HEAP";
        case ArkSteedWriteBarrierValueKind::HeapObject:
            return "HEAP_OBJECT";
        default:
            return "UNKNOWN";
    }
}

// 2: object and value inputs
class StoreTaggedFieldVertex : public FixedInputVertexMixin<NonControlVertex, StoreTaggedFieldVertex>,
                               public OffsetMixin {
public:
    enum Indices : uint32_t {
        OBJECT_INDEX = 0,
        VALUE_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::TAGGED,
        ValueRepresentation::TAGGED,
    };
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_WRITE;

    static constexpr uint32_t UNKNOWN_PROPERTY_ID = UINT32_MAX;

    explicit StoreTaggedFieldVertex(int32_t offset, uint32_t propertyId = UNKNOWN_PROPERTY_ID)
        : FixedInputVertexMixin(), OffsetMixin(offset), propertyId_(propertyId)
    {
    }

    uint32_t GetPropertyId() const
    {
        return propertyId_;
    }

    void SetValueLocationConstraints();

private:
    uint32_t propertyId_;
};

class StoreTaggedFieldWithBarrierVertex
    : public FixedInputVertexMixin<NonControlVertex, StoreTaggedFieldWithBarrierVertex>,
      public OffsetMixin {
public:
    enum Indices : uint32_t {
        GLUE_INDEX = 0,
        OBJECT_INDEX = 1,
        VALUE_INDEX = 2,
        NUM_INPUTS = 3,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::INT_PTR,
        ValueRepresentation::TAGGED,
        ValueRepresentation::TAGGED,
    };
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::CAN_WRITE |
        VertexPropertyFlag::IS_DEFERRED_CALL;

    explicit StoreTaggedFieldWithBarrierVertex(int32_t offset,
                                               ArkSteedWriteBarrierValueKind valueKind =
                                                   ArkSteedWriteBarrierValueKind::Unknown)
        : FixedInputVertexMixin(), OffsetMixin(offset), valueKind_(valueKind)
    {}

    ArkSteedWriteBarrierValueKind GetValueKind() const
    {
        return valueKind_;
    }

    void SetValueKind(ArkSteedWriteBarrierValueKind valueKind)
    {
        valueKind_ = valueKind;
    }

    void SetValueLocationConstraints();

private:
    ArkSteedWriteBarrierValueKind valueKind_ {ArkSteedWriteBarrierValueKind::Unknown};
};

class StoreTaggedElementVertex : public FixedInputVertexMixin<NonControlVertex, StoreTaggedElementVertex> {
public:
    enum Indices : uint32_t {
        OBJECT_INDEX = 0,
        INDEX_INDEX = 1,
        VALUE_INDEX = 2,
        NUM_INPUTS = 3,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::TAGGED,
        ValueRepresentation::INT32,
        ValueRepresentation::TAGGED,
    };
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_WRITE;

    explicit StoreTaggedElementVertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

class StoreTaggedElementWithBarrierVertex
    : public FixedInputVertexMixin<NonControlVertex, StoreTaggedElementWithBarrierVertex> {
public:
    enum Indices : uint32_t {
        GLUE_INDEX = 0,
        OBJECT_INDEX = 1,
        INDEX_INDEX = 2,
        VALUE_INDEX = 3,
        NUM_INPUTS = 4,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::INT_PTR,
        ValueRepresentation::TAGGED,
        ValueRepresentation::INT32,
        ValueRepresentation::TAGGED,
    };
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::CAN_WRITE |
        VertexPropertyFlag::IS_CALL;

    explicit StoreTaggedElementWithBarrierVertex(
        ArkSteedWriteBarrierValueKind valueKind = ArkSteedWriteBarrierValueKind::Unknown)
        : FixedInputVertexMixin(), valueKind_(valueKind)
    {}

    ArkSteedWriteBarrierValueKind GetValueKind() const
    {
        return valueKind_;
    }

    void SetValueKind(ArkSteedWriteBarrierValueKind valueKind)
    {
        valueKind_ = valueKind;
    }

    void SetValueLocationConstraints();

private:
    ArkSteedWriteBarrierValueKind valueKind_ {ArkSteedWriteBarrierValueKind::Unknown};
};

class StoreIntTypedArrayElementVertex
    : public FixedInputVertexMixin<NonControlVertex, StoreIntTypedArrayElementVertex> {
public:
    enum Indices : uint32_t {
        RECEIVER_INDEX = 0,
        INDEX_INDEX = 1,
        VALUE_INDEX = 2,
        NUM_INPUTS = 3,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::TAGGED,
        ValueRepresentation::INT32,
        ValueRepresentation::INT32,
    };
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_WRITE;

    explicit StoreIntTypedArrayElementVertex(JSType type, OnHeapMode onHeapMode)
        : FixedInputVertexMixin(), type_(type), onHeapMode_(onHeapMode)
    {}

    JSType GetType() const
    {
        return type_;
    }

    OnHeapMode GetOnHeapMode() const
    {
        return onHeapMode_;
    }

    void SetValueLocationConstraints();

private:
    JSType type_ {JSType::INVALID};
    OnHeapMode onHeapMode_ {OnHeapMode::NONE};
};

class StoreFloatTypedArrayElementVertex
    : public FixedInputVertexMixin<NonControlVertex, StoreFloatTypedArrayElementVertex> {
public:
    enum Indices : uint32_t {
        RECEIVER_INDEX = 0,
        INDEX_INDEX = 1,
        VALUE_INDEX = 2,
        NUM_INPUTS = 3,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::TAGGED,
        ValueRepresentation::INT32,
        ValueRepresentation::FLOAT64,
    };
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_WRITE;

    explicit StoreFloatTypedArrayElementVertex(JSType type, OnHeapMode onHeapMode)
        : FixedInputVertexMixin(), type_(type), onHeapMode_(onHeapMode)
    {}

    JSType GetType() const
    {
        return type_;
    }

    OnHeapMode GetOnHeapMode() const
    {
        return onHeapMode_;
    }

    void SetValueLocationConstraints();

private:
    JSType type_ {JSType::INVALID};
    OnHeapMode onHeapMode_ {OnHeapMode::NONE};
};

class StoreSharedFieldWithBarrierVertex
    : public FixedInputVertexMixin<NonControlVertex, StoreSharedFieldWithBarrierVertex>,
      public OffsetMixin {
public:
    enum Indices : uint32_t {
        GLUE_INDEX = 0,
        OBJECT_INDEX = 1,
        VALUE_INDEX = 2,
        NUM_INPUTS = 3,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::INT_PTR,
        ValueRepresentation::TAGGED,
        ValueRepresentation::TAGGED,
    };
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::CAN_WRITE |
        VertexPropertyFlag::IS_DEFERRED_CALL;

    explicit StoreSharedFieldWithBarrierVertex(int32_t offset,
                                               ArkSteedWriteBarrierValueKind valueKind =
                                                   ArkSteedWriteBarrierValueKind::Unknown)
        : FixedInputVertexMixin(), OffsetMixin(offset), valueKind_(valueKind)
    {
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

private:
    ArkSteedWriteBarrierValueKind valueKind_ {ArkSteedWriteBarrierValueKind::Unknown};
};

class TransitionHClassWithBarrierVertex
    : public FixedInputVertexMixin<NonControlVertex, TransitionHClassWithBarrierVertex> {
public:
    enum Indices : uint32_t {
        GLUE_INDEX = 0,
        OBJECT_INDEX = 1,
        HCLASS_INDEX = 2,
        NUM_INPUTS = 3,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::INT_PTR,
        ValueRepresentation::TAGGED,
        ValueRepresentation::TAGGED,
    };
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::CAN_WRITE |
        VertexPropertyFlag::IS_DEFERRED_CALL;

    explicit TransitionHClassWithBarrierVertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

class PrepareSharedStoreFieldVertex
    : public VertexMixin<ValueVertex, PrepareSharedStoreFieldVertex>,
      public ThrowableMixin,
      public LazyDeoptimizableMixin {
public:
    enum Indices : uint32_t {
        VALUE_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::TAGGED;
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::IS_CALL |
        VertexPropertyFlag::CAN_READ |
        VertexPropertyFlag::CAN_WRITE |
        VertexPropertyFlag::CAN_ALLOCATE |
        VertexPropertyFlag::CAN_LAZY_DEOPT |
        VertexPropertyFlag::CAN_THROW;

    PrepareSharedStoreFieldVertex(uint64_t handlerInfo)
        : VertexMixin(), handlerInfo_(handlerInfo)
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
    enum Indices : uint32_t {
        GLUE_INDEX = 0,
        OBJECT_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::TAGGED;
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::CAN_WRITE |
        VertexPropertyFlag::IS_CALL |
        VertexPropertyFlag::CAN_READ |
        VertexPropertyFlag::CAN_ALLOCATE |
        VertexPropertyFlag::CAN_LAZY_DEOPT |
        VertexPropertyFlag::CAN_THROW;

    EnsurePropertiesCapacityVertex(int32_t fieldIndex)
        : VertexMixin(), fieldIndex_(fieldIndex)
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

    void VerifyInputs() const
    {
        ASSERT(GetInputCount() == OBJECT_INDEX + 1);
        ASSERT(GetInput(GLUE_INDEX)->GetValueRepresentation() == ValueRepresentation::INT_PTR);
        ASSERT(GetInput(OBJECT_INDEX)->GetValueRepresentation() == ValueRepresentation::TAGGED);
    }

private:
    int32_t fieldIndex_ {0};
};

class StoreInt32FieldVertex : public FixedInputVertexMixin<NonControlVertex, StoreInt32FieldVertex>,
                              public OffsetMixin {
public:
    enum Indices : uint32_t {
        STORE_TARGET_INDEX = 0,
        VALUE_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::TAGGED,
        ValueRepresentation::INT32,
    };
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_WRITE;

    explicit StoreInt32FieldVertex(int32_t offset)
        : FixedInputVertexMixin(), OffsetMixin(offset)
    {
    }

    void SetValueLocationConstraints();
};

class StoreDoubleFieldVertex : public FixedInputVertexMixin<NonControlVertex, StoreDoubleFieldVertex>,
                               public OffsetMixin {
public:
    enum Indices : uint32_t {
        STORE_TARGET_INDEX = 0,
        VALUE_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::TAGGED,
        ValueRepresentation::FLOAT64,
    };
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_WRITE;

    explicit StoreDoubleFieldVertex(int32_t offset)
        : FixedInputVertexMixin(), OffsetMixin(offset)
    {
    }

    void SetValueLocationConstraints();
};

class StoreInt32FieldWithRepVertex
    : public VertexMixin<NonControlVertex, StoreInt32FieldWithRepVertex>,
      public EagerDeoptimizableMixin,
      public OffsetMixin {
public:
    enum Indices : uint32_t {
        STORE_TARGET_INDEX = 0,
        VALUE_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::CAN_WRITE |
        VertexPropertyFlag::CAN_EAGER_DEOPT;

    StoreInt32FieldWithRepVertex(Chunk *chunk, int32_t offset, uint32_t bytecodeOffset)
        : VertexMixin(),
          EagerDeoptimizableMixin(chunk, bytecodeOffset),
          OffsetMixin(offset)
    {
    }

    void SetValueLocationConstraints();

    void VerifyInputs() const
    {
        ASSERT(GetInputCount() == VALUE_INDEX + 1);
        ASSERT(GetInput(STORE_TARGET_INDEX)->GetValueRepresentation() == ValueRepresentation::TAGGED);
        ASSERT(GetInput(VALUE_INDEX)->GetValueRepresentation() == ValueRepresentation::TAGGED);
    }

private:
};

class StoreDoubleFieldWithRepVertex
    : public VertexMixin<NonControlVertex, StoreDoubleFieldWithRepVertex>,
      public EagerDeoptimizableMixin,
      public OffsetMixin {
public:
    enum Indices : uint32_t {
        STORE_TARGET_INDEX = 0,
        VALUE_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::CAN_WRITE |
        VertexPropertyFlag::CAN_EAGER_DEOPT;

    StoreDoubleFieldWithRepVertex(Chunk *chunk, int32_t offset, uint32_t bytecodeOffset)
        : VertexMixin(),
          EagerDeoptimizableMixin(chunk, bytecodeOffset),
          OffsetMixin(offset)
    {
    }

    void SetValueLocationConstraints();

    void VerifyInputs() const
    {
        ASSERT(GetInputCount() == VALUE_INDEX + 1);
        ASSERT(GetInput(STORE_TARGET_INDEX)->GetValueRepresentation() == ValueRepresentation::TAGGED);
        ASSERT(GetInput(VALUE_INDEX)->GetValueRepresentation() == ValueRepresentation::TAGGED);
    }

private:
};

struct StoreTaggedFieldByHClassCase {
    JSHClass *expectedHClass {nullptr};
    int32_t fieldOffset {0};
    bool propertiesArray {false};
};

class StoreTaggedFieldByHClassVertex
    : public VertexMixin<NonControlVertex, StoreTaggedFieldByHClassVertex>, public EagerDeoptimizableMixin {
public:
    enum Indices : uint32_t {
        GLUE_INDEX = 0,
        OBJECT_INDEX = 1,
        VALUE_INDEX = 2,
        NUM_INPUTS = 3,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::CAN_WRITE |
        VertexPropertyFlag::CAN_READ |
        VertexPropertyFlag::IS_DEFERRED_CALL |
        VertexPropertyFlag::CAN_EAGER_DEOPT;

    StoreTaggedFieldByHClassVertex(Chunk *chunk,
                                   const std::vector<StoreTaggedFieldByHClassCase> &cases,
                                   ArkSteedWriteBarrierValueKind valueKind, uint32_t bytecodeOffset)
        : VertexMixin(),
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

class StoreEnvSlotVertex : public FixedInputVertexMixin<NonControlVertex, StoreEnvSlotVertex>,
                           public OffsetMixin {
public:
    enum Indices : uint32_t {
        ENV_INDEX = 0,
        VALUE_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::TAGGED,
        ValueRepresentation::TAGGED,
    };
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_WRITE;

    explicit StoreEnvSlotVertex(int32_t offset) : FixedInputVertexMixin(), OffsetMixin(offset) {}

    void SetValueLocationConstraints();
};

class SetValueWithBarrierVertex : public FixedInputVertexMixin<NonControlVertex, SetValueWithBarrierVertex>,
                                  public OffsetMixin {
public:
    enum Indices : uint32_t {
        GLUE_INDEX = 0,
        OBJECT_INDEX = 1,
        VALUE_INDEX = 2,
        NUM_INPUTS = 3,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::INT_PTR,
        ValueRepresentation::TAGGED,
        ValueRepresentation::TAGGED,
    };
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::IS_CALL | VertexPropertyFlag::CAN_WRITE;

    explicit SetValueWithBarrierVertex(int32_t offset)
        : FixedInputVertexMixin(), OffsetMixin(offset)
    {
    }

    void SetValueLocationConstraints();
};

/**
 * CallRuntime vertex - for runtime function calls
 */
class CallRuntimeVertex : public VertexMixin<ValueVertex, CallRuntimeVertex>,
                          public ThrowableMixin,
                          public LazyDeoptimizableMixin,
                          public RuntimeStubIDMixin {
public:
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::TAGGED;
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::IS_CALL |
        VertexPropertyFlag::CAN_READ |
        VertexPropertyFlag::CAN_WRITE |
        VertexPropertyFlag::CAN_ALLOCATE |
        VertexPropertyFlag::CAN_LAZY_DEOPT |
        VertexPropertyFlag::CAN_THROW;

    CallRuntimeVertex(RuntimeStubID id, SideEffectKind sideEffectKind = SideEffectKind::UNKNOWN_CALL)
        : VertexMixin(),
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
    enum Indices : uint32_t {
        TARGET_INDEX = 0,
        NEW_TARGET_INDEX = 1,
        THIS_INDEX = 2,
        FIRST_ARG_INDEX = 3,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::TAGGED;
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::IS_CALL |
        VertexPropertyFlag::CAN_READ |
        VertexPropertyFlag::CAN_WRITE |
        VertexPropertyFlag::CAN_ALLOCATE |
        VertexPropertyFlag::CAN_LAZY_DEOPT |
        VertexPropertyFlag::CAN_THROW;

    CallVertex(uint32_t actualArgc) : VertexMixin(), actualArgc_(actualArgc) {}

    uint32_t GetActualArgc() const
    {
        return actualArgc_;
    }

    size_t GetArgCount() const
    {
        return GetInputCount();
    }

    void SetValueLocationConstraints();

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
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::TAGGED;
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::IS_CALL |
        VertexPropertyFlag::CAN_READ |
        VertexPropertyFlag::CAN_WRITE |
        VertexPropertyFlag::CAN_ALLOCATE |
        VertexPropertyFlag::CAN_LAZY_DEOPT |
        VertexPropertyFlag::CAN_THROW;

    CallCommonStubVertex(CommonStubID stubId,
                         SideEffectKind sideEffectKind = SideEffectKind::UNKNOWN_CALL)
        : VertexMixin(),
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

    void VerifyInputs() const
    {
        ASSERT(GetInputCount() >= 1);
    }

private:
    SideEffectKind sideEffectKind_;
};

class LineStringLoadElementVertex : public FixedInputVertexMixin<ValueVertex, LineStringLoadElementVertex> {
public:
    enum Indices : uint32_t {
        STRING_INDEX = 0,
        ELEMENT_INDEX = 1,
        LENGTH_AND_FLAGS_INDEX = 2,
        NUM_INPUTS = 3,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::INT32;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::TAGGED,
        ValueRepresentation::INT32,
        ValueRepresentation::INT32,
    };
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_READ;

    explicit LineStringLoadElementVertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

class TaggedIntToI32Vertex : public FixedInputVertexMixin<ValueVertex, TaggedIntToI32Vertex> {
public:
    enum Indices : uint32_t {
        INPUT_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::INT32;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::TAGGED,
    };
    static constexpr VertexPropertyFlag PROPERTIES = {};

    explicit TaggedIntToI32Vertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

class CheckedTaggedIntToI32Vertex : public VertexMixin<ValueVertex, CheckedTaggedIntToI32Vertex>,
                                    public EagerDeoptimizableMixin {
public:
    enum Indices : uint32_t {
        INPUT_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::INT32;
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::IS_NOT_IDEMPOTENT |
        VertexPropertyFlag::CAN_EAGER_DEOPT;

    explicit CheckedTaggedIntToI32Vertex(Chunk *chunk, uint32_t bytecodeOffset)
        : VertexMixin(), EagerDeoptimizableMixin(chunk, bytecodeOffset)
    {}

    void SetValueLocationConstraints();
    void VerifyInputs() const
    {
        ASSERT(GetInputCount() == NUM_INPUTS);
        ASSERT(GetInput(INPUT_INDEX)->GetValueRepresentation() == ValueRepresentation::TAGGED);
    }
};

class CheckedTaggedStringVertex : public VertexMixin<ValueVertex, CheckedTaggedStringVertex>,
                                  public EagerDeoptimizableMixin {
public:
    enum Indices : uint32_t {
        INPUT_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::TAGGED;
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::IS_NOT_IDEMPOTENT |
        VertexPropertyFlag::CAN_EAGER_DEOPT;

    explicit CheckedTaggedStringVertex(Chunk *chunk, uint32_t bytecodeOffset)
        : VertexMixin(), EagerDeoptimizableMixin(chunk, bytecodeOffset)
    {}

    void SetValueLocationConstraints();
    void VerifyInputs() const
    {
        ASSERT(GetInputCount() == NUM_INPUTS);
        ASSERT(GetInput(INPUT_INDEX)->GetValueRepresentation() == ValueRepresentation::TAGGED);
    }
};

class I32ConditionCheckVertex : public FixedInputVertexMixin<ValueVertex, I32ConditionCheckVertex>,
                                public ConditionMixin {
public:
    enum Indices : uint32_t {
        LEFT_INDEX = 0,
        RIGHT_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::TAGGED;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::INT32,
        ValueRepresentation::INT32,
    };
    static constexpr VertexPropertyFlag PROPERTIES = {};

    explicit I32ConditionCheckVertex(Condition condition)
        : FixedInputVertexMixin(), ConditionMixin(condition)
    {}

    void SetValueLocationConstraints();
};

class F64ConditionCheckVertex : public FixedInputVertexMixin<ValueVertex, F64ConditionCheckVertex>,
                                public ConditionMixin {
public:
    enum Indices : uint32_t {
        LEFT_INDEX = 0,
        RIGHT_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::TAGGED;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::FLOAT64,
        ValueRepresentation::FLOAT64,
    };
    static constexpr VertexPropertyFlag PROPERTIES = {};

    explicit F64ConditionCheckVertex(Condition condition)
        : FixedInputVertexMixin(), ConditionMixin(condition)
    {}

    void SetValueLocationConstraints();
};

class TaggedEqualVertex : public FixedInputVertexMixin<ValueVertex, TaggedEqualVertex> {
public:
    enum Indices : uint32_t {
        LEFT_INDEX = 0,
        RIGHT_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::TAGGED;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::TAGGED,
        ValueRepresentation::TAGGED,
    };
    static constexpr VertexPropertyFlag PROPERTIES = {};

    explicit TaggedEqualVertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

class TaggedNotEqualVertex : public FixedInputVertexMixin<ValueVertex, TaggedNotEqualVertex> {
public:
    enum Indices : uint32_t {
        LEFT_INDEX = 0,
        RIGHT_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::TAGGED;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::TAGGED,
        ValueRepresentation::TAGGED,
    };
    static constexpr VertexPropertyFlag PROPERTIES = {};

    explicit TaggedNotEqualVertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

class StringEqualVertex : public FixedInputVertexMixin<ValueVertex, StringEqualVertex> {
public:
    enum Indices : uint32_t {
        GLUE_INDEX = 0,
        LEFT_INDEX = 1,
        RIGHT_INDEX = 2,
        GLOBAL_ENV_INDEX = 3,
        NUM_INPUTS = 4,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::TAGGED;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::INT_PTR,
        ValueRepresentation::TAGGED,
        ValueRepresentation::TAGGED,
        ValueRepresentation::TAGGED,
    };
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::IS_CALL |
        VertexPropertyFlag::CAN_READ |
        VertexPropertyFlag::CAN_WRITE |
        VertexPropertyFlag::CAN_ALLOCATE;

    explicit StringEqualVertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

class I32AddWithOverflowVertex : public VertexMixin<ValueVertex, I32AddWithOverflowVertex>,
                                 public EagerDeoptimizableMixin {
public:
    enum Indices : uint32_t {
        LEFT_INDEX = 0,
        RIGHT_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::INT32;
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::IS_NOT_IDEMPOTENT |
        VertexPropertyFlag::CAN_EAGER_DEOPT;

    explicit I32AddWithOverflowVertex(Chunk *chunk, uint32_t bytecodeOffset)
        : VertexMixin(), EagerDeoptimizableMixin(chunk, bytecodeOffset)
    {}

    void SetValueLocationConstraints();
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
    enum Indices : uint32_t {
        LEFT_INDEX = 0,
        RIGHT_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::INT32;
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::IS_NOT_IDEMPOTENT |
        VertexPropertyFlag::CAN_EAGER_DEOPT;

    explicit I32SubWithOverflowVertex(Chunk *chunk, uint32_t bytecodeOffset)
        : VertexMixin(), EagerDeoptimizableMixin(chunk, bytecodeOffset)
    {}

    void SetValueLocationConstraints();
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
    enum Indices : uint32_t {
        LEFT_INDEX = 0,
        RIGHT_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::INT32;
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::IS_NOT_IDEMPOTENT |
        VertexPropertyFlag::CAN_EAGER_DEOPT;

    explicit I32MulWithOverflowVertex(Chunk *chunk, uint32_t bytecodeOffset)
        : VertexMixin(), EagerDeoptimizableMixin(chunk, bytecodeOffset)
    {}

    void SetValueLocationConstraints();
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
    enum Indices : uint32_t {
        LEFT_INDEX = 0,
        RIGHT_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::INT32;
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::IS_NOT_IDEMPOTENT |
        VertexPropertyFlag::CAN_EAGER_DEOPT;

    explicit I32DivWithOverflowVertex(Chunk *chunk, uint32_t bytecodeOffset)
        : VertexMixin(), EagerDeoptimizableMixin(chunk, bytecodeOffset)
    {}

    void SetValueLocationConstraints();
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
    enum Indices : uint32_t {
        INPUT_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::INT32;
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::IS_NOT_IDEMPOTENT |
        VertexPropertyFlag::CAN_EAGER_DEOPT;

    explicit I32DivByConstWithCheckVertex(Chunk *chunk, uint32_t bytecodeOffset,
                                          int32_t divisor, int32_t magic, uint32_t shift)
        : VertexMixin(),
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

    void VerifyInputs() const
    {
        ASSERT(GetInputCount() == NUM_INPUTS);
        ASSERT(GetInput(INPUT_INDEX)->GetValueRepresentation() == ValueRepresentation::INT32);
        ASSERT(divisor_ <= -2 || divisor_ >= 2);
    }

private:
    int32_t divisor_;
    int32_t magic_;
    uint32_t shift_;
};

class I32AddVertex : public FixedInputVertexMixin<ValueVertex, I32AddVertex> {
public:
    enum Indices : uint32_t {
        LEFT_INDEX = 0,
        RIGHT_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::INT32;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::INT32,
        ValueRepresentation::INT32,
    };
    static constexpr VertexPropertyFlag PROPERTIES = {};

    explicit I32AddVertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

class I32SubVertex : public FixedInputVertexMixin<ValueVertex, I32SubVertex> {
public:
    enum Indices : uint32_t {
        LEFT_INDEX = 0,
        RIGHT_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::INT32;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::INT32,
        ValueRepresentation::INT32,
    };
    static constexpr VertexPropertyFlag PROPERTIES = {};

    explicit I32SubVertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

class I32MulVertex : public FixedInputVertexMixin<ValueVertex, I32MulVertex> {
public:
    enum Indices : uint32_t {
        LEFT_INDEX = 0,
        RIGHT_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::INT32;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::INT32,
        ValueRepresentation::INT32,
    };
    static constexpr VertexPropertyFlag PROPERTIES = {};

    explicit I32MulVertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

class I32DivVertex : public FixedInputVertexMixin<ValueVertex, I32DivVertex> {
public:
    enum Indices : uint32_t {
        LEFT_INDEX = 0,
        RIGHT_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::INT32;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::INT32,
        ValueRepresentation::INT32,
    };
    static constexpr VertexPropertyFlag PROPERTIES = {};

    explicit I32DivVertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

class CheckedI32ModVertex : public VertexMixin<ValueVertex, CheckedI32ModVertex>,
                            public EagerDeoptimizableMixin {
public:
    enum Indices : uint32_t {
        LEFT_INDEX = 0,
        RIGHT_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::INT32;
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::IS_NOT_IDEMPOTENT |
        VertexPropertyFlag::CAN_EAGER_DEOPT;

    explicit CheckedI32ModVertex(Chunk *chunk, uint32_t bytecodeOffset)
        : VertexMixin(), EagerDeoptimizableMixin(chunk, bytecodeOffset)
    {}

    void SetValueLocationConstraints();
    void VerifyInputs() const
    {
        ASSERT(GetInputCount() == NUM_INPUTS);
        ASSERT(GetInput(LEFT_INDEX)->GetValueRepresentation() == ValueRepresentation::INT32);
        ASSERT(GetInput(RIGHT_INDEX)->GetValueRepresentation() == ValueRepresentation::INT32);
    }
};

class I32BitwiseBinaryVertex : public FixedInputVertexMixin<ValueVertex, I32BitwiseBinaryVertex> {
public:
    enum Indices : uint32_t {
        LEFT_INDEX = 0,
        RIGHT_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::INT32;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::INT32,
        ValueRepresentation::INT32,
    };
    static constexpr VertexPropertyFlag PROPERTIES = {};

    explicit I32BitwiseBinaryVertex(IntBitwiseKind kind)
        : FixedInputVertexMixin(), kind_(kind)
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

private:
    IntBitwiseKind kind_;
};

class CheckedNonNegativeI32ToTaggedIntVertex
    : public VertexMixin<ValueVertex, CheckedNonNegativeI32ToTaggedIntVertex>,
      public EagerDeoptimizableMixin {
public:
    enum Indices : uint32_t {
        INPUT_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::TAGGED;
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::IS_NOT_IDEMPOTENT |
        VertexPropertyFlag::CAN_EAGER_DEOPT;

    explicit CheckedNonNegativeI32ToTaggedIntVertex(Chunk *chunk, uint32_t bytecodeOffset)
        : VertexMixin(), EagerDeoptimizableMixin(chunk, bytecodeOffset)
    {}

    void SetValueLocationConstraints();
    void VerifyInputs() const
    {
        ASSERT(GetInputCount() == NUM_INPUTS);
        ASSERT(GetInput(INPUT_INDEX)->GetValueRepresentation() == ValueRepresentation::INT32);
    }
};

class I32BNotVertex : public FixedInputVertexMixin<ValueVertex, I32BNotVertex> {
public:
    enum Indices : uint32_t {
        VALUE_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::INT32;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::INT32,
    };
    static constexpr VertexPropertyFlag PROPERTIES = {};

    explicit I32BNotVertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

class I32NegWithOverflowVertex : public VertexMixin<ValueVertex, I32NegWithOverflowVertex>,
                                 public EagerDeoptimizableMixin {
public:
    enum Indices : uint32_t {
        VALUE_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::INT32;
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::IS_NOT_IDEMPOTENT |
        VertexPropertyFlag::CAN_EAGER_DEOPT;

    explicit I32NegWithOverflowVertex(Chunk *chunk, uint32_t bytecodeOffset)
        : VertexMixin(), EagerDeoptimizableMixin(chunk, bytecodeOffset)
    {}

    void SetValueLocationConstraints();
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
    enum Indices : uint32_t {
        VALUE_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::INT32;
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::IS_NOT_IDEMPOTENT |
        VertexPropertyFlag::CAN_EAGER_DEOPT;

    explicit I32IncWithOverflowVertex(Chunk *chunk, uint32_t bytecodeOffset)
        : VertexMixin(), EagerDeoptimizableMixin(chunk, bytecodeOffset)
    {}

    void SetValueLocationConstraints();
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
    enum Indices : uint32_t {
        VALUE_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::INT32;
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::IS_NOT_IDEMPOTENT |
        VertexPropertyFlag::CAN_EAGER_DEOPT;

    explicit I32DecWithOverflowVertex(Chunk *chunk, uint32_t bytecodeOffset)
        : VertexMixin(), EagerDeoptimizableMixin(chunk, bytecodeOffset)
    {}

    void SetValueLocationConstraints();
    void VerifyInputs() const
    {
        VerifyI32UnaryOpInputs();
    }

private:
    void VerifyI32UnaryOpInputs() const;
};

class I32ToF64Vertex : public FixedInputVertexMixin<ValueVertex, I32ToF64Vertex> {
public:
    enum Indices : uint32_t {
        INPUT_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::FLOAT64;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::INT32,
    };
    static constexpr VertexPropertyFlag PROPERTIES = {};

    explicit I32ToF64Vertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

class CheckedNumberToF64Vertex : public VertexMixin<ValueVertex, CheckedNumberToF64Vertex>,
                                 public EagerDeoptimizableMixin {
public:
    enum Indices : uint32_t {
        INPUT_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::FLOAT64;
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::IS_NOT_IDEMPOTENT |
        VertexPropertyFlag::CAN_EAGER_DEOPT;

    explicit CheckedNumberToF64Vertex(Chunk *chunk, uint32_t bytecodeOffset)
        : VertexMixin(), EagerDeoptimizableMixin(chunk, bytecodeOffset)
    {}

    void SetValueLocationConstraints();
    void VerifyInputs() const
    {
        ASSERT(GetInputCount() == NUM_INPUTS);
        ASSERT(GetInput(INPUT_INDEX)->GetValueRepresentation() == ValueRepresentation::TAGGED);
    }
};

class F64ToI32TruncVertex : public FixedInputVertexMixin<ValueVertex, F64ToI32TruncVertex> {
public:
    enum Indices : uint32_t {
        INPUT_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::INT32;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::FLOAT64,
    };
    static constexpr VertexPropertyFlag PROPERTIES = {};

    explicit F64ToI32TruncVertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

class I32ToUint8ClampedVertex : public FixedInputVertexMixin<ValueVertex, I32ToUint8ClampedVertex> {
public:
    enum Indices : uint32_t {
        INPUT_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::INT32;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::INT32,
    };
    static constexpr VertexPropertyFlag PROPERTIES = {};

    explicit I32ToUint8ClampedVertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

class F64ToUint8ClampedVertex : public FixedInputVertexMixin<ValueVertex, F64ToUint8ClampedVertex> {
public:
    enum Indices : uint32_t {
        INPUT_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::INT32;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::FLOAT64,
    };
    static constexpr VertexPropertyFlag PROPERTIES = {};

    explicit F64ToUint8ClampedVertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

class DoubleToInt32CallVertex : public FixedInputVertexMixin<ValueVertex, DoubleToInt32CallVertex> {
public:
    enum Indices : uint32_t {
        INPUT_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::INT32;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::FLOAT64,
    };
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::IS_CALL;

    explicit DoubleToInt32CallVertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

class F64ToTaggedDoubleVertex : public FixedInputVertexMixin<ValueVertex, F64ToTaggedDoubleVertex> {
public:
    enum Indices : uint32_t {
        INPUT_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::TAGGED;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::FLOAT64,
    };
    static constexpr VertexPropertyFlag PROPERTIES = {};

    explicit F64ToTaggedDoubleVertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

class F64NegVertex : public FixedInputVertexMixin<ValueVertex, F64NegVertex> {
public:
    enum Indices : uint32_t {
        VALUE_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::FLOAT64;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::FLOAT64,
    };
    static constexpr VertexPropertyFlag PROPERTIES = {};

    explicit F64NegVertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

class F64AddVertex : public FixedInputVertexMixin<ValueVertex, F64AddVertex> {
public:
    enum Indices : uint32_t {
        LEFT_INDEX = 0,
        RIGHT_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::FLOAT64;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::FLOAT64,
        ValueRepresentation::FLOAT64,
    };
    static constexpr VertexPropertyFlag PROPERTIES = {};

    explicit F64AddVertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

class F64SubVertex : public FixedInputVertexMixin<ValueVertex, F64SubVertex> {
public:
    enum Indices : uint32_t {
        LEFT_INDEX = 0,
        RIGHT_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::FLOAT64;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::FLOAT64,
        ValueRepresentation::FLOAT64,
    };
    static constexpr VertexPropertyFlag PROPERTIES = {};

    explicit F64SubVertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

class F64MulVertex : public FixedInputVertexMixin<ValueVertex, F64MulVertex> {
public:
    enum Indices : uint32_t {
        LEFT_INDEX = 0,
        RIGHT_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::FLOAT64;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::FLOAT64,
        ValueRepresentation::FLOAT64,
    };
    static constexpr VertexPropertyFlag PROPERTIES = {};

    explicit F64MulVertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

class F64DivVertex : public FixedInputVertexMixin<ValueVertex, F64DivVertex> {
public:
    enum Indices : uint32_t {
        LEFT_INDEX = 0,
        RIGHT_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::FLOAT64;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::FLOAT64,
        ValueRepresentation::FLOAT64,
    };
    static constexpr VertexPropertyFlag PROPERTIES = {};

    explicit F64DivVertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

//==============================================================================
// Load/Store Vertices
//==============================================================================

class DeoptIfHClassMismatchVertex : public VertexMixin<NonControlVertex, DeoptIfHClassMismatchVertex>,
                                    public EagerDeoptimizableMixin {
public:
    enum Indices : uint32_t {
        RECEIVER_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::CAN_EAGER_DEOPT |
        VertexPropertyFlag::CAN_READ;

    explicit DeoptIfHClassMismatchVertex(Chunk *chunk,
                                         JSHClass *expectedHClass,
                                         uint32_t bytecodeOffset)
        : VertexMixin(),
          EagerDeoptimizableMixin(chunk, bytecodeOffset),
          expectedHClass_(expectedHClass)
    {}

    JSHClass *GetExpectedHClass() const
    {
        return expectedHClass_;
    }

    void SetValueLocationConstraints();

    void VerifyInputs() const
    {
        ASSERT(expectedHClass_ != nullptr);
        ASSERT(GetInputCount() == NUM_INPUTS);
    }

private:
    JSHClass *expectedHClass_;
};

class DeoptIfHClassNotInVertex : public VertexMixin<NonControlVertex, DeoptIfHClassNotInVertex>,
                                 public EagerDeoptimizableMixin {
public:
    enum Indices : uint32_t {
        RECEIVER_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_EAGER_DEOPT | VertexPropertyFlag::CAN_READ;

    explicit DeoptIfHClassNotInVertex(Chunk *chunk,
                                      std::vector<JSHClass *> expectedHClasses,
                                      uint32_t bytecodeOffset)
        : VertexMixin(),
          EagerDeoptimizableMixin(chunk, bytecodeOffset),
          expectedHClasses_(std::move(expectedHClasses))
    {}

    const std::vector<JSHClass *> &GetExpectedHClasses() const
    {
        return expectedHClasses_;
    }

    void SetValueLocationConstraints();

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
    enum Indices : uint32_t {
        RECEIVER_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_EAGER_DEOPT | VertexPropertyFlag::CAN_READ;
    DeoptIfPrototypeChangedVertex(Chunk *chunk, bool checkProtoChangeMarker,
                                  bool checkNotPrototype, uint32_t bytecodeOffset)
        : VertexMixin(),
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

    void VerifyInputs() const
    {
        ASSERT(checkProtoChangeMarker_ || checkNotPrototype_);
        ASSERT(GetInputCount() == RECEIVER_INDEX + 1);
    }

private:
    bool checkProtoChangeMarker_ {false};
    bool checkNotPrototype_ {false};
};

class DeoptIfTaggedConditionVertex : public VertexMixin<NonControlVertex, DeoptIfTaggedConditionVertex>,
                                     public EagerDeoptimizableMixin,
                                     public ConditionMixin {
public:
    enum Indices : uint32_t {
        LEFT_INDEX = 0,
        RIGHT_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_EAGER_DEOPT;

    explicit DeoptIfTaggedConditionVertex(Chunk *chunk, uint32_t bytecodeOffset,
                                          Condition condition, kungfu::DeoptType deoptType)
        : VertexMixin(),
          EagerDeoptimizableMixin(chunk, bytecodeOffset),
          ConditionMixin(condition),
          deoptType_(deoptType)
    {
        ASSERT(condition == Condition::EQUAL || condition == Condition::NOT_EQUAL);
    }

    kungfu::DeoptType GetDeoptType() const
    {
        return deoptType_;
    }

    void SetValueLocationConstraints();

    void VerifyInputs() const
    {
        ASSERT(GetInputCount() == NUM_INPUTS);
        ASSERT(GetInput(LEFT_INDEX)->GetValueRepresentation() == ValueRepresentation::TAGGED);
        ASSERT(GetInput(RIGHT_INDEX)->GetValueRepresentation() == ValueRepresentation::TAGGED);
    }

private:
    kungfu::DeoptType deoptType_;
};

class DeoptIfInt32ConditionVertex : public VertexMixin<NonControlVertex, DeoptIfInt32ConditionVertex>,
                                    public EagerDeoptimizableMixin,
                                    public ConditionMixin {
public:
    enum Indices : uint32_t {
        LEFT_INDEX = 0,
        RIGHT_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_EAGER_DEOPT;

    explicit DeoptIfInt32ConditionVertex(Chunk *chunk, uint32_t bytecodeOffset,
                                         Condition condition, kungfu::DeoptType deoptType)
        : VertexMixin(),
          EagerDeoptimizableMixin(chunk, bytecodeOffset),
          ConditionMixin(condition),
          deoptType_(deoptType)
    {}

    kungfu::DeoptType GetDeoptType() const
    {
        return deoptType_;
    }

    void SetValueLocationConstraints();

    void VerifyInputs() const
    {
        ASSERT(GetInputCount() == NUM_INPUTS);
        ASSERT(GetInput(LEFT_INDEX)->GetValueRepresentation() == ValueRepresentation::INT32);
        ASSERT(GetInput(RIGHT_INDEX)->GetValueRepresentation() == ValueRepresentation::INT32);
    }

private:
    kungfu::DeoptType deoptType_;
};

class DeoptIfFloat64ConditionVertex : public VertexMixin<NonControlVertex, DeoptIfFloat64ConditionVertex>,
                                      public EagerDeoptimizableMixin,
                                      public ConditionMixin {
public:
    enum Indices : uint32_t {
        LEFT_INDEX = 0,
        RIGHT_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_EAGER_DEOPT;

    explicit DeoptIfFloat64ConditionVertex(Chunk *chunk, uint32_t bytecodeOffset,
                                           Condition condition, kungfu::DeoptType deoptType)
        : VertexMixin(),
          EagerDeoptimizableMixin(chunk, bytecodeOffset),
          ConditionMixin(condition),
          deoptType_(deoptType)
    {}

    kungfu::DeoptType GetDeoptType() const
    {
        return deoptType_;
    }

    void SetValueLocationConstraints();

    void VerifyInputs() const
    {
        ASSERT(GetInputCount() == NUM_INPUTS);
        ASSERT(GetInput(LEFT_INDEX)->GetValueRepresentation() == ValueRepresentation::FLOAT64);
        ASSERT(GetInput(RIGHT_INDEX)->GetValueRepresentation() == ValueRepresentation::FLOAT64);
    }

private:
    kungfu::DeoptType deoptType_;
};

class DeoptIfNotNumberVertex : public VertexMixin<NonControlVertex, DeoptIfNotNumberVertex>,
                               public EagerDeoptimizableMixin {
public:
    enum Indices : uint32_t {
        VALUE_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_EAGER_DEOPT;

    explicit DeoptIfNotNumberVertex(Chunk *chunk, uint32_t bytecodeOffset)
        : VertexMixin(),
          EagerDeoptimizableMixin(chunk, bytecodeOffset)
    {}

    void SetValueLocationConstraints();

    void VerifyInputs() const
    {
        ASSERT(GetInputCount() == NUM_INPUTS);
        ASSERT(GetInput(VALUE_INDEX)->GetValueRepresentation() == ValueRepresentation::TAGGED);
    }
};

class DeoptIfNotHeapObjectVertex : public VertexMixin<NonControlVertex, DeoptIfNotHeapObjectVertex>,
                                   public EagerDeoptimizableMixin {
public:
    enum Indices : uint32_t {
        VALUE_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_EAGER_DEOPT;

    explicit DeoptIfNotHeapObjectVertex(Chunk *chunk, uint32_t bytecodeOffset)
        : VertexMixin(), EagerDeoptimizableMixin(chunk, bytecodeOffset)
    {}

    void SetValueLocationConstraints();

    void VerifyInputs() const
    {
        ASSERT(GetInputCount() == NUM_INPUTS);
        ASSERT(GetInput(VALUE_INDEX)->GetValueRepresentation() == ValueRepresentation::TAGGED);
    }
};

class DeoptIfArrayBufferDetachedVertex
    : public VertexMixin<NonControlVertex, DeoptIfArrayBufferDetachedVertex>,
      public EagerDeoptimizableMixin {
public:
    enum Indices : uint32_t {
        RECEIVER_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::CAN_EAGER_DEOPT |
        VertexPropertyFlag::CAN_READ;

    DeoptIfArrayBufferDetachedVertex(Chunk *chunk, uint32_t bytecodeOffset, OnHeapMode onHeapMode)
        : VertexMixin(), EagerDeoptimizableMixin(chunk, bytecodeOffset), onHeapMode_(onHeapMode)
    {
        ASSERT(!OnHeap::IsOnHeap(onHeapMode_));
    }

    OnHeapMode GetOnHeapMode() const
    {
        return onHeapMode_;
    }

    void SetValueLocationConstraints();

    void VerifyInputs() const
    {
        ASSERT(GetInputCount() == NUM_INPUTS);
        ASSERT(GetInput(RECEIVER_INDEX)->GetValueRepresentation() == ValueRepresentation::TAGGED);
    }

private:
    OnHeapMode onHeapMode_ {OnHeapMode::NONE};
};

class DeoptIfCOWElementsVertex : public VertexMixin<NonControlVertex, DeoptIfCOWElementsVertex>,
                                 public EagerDeoptimizableMixin {
public:
    enum Indices : uint32_t {
        ELEMENTS_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::CAN_EAGER_DEOPT |
        VertexPropertyFlag::CAN_READ;

    explicit DeoptIfCOWElementsVertex(Chunk *chunk, uint32_t bytecodeOffset)
        : VertexMixin(), EagerDeoptimizableMixin(chunk, bytecodeOffset)
    {}

    void SetValueLocationConstraints();

    void VerifyInputs() const
    {
        ASSERT(GetInputCount() == NUM_INPUTS);
        ASSERT(GetInput(ELEMENTS_INDEX)->GetValueRepresentation() == ValueRepresentation::TAGGED);
    }
};

class DeoptIfElementsUnstableVertex : public VertexMixin<NonControlVertex, DeoptIfElementsUnstableVertex>,
                                      public EagerDeoptimizableMixin {
public:
    enum Indices : uint32_t {
        RECEIVER_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::CAN_EAGER_DEOPT |
        VertexPropertyFlag::CAN_READ;

    explicit DeoptIfElementsUnstableVertex(Chunk *chunk, uint32_t bytecodeOffset)
        : VertexMixin(), EagerDeoptimizableMixin(chunk, bytecodeOffset)
    {}

    void SetValueLocationConstraints();

    void VerifyInputs() const
    {
        ASSERT(GetInputCount() == NUM_INPUTS);
        ASSERT(GetInput(RECEIVER_INDEX)->GetValueRepresentation() == ValueRepresentation::TAGGED);
    }
};

class DeoptVertex : public FixedInputVertexMixin<ControlVertex, DeoptVertex>, public EagerDeoptimizableMixin {
public:
    static constexpr uint32_t NUM_INPUTS = 0;
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_EAGER_DEOPT;

    explicit DeoptVertex(Chunk *chunk,
                         kungfu::DeoptType type,
                         uint32_t bytecodeOffset)
        : FixedInputVertexMixin(),
          EagerDeoptimizableMixin(chunk, bytecodeOffset),
          deoptType_(type)
    {}

    kungfu::DeoptType GetDeoptType() const
    {
        return deoptType_;
    }

    void SetValueLocationConstraints();

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
    explicit UnconditionalControlVertex(BB *target)
        : ControlVertex(), target_(target), predecessorId_(0)
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
class UnconditionalControlVertexT : public FixedInputVertexMixin<UnconditionalControlVertex, Derived> {
protected:
    UnconditionalControlVertexT(BB *target)
        : FixedInputVertexMixin<UnconditionalControlVertex, Derived>(target)
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
    BranchControlVertex(BB *ifTrue, BB *ifFalse)
        : ControlVertex(), ifTrue_(ifTrue), ifFalse_(ifFalse)
    {}

private:
    BB *ifTrue_;
    BB *ifFalse_;
};

/**
 * BranchControlVertexT - CRTP mixin for conditional control vertices
 * Provides compile-time type checking and branch target management.
 */
template <typename Derived>
class BranchControlVertexT : public FixedInputVertexMixin<BranchControlVertex, Derived> {
protected:
    BranchControlVertexT(BB *ifTrue, BB *ifFalse)
        : FixedInputVertexMixin<BranchControlVertex, Derived>(ifTrue, ifFalse)
    {}
};

/**
 * BranchIfTrue vertex - conditional branch based on boolean true value
 * Jumps to if_true if the condition is exactly the boolean true value,
 * otherwise jumps to if_false
 */
class BranchIfTrueVertex : public BranchControlVertexT<BranchIfTrueVertex> {
public:
    static constexpr uint32_t NUM_INPUTS = 1;
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr VertexPropertyFlag PROPERTIES = {};

    BranchIfTrueVertex(BB *ifTrue, BB *ifFalse)
        : BranchControlVertexT(ifTrue, ifFalse)
    {}

    void SetValueLocationConstraints();
};

class BranchIfTaggedStringVertex : public BranchControlVertexT<BranchIfTaggedStringVertex> {
public:
    enum Indices : uint32_t {
        VALUE_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::TAGGED,
    };
    static constexpr VertexPropertyFlag PROPERTIES = {};

    BranchIfTaggedStringVertex(BB *ifTrue, BB *ifFalse)
        : BranchControlVertexT(ifTrue, ifFalse)
    {}

    void SetValueLocationConstraints();
};

class BranchIfHClassInVertex : public BranchControlVertexT<BranchIfHClassInVertex> {
public:
    enum Indices : uint32_t {
        RECEIVER_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::TAGGED,
    };
    static constexpr VertexPropertyFlag PROPERTIES = VertexPropertyFlag::CAN_READ;

    BranchIfHClassInVertex(BB *ifTrue, BB *ifFalse,
                           std::vector<JSHClass *> expectedHClasses)
        : BranchControlVertexT(ifTrue, ifFalse), expectedHClasses_(std::move(expectedHClasses))
    {}

    const std::vector<JSHClass *> &GetExpectedHClasses() const
    {
        return expectedHClasses_;
    }

    void SetValueLocationConstraints();

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

class BranchIfInt32CompareVertex : public BranchControlVertexT<BranchIfInt32CompareVertex>,
                                   public ConditionMixin {
public:
    enum Indices : uint32_t {
        LEFT_INDEX = 0,
        RIGHT_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::INT32,
        ValueRepresentation::INT32,
    };
    static constexpr VertexPropertyFlag PROPERTIES = {};

    BranchIfInt32CompareVertex(BB *ifTrue, BB *ifFalse, Condition condition)
        : BranchControlVertexT(ifTrue, ifFalse), ConditionMixin(condition)
    {}

    void SetValueLocationConstraints();
};

class BranchIfInt64CompareVertex : public BranchControlVertexT<BranchIfInt64CompareVertex>,
                                   public ConditionMixin {
public:
    enum Indices : uint32_t {
        LEFT_INDEX = 0,
        RIGHT_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::INT64,
        ValueRepresentation::INT64,
    };
    static constexpr VertexPropertyFlag PROPERTIES = {};

    BranchIfInt64CompareVertex(BB *ifTrue, BB *ifFalse, Condition condition)
        : BranchControlVertexT(ifTrue, ifFalse), ConditionMixin(condition)
    {}

    void SetValueLocationConstraints();
};

class BranchIfFloat64CompareVertex : public BranchControlVertexT<BranchIfFloat64CompareVertex>,
                                     public ConditionMixin {
public:
    enum Indices : uint32_t {
        LEFT_INDEX = 0,
        RIGHT_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::FLOAT64,
        ValueRepresentation::FLOAT64,
    };
    static constexpr VertexPropertyFlag PROPERTIES = {};

    BranchIfFloat64CompareVertex(BB *ifTrue, BB *ifFalse, Condition condition)
        : BranchControlVertexT(ifTrue, ifFalse), ConditionMixin(condition)
    {}

    void SetValueLocationConstraints();
};

class BranchIfReferenceEqualVertex : public BranchControlVertexT<BranchIfReferenceEqualVertex> {
public:
    enum Indices : uint32_t {
        LEFT_INDEX = 0,
        RIGHT_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::TAGGED,
        ValueRepresentation::TAGGED,
    };
    static constexpr VertexPropertyFlag PROPERTIES = {};

    BranchIfReferenceEqualVertex(BB *ifTrue, BB *ifFalse)
        : BranchControlVertexT(ifTrue, ifFalse)
    {}

    void SetValueLocationConstraints();
};

class BranchIfObjectTypeVertex : public BranchControlVertexT<BranchIfObjectTypeVertex> {
public:
    enum Indices : uint32_t {
        VALUE_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::TAGGED,
    };
    static constexpr VertexPropertyFlag PROPERTIES = {};

    BranchIfObjectTypeVertex(BB *ifTrue, BB *ifFalse, JSType expectedType)
        : BranchControlVertexT(ifTrue, ifFalse), expectedType_(expectedType)
    {}

    JSType GetExpectedType() const
    {
        return expectedType_;
    }

    void SetValueLocationConstraints();

private:
    JSType expectedType_;
};

/**
 * BranchIfTaggedHeapObject vertex - branch based on whether a tagged value is a heap object.
 * Jumps to ifTrue if the value is a tagged heap object (tag bits == 0),
 * otherwise jumps to ifFalse.
 */
class BranchIfTaggedHeapObjectVertex : public BranchControlVertexT<BranchIfTaggedHeapObjectVertex> {
public:
    enum Indices : uint32_t {
        VALUE_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::TAGGED,
    };
    static constexpr VertexPropertyFlag PROPERTIES = {};

    BranchIfTaggedHeapObjectVertex(BB *ifTrue, BB *ifFalse)
        : BranchControlVertexT(ifTrue, ifFalse)
    {}

    void SetValueLocationConstraints();
};

/**
 * Jump vertex - unconditional branch
 */
class JumpVertex : public UnconditionalControlVertexT<JumpVertex> {
public:
    static constexpr uint32_t NUM_INPUTS = 0;
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr VertexPropertyFlag PROPERTIES = {};

    JumpVertex(BB *target) : UnconditionalControlVertexT(target) {}

    void SetValueLocationConstraints();
};

/**
 * JumpLoop vertex - unconditional branch to loop header (back edge)
 */
class JumpLoopVertex : public UnconditionalControlVertexT<JumpLoopVertex> {
public:
    static constexpr uint32_t NUM_INPUTS = 0;
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr VertexPropertyFlag PROPERTIES = {};

    explicit JumpLoopVertex(Chunk *chunk, BB *target)
        : UnconditionalControlVertexT(target), usedVertices_(chunk) {}

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
class ReturnVertex : public FixedInputVertexMixin<ControlVertex, ReturnVertex> {
public:
    static constexpr uint32_t NUM_INPUTS = 1;
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr VertexPropertyFlag PROPERTIES = {};

    explicit ReturnVertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

/**
 * Throw vertex - exception throw
 */
class ThrowVertex : public VertexMixin<ControlVertex, ThrowVertex>, public ThrowableMixin, public RuntimeStubIDMixin {
public:
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr VertexPropertyFlag PROPERTIES =
        VertexPropertyFlag::CAN_THROW |
        VertexPropertyFlag::IS_CALL |
        VertexPropertyFlag::IS_NOT_IDEMPOTENT;

    explicit ThrowVertex(kungfu::RuntimeStubCSigns::ID id)
        : VertexMixin(), RuntimeStubIDMixin(id)
    {}

    size_t GetArgCount() const
    {
        return GetInputCount();
    }

    void SetValueLocationConstraints();

    // VerifyInputs: Variable-input vertex
    void VerifyInputs() const {}
};

class GapMoveVertex : public FixedInputVertexMixin<NonControlVertex, GapMoveVertex> {
public:
    static constexpr uint32_t NUM_INPUTS = 0;
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr VertexPropertyFlag PROPERTIES = {};

    explicit GapMoveVertex(AllocatedState source, AllocatedState target)
        : FixedInputVertexMixin(), source_(source), target_(target)
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

private:
    AllocatedState source_;
    AllocatedState target_;
};

class ConstantGapMoveVertex : public FixedInputVertexMixin<NonControlVertex, ConstantGapMoveVertex> {
public:
    static constexpr uint32_t NUM_INPUTS = 0;
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::NONE;
    static constexpr VertexPropertyFlag PROPERTIES = {};

    explicit ConstantGapMoveVertex(ValueVertex *vertex, AllocatedState target)
        : FixedInputVertexMixin(), vertex_(vertex), target_(target)
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

private:
    ValueVertex *vertex_;
    AllocatedState target_;
};

/**
 * Phi vertex - SSA merge point for values from different predecessor blocks
 */
class PhiVertex : public VertexMixin<ValueVertex, PhiVertex> {
public:
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::TAGGED;
    static constexpr VertexPropertyFlag PROPERTIES = {};

    explicit PhiVertex(VirtualRegister owner)
        : VertexMixin(), owner_(owner)
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

class I32ToTaggedIntVertex : public FixedInputVertexMixin<ValueVertex, I32ToTaggedIntVertex> {
public:
    enum Indices : uint32_t {
        INPUT_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::TAGGED;
    static constexpr VertexPropertyFlag PROPERTIES = {};

    explicit I32ToTaggedIntVertex() : FixedInputVertexMixin() {}

    const ValueVertex *GetInputValue() const
    {
        return GetInput(INPUT_INDEX);
    }

    void SetValueLocationConstraints();
};

class RawI64ToTaggedVertex : public FixedInputVertexMixin<ValueVertex, RawI64ToTaggedVertex> {
public:
    enum Indices : uint32_t {
        INPUT_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::TAGGED;
    static constexpr VertexPropertyFlag PROPERTIES = {};

    explicit RawI64ToTaggedVertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

class TaggedToRawI64Vertex : public FixedInputVertexMixin<ValueVertex, TaggedToRawI64Vertex> {
public:
    enum Indices : uint32_t {
        INPUT_INDEX = 0,
        NUM_INPUTS = 1,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::INT64;
    static constexpr VertexPropertyFlag PROPERTIES = {};

    explicit TaggedToRawI64Vertex() : FixedInputVertexMixin() {}

    void SetValueLocationConstraints();
};

class I64BitwiseBinaryVertex : public FixedInputVertexMixin<ValueVertex, I64BitwiseBinaryVertex> {
public:
    enum Indices : uint32_t {
        LEFT_INDEX = 0,
        RIGHT_INDEX = 1,
        NUM_INPUTS = 2,
    };
    static constexpr ValueRepresentation VALUE_TYPE = ValueRepresentation::INT64;
    static constexpr ValueRepresentationArray<NUM_INPUTS> INPUT_TYPES = {
        ValueRepresentation::INT64,
        ValueRepresentation::INT64,
    };
    static constexpr VertexPropertyFlag PROPERTIES = {};

    explicit I64BitwiseBinaryVertex(IntBitwiseKind kind)
        : FixedInputVertexMixin(), kind_(kind)
    {}

    IntBitwiseKind GetKind() const
    {
        return kind_;
    }

    void SetValueLocationConstraints();

private:
    IntBitwiseKind kind_;
};

#define CASE(type) \
    case VertexOpcode::type:  \
        return getter(vertex->Cast<type##Vertex>());

#define MIXIN_GETTER(MixinType)                                                         \
    inline MixinType *MixinType##Of(Vertex *vertex)                                     \
    {                                                                                   \
        auto getter = [](auto *vertex) -> MixinType * {                                 \
            using VertexT = std::remove_cv_t<std::remove_pointer_t<decltype(vertex)>>;  \
            if constexpr (std::is_base_of_v<MixinType, VertexT>) {                      \
                return static_cast<MixinType *>(vertex);                                \
            } else {                                                                    \
                return nullptr;                                                         \
            }                                                                           \
        };                                                                              \
        switch (vertex->GetOpcode()) {                                                  \
            ALL_VERTEX_LIST(CASE)                                                       \
            default:                                                                    \
                return nullptr;                                                         \
        }                                                                               \
    }                                                                                   \
    inline const MixinType *MixinType##Of(const Vertex *vertex)                         \
    {                                                                                   \
        auto getter = [](const auto *vertex) -> const MixinType * {                     \
            using VertexT = std::remove_cv_t<std::remove_pointer_t<decltype(vertex)>>;  \
            if constexpr (std::is_base_of_v<MixinType, VertexT>) {                      \
                return static_cast<const MixinType *>(vertex);                          \
            } else {                                                                    \
                return nullptr;                                                         \
            }                                                                           \
        };                                                                              \
        switch (vertex->GetOpcode()) {                                                  \
            ALL_VERTEX_LIST(CASE)                                                       \
            default:                                                                    \
                return nullptr;                                                         \
        }                                                                               \
    }

MIXIN_GETTER(ThrowableMixin)
MIXIN_GETTER(EagerDeoptimizableMixin)
MIXIN_GETTER(LazyDeoptimizableMixin)
#undef CASE

inline BB *CatchBlockOf(Vertex *vertex)
{
    ThrowableMixin *mixin = ThrowableMixinOf(vertex);
    return mixin != nullptr ? mixin->GetCatchBlock() : nullptr;
}

inline uint32_t CatchPredecessorIndexOf(Vertex *vertex)
{
    ThrowableMixin *mixin = ThrowableMixinOf(vertex);
    return mixin != nullptr ? mixin->GetCatchPredecessorIndex() : static_cast<uint32_t>(-1);
}

inline bool HasExceptionLazyDeopt(const Vertex *vertex)
{
    const ThrowableMixin *mixin = ThrowableMixinOf(vertex);
    return mixin != nullptr && mixin->HasExceptionLazyDeopt();
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
