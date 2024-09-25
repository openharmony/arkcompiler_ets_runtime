/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_JSPANDAFILE_METHOD_LITERAL_H
#define ECMASCRIPT_JSPANDAFILE_METHOD_LITERAL_H

#include <set>

#include "ecmascript/base/aligned_struct.h"
#include "ecmascript/js_function_kind.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/c_string.h"
#include "libpandafile/file.h"

static constexpr uint32_t CALL_TYPE_MASK = 0xF;  // 0xF: the last 4 bits are used as callType

namespace panda::ecmascript {
class JSPandaFile;
using EntityId = panda_file::File::EntityId;
using StringData = panda_file::File::StringData;
struct PUBLIC_API MethodLiteral : public base::AlignedStruct<sizeof(uint64_t),
                                                        base::AlignedUint64,
                                                        base::AlignedPointer,
                                                        base::AlignedUint64,
                                                        base::AlignedUint64> {
public:
    static constexpr uint8_t INVALID_IC_SLOT = 0xFFU;
    static constexpr uint16_t MAX_SLOT_SIZE = 0xFFFFU;
    static constexpr size_t EXTEND_SLOT_SIZE = 2;

    PUBLIC_API explicit MethodLiteral(EntityId methodId);
    MethodLiteral() = delete;
    ~MethodLiteral() = default;

    NO_COPY_SEMANTIC(MethodLiteral);
    NO_MOVE_SEMANTIC(MethodLiteral);

    static constexpr size_t VREGS_ARGS_NUM_BITS = 28; // 28: maximum 268,435,455
    using HaveThisBit = BitField<bool, 0, 1>;  // offset 0
    using HaveNewTargetBit = HaveThisBit::NextFlag;  // offset 1
    using HaveExtraBit = HaveNewTargetBit::NextFlag;  // offset 2
    using HaveFuncBit = HaveExtraBit::NextFlag;  // offset 3
    using NumVregsBits = HaveFuncBit::NextField<uint32_t, VREGS_ARGS_NUM_BITS>;  // offset 4-31
    using NumArgsBits = NumVregsBits::NextField<uint32_t, VREGS_ARGS_NUM_BITS>;  // offset 32-59
    using IsNativeBit = NumArgsBits::NextFlag;  // offset 60
    using IsAotCodeBit = IsNativeBit::NextFlag; // offset 61
    using IsFastBuiltinBit = IsAotCodeBit::NextFlag; // offset 62
    using IsFastCallBit = IsFastBuiltinBit::NextFlag; // offset 63

    uint64_t GetCallField() const
    {
        return callField_;
    }

    void SetNativeBit(bool isNative)
    {
        callField_ = IsNativeBit::Update(callField_, isNative);
    }

    void SetAotCodeBit(bool isCompiled)
    {
        callField_ = IsAotCodeBit::Update(callField_, isCompiled);
    }

    void PUBLIC_API Initialize(const JSPandaFile *jsPandaFile, const JSThread *thread = nullptr);

    bool HaveThisWithCallField() const
    {
        return HaveThisWithCallField(callField_);
    }

    bool HaveNewTargetWithCallField() const
    {
        return HaveNewTargetWithCallField(callField_);
    }

    bool HaveExtraWithCallField() const
    {
        return HaveExtraWithCallField(callField_);
    }

    bool HaveFuncWithCallField() const
    {
        return HaveFuncWithCallField(callField_);
    }

    bool IsNativeWithCallField() const
    {
        return IsNativeWithCallField(callField_);
    }

    uint32_t GetNumArgsWithCallField() const
    {
        return GetNumArgsWithCallField(callField_);
    }

    uint32_t GetNumArgs() const
    {
        return GetNumArgsWithCallField() + HaveFuncWithCallField() +
            HaveNewTargetWithCallField() + HaveThisWithCallField();
    }

    uint32_t GetNumberVRegs() const
    {
        return GetNumVregsWithCallField() + GetNumArgs();
    }

    uint32_t GetNewTargetVregIndex() const
    {
        if (!HaveNewTargetWithCallField()) {
            return 0;
        }
        uint32_t numVregs = GetNumVregsWithCallField();
        return HaveFuncWithCallField() ? (numVregs + 1) : numVregs;
    }

    static uint64_t SetNativeBit(uint64_t callField, bool isNative)
    {
        return IsNativeBit::Update(callField, isNative);
    }

    static uint64_t SetAotCodeBit(uint64_t callField, bool isCompiled)
    {
        return IsAotCodeBit::Update(callField, isCompiled);
    }

    static bool HaveThisWithCallField(uint64_t callField)
    {
        return HaveThisBit::Decode(callField);
    }

    static bool HaveNewTargetWithCallField(uint64_t callField)
    {
        return HaveNewTargetBit::Decode(callField);
    }

    static bool HaveExtraWithCallField(uint64_t callField)
    {
        return HaveExtraBit::Decode(callField);
    }

    static bool HaveFuncWithCallField(uint64_t callField)
    {
        return HaveFuncBit::Decode(callField);
    }

    static bool IsNativeWithCallField(uint64_t callField)
    {
        return IsNativeBit::Decode(callField);
    }

    static bool IsAotWithCallField(uint64_t callField)
    {
        return IsAotCodeBit::Decode(callField);
    }

    static bool OnlyHaveThisWithCallField(uint64_t callField)
    {
        return (callField & CALL_TYPE_MASK) == 1;  // 1: the first bit of callFiled is HaveThisBit
    }

    static bool OnlyHaveNewTagetAndThisWithCallField(uint64_t callField)
    {
        return (callField & CALL_TYPE_MASK) == 0b11;  // the first two bit of callFiled is `This` and `NewTarget`
    }

    static uint32_t GetNumVregsWithCallField(uint64_t callField)
    {
        return NumVregsBits::Decode(callField);
    }

    uint32_t GetNumVregsWithCallField() const
    {
        return NumVregsBits::Decode(callField_);
    }

    static uint32_t GetNumArgsWithCallField(uint64_t callField)
    {
        return NumArgsBits::Decode(callField);
    }

    static uint64_t SetIsFastCall(uint64_t callField, bool isFastCall)
    {
        return IsFastCallBit::Update(callField, isFastCall);
    }

    void SetIsFastCall(bool isFastCall)
    {
        callField_ = IsFastCallBit::Update(callField_, isFastCall);
    }

    static bool IsFastCall(uint64_t callField)
    {
        return IsFastCallBit::Decode(callField);
    }

    bool IsFastCall() const
    {
        return IsFastCallBit::Decode(callField_);
    }

    static constexpr size_t METHOD_ARGS_NUM_BITS = 16;
    static constexpr size_t METHOD_ARGS_METHODID_BITS = 32;
    static constexpr size_t METHOD_SLOT_SIZE_BITS = 16;
    using HotnessCounterBits = BitField<int16_t, 0, METHOD_ARGS_NUM_BITS>; // offset 0-15
    using MethodIdBits = HotnessCounterBits::NextField<uint32_t, METHOD_ARGS_METHODID_BITS>; // offset 16-47
    using SlotSizeBits = MethodIdBits::NextField<uint16_t, METHOD_SLOT_SIZE_BITS>; // offset 48-63

    static constexpr size_t BUILTINID_NUM_BITS = 8;
    static constexpr size_t FUNCTION_KIND_NUM_BITS = 4;
    static constexpr size_t EMPTY_BITS = 16;
    using BuiltinIdBits = BitField<uint8_t, 0, BUILTINID_NUM_BITS>; // offset 0-7
    using FunctionKindBits = BuiltinIdBits::NextField<FunctionKind, FUNCTION_KIND_NUM_BITS>; // offset 8-11
    using IsNoGCBit = FunctionKindBits::NextFlag; // offset 12
    using HasDebuggerStmtBit = IsNoGCBit::NextFlag; // offset 13
    using EmptyBit = HasDebuggerStmtBit::NextField<uint8_t, EMPTY_BITS>; // offset 14-29
    using IsSharedBit = EmptyBit::NextFlag; // offset 30
    using CanTypedCall = IsSharedBit::NextFlag; // offset 31

    inline NO_THREAD_SANITIZE void SetHotnessCounter(int16_t counter)
    {
        literalInfo_ = HotnessCounterBits::Update(literalInfo_, counter);
    }

    EntityId GetMethodId() const
    {
        return EntityId(MethodIdBits::Decode(literalInfo_));
    }

    uint32_t GetSlotSize() const
    {
        auto size = SlotSizeBits::Decode(literalInfo_);
        return size == MAX_SLOT_SIZE ? MAX_SLOT_SIZE + EXTEND_SLOT_SIZE : size;
    }

    uint8_t UpdateSlotSizeWith8Bit(uint16_t size)
    {
        uint16_t start = SlotSizeBits::Decode(literalInfo_);
        uint32_t end = start + size;
        // ic overflow
        if (end >= INVALID_IC_SLOT) {
            if (start < INVALID_IC_SLOT + 1) {
                literalInfo_ = SlotSizeBits::Update(literalInfo_, INVALID_IC_SLOT + 1);
            }
            return INVALID_IC_SLOT;
        }
        literalInfo_ = SlotSizeBits::Update(literalInfo_, static_cast<uint8_t>(end));
        return start;
    }

    void SetFunctionKind(FunctionKind kind)
    {
        extraLiteralInfo_ = FunctionKindBits::Update(extraLiteralInfo_, kind);
    }

    void SetNoGCBit(bool isNoGC)
    {
        extraLiteralInfo_ = IsNoGCBit::Update(extraLiteralInfo_, isNoGC);
    }

    bool IsNoGC() const
    {
        return IsNoGCBit::Decode(extraLiteralInfo_);
    }

    void SetHasDebuggerStmtBit(bool isDebuggerStmt)
    {
        extraLiteralInfo_ = HasDebuggerStmtBit::Update(extraLiteralInfo_, isDebuggerStmt);
    }

    bool HasDebuggerStmt() const
    {
        return HasDebuggerStmtBit::Decode(extraLiteralInfo_);
    }

    void SetIsShared(bool isShared)
    {
        extraLiteralInfo_ = IsSharedBit::Update(extraLiteralInfo_, isShared);
    }

    bool IsShared() const
    {
        return IsSharedBit::Decode(extraLiteralInfo_);
    }

    void SetCanTypedCall(bool isTypedCall)
    {
        extraLiteralInfo_ = CanTypedCall::Update(extraLiteralInfo_, isTypedCall);
    }

    bool IsTypedCall() const
    {
        return CanTypedCall::Decode(extraLiteralInfo_);
    }

    FunctionKind GetFunctionKind() const
    {
        return static_cast<FunctionKind>(FunctionKindBits::Decode(extraLiteralInfo_));
    }

    inline bool IsClassConstructor() const
    {
        return GetFunctionKind() == FunctionKind::CLASS_CONSTRUCTOR;
    }

    static inline int16_t GetHotnessCounter(uint64_t literalInfo)
    {
        return HotnessCounterBits::Decode(literalInfo);
    }

    static uint64_t SetHotnessCounter(uint64_t literalInfo, int16_t counter)
    {
        return HotnessCounterBits::Update(literalInfo, counter);
    }

    static uint64_t SetFunctionKind(uint64_t extraLiteralInfo, FunctionKind kind)
    {
        return FunctionKindBits::Update(extraLiteralInfo, kind);
    }

    static FunctionKind GetFunctionKind(uint64_t extraLiteralInfo)
    {
        return static_cast<FunctionKind>(FunctionKindBits::Decode(extraLiteralInfo));
    }

    static EntityId GetMethodId(uint64_t literalInfo)
    {
        return EntityId(MethodIdBits::Decode(literalInfo));
    }

    static uint32_t GetSlotSize(uint64_t literalInfo)
    {
        auto size = SlotSizeBits::Decode(literalInfo);
        return size == MAX_SLOT_SIZE ? MAX_SLOT_SIZE + EXTEND_SLOT_SIZE : size;
    }

    static const char PUBLIC_API *GetMethodName(const JSPandaFile *jsPandaFile, EntityId methodId,
                                                bool cpuProfiler = false);
    static std::string PUBLIC_API ParseFunctionName(const JSPandaFile *jsPandaFile, EntityId methodId);
    static std::pair<std::string_view, bool> PUBLIC_API ParseFunctionNameView(const JSPandaFile* jsPandaFile,
                                                                              EntityId methodId);
    static CString PUBLIC_API ParseFunctionNameToCString(const JSPandaFile *jsPandaFile, EntityId methodId);

    static uint32_t PUBLIC_API GetCodeSize(const JSPandaFile *jsPandaFile, EntityId methodId);
    static CString PUBLIC_API GetRecordName(const JSPandaFile *jsPandaFile, EntityId methodId);
    static const char PUBLIC_API *GetRecordNameWithSymbol(const JSPandaFile *jsPandaFile, EntityId methodId);

    const uint8_t *GetBytecodeArray() const
    {
        return reinterpret_cast<const uint8_t *>(nativePointerOrBytecodeArray_);
    }

    const void* GetNativePointer() const
    {
        return nativePointerOrBytecodeArray_;
    }

    uint64_t GetLiteralInfo() const
    {
        return literalInfo_;
    }

    uint64_t GetExtraLiteralInfo() const
    {
        return extraLiteralInfo_;
    }

    std::optional<std::set<uint32_t>> GetConcurrentRequestedModules(const JSPandaFile *jsPandaFile) const;

private:
    enum class Index : size_t {
        CALL_FIELD_INDEX = 0,
        NATIVE_POINTER_OR_BYTECODE_ARRAY_INDEX,
        LITERAL_INFO_INDEX,
        EXTRA_LITERAL_INFO_INDEX,
        NUM_OF_MEMBERS
    };
    static_assert(static_cast<size_t>(Index::NUM_OF_MEMBERS) == NumOfTypes);

    static std::pair<std::string_view, bool> GetMethodNameView(const JSPandaFile* jsPandaFile, EntityId methodId,
                                                               bool cpuProfiler = false);

    void SetMethodId(EntityId methodId)
    {
        literalInfo_ = MethodIdBits::Update(literalInfo_, methodId.GetOffset());
    }

    void SetSlotSize(uint32_t size)
    {
        if (size > MAX_SLOT_SIZE) {
            size = MAX_SLOT_SIZE;
        } else if (size + EXTEND_SLOT_SIZE > INVALID_IC_SLOT && size <= INVALID_IC_SLOT) {
            // for compatibility: ensure there's always 0xff slot in this situation
            size = INVALID_IC_SLOT + 1;
        }
        literalInfo_ = SlotSizeBits::Update(literalInfo_, size);
    }

    alignas(EAS) uint64_t callField_ {0ULL};
    // Native method decides this filed is NativePointer or BytecodeArray pointer.
    alignas(EAS) const void *nativePointerOrBytecodeArray_ {nullptr};
    // hotnessCounter, methodId and slotSize are encoded in literalInfo_.
    alignas(EAS) uint64_t literalInfo_ {0ULL};
    // BuiltinId, FunctionKind are encoded in extraLiteralInfo_.
    alignas(EAS) uint64_t extraLiteralInfo_ {0ULL};
};
STATIC_ASSERT_EQ_ARCH(sizeof(MethodLiteral), MethodLiteral::SizeArch32, MethodLiteral::SizeArch64);
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JSPANDAFILE_METHOD_LITERAL_H
