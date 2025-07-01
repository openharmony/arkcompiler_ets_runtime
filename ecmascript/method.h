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

#ifndef ECMASCRIPT_METHOD_H
#define ECMASCRIPT_METHOD_H

#include "ecmascript/compiler/deopt_type.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/barriers.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/mem/visitor.h"

#include "libpandafile/file.h"

namespace panda::ecmascript {
class JSPandaFile;
struct Reference;
using EntityId = panda_file::File::EntityId;
class Method : public TaggedObject {
public:
    CAST_CHECK(Method, IsMethod);

    uint64_t SetNumArgsWithCallField(uint64_t callField, uint32_t numargs)
    {
        return NumArgsBits::Update(callField, numargs);
    }

    uint64_t SetNativeBit(uint64_t callField, bool isNative)
    {
        return IsNativeBit::Update(callField, isNative);
    }

    uint64_t SetAotCodeBit(uint64_t callField, bool isCompiled)
    {
        return IsAotCodeBit::Update(callField, isCompiled);
    }

    uint64_t SetFastBuiltinBit(uint64_t callField, bool isFastBuiltin)
    {
        return IsFastBuiltinBit::Update(callField, isFastBuiltin);
    }

    static bool HaveThisWithCallField(uint64_t callField)
    {
        return HaveThisBit::Decode(callField);
    }

    static bool HaveNewTargetWithCallField(uint64_t callField)
    {
        return HaveNewTargetBit::Decode(callField);
    }

    bool HaveExtraWithCallField(uint64_t callField)
    {
        return HaveExtraBit::Decode(callField);
    }

    static bool HaveFuncWithCallField(uint64_t callField)
    {
        return HaveFuncBit::Decode(callField);
    }

    bool IsNativeWithCallField(uint64_t callField) const
    {
        return IsNativeBit::Decode(callField);
    }

    bool OnlyHaveThisWithCallField(uint64_t callField) const
    {
        return (callField & CALL_TYPE_MASK) == 1;  // 1: the first bit of callFiled is HaveThisBit
    }

    bool OnlyHaveNewTagetAndThisWithCallField(uint64_t callField) const
    {
        return (callField & CALL_TYPE_MASK) == 0b11;  // the first two bit of callFiled is `This` and `NewTarget`
    }

    static uint32_t GetNumArgsWithCallField(uint64_t callField)
    {
        return NumArgsBits::Decode(callField);
    }

    static uint64_t SetCallNapi(uint64_t extraLiteralInfo, bool isCallNapi)
    {
        return IsCallNapiBit::Update(extraLiteralInfo, isCallNapi);
    }

    static bool IsCallNapi(uint64_t extraLiteralInfo)
    {
        return IsCallNapiBit::Decode(extraLiteralInfo);
    }

    static uint64_t SetIsFastCall(uint64_t callField, bool isFastCall)
    {
        return IsFastCallBit::Update(callField, isFastCall);
    }

    static bool IsFastCall(uint64_t callField)
    {
        return IsFastCallBit::Decode(callField);
    }

    void SetNumArgsWithCallField(uint32_t numargs)
    {
        uint64_t callField = GetCallField();
        uint64_t newValue = SetNumArgsWithCallField(callField, numargs);
        SetCallField(newValue);
    }

    void SetNativeBit(bool isNative)
    {
        uint64_t callField = GetCallField();
        uint64_t newValue = SetNativeBit(callField, isNative);
        SetCallField(newValue);
    }

    void SetAotCodeBit(bool isCompiled)
    {
        uint64_t callField = GetCallField();
        uint64_t newValue = SetAotCodeBit(callField, isCompiled);
        SetCallField(newValue);
    }

    void SetFastBuiltinBit(bool isFastBuiltin)
    {
        uint64_t callField = GetCallField();
        uint64_t newValue = SetFastBuiltinBit(callField, isFastBuiltin);
        SetCallField(newValue);
    }

    bool HaveThisWithCallField() const
    {
        uint64_t callField = GetCallField();
        return HaveThisWithCallField(callField);
    }

    bool HaveNewTargetWithCallField() const
    {
        uint64_t callField = GetCallField();
        return HaveNewTargetWithCallField(callField);
    }

    bool HaveExtraWithCallField()
    {
        uint64_t callField = GetCallField();
        return HaveExtraWithCallField(callField);
    }

    bool HaveFuncWithCallField() const
    {
        uint64_t callField = GetCallField();
        return HaveFuncWithCallField(callField);
    }

