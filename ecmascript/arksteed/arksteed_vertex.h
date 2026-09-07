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

#include <array>
#include <cstdint>
#include <string>
#include <type_traits>

#include "common_interfaces/base/bit_field.h"
#include "ecmascript/arksteed/arksteed_opcode_list.h"
#include "ecmascript/arksteed/arksteed_regalloc_types.h"
#include "ecmascript/arksteed/arksteed_regalloc_vertex_info.h"
#include "ecmascript/mem/chunk.h"
#include "libpandabase/macros.h"

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

// Accesses the internal thread-local vertex label counter.
#if !defined(NDEBUG)
uint32_t NextVertexLabel();
#endif

std::string FormatVertexLabel(const Vertex *vertex);

// RAII scope to guard the internal vertex label counter.
class VertexLabelScope {
public:
    VertexLabelScope();
    ~VertexLabelScope();
    NO_COPY_SEMANTIC(VertexLabelScope);
    NO_MOVE_SEMANTIC(VertexLabelScope);
};

enum class SideEffectKind : uint8_t {
    NO_SIDE_EFFECT,
    FIELD_WRITE,
    ELEMENTS_WRITE,
    ENV_SLOT_WRITE,
    MAP_TRANSITION,
    UNKNOWN_CALL,
    SAFE_CALL,
};

#define VERTEX_VALUE_REPRESENTATIONS(V) \
    V(Tagged,       TAGGED)             \
    V(Int32,        INT32)              \
    V(UInt32,       UINT32)             \
    V(Int64,        INT64)              \
    V(Float64,      FLOAT64)            \
    V(HoleyFloat64, HOLEY_FLOAT64)

#define VERTEX_VALUE_COMPOUND_REPRESENTATIONS(V)    \
    V(IntPtr)                                       \
    V(AnyInt32)                                     \
    V(AnyFloat64)

// ValueRepresentation describes the machine representation of a value
enum class ValueRepresentation : uint8_t {
    NONE = 0,

#define DEFINE_VALUE_REPRESENTATION(_, NAME) NAME,
    VERTEX_VALUE_REPRESENTATIONS(DEFINE_VALUE_REPRESENTATION)
#undef DEFINE_VALUE_REPRESENTATION

    // TODO: Adaptation to 32-bit platform
    INT_PTR = INT64,
};

#define VALUE_REPRESENTATION_PREDICATE(Name, NAME)      \
    constexpr bool Is##Name(ValueRepresentation repr)   \
    {                                                   \
        return repr == ValueRepresentation::NAME;       \
    }
VERTEX_VALUE_REPRESENTATIONS(VALUE_REPRESENTATION_PREDICATE)
#undef VALUE_REPRESENTATION_PREDICATE

constexpr bool IsIntPtr(ValueRepresentation repr)
{
    return repr == ValueRepresentation::INT_PTR;
}

constexpr bool IsAnyInt32(ValueRepresentation repr)
{
    return repr == ValueRepresentation::INT32 || repr == ValueRepresentation::UINT32;
}

constexpr bool IsAnyFloat64(ValueRepresentation repr)
{
    return repr == ValueRepresentation::FLOAT64 || repr == ValueRepresentation::HOLEY_FLOAT64;
}

template <size_t N>
using ValueRepresentationArray = std::array<ValueRepresentation, N>;

constexpr const char *ValueRepresentationName(ValueRepresentation repr)
{
    switch (repr) {
#define CASE(Name, NAME)                    \
        case (ValueRepresentation::NAME):   \
            return #Name;
        VERTEX_VALUE_REPRESENTATIONS(CASE)
#undef CASE
        default:
            return "unknown";
    }
}

enum class VertexPropertyFlag : uint16_t {
    NONE                = 0,
    CAN_EAGER_DEOPT     = 1u << 0,
    CAN_LAZY_DEOPT      = 1u << 1,
    CAN_THROW           = 1u << 2,
    CAN_READ            = 1u << 3,
    CAN_WRITE           = 1u << 4,
    CAN_ALLOCATE        = 1u << 5,
    IS_NOT_IDEMPOTENT   = 1u << 6,
    IS_CALL             = 1u << 7,
    IS_DEFERRED_CALL    = 1u << 8,
};

constexpr VertexPropertyFlag operator|(VertexPropertyFlag lhs, VertexPropertyFlag rhs)
{
    unsigned u = static_cast<unsigned>(lhs);
    unsigned v = static_cast<unsigned>(rhs);
    return static_cast<VertexPropertyFlag>(u | v);
}

