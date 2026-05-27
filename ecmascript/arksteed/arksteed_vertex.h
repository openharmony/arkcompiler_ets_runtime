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

#ifndef ECMASCRIPT_ARKSTEED_VERTEX_H
#define ECMASCRIPT_ARKSTEED_VERTEX_H

#include "common_interfaces/base/bit_field.h"
#include "ecmascript/arksteed/arksteed_opcode_list.h"
#include "ecmascript/arksteed/arksteed_regalloc_types.h"
#include "ecmascript/arksteed/arksteed_regalloc_vertex_info.h"
#include "ecmascript/mem/chunk.h"
#include "ecmascript/mem/native_area_allocator.h"
#include "libpandabase/macros.h"
#include "libpandabase/mem/arena_allocator.h"

namespace panda::ecmascript::arksteed {

class BB;
class Vertex;
class ValueVertex;
class ControlVertex;
class UnconditionalControlVertex;
class BranchControlVertex;
class InputLocation;
class Input;
class ConstInput;

// A singly linked list that temporarily stores predecessors referring to this block.
// All predecessors point to the same target block after Bind() is called.
class BBRef {
public:
    BBRef() : nextRef_(nullptr) {}
    explicit BBRef(BB *basicBlock) : basicBlock_(basicBlock) {}
    explicit BBRef(BBRef *head) : BBRef()
    {
        MoveToListHead(head);
    }

    void Bind(BB *basicBlock)
    {
        BBRef *nextRef = SetToBlockAndReturnNext(basicBlock);
        while (nextRef != nullptr) {
            nextRef = nextRef->SetToBlockAndReturnNext(basicBlock);
        }
    }

    BBRef *MoveToListHead(BBRef *head)
    {
        BBRef *oldNextRef = head->nextRef_;
        nextRef_ = oldNextRef;
        head->nextRef_ = this;
        return oldNextRef;
    }

    BBRef *SetToBlockAndReturnNext(BB *basicBlock)
    {
        BBRef *ref = nextRef_;
        basicBlock_ = basicBlock;
        return ref;
    }

    BBRef *Reset()
    {
        BBRef *ref = nextRef_;
        nextRef_ = nullptr;
        return ref;
    }

    BB *BlockRef() const
    {
        return basicBlock_;
    }

    void SetBlockRef(BB *basicBlock)
    {
        basicBlock_ = basicBlock;
    }

private:
    union {
        BB *basicBlock_;
        BBRef *nextRef_;
    };
};

// ValueRepresentation describes the machine representation of a value
enum class ValueRepresentation : uint8_t {
    TAGGED,         // Tagged pointer to JS objects
    INT32,          // 32-bit signed integer
    UINT32,         // 32-bit unsigned integer
    FLOAT64,        // 64-bit floating point (non-NaN values)
    HOLEY_FLOAT64,  // 64-bit floating point (may contain NaN/holes)
    INT_PTR,        // Pointer-sized integer (depends on architecture)
    NONE,           // No specific representation
};

namespace detail {
template <size_t Size>
class InputTypes {
public:
    constexpr InputTypes() : data_() {}

    template <typename... Args>
    constexpr explicit InputTypes(ValueRepresentation first, Args... rest) : data_()
    {
        FillArray<0>(data_, first, rest...);
        static_assert(1 + sizeof...(rest) == Size);
    }

    constexpr ValueRepresentation operator[](size_t index) const
    {
        return data_[index];
    }

    constexpr size_t size() const
    {
        return Size;
    }

private:
    ValueRepresentation data_[Size];

    template <size_t Index>
    constexpr void FillArray(ValueRepresentation *arr, ValueRepresentation value) const
    {
        arr[Index] = value;
    }