    bool IsNativeWithCallField() const
    {
        uint64_t callField = GetCallField();
        return IsNativeWithCallField(callField);
    }

    bool IsAotWithCallField() const
    {
        uint64_t callField = GetCallField();
        return MethodLiteral::IsAotWithCallField(callField);
    }

    bool IsDeoptimized() const;

    bool OnlyHaveThisWithCallField() const
    {
        uint64_t callField = GetCallField();
        return MethodLiteral::OnlyHaveThisWithCallField(callField);
    }

    bool OnlyHaveNewTagetAndThisWithCallField() const
    {
        uint64_t callField = GetCallField();
        return MethodLiteral::OnlyHaveNewTagetAndThisWithCallField(callField);
    }

    static uint32_t GetNumVregsWithCallField(uint64_t callField)
    {
        return NumVregsBits::Decode(callField);
    }

    uint32_t GetNumVregsWithCallField() const
    {
        uint64_t callField = GetCallField();
        return GetNumVregsWithCallField(callField);
    }

    uint32_t GetNumArgsWithCallField() const
    {
        uint64_t callField = GetCallField();
        return MethodLiteral::GetNumArgsWithCallField(callField);
    }

    uint32_t GetNumArgs() const
    {
        return GetNumArgsWithCallField() + GetNumRevervedArgs();
    }

    uint32_t GetNumRevervedArgs() const
    {
        return HaveFuncWithCallField() +
            HaveNewTargetWithCallField() + HaveThisWithCallField();
    }

    uint32_t GetNumberVRegs() const
    {
        return GetNumVregsWithCallField() + GetNumArgs();
    }

    inline int16_t GetHotnessCounter() const
    {
        uint64_t literalInfo = GetLiteralInfo();
        return MethodLiteral::GetHotnessCounter(literalInfo);
    }

    inline NO_THREAD_SANITIZE void SetHotnessCounter(int16_t counter)
    {
        uint64_t literalInfo = GetLiteralInfo();
        uint64_t newValue = MethodLiteral::SetHotnessCounter(literalInfo, counter);
        SetLiteralInfo(newValue);
    }

    EntityId GetMethodId() const
    {
        uint64_t literalInfo = GetLiteralInfo();
        return MethodLiteral::GetMethodId(literalInfo);
    }

    uint32_t GetSlotSize() const
    {
        uint64_t literalInfo = GetLiteralInfo();
        return MethodLiteral::GetSlotSize(literalInfo);
    }

    uint8_t GetBuiltinId(uint64_t literalInfo) const
    {
        return BuiltinIdBits::Decode(literalInfo);
    }

    uint64_t SetBuiltinId(uint64_t literalInfo, uint8_t id)
    {
        return BuiltinIdBits::Update(literalInfo, id);
    }

    uint64_t SetFunctionKind(uint64_t extraLiteralInfo, FunctionKind kind)
    {
        return FunctionKindBits::Update(extraLiteralInfo, kind);
    }

    FunctionKind GetFunctionKind(uint64_t extraLiteralInfo) const
    {
        return static_cast<FunctionKind>(FunctionKindBits::Decode(extraLiteralInfo));
    }

    uint64_t SetDeoptThreshold(uint64_t literalInfo, uint8_t count)
    {
        return DeoptCountBits::Update(literalInfo, count);
    }

    uint16_t GetDeoptThreshold(uint64_t literalInfo) const
    {
        return DeoptCountBits::Decode(literalInfo);
    }

    uint64_t SetDeoptType(uint64_t extraLiteralInfo, kungfu::DeoptType type)
    {
        return DeoptTypeBits::Update(extraLiteralInfo, type);
    }
    
    uint64_t SetIsSendable(uint64_t extraLiteralInfo, bool isSendable)
    {
        return IsSharedBit::Update(extraLiteralInfo, isSendable);
    }

    uint64_t SetFpDelta(uint64_t extraLitearalInfo, int32_t delta)
    {
        return FpDeltaBits::Update(extraLitearalInfo, delta);
    }

    int32_t GetFpDelta(uint64_t extraLiteralInfo) const
    {
        return static_cast<int32_t>(FpDeltaBits::Decode(extraLiteralInfo));
    }

    void SetDeoptType(kungfu::DeoptType type)
    {
        uint64_t extraLiteralInfo = GetExtraLiteralInfo();
        uint64_t newValue = SetDeoptType(extraLiteralInfo, type);
        SetExtraLiteralInfo(newValue);
    }