#define VERTEX_PROPERTY_FLAGS(V)            \
    V(CanEagerDeopt,    CAN_EAGER_DEOPT)    \
    V(CanLazyDeopt,     CAN_LAZY_DEOPT)     \
    V(CanThrow,         CAN_THROW)          \
    V(CanRead,          CAN_READ)           \
    V(CanWrite,         CAN_WRITE)          \
    V(CanAllocate,      CAN_ALLOCATE)       \
    V(IsNotIdempotent,  IS_NOT_IDEMPOTENT)  \
    V(IsCall,           IS_CALL)            \
    V(IsDeferredCall,   IS_DEFERRED_CALL)

#define VERTEX_PROPERTY_COMPOUND_FLAGS(V)   \
    V(CanDeopt)                             \
    V(CanParticipateInCSE)                  \
    V(MayHaveSideEffects)

#define VERTEX_PROPERTY_FLAGS_PREDICATE(Name, FLAG)                     \
    constexpr bool Name(VertexPropertyFlag flag)                        \
    {                                                                   \
        uint32_t u = static_cast<uint32_t>(flag);                       \
        uint32_t v = static_cast<uint32_t>(VertexPropertyFlag::FLAG);   \
        return (u & v) != 0;                                            \
    }
    VERTEX_PROPERTY_FLAGS(VERTEX_PROPERTY_FLAGS_PREDICATE)
#undef VERTEX_PROPERTY_FLAGS_PREDICATE

constexpr bool CanDeopt(VertexPropertyFlag flag)
{
    return CanEagerDeopt(flag) || CanLazyDeopt(flag);
}

constexpr bool CanParticipateInCSE(VertexPropertyFlag flag)
{
    return !IsNotIdempotent(flag) &&
           !CanWrite(flag) &&
           !CanAllocate(flag) &&
           !CanThrow(flag) &&
           !IsCall(flag) &&
           !CanDeopt(flag);
}

constexpr bool MayHaveSideEffects(VertexPropertyFlag flag)
{
    return CanRead(flag) || CanWrite(flag) || CanAllocate(flag);
}

class Vertex {
public:
    NO_COPY_SEMANTIC(Vertex);
    NO_MOVE_SEMANTIC(Vertex);

    friend std::string FormatVertexLabel(const Vertex *vertex);

    static constexpr size_t MAX_INPUTS = (1u << 16) - 1;  // 16: input count field bit width

    using OpcodeField = common::BitField<VertexOpcode, 0, 16>;                    // 16: opcode field bit width
    using InputCountField = OpcodeField::NextField<uint32_t, 16>;                 // 16: input count field bit width
    using PropertyFlagsField = InputCountField::NextField<uint32_t, 16>;          // 16: properties field bit width
    using ValueRepresentationField = PropertyFlagsField::NextField<uint32_t, 3>;  // 3 : Value repr field width
    using NumTempField = ValueRepresentationField::NextField<uint32_t, 2>;        // 2: NumTemp bit width
    using NumDoubleTempField = NumTempField::NextField<uint32_t, 1>;

    // Get opcode of this vertex
    constexpr VertexOpcode GetOpcode() const
    {
        return OpcodeField::Decode(bitfield_);
    }

#define DEFINE_OPCODE_PREDICATE(CUR_OPCODE_LIST, FunctionName)  \
    constexpr bool FunctionName() const                         \
    {                                                           \
        return arksteed::FunctionName(GetOpcode());             \
    }
    VERTEX_LISTS_FOR_EACH(DEFINE_OPCODE_PREDICATE)
#undef DEFINE_OPCODE_PREDICATE

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

    constexpr bool HasInputs() const
    {
        return GetInputCount() > 0;
    }

    constexpr uint32_t GetInputCount() const
    {
        return static_cast<uint32_t>(InputCountField::Decode(bitfield_));
    }

    void SetInput(uint32_t index, ValueVertex *vertex);
    ValueVertex *GetInput(uint32_t index);
    const ValueVertex *GetInput(uint32_t index) const;

    Input Arg(uint32_t index);
    ConstInput Arg(uint32_t index) const;

    // Input allocation order for register allocation
    // Iterates inputs in the order expected by the register allocator:
    // first fixed register inputs, then arbitrary register inputs, then any inputs
    enum class InputAllocationPolicy {
        FIXED_REGISTER,      // FIXED_REGISTER or FIXED_FP_REGISTER
        ARBITRARY_REGISTER,  // MUST_HAVE_REGISTER
        ANY,                 // REGISTER_OR_SLOT, REGISTER_OR_SLOT_OR_CONSTANT
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

#define VERTEX_PROPERTY_FLAGS_GETTER(Name, ...)                             \
    constexpr bool Name() const                                             \
    {                                                                       \
        uint32_t underlying = PropertyFlagsField::Decode(bitfield_);        \
        return arksteed::Name(static_cast<VertexPropertyFlag>(underlying)); \
    }
    VERTEX_PROPERTY_FLAGS(VERTEX_PROPERTY_FLAGS_GETTER)
    VERTEX_PROPERTY_COMPOUND_FLAGS(VERTEX_PROPERTY_FLAGS_GETTER)
#undef VERTEX_PROPERTY_FLAGS_GETTER

