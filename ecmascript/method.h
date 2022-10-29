/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_JS_METHOD_H
#define ECMASCRIPT_JS_METHOD_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/barriers.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/mem/visitor.h"

#include "libpandafile/file.h"

namespace panda::ecmascript {
class JSPandaFile;
using EntityId = panda_file::File::EntityId;
class Method : public TaggedObject {
public:
    CAST_CHECK(Method, IsMethod);

    void SetNumArgsWithCallField(uint32_t numargs)
    {
        uint64_t callField = GetCallField();
        uint64_t newValue = MethodLiteral::SetNumArgsWithCallField(callField, numargs);
        SetCallField(newValue);
    }

    void SetNativeBit(bool isNative)
    {
        uint64_t callField = GetCallField();
        uint64_t newValue = MethodLiteral::SetNativeBit(callField, isNative);
        SetCallField(newValue);
    }

    void SetAotCodeBit(bool isCompiled)
    {
        uint64_t callField = GetCallField();
        uint64_t newValue = MethodLiteral::SetAotCodeBit(callField, isCompiled);
        SetCallField(newValue);
    }

    void SetFastBuiltinBit(bool isFastBuiltin)
    {
        uint64_t callField = GetCallField();
        uint64_t newValue = MethodLiteral::SetFastBuiltinBit(callField, isFastBuiltin);
        SetCallField(newValue);
    }

    bool HaveThisWithCallField() const
    {
        uint64_t callField = GetCallField();
        return MethodLiteral::HaveThisWithCallField(callField);
    }

    bool HaveNewTargetWithCallField() const
    {
        uint64_t callField = GetCallField();
        return MethodLiteral::HaveNewTargetWithCallField(callField);
    }

    bool HaveExtraWithCallField() const
    {
        uint64_t callField = GetCallField();
        return MethodLiteral::HaveExtraWithCallField(callField);
    }

    bool HaveFuncWithCallField() const
    {
        uint64_t callField = GetCallField();
        return MethodLiteral::HaveFuncWithCallField(callField);
    }

    bool IsNativeWithCallField() const
    {
        uint64_t callField = GetCallField();
        return MethodLiteral::IsNativeWithCallField(callField);
    }

    bool IsAotWithCallField() const
    {
        uint64_t callField = GetCallField();
        return MethodLiteral::IsAotWithCallField(callField);
    }

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

    uint32_t GetNumVregsWithCallField() const
    {
        uint64_t callField = GetCallField();
        return MethodLiteral::GetNumVregsWithCallField(callField);
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

    uint16_t GetSlotSize() const
    {
        uint64_t literalInfo = GetLiteralInfo();
        return MethodLiteral::GetSlotSize(literalInfo);
    }

    void SetFunctionKind(FunctionKind kind)
    {
        uint64_t extraLiteralInfo = GetExtraLiteralInfo();
        uint64_t newValue = MethodLiteral::SetFunctionKind(extraLiteralInfo, kind);
        SetExtraLiteralInfo(newValue);
    }

    FunctionKind GetFunctionKind() const
    {
        uint64_t extraLiteralInfo = GetExtraLiteralInfo();
        return MethodLiteral::GetFunctionKind(extraLiteralInfo);
    }

    uint8_t GetBuiltinId() const
    {
        uint64_t extraLiteralInfo = GetExtraLiteralInfo();
        return MethodLiteral::GetBuiltinId(extraLiteralInfo);
    }

    void SetCallNative(bool isCallNative)
    {
        uint64_t callField = GetCallField();
        uint64_t newValue = MethodLiteral::SetCallNative(callField, isCallNative);
        SetCallField(newValue);
    }

    bool IsCallNative() const
    {
        uint64_t callField = GetCallField();
        return MethodLiteral::IsCallNative(callField);
    }

    void SetBuiltinId(uint8_t id)
    {
        uint64_t extraLiteralInfo = GetExtraLiteralInfo();
        uint64_t newValue = MethodLiteral::SetBuiltinId(extraLiteralInfo, id);
        SetExtraLiteralInfo(newValue);
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

    // add for AOT
    void SetCodeEntryAndMarkAOT(uintptr_t codeEntry)
    {
        SetAotCodeBit(true);
        SetNativeBit(false);
        SetCodeEntryOrLiteral(codeEntry);
    }

    static constexpr size_t Size()
    {
        return sizeof(Method);
    }

    const JSPandaFile *PUBLIC_API GetJSPandaFile() const;
    const panda_file::File *GetPandaFile() const;
    uint32_t GetCodeSize() const;
    MethodLiteral *GetMethodLiteral() const;

    const char *PUBLIC_API GetMethodName() const;
    const char *PUBLIC_API GetMethodName(const JSPandaFile* file) const;
    std::string PUBLIC_API ParseFunctionName() const;

    static constexpr size_t CONSTANT_POOL_OFFSET = TaggedObjectSize();
    ACCESSORS(ConstantPool, CONSTANT_POOL_OFFSET, PROFILE_TYPE_INFO_OFFSET)
    ACCESSORS(ProfileTypeInfo, PROFILE_TYPE_INFO_OFFSET, CALL_FIELD_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(CallField, uint64_t, CALL_FIELD_OFFSET, NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET)
    // Native method decides this filed is NativePointer or BytecodeArray pointer.
    ACCESSORS_NATIVE_FIELD(
        NativePointerOrBytecodeArray, void, NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET, CODE_ENTRY_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(CodeEntryOrLiteral, uintptr_t, CODE_ENTRY_OFFSET, LITERAL_INFO_OFFSET)
    // hotness counter is encoded in a js method field, the first uint16_t in a uint64_t.
    ACCESSORS_PRIMITIVE_FIELD(LiteralInfo, uint64_t, LITERAL_INFO_OFFSET, EXTRA_LITERAL_INFO_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(ExtraLiteralInfo, uint64_t, EXTRA_LITERAL_INFO_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    DECL_VISIT_OBJECT(CONSTANT_POOL_OFFSET, CALL_FIELD_OFFSET);

    DECL_DUMP()
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_METHOD_H