    template <size_t Index, typename... Rest>
    constexpr void FillArray(ValueRepresentation *arr, ValueRepresentation first, Rest... rest) const
    {
        arr[Index] = first;
        FillArray<Index + 1>(arr, rest...);
    }
};
}  // namespace detail

enum class DeoptInfo : uint8_t { NONE = 0, EAGER = 1, LAZY = 2, CHECKPOINT = 3 };

// Vertex operation properties using common::BitField
class VertexProperties {
public:
    using DeoptInfoBit = common::BitField<DeoptInfo, 0, 2>;  // 2: DeoptInfo bit width
    using CanThrowBit = DeoptInfoBit::NextField<bool, 1>;
    using CanReadBit = CanThrowBit::NextField<bool, 1>;
    using CanWriteBit = CanReadBit::NextField<bool, 1>;
    using CanAllocateBit = CanWriteBit::NextField<bool, 1>;
    using NotIdempotentBit = CanAllocateBit::NextField<bool, 1>;
    // 3: ValueRepresentation bit width
    using ValueRepresentationBit = NotIdempotentBit::NextField<ValueRepresentation, 3>;
    using IsConversionBit = ValueRepresentationBit::NextField<bool, 1>;
    using NeedsRegSnapshotBit = IsConversionBit::NextField<bool, 1>;
    using IsCallBit = NeedsRegSnapshotBit::NextField<bool, 1>;
    using IsDeferredCallBit = IsCallBit::NextField<bool, 1>;

    constexpr bool IsDeoptCheckpoint() const
    {
        return DeoptInfoBit::Decode(bitfield_) == DeoptInfo::CHECKPOINT;
    }

    constexpr bool CanEagerDeopt() const
    {
        return DeoptInfoBit::Decode(bitfield_) == DeoptInfo::EAGER;
    }

    constexpr bool CanLazyDeopt() const
    {
        return DeoptInfoBit::Decode(bitfield_) == DeoptInfo::LAZY;
    }

    constexpr bool CanDeopt() const
    {
        DeoptInfo info = DeoptInfoBit::Decode(bitfield_);
        return info == DeoptInfo::EAGER || info == DeoptInfo::LAZY;
    }

    constexpr bool CanThrow() const
    {
        return CanThrowBit::Decode(bitfield_) && CanLazyDeopt();
    }

    constexpr bool IsCall() const
    {
        return IsCallBit::Decode(bitfield_);
    }

    constexpr bool IsDeferredCall() const
    {
        return IsDeferredCallBit::Decode(bitfield_);
    }

    constexpr bool IsAnyCall() const
    {
        return IsCall() || IsDeferredCall();
    }

    constexpr bool CanRead() const
    {
        return CanReadBit::Decode(bitfield_);
    }

    constexpr bool CanWrite() const
    {
        return CanWriteBit::Decode(bitfield_);
    }

    constexpr bool CanAllocate() const
    {
        return CanAllocateBit::Decode(bitfield_);
    }

    constexpr bool IsNotIdempotent() const
    {
        return NotIdempotentBit::Decode(bitfield_);
    }

    constexpr bool MayHasSideEffect() const
    {
        uint32_t mask = static_cast<uint32_t>(CanReadBit::Mask() | CanWriteBit::Mask() | CanAllocateBit::Mask());
        return (bitfield_ & mask) != 0;
    }

    constexpr bool IsConversion() const
    {
        return IsConversionBit::Decode(bitfield_);
    }

    constexpr ValueRepresentation GetValueRepresentation() const
    {
        return ValueRepresentationBit::Decode(bitfield_);
    }

    constexpr bool NeedsRegisterSnapshot() const
    {
        return NeedsRegSnapshotBit::Decode(bitfield_);
    }

    // Factory methods for creating properties
    static constexpr VertexProperties Pure()
    {
        return VertexProperties(0);
    }

    static constexpr VertexProperties EagerDeopt()
    {
        return VertexProperties(DeoptInfoBit::Encode(DeoptInfo::EAGER));
    }

    static constexpr VertexProperties LazyDeopt()
    {
        return VertexProperties(DeoptInfoBit::Encode(DeoptInfo::LAZY));
    }

    static constexpr VertexProperties DeoptCheckpoint()
    {
        return VertexProperties(DeoptInfoBit::Encode(DeoptInfo::CHECKPOINT));
    }

    static constexpr VertexProperties CanThrowProp()
    {
        return VertexProperties(CanThrowBit::Encode(true));
    }

    static constexpr VertexProperties CanReadProp()
    {
        return VertexProperties(CanReadBit::Encode(true));
    }

    static constexpr VertexProperties CanWriteProp()
    {
        return VertexProperties(CanWriteBit::Encode(true));
    }

