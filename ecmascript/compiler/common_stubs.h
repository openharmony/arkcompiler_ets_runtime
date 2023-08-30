/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_COMMON_STUBS_H
#define ECMASCRIPT_COMPILER_COMMON_STUBS_H

#include "ecmascript/compiler/stub_builder.h"

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
    V(Instanceof)                     \
    V(TypeOf)                         \
    V(Inc)                            \
    V(Dec)                            \
    V(Neg)                            \
    V(Not)                            \
    V(ToBoolean)                      \
    V(GetPropertyByName)              \
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
    V(StGlobalVar)                    \
    V(SetPropertyByValueWithOwn)      \
    V(TryLoadICByName)                \
    V(TryLoadICByValue)               \
    V(TryStoreICByName)               \
    V(TryStoreICByValue)              \
    V(SetValueWithBarrier)            \
    V(NewLexicalEnv)                  \
    V(GetUnmapedArgs)                 \
    V(NewThisObjectChecked)           \
    V(ConstructorCheck)               \
    V(CreateEmptyArray)               \
    V(CreateArrayWithBuffer)          \
    V(NewJSObject)                    \
    V(JsBoundCallInternal)            \
    V(JsProxyCallInternal)            \
    V(GetCharFromEcmaString)

#define COMMON_STUB_ID_LIST(V)          \
    COMMON_STUB_LIST(V)

#define DECLARE_STUB_CLASS(name)                                                   \
    class name##StubBuilder : public StubBuilder {                                 \
    public:                                                                        \
        explicit name##StubBuilder(CallSignature *callSignature, Environment *env) \
            : StubBuilder(callSignature, env) {}                                   \
        ~name##StubBuilder() = default;                                            \
        NO_MOVE_SEMANTIC(name##StubBuilder);                                       \
        NO_COPY_SEMANTIC(name##StubBuilder);                                       \
        void GenerateCircuit() override;                                           \
    };
    COMMON_STUB_LIST(DECLARE_STUB_CLASS)
#undef DECLARE_STUB_CLASS

class CommonStubCSigns {
public:
    enum ID {
#define DEF_STUB_ID(name) name,
        COMMON_STUB_ID_LIST(DEF_STUB_ID)
#undef DEF_STUB_ID
        NUM_OF_STUBS
    };

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

private:
    static CallSignature callSigns_[NUM_OF_STUBS];
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_COMMON_STUBS_H
