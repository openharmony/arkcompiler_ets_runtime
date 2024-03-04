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

#ifndef ECMASCRIPT_COMPILER_BUILTINS_CALL_SIGNATURE_H
#define ECMASCRIPT_COMPILER_BUILTINS_CALL_SIGNATURE_H

#include "ecmascript/base/config.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/global_env_constants.h"

namespace panda::ecmascript::kungfu {

#define PADDING_BUILTINS_STUB_LIST(V)               \
    V(NONE)

// BUILTINS_STUB_LIST is shared both ASM Interpreter and AOT.
// AOT_BUILTINS_STUB_LIST is used in AOT only.
#define BUILTINS_STUB_LIST(V)                       \
    BUILTINS_METHOD_STUB_LIST(V)                    \
    BUILTINS_CONSTRUCTOR_STUB_LIST(V)               \
    AOT_AND_BUILTINS_STUB_LIST(V)

#define BUILTINS_METHOD_STUB_LIST(V)                \
    V(StringCharCodeAt)                             \
    V(StringCodePointAt)                            \
    V(StringIndexOf)                                \
    V(StringSubstring)                              \
    V(StringReplace)                                \
    V(StringCharAt)                                 \
    V(StringFromCharCode)                           \
    V(StringTrim)                                   \
    V(StringSlice)                                  \
    V(StringConcat)                                 \
    V(ObjectToString)                               \
    V(ObjectCreate)                                 \
    V(ObjectAssign)                                 \
    V(ObjectHasOwnProperty)                         \
    V(ObjectKeys)                                   \
    V(VectorForEach)                                \
    V(VectorReplaceAllElements)                     \
    V(StackForEach)                                 \
    V(PlainArrayForEach)                            \
    V(QueueForEach)                                 \
    V(DequeForEach)                                 \
    V(LightWeightMapForEach)                        \
    V(LightWeightSetForEach)                        \
    V(HashMapForEach)                               \
    V(HashSetForEach)                               \
    V(LinkedListForEach)                            \
    V(ListForEach)                                  \
    V(ArrayListForEach)                             \
    V(ArrayListReplaceAllElements)                  \
    V(FunctionPrototypeApply)                       \
    V(ArrayConcat)                                  \
    V(ArrayFilter)                                  \
    V(ArrayFind)                                    \
    V(ArrayFindIndex)                               \
    V(ArrayForEach)                                 \
    V(ArrayIndexOf)                                 \
    V(ArrayLastIndexOf)                             \
    V(ArrayPop)                                     \
    V(ArraySlice)                                   \
    V(ArrayValues)                                  \
    V(ArrayReduce)                                  \
    V(ArrayReverse)                                 \
    V(ArrayPush)                                    \
    V(ArrayIncludes)                                \
    V(ArrayFrom)                                    \
    V(ArraySplice)                                  \
    V(SetClear)                                     \
    V(SetValues)                                    \
    V(SetEntries)                                   \
    V(SetForEach)                                   \
    V(SetAdd)                                       \
    V(SetDelete)                                    \
    V(SetHas)                                       \
    V(MapClear)                                     \
    V(MapValues)                                    \
    V(MapEntries)                                   \
    V(MapKeys)                                      \
    V(MapForEach)                                   \
    V(MapSet)                                       \
    V(MapDelete)                                    \
    V(MapHas)                                       \
    V(NumberParseFloat)                             \
    V(TypedArraySubArray)                           \
    V(TypedArrayGetByteLength)                      \
    V(TypedArrayGetByteOffset)

#define BUILTINS_CONSTRUCTOR_STUB_LIST(V)           \
    V(BooleanConstructor)                           \
    V(NumberConstructor)                            \
    V(DateConstructor)                              \
    V(ArrayConstructor)

#define AOT_AND_BUILTINS_STUB_LIST(V)               \
    V(LocaleCompare)                                \
    V(SORT)

#define AOT_BUILTINS_STUB_LIST(V)                   \
    V(SQRT)  /* list start and math list start */   \
    V(COS)                                          \
    V(SIN)                                          \
    V(ACOS)                                         \
    V(ATAN)                                         \
    V(ABS)                                          \
    V(FLOOR)  /* math list end */                   \
    V(STRINGIFY)                                    \
    V(MAP_PROTO_ITERATOR)                           \
    V(SET_PROTO_ITERATOR)                           \
    V(STRING_PROTO_ITERATOR)                        \
    V(ARRAY_PROTO_ITERATOR)                         \
    V(TYPED_ARRAY_PROTO_ITERATOR)                   \
    V(MAP_ITERATOR_PROTO_NEXT)                      \
    V(SET_ITERATOR_PROTO_NEXT)                      \
    V(STRING_ITERATOR_PROTO_NEXT)                   \
    V(ARRAY_ITERATOR_PROTO_NEXT)

class BuiltinsStubCSigns {
public:
    enum ID {
#define DEF_STUB_ID(name) name,
        PADDING_BUILTINS_STUB_LIST(DEF_STUB_ID)
        BUILTINS_STUB_LIST(DEF_STUB_ID)
#undef DEF_STUB_ID
        NUM_OF_BUILTINS_STUBS,
#define DEF_STUB_ID(name) name,
        AOT_BUILTINS_STUB_LIST(DEF_STUB_ID)
#undef DEF_STUB_ID
        BUILTINS_CONSTRUCTOR_STUB_FIRST = BooleanConstructor,
        TYPED_BUILTINS_FIRST = SQRT,
        TYPED_BUILTINS_LAST = ARRAY_ITERATOR_PROTO_NEXT,
        TYPED_BUILTINS_MATH_FIRST = SQRT,
        TYPED_BUILTINS_MATH_LAST = FLOOR,
        INVALID = 0xFF,
    };
    static_assert(ID::NONE == 0);