    static constexpr VertexProperties CanAllocateProp()
    {
        return VertexProperties(CanAllocateBit::Encode(true));
    }

    static constexpr VertexProperties NotIdempotent()
    {
        return VertexProperties(NotIdempotentBit::Encode(true));
    }

    static constexpr VertexProperties TaggedValue()
    {
        return VertexProperties(ValueRepresentationBit::Encode(ValueRepresentation::TAGGED));
    }

    static constexpr VertexProperties Int32()
    {
        return VertexProperties(ValueRepresentationBit::Encode(ValueRepresentation::INT32));
    }

    static constexpr VertexProperties Uint32()
    {
        return VertexProperties(ValueRepresentationBit::Encode(ValueRepresentation::UINT32));
    }

    static constexpr VertexProperties Float64()
    {
        return VertexProperties(ValueRepresentationBit::Encode(ValueRepresentation::FLOAT64));
    }

    static constexpr VertexProperties IntPtr()
    {
        return VertexProperties(ValueRepresentationBit::Encode(ValueRepresentation::INT_PTR));
    }

    static constexpr VertexProperties ConversionVertex()
    {
        return VertexProperties(IsConversionBit::Encode(true));
    }

    static constexpr VertexProperties AnySideEffects()
    {
        return CanReadProp() | CanWriteProp() | CanAllocateProp();
    }

    static constexpr VertexProperties DeferredCall()
    {
        return VertexProperties(NeedsRegSnapshotBit::Encode(true));
    }

    static constexpr VertexProperties Call()
    {
        return VertexProperties(IsCallBit::Encode(true));
    }

    static constexpr VertexProperties CanCallUserCode()
    {
        return AnySideEffects() | LazyDeopt() | VertexProperties(CanThrowBit::Encode(true));
    }

    static constexpr VertexProperties JsCall()
    {
        return Call() | CanCallUserCode();
    }

    constexpr VertexProperties WithNewValueRepresentation(ValueRepresentation newRepr) const
    {
        return VertexProperties(ValueRepresentationBit::Update(bitfield_, newRepr));
    }

    constexpr VertexProperties WithoutDeopt() const
    {
        uint16_t newBitfield = DeoptInfoBit::Update(bitfield_, DeoptInfo::NONE);
        return VertexProperties(newBitfield);
    }

    constexpr VertexProperties operator|(VertexProperties other) const
    {
        return VertexProperties(bitfield_ | other.bitfield_);
    }

    constexpr explicit VertexProperties(uint16_t bitfield) : bitfield_(bitfield) {}

    constexpr operator uint16_t() const
    {
        return bitfield_;
    }

private:
    uint16_t bitfield_;
};

// Check if opcode is a value vertex
inline constexpr bool IsValueVertex(VertexOpcode opcode)
{
    switch (opcode) {
#define CASE(type) case VertexOpcode::type:
        VALUE_VERTEX_LIST(CASE)
#undef CASE
        return true;
        default:
            return false;
    }
}

// Check if opcode is a control vertex
inline constexpr bool IsControlVertex(VertexOpcode opcode)
{
    switch (opcode) {
#define CASE(type) case VertexOpcode::type:
        CONTROL_VERTEX_LIST(CASE)
#undef CASE
        return true;
        default:
            return false;
    }
}

inline constexpr bool IsUnconditionalControlVertex(VertexOpcode opcode)
{
    return opcode >= FIRST_UNCONDITIONAL_CONTROL_VERTEX_OPCODE && opcode <= LAST_UNCONDITIONAL_CONTROL_VERTEX_OPCODE;
}

inline constexpr bool IsBranchControlVertex(VertexOpcode opcode)
{
    switch (opcode) {
#define CASE(type) case VertexOpcode::type:
        BRANCH_CONTROL_VERTEX_LIST(CASE)
#undef CASE
        return true;
        default:
            return false;
    }
}

// Input to a vertex
// to do: The operand index currently serves as a placeholder.
//        When register allocator is implemented, indices will map to physical registers and GraphLabeller will display
//        them as "n<label>:<operand>".
class VertexInput {
public:
    VertexInput(ValueVertex *vertex, int index) : vertex_(vertex), index_(index) {}

    inline ValueVertex *GetVertex() const;
    int GetIndex() const
    {
        return index_;
    }