    kungfu::DeoptType GetDeoptType(uint64_t extraLiteralInfo) const
    {
        return static_cast<kungfu::DeoptType>(DeoptTypeBits::Decode(extraLiteralInfo));
    }

    kungfu::DeoptType GetDeoptType() const
    {
        uint64_t extraLiteralInfo = GetExtraLiteralInfo();
        return GetDeoptType(extraLiteralInfo);
    }

    void SetFunctionKind(FunctionKind kind)
    {
        uint64_t extraLiteralInfo = GetExtraLiteralInfo();
        uint64_t newValue = SetFunctionKind(extraLiteralInfo, kind);
        SetExtraLiteralInfo(newValue);
    }

    FunctionKind GetFunctionKind() const
    {
        uint64_t extraLiteralInfo = GetExtraLiteralInfo();
        return GetFunctionKind(extraLiteralInfo);
    }

    uint8_t GetBuiltinId() const
    {
        uint64_t extraLiteralInfo = GetExtraLiteralInfo();
        return GetBuiltinId(extraLiteralInfo);
    }

    void SetIsFastCall(bool isFastCall)
    {
        uint64_t callFiled = GetCallField();
        uint64_t newValue = SetIsFastCall(callFiled, isFastCall);
        SetCallField(newValue);
    }

    bool IsFastCall() const
    {
        uint64_t callFiled = GetCallField();
        return IsFastCall(callFiled);
    }

    void SetCallNapi(bool isCallNapi)
    {
        uint64_t extraLiteralInfo = GetExtraLiteralInfo();
        uint64_t newValue = SetCallNapi(extraLiteralInfo, isCallNapi);
        SetExtraLiteralInfo(newValue);
    }

    bool IsCallNapi() const
    {
        uint64_t extraLiteralInfo = GetExtraLiteralInfo();
        return IsCallNapi(extraLiteralInfo);
    }

    void SetBuiltinId(uint8_t id)
    {
        uint64_t extraLiteralInfo = GetExtraLiteralInfo();
        uint64_t newValue = SetBuiltinId(extraLiteralInfo, id);
        SetExtraLiteralInfo(newValue);
    }

    void SetDeoptThreshold(uint8_t count)
    {
        uint64_t extraLiteralInfo = GetExtraLiteralInfo();
        uint64_t newValue = SetDeoptThreshold(extraLiteralInfo, count);
        SetExtraLiteralInfo(newValue);
    }

    uint16_t GetDeoptThreshold() const
    {
        uint64_t extraLiteralInfo = GetExtraLiteralInfo();
        return GetDeoptThreshold(extraLiteralInfo);
    }

    void SetFpDelta(int32_t delta)
    {
        uint64_t extraLiteralInfo = GetExtraLiteralInfo();
        uint64_t newValue = SetFpDelta(extraLiteralInfo, delta);
        SetExtraLiteralInfo(newValue);
    }

    int32_t GetFpDelta() const
    {
        uint64_t extraLiteralInfo = GetExtraLiteralInfo();
        return GetFpDelta(extraLiteralInfo);
    }

    const void* GetNativePointer() const
    {
        return GetNativePointerOrBytecodeArray();
    }

    void SetNativePointer(void *nativePointer)
    {
        SetNativePointerOrBytecodeArray(nativePointer);
    }

    const uint8_t *GetBytecodeArray() const
    {
        return reinterpret_cast<const uint8_t *>(GetNativePointerOrBytecodeArray());
    }

    bool IsSendableMethod() const
    {
        uint64_t extraLiteralInfo = GetExtraLiteralInfo();
        return IsSharedBit::Decode(extraLiteralInfo);
    }

    void SetIsSendable(bool isSendable)
    {
        uint64_t extraLiteralInfo = GetExtraLiteralInfo();
        uint64_t newValue = SetIsSendable(extraLiteralInfo, isSendable);
        SetExtraLiteralInfo(newValue);
    }

    // add for AOT
    void SetCodeEntryAndMarkAOTWhenBinding(uintptr_t codeEntry);

    void ClearAOTStatusWhenDeopt(uintptr_t entry);

    void ClearAOTFlagsWhenInit();

    void InitInterpreterStatusForCompiledMethod(const JSThread *thread);

    void SetCompiledFuncEntry(uintptr_t codeEntry, bool isFastCall);