    static void Initialize();

    static void GetCSigns(std::vector<const CallSignature*>& callSigns);

    static const CallSignature *Get(size_t index)
    {
        ASSERT(index < NUM_OF_BUILTINS_STUBS);
        return &callSigns_[index];
    }

    static const std::string &GetName(int index)
    {
        ASSERT(index < NUM_OF_BUILTINS_STUBS);
        return callSigns_[index].GetName();
    }

    static const CallSignature* BuiltinsCSign()
    {
        return &builtinsCSign_;
    }

    static const CallSignature* BuiltinsWithArgvCSign()
    {
        return &builtinsWithArgvCSign_;
    }

    static bool IsFastBuiltin(ID builtinId)
    {
        return builtinId > NONE && builtinId < NUM_OF_BUILTINS_STUBS;
    }

    static bool IsTypedBuiltin(ID builtinId)
    {
        return (BuiltinsStubCSigns::ID::LocaleCompare == builtinId) ||
               (BuiltinsStubCSigns::ID::SORT == builtinId) ||
               ((BuiltinsStubCSigns::ID::TYPED_BUILTINS_FIRST <= builtinId) &&
               (builtinId <= BuiltinsStubCSigns::ID::TYPED_BUILTINS_LAST));
    }

    static bool IsTypedInlineBuiltin(ID builtinId)
    {
        return BuiltinsStubCSigns::ID::StringFromCharCode == builtinId;
    }

    static bool IsTypedBuiltinMath(ID builtinId)
    {
        return (BuiltinsStubCSigns::ID::TYPED_BUILTINS_MATH_FIRST <= builtinId) &&
               (builtinId <= BuiltinsStubCSigns::ID::TYPED_BUILTINS_MATH_LAST);
    }

    static bool IsTypedBuiltinNumber(ID builtinId)
    {
        return BuiltinsStubCSigns::ID::NumberConstructor == builtinId;
    }

    static bool IsTypedBuiltinCallThis0(ID builtinId)
    {
        switch (builtinId) {
            case BuiltinsStubCSigns::ID::MAP_ITERATOR_PROTO_NEXT:
            case BuiltinsStubCSigns::ID::SET_ITERATOR_PROTO_NEXT:
            case BuiltinsStubCSigns::ID::STRING_ITERATOR_PROTO_NEXT:
            case BuiltinsStubCSigns::ID::ARRAY_ITERATOR_PROTO_NEXT:
                return true;
            default:
                return false;
        }
    }

    static bool IsTypedBuiltinCallThis3(ID builtinId)
    {
        switch (builtinId) {
            case BuiltinsStubCSigns::ID::LocaleCompare:
                return true;
            default:
                return false;
        }
    }