    bool operator==(const VertexInput &other) const
    {
        return GetVertex() == other.GetVertex() && index_ == other.index_;
    }

private:
    ValueVertex *vertex_;
    int index_;
};

class Vertex {
public:
    NO_COPY_SEMANTIC(Vertex);
    NO_MOVE_SEMANTIC(Vertex);

    static constexpr int MAX_INPUTS = (1 << 16) - 1;  // 16: input count field bit width

    using OpcodeField = common::BitField<VertexOpcode, 0, 16>;  // 16: opcode field bit width
    using InputCountField = OpcodeField::NextField<uint32_t, 16>;  // 16: input count field bit width
    using PropertiesField = InputCountField::NextField<uint32_t, 16>;  // 16: properties field bit width
    using NumTempField = PropertiesField::NextField<uint32_t, 2>;  // 2: NumTemp bit width
    using NumDoubleTempField = NumTempField::NextField<uint32_t, 1>;

    template <typename T>
    static constexpr VertexOpcode opcode_of();

    // Get opcode of this vertex
    constexpr VertexOpcode GetOpcode() const
    {
        return OpcodeField::Decode(bitfield_);
    }

    // Get properties of this vertex
    constexpr VertexProperties GetProperties() const
    {
        uint32_t props = PropertiesField::Decode(bitfield_);
        return VertexProperties(props);
    }

    void SetProperties(VertexProperties properties)
    {
        bitfield_ = PropertiesField::Update(bitfield_, static_cast<uint32_t>(properties));
    }

    constexpr bool IsConversion() const
    {
        return GetProperties().IsConversion();
    }

    template <class T>
    constexpr bool Is() const;

    template <class T>
    constexpr T *Cast()
    {
        ASSERT(Is<T>());
        return static_cast<T *>(this);
    }

    template <class T>
    constexpr const T *Cast() const
    {
        ASSERT(Is<T>());
        return static_cast<const T *>(this);
    }

    template <class T>
    constexpr T *TryCast()
    {
        return Is<T>() ? static_cast<T *>(this) : nullptr;
    }

    template <class T>
    constexpr const T *TryCast() const
    {
        return Is<T>() ? static_cast<const T *>(this) : nullptr;
    }

    // Input management
    constexpr bool HasInputs() const
    {
        return GetInputCount() > 0;
    }

    constexpr int GetInputCount() const
    {
        return static_cast<int>(InputCountField::Decode(bitfield_));
    }

    void SetInput(int index, ValueVertex *vertex);
    ValueVertex *GetInput(int index);
    const ValueVertex *GetInput(int index) const;
    void ClearInput(int index);

    Input Arg(int index);
    ConstInput Arg(int index) const;

    // Input allocation order for register allocation
    // Iterates inputs in the order expected by the register allocator:
    // first fixed register inputs, then arbitrary register inputs, then any inputs
    enum class InputAllocationPolicy {
        FIXED_REGISTER,      // FIXED_REGISTER or FIXED_FP_REGISTER
        ARBITRARY_REGISTER,  // MUST_HAVE_REGISTER
        ANY,                 // MUST_HAVE_SLOT, REGISTER_OR_SLOT, REGISTER_OR_SLOT_OR_CONSTANT
    };

    template <typename Function>
    void ForAllInputsInRegallocAssignmentOrder(Function &&f);

    bool HasId() const
    {
        return regallocInfo_ != nullptr && regallocInfo_->HasId();
    }

    VertexId GetId() const
    {
        ASSERT(regallocInfo_ != nullptr);
        return regallocInfo_->GetId();
    }

    void SetId(VertexId id)
    {
        ASSERT(regallocInfo_ != nullptr);
        regallocInfo_->SetId(id);
    }

    // Temporaries needed for register allocation
    uint32_t GetTemporariesNeeded() const
    {
        return NumTempField::Decode(bitfield_);
    }

    uint32_t GetDoubleTemporariesNeeded() const
    {
        return NumDoubleTempField::Decode(bitfield_);
    }

    template <typename RegisterT>
    uint32_t GetNumTemporariesNeeded() const
    {
        if constexpr (std::is_same_v<RegisterT, ArkSteedRegister>) {
            return GetTemporariesNeeded();
        } else {
            return GetDoubleTemporariesNeeded();
        }
    }