    static constexpr size_t Size()
    {
        return sizeof(Method);
    }

    const JSPandaFile *PUBLIC_API GetJSPandaFile(const JSThread *thread) const;
    uint32_t PUBLIC_API GetCodeSize(const JSThread *thread) const;
    MethodLiteral *PUBLIC_API GetMethodLiteral(const JSThread *thread) const;

    const char *PUBLIC_API GetMethodName(const JSThread *thread) const;
    const char *PUBLIC_API GetMethodName(const JSPandaFile *file) const;
    std::string PUBLIC_API ParseFunctionName(const JSThread *thread) const;
    std::pair<std::string_view, bool> PUBLIC_API ParseFunctionNameView(const JSThread *thread) const;
    const CString PUBLIC_API GetRecordNameStr(const JSThread *thread) const;

    uint32_t FindCatchBlock(const JSThread *thread, uint32_t pc) const;
    bool HasCatchBlock(const JSThread *thread) const;

    /* callfield */
    static constexpr size_t VREGS_ARGS_NUM_BITS = 28; // 28: maximum 268,435,455
    static constexpr uint64_t AOT_FASTCALL_BITS = 0x5; // 0x5LU: aot and fastcall bit field
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

    /*  ExtraLiteralInfo */
    static constexpr size_t BUILTINID_NUM_BITS = 8;
    static constexpr size_t FUNCTION_KIND_NUM_BITS = 4;
    static constexpr size_t DEOPT_THRESHOLD_BITS = 8;
    static constexpr size_t DEOPTTYPE_NUM_BITS = 8;
    static constexpr size_t FP_DELTA_BITS = 32;
    using BuiltinIdBits = BitField<uint8_t, 0, BUILTINID_NUM_BITS>; // offset 0-7
    using FunctionKindBits = BuiltinIdBits::NextField<FunctionKind, FUNCTION_KIND_NUM_BITS>; // offset 8-11
    using DeoptCountBits = FunctionKindBits::NextField<uint8_t, DEOPT_THRESHOLD_BITS>; // offset 12-19
    using DeoptTypeBits = DeoptCountBits::NextField<kungfu::DeoptType, DEOPTTYPE_NUM_BITS>; // offset 20-27
    using IsCallNapiBit = DeoptTypeBits::NextFlag; // offset 28
    using IsJitCompiledCodeBit = IsCallNapiBit::NextFlag; // offset 29
    using IsSharedBit = IsJitCompiledCodeBit::NextFlag; // offset 30
    using FpDeltaBits = IsSharedBit::NextField<int32_t, FP_DELTA_BITS>; // offset 31-62

    static constexpr size_t CONSTANT_POOL_OFFSET = TaggedObjectSize();
    ACCESSORS(ConstantPool, CONSTANT_POOL_OFFSET, CALL_FIELD_OFFSET)
    ACCESSORS_SYNCHRONIZED_PRIMITIVE_FIELD(
        CallField, uint64_t, CALL_FIELD_OFFSET, NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET)
    // Native method decides this filed is NativePointer or BytecodeArray pointer.
    ACCESSORS_NATIVE_FIELD(
        NativePointerOrBytecodeArray, void, NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET, CODEENTRY_LITERAL_OFFSET)
    ACCESSORS_SYNCHRONIZED_PRIMITIVE_FIELD(
        CodeEntryOrLiteral, uintptr_t, CODEENTRY_LITERAL_OFFSET, LITERAL_INFO_OFFSET)
    // hotness counter is encoded in a js method field, the first uint16_t in a uint64_t.
    ACCESSORS_PRIMITIVE_FIELD(LiteralInfo, uint64_t, LITERAL_INFO_OFFSET, EXTRA_LITERAL_INFO_OFFSET)
    ACCESSORS_SYNCHRONIZED_PRIMITIVE_FIELD(
        ExtraLiteralInfo, uint64_t, EXTRA_LITERAL_INFO_OFFSET, EXPECTED_PROPERTY_COUNT_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(ExpectedPropertyCount, uint32_t, EXPECTED_PROPERTY_COUNT_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    DECL_VISIT_OBJECT(CONSTANT_POOL_OFFSET, CALL_FIELD_OFFSET);

    DECL_DUMP()

private:
    static JSHandle<Method> Create(JSThread *thread, const JSPandaFile *jsPandaFile, MethodLiteral *methodLiteral);

    friend class ObjectFactory;
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_METHOD_H
