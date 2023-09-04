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
    BUILTINS_CONSTRUCTOR_STUB_LIST(V)

#define BUILTINS_METHOD_STUB_LIST(V)                \
    V(CharCodeAt)                                   \
    V(IndexOf)                                      \
    V(Substring)                                    \
    V(CharAt)                                       \
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
    V(ArrayForEach)                                 \
    V(ArrayIndexOf)                                 \
    V(ArrayLastIndexOf)                             \
    V(ArraySlice)                                   \
    V(ArrayReverse)                                 \
    V(SetClear)                                     \
    V(SetValues)                                    \
    V(SetEntries)                                   \
    V(SetForEach)                                   \
    V(MapClear)                                     \
    V(MapValues)                                    \
    V(MapEntries)                                   \
    V(MapKeys)                                      \
    V(MapForEach)

#define BUILTINS_CONSTRUCTOR_STUB_LIST(V)           \
    V(BooleanConstructor)                           \
    V(DateConstructor)                              \
    V(ArrayConstructor)

#define AOT_BUILTINS_STUB_LIST(V)                   \
    V(SQRT)  /* list start and math list start */   \
    V(COS)                                          \
    V(SIN)                                          \
    V(ACOS)                                         \
    V(ATAN)                                         \
    V(ABS)                                          \
    V(FLOOR)  /* math list end */                   \
    V(LocaleCompare)                                \
    V(SORT)                                         \
    V(STRINGIFY)

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
        TYPED_BUILTINS_LAST = STRINGIFY,
        TYPED_BUILTINS_MATH_FIRST = SQRT,
        TYPED_BUILTINS_MATH_LAST = FLOOR,
        INVALID = 0xFF,
    };

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
        return (BuiltinsStubCSigns::ID::TYPED_BUILTINS_FIRST <= builtinId) &&
               (builtinId <= BuiltinsStubCSigns::ID::TYPED_BUILTINS_LAST);
    }

    static bool IsTypedBuiltinMath(ID builtinId)
    {
        return (BuiltinsStubCSigns::ID::TYPED_BUILTINS_MATH_FIRST <= builtinId) &&
               (builtinId <= BuiltinsStubCSigns::ID::TYPED_BUILTINS_MATH_LAST);
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
            default:
                LOG_COMPILER(FATAL) << "this branch is unreachable";
                UNREACHABLE();
        }
    }

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
#define IS_TYPED_BUILTINS_ID(id) kungfu::BuiltinsStubCSigns::IsTypedBuiltin(id)
#define IS_TYPED_BUILTINS_MATH_ID(id) kungfu::BuiltinsStubCSigns::IsTypedBuiltinMath(id)
#define GET_TYPED_CONSTANT_INDEX(id) kungfu::BuiltinsStubCSigns::GetConstantIndex(id)
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_CALL_SIGNATURE_H