    // Owner block
    void SetOwner(BB *block)
    {
        owner_ = block;
    }

    BB *GetOwner() const
    {
        return owner_;
    }

    // Print for debugging
    void Print() const;

    // Factory method to create vertices
    template <class Derived, typename... Args>
    static Derived *New(Chunk *chunk, size_t inputCount, Args &&...args);

    template <class Derived, typename Container, typename... Args,
              typename std::enable_if<!std::is_integral<Container>::value, int>::type = 0>
    static Derived *New(Chunk *chunk, const Container &inputs, Args &&...args);

    // Reduce input count (used by Phi when merging dead control flow)
    void ReduceInputCount(int num = 1);

    RegallocVertexInfo *GetRegallocInfo() const
    {
        return regallocInfo_;
    }

    void SetRegallocInfo(RegallocVertexInfo *info)
    {
        regallocInfo_ = info;
    }

    InputLocation *GetInputLocation(int predecessorIdx) const
    {
        return GetRegallocInfo()->GetInputLocation(predecessorIdx);
    }

protected:
    explicit Vertex(uint64_t bitfield) : bitfield_(bitfield) {}

    // Allow updating bits from subclasses
    constexpr uint64_t GetBitfield() const
    {
        return bitfield_;
    }

    void SetBitfield(uint64_t newBitfield)
    {
        bitfield_ = newBitfield;
    }

    void SetTemporariesNeeded(uint8_t value)
    {
        bitfield_ = NumTempField::Update(bitfield_, value);
    }

    void SetDoubleTemporariesNeeded(uint8_t value)
    {
        bitfield_ = NumDoubleTempField::Update(bitfield_, value);
    }

private:
    ValueVertex **GetInputBase()
    {
        // Input array is before the Vertex object
        // this points to Vertex, input array is at this - 1 (for input_count elements)
        return reinterpret_cast<ValueVertex **>(this) - 1;
    }

    ValueVertex *const *GetInputBase() const
    {
        // Input array is before the Vertex object
        return reinterpret_cast<ValueVertex *const *>(this) - 1;
    }

    ValueVertex **GetInputPtr(int index)
    {
        // Access input at given index: base is at -1, so input[index] is at -1 - index
        return GetInputBase() - index;
    }

    ValueVertex *const *GetInputPtr(int index) const
    {
        // Access input at given index: base is at -1, so input[index] is at -1 - index
        return GetInputBase() - index;
    }

    uint64_t bitfield_;
    BB *owner_ = nullptr;
    RegallocVertexInfo *regallocInfo_ = nullptr;
};

class NonControlVertex : public Vertex {
protected:
    using Vertex::Vertex;
};

// ValueVertex is a vertex that produces a value
class ValueVertex : public NonControlVertex {
public:
    static constexpr VertexProperties DEFAULT_PROPERTIES = VertexProperties::Pure() | VertexProperties::TaggedValue();

    constexpr MachineRepresentation GetMachineRepresentation() const
    {
        switch (GetValueRepresentation()) {
            case ValueRepresentation::TAGGED:
                return MachineRepresentation::Tagged;
            case ValueRepresentation::INT32:
            case ValueRepresentation::UINT32:
                return MachineRepresentation::Word32;
            case ValueRepresentation::FLOAT64:
            case ValueRepresentation::HOLEY_FLOAT64:
                return MachineRepresentation::Float64;
            case ValueRepresentation::INT_PTR:
                return MachineRepresentation::Word64;
            case ValueRepresentation::NONE:
                return MachineRepresentation::None;
        }
        return MachineRepresentation::None;
    }

    constexpr ValueRepresentation GetValueRepresentation() const
    {
        return GetProperties().GetValueRepresentation();
    }

    bool IsTagged() const
    {
        return GetValueRepresentation() == ValueRepresentation::TAGGED;
    }

    bool IsInt32() const
    {
        return GetValueRepresentation() == ValueRepresentation::INT32;
    }

    bool IsUint32() const
    {
        return GetValueRepresentation() == ValueRepresentation::UINT32;
    }

    bool IsIntPtr() const
    {
        return GetValueRepresentation() == ValueRepresentation::INT_PTR;
    }

