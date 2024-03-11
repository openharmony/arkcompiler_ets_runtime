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
    V(StringStartsWith)                             \
    V(StringToLowerCase)                            \
    V(StringEndsWith)                               \
    V(GetStringIterator)                            \
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
    V(ArrayMap)                                     \
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
    V(MapGet)                                       \
    V(NumberParseFloat)                             \
    V(TypedArraySubArray)                           \
    V(TypedArrayGetByteLength)                      \
    V(TypedArrayGetByteOffset)

#define BUILTINS_CONSTRUCTOR_STUB_LIST(V)           \
    V(BooleanConstructor)                           \
    V(NumberConstructor)                            \
    V(DateConstructor)                              \
    V(ArrayConstructor)                             \
    V(SetConstructor)                               \
    V(MapConstructor)

#define AOT_AND_BUILTINS_STUB_LIST(V)               \
    V(LocaleCompare)                                \
    V(SORT)

#define AOT_BUILTINS_STUB_LIST(V)                   \
    V(SQRT)  /* list start and math list start */   \
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
    V(ARRAY_ITERATOR_PROTO_NEXT)                    \
    V(ITERATOR_PROTO_RETURN)

// List of builtins which will try to be inlined in TypedNativeInlineLoweringPass
#define AOT_BUILTINS_INLINE_LIST(V)                 \
    V(MathAcos)                                     \
    V(MathAcosh)                                    \
    V(MathAsin)                                     \
    V(MathAsinh)                                    \
    V(MathAtan)                                     \
    V(MathAtan2)                                    \
    V(MathAtanh)                                    \
    V(MathCos)                                      \
    V(MathCosh)                                     \
    V(MathSin)                                      \
    V(MathSinh)                                     \
    V(MathTan)                                      \
    V(MathTanh)                                     \
    V(MathLog)                                      \
    V(MathLog2)                                     \
    V(MathLog10)                                    \
    V(MathLog1p)                                    \
    V(MathPow)                                      \
    V(MathAbs)                                      \
    V(TYPED_BUILTINS_INLINE_FIRST = MathAcos)       \
    V(TYPED_BUILTINS_INLINE_LAST = MathAbs)

class BuiltinsStubCSigns {
public:
    enum ID {
#define DEF_STUB_ID(name) name,
        PADDING_BUILTINS_STUB_LIST(DEF_STUB_ID)
        BUILTINS_STUB_LIST(DEF_STUB_ID)
        NUM_OF_BUILTINS_STUBS,
        AOT_BUILTINS_STUB_LIST(DEF_STUB_ID)
        AOT_BUILTINS_INLINE_LIST(DEF_STUB_ID)
#undef DEF_STUB_ID
        BUILTINS_CONSTRUCTOR_STUB_FIRST = BooleanConstructor,
        TYPED_BUILTINS_FIRST = SQRT,
        TYPED_BUILTINS_LAST = ITERATOR_PROTO_RETURN,
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
        if (TYPED_BUILTINS_INLINE_FIRST <= builtinId && builtinId <= TYPED_BUILTINS_INLINE_LAST) {
            return true;
        }
        // NOTE(schernykh): try to remove this switch and move StringFromCharCode to TYPED_BUILTINS_INLINE list
        switch (builtinId) {
            case BuiltinsStubCSigns::ID::StringFromCharCode:
                return true;
            default:
                return false;
        }
        return false;
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
            case BuiltinsStubCSigns::ID::ITERATOR_PROTO_RETURN:
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
            case BuiltinsStubCSigns::ID::MathAcos:
                return ConstantIndex::MATH_ACOS_INDEX;
            case BuiltinsStubCSigns::ID::MathAcosh:
                return ConstantIndex::MATH_ACOSH_INDEX;
            case BuiltinsStubCSigns::ID::MathAsin:
                return ConstantIndex::MATH_ASIN_INDEX;
            case BuiltinsStubCSigns::ID::MathAsinh:
                return ConstantIndex::MATH_ASINH_INDEX;
            case BuiltinsStubCSigns::ID::MathAtan:
                return ConstantIndex::MATH_ATAN_INDEX;
            case BuiltinsStubCSigns::ID::MathAtan2:
                return ConstantIndex::MATH_ATAN2_INDEX;
            case BuiltinsStubCSigns::ID::MathAtanh:
                return ConstantIndex::MATH_ATANH_INDEX;
            case BuiltinsStubCSigns::ID::MathCos:
                return ConstantIndex::MATH_COS_INDEX;
            case BuiltinsStubCSigns::ID::MathCosh:
                return ConstantIndex::MATH_COSH_INDEX;
            case BuiltinsStubCSigns::ID::MathSin:
                return ConstantIndex::MATH_SIN_INDEX;
            case BuiltinsStubCSigns::ID::MathSinh:
                return ConstantIndex::MATH_SINH_INDEX;
            case BuiltinsStubCSigns::ID::MathTan:
                return ConstantIndex::MATH_TAN_INDEX;
            case BuiltinsStubCSigns::ID::MathTanh:
                return ConstantIndex::MATH_TANH_INDEX;
            case BuiltinsStubCSigns::ID::MathAbs:
                return ConstantIndex::MATH_ABS_INDEX;
            case BuiltinsStubCSigns::ID::MathLog:
                return ConstantIndex::MATH_LOG_INDEX;
            case BuiltinsStubCSigns::ID::MathLog2:
                return ConstantIndex::MATH_LOG2_INDEX;
            case BuiltinsStubCSigns::ID::MathLog10:
                return ConstantIndex::MATH_LOG10_INDEX;
            case BuiltinsStubCSigns::ID::MathLog1p:
                return ConstantIndex::MATH_LOG1P_INDEX;
            case BuiltinsStubCSigns::ID::MathPow:
                return ConstantIndex::MATH_POW_INDEX;
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
            case BuiltinsStubCSigns::ID::ITERATOR_PROTO_RETURN:
                return ConstantIndex::ITERATOR_PROTO_RETURN_INDEX;
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
            {"Acos", MathAcos},
            {"Acosh", MathAcosh},
            {"Asin", MathAsin},
            {"Asinh", MathAsinh},
            {"Atan", MathAtan},
            {"Atan2", MathAtan2},
            {"Atanh", MathAtanh},
            {"Cos", MathCos},
            {"Cosh", MathCosh},
            {"Sin", MathSin},
            {"Sinh", MathSinh},
            {"Tan", MathTan},
            {"Tanh", MathTanh},
            {"Log", MathLog},
            {"Log2", MathLog2},
            {"Log10", MathLog10},
            {"Log1p", MathLog1p},
            {"sqrt", SQRT},
            {"abs", MathAbs},
            {"pow", MathPow},
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
