/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_COMMON_STUB_CSIGNS_H
#define ECMASCRIPT_COMPILER_COMMON_STUB_CSIGNS_H

#include "ecmascript/compiler/call_signature.h"

namespace panda::ecmascript::kungfu {
#define COMMON_STUB_LIST(V)           \
    V(Add)                            \
    V(Sub)                            \
    V(Mul)                            \
    V(Div)                            \
    V(Mod)                            \
    V(Equal)                          \
    V(NotEqual)                       \
    V(StrictEqual)                    \
    V(StrictNotEqual)                 \
    V(Less)                           \
    V(LessEq)                         \
    V(Greater)                        \
    V(GreaterEq)                      \
    V(Shl)                            \
    V(Shr)                            \
    V(Ashr)                           \
    V(And)                            \
    V(Or)                             \
    V(Xor)                            \
    V(IsIn)                           \
    V(Instanceof)                     \
    V(OrdinaryHasInstance)            \
    V(TypeOf)                         \
    V(Inc)                            \
    V(Dec)                            \
    V(Neg)                            \
    V(Not)                            \
    V(ToBooleanTrue)                  \
    V(ToBooleanFalse)                 \
    V(JSTaggedValueHasProperty)       \
    V(GetPropertyByName)              \
    V(SetPropertyByNameWithMega)      \
    V(GetPropertyByNameWithMega)      \
    V(DeprecatedGetPropertyByName)    \
    V(SetPropertyByName)              \
    V(DeprecatedSetPropertyByName)    \
    V(SetPropertyByNameWithOwn)       \
    V(GetPropertyByIndex)             \
    V(SetPropertyByIndex)             \
    V(SetPropertyByIndexWithOwn)      \
    V(GetPropertyByValue)             \
    V(DeprecatedGetPropertyByValue)   \
    V(SetPropertyByValue)             \
    V(DeprecatedSetPropertyByValue)   \
    V(TryLdGlobalByName)              \
    V(TryStGlobalByName)              \
    V(LdGlobalVar)                    \
    V(LdObjByIndex)                   \
    V(StGlobalVar)                    \
    V(StObjByIndex)                   \
    V(StOwnByIndex)                   \
    V(StOwnByName)                    \
    V(StOwnByNameWithNameSet)         \
    V(StOwnByValue)                   \
    V(StOwnByValueWithNameSet)        \
    V(SetPropertyByValueWithOwn)      \
    V(TryLoadICByName)                \
    V(TryLoadICByValue)               \
    V(TryStoreICByName)               \
    V(TryStoreICByValue)              \
    V(SetValueWithBarrier)            \
    V(SetNonSValueWithBarrier)        \
    V(SetSValueWithBarrier)           \
    V(NewLexicalEnv)                  \
    V(CopyRestArgs)                   \
    V(GetUnmappedArgs)                \
    V(GetCallSpreadArgs)              \
    V(NewThisObjectChecked)           \
    V(ConstructorCheck)               \
    V(CreateEmptyArray)               \
    V(CreateArrayWithBuffer)          \
    V(CreateObjectHavingMethod)       \
    V(NewJSObject)                    \
    V(FastNewThisObject)              \
    V(JsBoundCallInternal)            \
    V(CreateStringBySingleCharCode)   \
    V(Getpropiterator)                \
    V(Getnextpropname)                \
    V(CreateJSSetIterator)            \
    V(JSSetEntries)                   \
    V(CreateJSMapIterator)            \
    V(JSMapKeys)                      \
    V(JSMapValues)                    \
    V(JSMapGet)                       \
    V(JSMapHas)                       \
    V(JSSetHas)                       \
    V(JSSetAdd)                       \
    V(JSProxyGetProperty)             \
    V(JSProxySetProperty)             \
    V(JSProxySetPropertyNoThrow)      \
    V(CreateJSTypedArrayEntries)      \
    V(CreateJSTypedArrayKeys)         \
    V(CreateJSTypedArrayValues)       \
    V(JSMapDelete)                    \
    V(JSSetDelete)                    \
    V(GetSingleCharCodeByIndex)       \
    V(FastStringEqual)                \
    V(FastStringAdd)                  \
    V(StringAdd)                      \
    V(Definefunc)                     \
    V(DefineField)                    \
    V(CallArg0Stub)                   \
    V(CallArg1Stub)                   \
    V(CallArg2Stub)                   \
    V(CallArg3Stub)                   \
    V(CallThis0Stub)                  \
    V(CallThis1Stub)                  \
    V(CallThis2Stub)                  \
    V(CallThis3Stub)                  \
    V(NewFloat32ArrayWithNoArgs)      \
    V(NewFloat32Array)                \
    V(StringLoadElement)              \
    V(GetStringFromConstPool)         \
    V(GetPrototype)                   \
    V(FastCallSelector)               \
    V(CheckSuperAndNew)               \
    V(SuperCallAndConstructorCheck)   \
    V(ConvertCharToInt32)             \
    V(ConvertCharToDouble)            \
    V(ConvertCharToString)            \
    V(DeleteObjectProperty)           \
    V(SameValue)                      \
    V(StringIteratorNext)             \
    V(ArrayIteratorNext)              \
    V(MapIteratorNext)                \
    V(SetIteratorNext)                \
    V(GetIterator)                    \
    V(GrowElementsCapacity)           \
    V(BatchBarrier)                   \
    V(VerifyBarrier)                  \
    V(MoveBarrierInRegion)            \
    V(MoveBarrierCrossRegion)         \
    V(FindEntryFromNameDictionary)    \
    V(ReverseBarrier)                 \
    V(GetValueWithBarrier)            \
    V(ComputeStringHashcode)          \
    V(DefineNormalFuncForJit)         \
    V(DefineArrowFuncForJit)          \
    V(DefineBaseConstructorForJit)

#define COMMON_STUB_ID_LIST(V)          \
    COMMON_STUB_LIST(V)

#define COMMON_STW_COPY_STUB_LIST(V) \
    COMMON_STUB_LIST(V)

class CommonStubCSigns {
public:
    enum ID {
#define DEF_STUB_ID(name) name,
        COMMON_STUB_ID_LIST(DEF_STUB_ID)
#undef DEF_STUB_ID
#define DEF_STUB_ID(name) name##StwCopy,
        COMMON_STW_COPY_STUB_LIST(DEF_STUB_ID)
#undef DEF_STUB_ID
        NUM_OF_STUBS,
        NUM_OF_ALL_NORMAL_STUBS = DefineBaseConstructorForJit + 1,
    };
#define ASSERT_ID_EQUAL(name)                                 \
    static_assert((static_cast<uint32_t>(ID::name##StwCopy)) == \
                  (static_cast<uint32_t>(ID::name) + ID::NUM_OF_ALL_NORMAL_STUBS));

    COMMON_STUB_ID_LIST(ASSERT_ID_EQUAL)
#undef ASSERT_ID_EQUAL

    static void Initialize();

    static void GetCSigns(std::vector<const CallSignature*>& callSigns);

    static const CallSignature *Get(size_t index)
    {
        ASSERT(index < NUM_OF_STUBS);
        return &callSigns_[index];
    }

    static const std::string &GetName(size_t index)
    {
        ASSERT(index < NUM_OF_STUBS);
        return callSigns_[index].GetName();
    }

    static bool IsReadOnly(size_t index)
    {
        ASSERT(index < NUM_OF_STUBS);
        switch (index) {
            case GetValueWithBarrier:
                return true;
            default:
                return false;
        }
        return false;
    }

    static bool IsCold(size_t index)
    {
        ASSERT(index < NUM_OF_STUBS);
        switch (index) {
            case GetValueWithBarrier:
                return true;
            default:
                return false;
        }
        return false;
    }

private:
    static CallSignature callSigns_[NUM_OF_STUBS];
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_COMMON_STUB_CSIGNS_H