    bool IsFloat64() const
    {
        return GetValueRepresentation() == ValueRepresentation::FLOAT64;
    }

    bool IsHoleyFloat64() const
    {
        return GetValueRepresentation() == ValueRepresentation::HOLEY_FLOAT64;
    }

    bool IsAnyFloat64() const
    {
        auto repr = GetValueRepresentation();
        return repr == ValueRepresentation::FLOAT64 || repr == ValueRepresentation::HOLEY_FLOAT64;
    }

    bool IsAnyInt32() const
    {
        auto repr = GetValueRepresentation();
        return repr == ValueRepresentation::INT32 || repr == ValueRepresentation::UINT32;
    }

    ValueLocation &Result();
    const ValueLocation &Result() const;

    RegallocValueVertexInfo *GetRegallocInfo() const
    {
        return static_cast<RegallocValueVertexInfo *>(Vertex::GetRegallocInfo());
    }

protected:
    explicit ValueVertex(uint64_t bitfield) : NonControlVertex(bitfield) {}
};

// ControlVertex is a vertex that affects control flow
class ControlVertex : public Vertex {
public:
    static constexpr VertexProperties DEFAULT_PROPERTIES = VertexProperties::Pure();

protected:
    explicit ControlVertex(uint64_t bitfield) : Vertex(bitfield) {}
};

class Input {
public:
    Input(Vertex *base, int index) : base_(base), index_(index) {}

    ValueVertex *vertex() const
    {
        return base_->GetInput(index_);
    }

    void clear()
    {
        base_->ClearInput(index_);
    }

    InputLocation *GetLocation() const;
    const InstructionOperand &GetOperand() const;

    bool operator==(const Input &other) const
    {
        return vertex() == other.vertex() && GetLocation() == other.GetLocation();
    }

private:
    friend class ConstInput;
    Vertex *base_;
    int index_;
};

class ConstInput {
public:
    ConstInput(const Vertex *base, int index) : base_(base), index_(index) {}

    ConstInput(const Input &input)
    {
        base_ = input.base_;
        index_ = input.index_;
    }

    const ValueVertex *vertex() const
    {
        return const_cast<Vertex *>(base_)->GetInput(index_);
    }

    const InputLocation *GetLocation() const;
    const InstructionOperand &GetOperand() const;

private:
    const Vertex *base_;
    int index_;
};

inline Input Vertex::Arg(int index)
{
    return Input(this, index);
}

inline ConstInput Vertex::Arg(int index) const
{
    return ConstInput(this, index);
}

#define DEF_FORWARD_DECLARATION(type) class type##Vertex;
ALL_VERTEX_LIST(DEF_FORWARD_DECLARATION)
#undef DEF_FORWARD_DECLARATION

namespace detail {
template <typename T>
struct OpcodeOfHelper;

#define DEF_OPCODE_HELPER(type)                                   \
    template <>                                                   \
    struct OpcodeOfHelper<type##Vertex> {                         \
        static constexpr VertexOpcode value = VertexOpcode::type; \
    };
ALL_VERTEX_LIST(DEF_OPCODE_HELPER)
#undef DEF_OPCODE_HELPER
}  // namespace detail

// Implement generic Is<T> for specific vertex types
template <class T>
constexpr bool Vertex::Is() const
{
    return GetOpcode() == detail::OpcodeOfHelper<T>::value;
}

// Specialized Is<ValueVertex>
template <>
constexpr bool Vertex::Is<ValueVertex>() const
{
    return IsValueVertex(GetOpcode());
}

// Specialized Is<ControlVertex>
template <>
constexpr bool Vertex::Is<ControlVertex>() const
{
    return IsControlVertex(GetOpcode());
}

// Specialized Is<UnconditionalControlVertex>
// checks if opcode is in the unconditional control range [Jump, JumpLoop]
template <>
constexpr bool Vertex::Is<UnconditionalControlVertex>() const
{
    return IsUnconditionalControlVertex(GetOpcode());
}

template <>
constexpr bool Vertex::Is<BranchControlVertex>() const
{
    return IsBranchControlVertex(GetOpcode());
}