    constexpr VertexPropertyFlag GetPropertyFlags() const
    {
        uint32_t underlying = PropertyFlagsField::Decode(bitfield_);
        return static_cast<VertexPropertyFlag>(underlying);
    }

    constexpr ValueRepresentation GetValueRepresentation() const
    {
        uint32_t underlying = ValueRepresentationField::Decode(bitfield_);
        return static_cast<ValueRepresentation>(underlying);
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

    void Dump(std::ostream &out, bool withColors = false) const;
    std::string Dump(bool withColors = false) const;

    // Factory method to create vertices
    template <class Derived, typename... Args>
    static Derived *New(Chunk *chunk, size_t inputCount, Args &&...args);

    template <class Derived, typename Container, typename... Args,
              typename std::enable_if<!std::is_integral<Container>::value, int>::type = 0>
    static Derived *New(Chunk *chunk, const Container &inputs, Args &&...args);

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
    Vertex() = default;

    // Allow updating bits from subclasses
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

    uint64_t bitfield_ {0};
    BB *owner_ = nullptr;
    RegallocVertexInfo *regallocInfo_ = nullptr;
#if !defined(NDEBUG)
    VertexId label_ = INVALID_VERTEX_ID;
#endif
};

class NonControlVertex : public Vertex {
protected:
    NonControlVertex() = default;
};

// ValueVertex is a vertex that produces a value
class ValueVertex : public NonControlVertex {
public:
    constexpr MachineRepresentation GetMachineRepresentation() const
    {
        switch (GetValueRepresentation()) {
            case ValueRepresentation::TAGGED:
                return MachineRepresentation::Tagged;
            case ValueRepresentation::INT32:
            case ValueRepresentation::UINT32:
                return MachineRepresentation::Word32;
            case ValueRepresentation::INT64:
                return MachineRepresentation::Word64;
            case ValueRepresentation::FLOAT64:
            case ValueRepresentation::HOLEY_FLOAT64:
                return MachineRepresentation::Float64;
            case ValueRepresentation::NONE:
                return MachineRepresentation::None;
        }
        return MachineRepresentation::None;
    }

#define VALUE_REPRESENTATION_PREDICATE(Name, ...)               \
    constexpr bool Is##Name() const                             \
    {                                                           \
        return arksteed::Is##Name(GetValueRepresentation());    \
    }
    VERTEX_VALUE_REPRESENTATIONS(VALUE_REPRESENTATION_PREDICATE)
    VERTEX_VALUE_COMPOUND_REPRESENTATIONS(VALUE_REPRESENTATION_PREDICATE)
#undef VALUE_REPRESENTATION_PREDICATE

    ValueLocation &Result();
    const ValueLocation &Result() const;

    RegallocValueVertexInfo *GetRegallocInfo() const
    {
        return static_cast<RegallocValueVertexInfo *>(Vertex::GetRegallocInfo());
    }

    void SetHint(InstructionOperand hint);

protected:
    ValueVertex() = default;
};

// ControlVertex is a vertex that affects control flow
class ControlVertex : public Vertex {
protected:
    ControlVertex() = default;
};

class Input {
public:
    Input(Vertex *base, uint32_t index) : base_(base), index_(index) {}

    ValueVertex *vertex() const
    {
        return base_->GetInput(index_);
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
    uint32_t index_;
};

class ConstInput {
public:
    ConstInput(const Vertex *base, uint32_t index) : base_(base), index_(index) {}

    ConstInput(const Input &input)
    {
        base_ = input.base_;
        index_ = input.index_;
    }

    const ValueVertex *vertex() const
    {
        return base_->GetInput(index_);
    }

