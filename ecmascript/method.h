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
        return GetNumArgsWithCallField() + HaveFuncWithCallField() +
            HaveNewTargetWithCallField() + HaveThisWithCallField();
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

    uint8_t GetSlotSize() const
    {
        uint64_t literalInfo = GetLiteralInfo();
        return MethodLiteral::GetSlotSize(literalInfo);
    }

    uint8_t GetBuiltinId() const
    {
        uint64_t literalInfo = GetLiteralInfo();
        return MethodLiteral::GetBuiltinId(literalInfo);
    }

    void SetBuiltinId(uint8_t id)
    {
        uint64_t literalInfo = GetLiteralInfo();
        uint64_t newValue = MethodLiteral::SetBuiltinId(literalInfo, id);
        SetLiteralInfo(newValue);
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

    static constexpr size_t Size()
    {
        return sizeof(Method);
    }

    const JSPandaFile *PUBLIC_API GetJSPandaFile() const;
    const panda_file::File *GetPandaFile() const;
    uint32_t GetCodeSize() const;

    panda_file::File::StringData GetName() const;
    const char *PUBLIC_API GetMethodName() const;
    std::string PUBLIC_API ParseFunctionName() const;

    static constexpr size_t CALL_FIELD_OFFSET = TaggedObjectSize();
    ACCESSORS_PRIMITIVE_FIELD(CallField, uint64_t, CALL_FIELD_OFFSET, LITERAL_INFO_OFFSET)
    // hotnessCounter, methodId and slotSize are encoded in literalInfo.
    // hotness counter is encoded in a js method field, the first uint16_t in a uint64_t.
    ACCESSORS_PRIMITIVE_FIELD(LiteralInfo, uint64_t, LITERAL_INFO_OFFSET, CONSTANT_POOL_OFFSET)
    ACCESSORS(ConstantPool, CONSTANT_POOL_OFFSET, NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET)
    // Native method decides this filed is NativePointer or BytecodeArray pointer.
    ACCESSORS_NATIVE_FIELD(
        NativePointerOrBytecodeArray, void, NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    DECL_VISIT_OBJECT(CONSTANT_POOL_OFFSET, NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET);
    DECL_VISIT_NATIVE_FIELD(NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET, LAST_OFFSET);

    DECL_DUMP()
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_METHOD_H