// Factory method implementation
template <class Derived, typename... Args>
Derived *Vertex::New(Chunk *chunk, size_t inputCount, Args &&...args)
{
    ASSERT(inputCount <= static_cast<size_t>(MAX_INPUTS));

    // Allocate memory: inputs stored before the vertex object
    size_t sizeBeforeVertex = inputCount * sizeof(ValueVertex *);
    size_t totalSize = sizeBeforeVertex + sizeof(Derived);

    // Allocate from chunk (fast allocation from pre-allocated memory pool)
    uint8_t *rawBuffer = chunk->NewArray<uint8_t>(totalSize);
    if (rawBuffer == nullptr) {
        return nullptr;
    }

    uint8_t *vertexBuffer = rawBuffer + sizeBeforeVertex;

    uint64_t bitfield = OpcodeField::Encode(detail::OpcodeOfHelper<Derived>::value) |
                        InputCountField::Encode(inputCount) |
                        PropertiesField::Encode(static_cast<uint32_t>(Derived::PROPERTIES));

    return new (vertexBuffer) Derived(bitfield, std::forward<Args>(args)...);
}

template <class Derived, typename Container, typename... Args,
          typename std::enable_if<!std::is_integral<Container>::value, int>::type>
Derived *Vertex::New(Chunk *chunk, const Container &inputs, Args &&...args)
{
    Derived *vertex = New<Derived>(chunk, inputs.size(), std::forward<Args>(args)...);
    int i = 0;
    for (ValueVertex *input : inputs) {
        vertex->SetInput(i++, input);
    }
    return vertex;
}

// Inline implementations
inline void Vertex::SetInput(int index, ValueVertex *vertex)
{
    ASSERT(index < GetInputCount());
    *GetInputPtr(index) = vertex;
}

inline ValueVertex *Vertex::GetInput(int index)
{
    ASSERT(index < GetInputCount());
    return *GetInputPtr(index);
}

inline const ValueVertex *Vertex::GetInput(int index) const
{
    ASSERT(index < GetInputCount());
    return *GetInputPtr(index);
}

inline void Vertex::ClearInput(int index)
{
    ASSERT(index < GetInputCount());
    *GetInputPtr(index) = nullptr;
}

inline ValueVertex *VertexInput::GetVertex() const
{
    return vertex_;
}

template <typename T>
inline constexpr VertexOpcode Vertex::opcode_of()
{
    return detail::OpcodeOfHelper<T>::value;
}

template <typename Function>
void Vertex::ForAllInputsInRegallocAssignmentOrder(Function &&f)
{
    auto iterateInputs = [&](InputAllocationPolicy category) {
        for (int i = 0; i < GetInputCount(); i++) {
            Input input(this, i);
            InputLocation *location = input.GetLocation();
            const InstructionOperand &operand = location->GetOperand();
            ASSERT(operand.IsUnallocated());
            switch (UnallocatedState::Cast(const_cast<InstructionOperand *>(&operand))->GetExtendedPolicy()) {
                case UnallocatedState::ExtendedPolicy::MUST_HAVE_REGISTER:
                    if (category == InputAllocationPolicy::ARBITRARY_REGISTER) {
                        f(input);
                    }
                    break;
                case UnallocatedState::ExtendedPolicy::REGISTER_OR_SLOT_OR_CONSTANT:
                    if (category == InputAllocationPolicy::ANY) {
                        f(input);
                    }
                    break;
                case UnallocatedState::ExtendedPolicy::FIXED_REGISTER:
                case UnallocatedState::ExtendedPolicy::FIXED_FP_REGISTER:
                    if (category == InputAllocationPolicy::FIXED_REGISTER) {
                        f(input);
                    }
                    break;
                case UnallocatedState::ExtendedPolicy::REGISTER_OR_SLOT:
                case UnallocatedState::ExtendedPolicy::SAME_AS_INPUT:
                case UnallocatedState::ExtendedPolicy::NONE:
                case UnallocatedState::ExtendedPolicy::MUST_HAVE_SLOT:
                    UNREACHABLE();
                    break;
            }
        }
    };

    iterateInputs(InputAllocationPolicy::FIXED_REGISTER);
    iterateInputs(InputAllocationPolicy::ARBITRARY_REGISTER);
    iterateInputs(InputAllocationPolicy::ANY);
}

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_VERTEX_H