    const InputLocation *GetLocation() const;
    const InstructionOperand &GetOperand() const;

private:
    const Vertex *base_;
    uint32_t index_;
};

inline Input Vertex::Arg(uint32_t index)
{
    return Input(this, index);
}

inline ConstInput Vertex::Arg(uint32_t index) const
{
    return ConstInput(this, index);
}

// Implement generic Is<T> for specific vertex types
template <class T>
constexpr bool Vertex::Is() const
{
    return GetOpcode() == OpcodeOf<T>;
}

// Specialized Is<ValueVertex>
template <>
constexpr bool Vertex::Is<ValueVertex>() const
{
    return IsValueVertex();
}

// Specialized Is<ControlVertex>
template <>
constexpr bool Vertex::Is<ControlVertex>() const
{
    return IsControlVertex();
}

// Specialized Is<UnconditionalControlVertex>
// checks if opcode is in the unconditional control range [Jump, JumpLoop]
template <>
constexpr bool Vertex::Is<UnconditionalControlVertex>() const
{
    return IsUnconditionalControlVertex();
}

template <>
constexpr bool Vertex::Is<BranchControlVertex>() const
{
    return IsBranchControlVertex();
}

// Factory method implementation
template <class Derived, typename... Args>
Derived *Vertex::New(Chunk *chunk, size_t inputCount, Args &&...args)
{
    ASSERT(inputCount <= MAX_INPUTS);

    // Allocate memory: inputs stored before the vertex object
    size_t sizeBeforeVertex = inputCount * sizeof(ValueVertex *);
    size_t totalSize = sizeBeforeVertex + sizeof(Derived);

    // Allocate from chunk (fast allocation from pre-allocated memory pool)
    uint8_t *rawBuffer = chunk->NewArray<uint8_t>(totalSize);
    if (rawBuffer == nullptr) {
        return nullptr;
    }

    uint8_t *vertexBuffer = rawBuffer + sizeBeforeVertex;
    Derived *vertex = new (vertexBuffer) Derived(std::forward<Args>(args)...);
    Vertex *base = vertex;
    base->bitfield_ =  OpcodeField::Encode(OpcodeOf<Derived>);
    base->bitfield_ |= InputCountField::Encode(inputCount);
    base->bitfield_ |= PropertyFlagsField::Encode(static_cast<uint32_t>(Derived::PROPERTIES));
    base->bitfield_ |= ValueRepresentationField::Encode(static_cast<uint32_t>(Derived::VALUE_TYPE));

#if !defined(NDEBUG)
    vertex->label_ = NextVertexLabel();
#endif

    ASSERT(vertex->GetOpcode() == OpcodeOf<Derived>);
    ASSERT(vertex->GetPropertyFlags() == Derived::PROPERTIES);
    ASSERT(vertex->GetValueRepresentation() == Derived::VALUE_TYPE);
    ASSERT(vertex->GetInputCount() == inputCount);
    return vertex;
}

template <class Derived, typename Container, typename... Args,
          typename std::enable_if<!std::is_integral<Container>::value, int>::type>
Derived *Vertex::New(Chunk *chunk, const Container &inputs, Args &&...args)
{
    Derived *vertex = New<Derived>(chunk, inputs.size(), std::forward<Args>(args)...);
    uint32_t i = 0;
    for (ValueVertex *input : inputs) {
        vertex->SetInput(i++, input);
    }
    return vertex;
}

// Inline implementations
inline void Vertex::SetInput(uint32_t index, ValueVertex *vertex)
{
    ASSERT(index < GetInputCount());
    *GetInputPtr(index) = vertex;
}

inline ValueVertex *Vertex::GetInput(uint32_t index)
{
    ASSERT(index < GetInputCount());
    return *GetInputPtr(index);
}

inline const ValueVertex *Vertex::GetInput(uint32_t index) const
{
    ASSERT(index < GetInputCount());
    return *GetInputPtr(index);
}

template <typename Function>
void Vertex::ForAllInputsInRegallocAssignmentOrder(Function &&f)
{
    auto iterateInputs = [&](InputAllocationPolicy category) {
        for (uint32_t i = 0, n = GetInputCount(); i < n; i++) {
            Input input(this, i);
            InputLocation *location = input.GetLocation();
            const InstructionOperand &operand = location->GetOperand();
            ASSERT(operand.IsUnallocated());
            switch (UnallocatedState::Cast(operand).GetExtendedPolicy()) {
                case UnallocatedState::ExtendedPolicy::MUST_HAVE_REGISTER:
                    if (category == InputAllocationPolicy::ARBITRARY_REGISTER) {
                        f(input);
                    }
                    break;
                case UnallocatedState::ExtendedPolicy::MUST_HAVE_SLOT:
                    if (category == InputAllocationPolicy::FIXED_REGISTER) {
                        f(input);
                    }
                    break;
                case UnallocatedState::ExtendedPolicy::REGISTER_OR_SLOT_OR_CONSTANT:
                case UnallocatedState::ExtendedPolicy::REGISTER_OR_SLOT:
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
                case UnallocatedState::ExtendedPolicy::SAME_AS_INPUT:
                case UnallocatedState::ExtendedPolicy::NONE:
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