    static ConstantIndex GetConstantIndex(ID builtinId)
    {
        switch (builtinId) {
            case BuiltinsStubCSigns::ID::COS:
                return ConstantIndex::MATH_COS_FUNCTION_INDEX;
            case BuiltinsStubCSigns::ID::SIN:
                return ConstantIndex::MATH_SIN_FUNCTION_INDEX;
            case BuiltinsStubCSigns::ID::ACOS:
                return ConstantIndex::MATH_ACOS_FUNCTION_INDEX;
            case BuiltinsStubCSigns::ID::ATAN:
                return ConstantIndex::MATH_ATAN_FUNCTION_INDEX;
            case BuiltinsStubCSigns::ID::ABS:
                return ConstantIndex::MATH_ABS_FUNCTION_INDEX;
            case BuiltinsStubCSigns::ID::FLOOR:
                return ConstantIndex::MATH_FLOOR_FUNCTION_INDEX;
            case BuiltinsStubCSigns::ID::SQRT:
                return ConstantIndex::MATH_SQRT_FUNCTION_INDEX;
            case BuiltinsStubCSigns::ID::LocaleCompare:
                return ConstantIndex::LOCALE_COMPARE_FUNCTION_INDEX;
            case BuiltinsStubCSigns::ID::SORT:
                return ConstantIndex::ARRAY_SORT_FUNCTION_INDEX;
            case BuiltinsStubCSigns::ID::STRINGIFY:
                return ConstantIndex::JSON_STRINGIFY_FUNCTION_INDEX;
            case BuiltinsStubCSigns::ID::MAP_ITERATOR_PROTO_NEXT:
                return ConstantIndex::MAP_ITERATOR_PROTO_NEXT_INDEX;
            case BuiltinsStubCSigns::ID::SET_ITERATOR_PROTO_NEXT:
                return ConstantIndex::SET_ITERATOR_PROTO_NEXT_INDEX;
            case BuiltinsStubCSigns::ID::STRING_ITERATOR_PROTO_NEXT:
                return ConstantIndex::STRING_ITERATOR_PROTO_NEXT_INDEX;
            case BuiltinsStubCSigns::ID::ARRAY_ITERATOR_PROTO_NEXT:
                return ConstantIndex::ARRAY_ITERATOR_PROTO_NEXT_INDEX;
            case BuiltinsStubCSigns::ID::StringFromCharCode:
                return ConstantIndex::STRING_FROM_CHAR_CODE_INDEX;
            default:
                LOG_COMPILER(FATAL) << "this branch is unreachable";
                UNREACHABLE();
        }
    }

    static size_t GetGlobalEnvIndex(ID builtinId);

    static ID GetBuiltinId(std::string idStr)
    {
        const std::map<std::string, BuiltinsStubCSigns::ID> str2BuiltinId = {
            {"sqrt", SQRT},
            {"cos", COS},
            {"sin", SIN},
            {"acos", ACOS},
            {"atan", ATAN},
            {"abs", ABS},
            {"floor", FLOOR},
            {"localeCompare", LocaleCompare},
            {"sort", SORT},
            {"stringify", STRINGIFY},
        };
        if (str2BuiltinId.count(idStr) > 0) {
            return str2BuiltinId.at(idStr);
        }
        return NONE;
    }

private:
    static CallSignature callSigns_[NUM_OF_BUILTINS_STUBS];
    static CallSignature builtinsCSign_;
    static CallSignature builtinsWithArgvCSign_;
};

enum class BuiltinsArgs : size_t {
    GLUE = 0,
    NATIVECODE,
    FUNC,
    NEWTARGET,
    THISVALUE,
    NUMARGS,
    ARG0_OR_ARGV,
    ARG1,
    ARG2,
    NUM_OF_INPUTS,
};

#define BUILTINS_STUB_ID(name) kungfu::BuiltinsStubCSigns::name
// to distinguish with the positive method offset of js function
#define PGO_BUILTINS_STUB_ID(name) ((-1) * kungfu::BuiltinsStubCSigns::name)
#define IS_TYPED_BUILTINS_ID(id) kungfu::BuiltinsStubCSigns::IsTypedBuiltin(id)
#define IS_TYPED_INLINE_BUILTINS_ID(id) kungfu::BuiltinsStubCSigns::IsTypedInlineBuiltin(id)
#define IS_TYPED_BUILTINS_MATH_ID(id) kungfu::BuiltinsStubCSigns::IsTypedBuiltinMath(id)
#define IS_TYPED_BUILTINS_NUMBER_ID(id) kungfu::BuiltinsStubCSigns::IsTypedBuiltinNumber(id)
#define IS_TYPED_BUILTINS_ID_CALL_THIS0(id) kungfu::BuiltinsStubCSigns::IsTypedBuiltinCallThis0(id)
#define IS_TYPED_BUILTINS_ID_CALL_THIS3(id) kungfu::BuiltinsStubCSigns::IsTypedBuiltinCallThis3(id)
#define GET_TYPED_CONSTANT_INDEX(id) kungfu::BuiltinsStubCSigns::GetConstantIndex(id)
#define GET_TYPED_GLOBAL_ENV_INDEX(id) kungfu::BuiltinsStubCSigns::GetGlobalEnvIndex(id)
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_CALL_SIGNATURE_H
