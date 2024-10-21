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
#include "ecmascript/compiler/builtins/builtins_call_signature.h"
#include "ecmascript/compiler/builtins/builtins_stubs.h"
#include "ecmascript/compiler/call_signature.h"
#include "ecmascript/global_env_fields.h"

namespace panda::ecmascript::kungfu {
CallSignature BuiltinsStubCSigns::callSigns_[BuiltinsStubCSigns::NUM_OF_BUILTINS_STUBS];
CallSignature BuiltinsStubCSigns::builtinsCSign_;
CallSignature BuiltinsStubCSigns::builtinsWithArgvCSign_;

void BuiltinsStubCSigns::Initialize()
{
#define COMMON_INIT(name)                                            \
    callSigns_[name].SetID(name);                                    \
    callSigns_[name].SetName(std::string("BuiltinStub_") + #name);   \
    callSigns_[name].SetConstructor(                                 \
    [](void* env) {                                                  \
        return static_cast<void*>(                                   \
            new name##StubBuilder(&callSigns_[name],                 \
                static_cast<Environment*>(env)));                    \
    });

#define INIT_BUILTINS_METHOD(name)                                   \
    BuiltinsCallSignature::Initialize(&callSigns_[name]);            \
    COMMON_INIT(name)

#define INIT_BUILTINS_METHOD_DYN(name, type, ...)                    \
    BuiltinsCallSignature::Initialize(&callSigns_[type##name]);      \
    COMMON_INIT(type##name)

#define INIT_BUILTINS_CONSTRUCTOR_METHOD(name)                       \
    BuiltinsWithArgvCallSignature::Initialize(&callSigns_[name]);    \
    COMMON_INIT(name)

    BUILTINS_STUB_LIST(INIT_BUILTINS_METHOD, INIT_BUILTINS_METHOD_DYN, INIT_BUILTINS_CONSTRUCTOR_METHOD)
#undef INIT_BUILTINS_METHOD_DYN
#undef INIT_BUILTINS_METHOD

#undef COMMON_INIT
    BuiltinsCallSignature::Initialize(&builtinsCSign_);
    BuiltinsWithArgvCallSignature::Initialize(&builtinsWithArgvCSign_);
}

void BuiltinsStubCSigns::GetCSigns(std::vector<const CallSignature*>& outCSigns)
{
    const size_t firstStubId = BUILTINS_STUB_ID(NONE) + 1;
    for (size_t i = firstStubId; i < NUM_OF_BUILTINS_STUBS; i++) {
        outCSigns.push_back(&callSigns_[i]);
    }
}

size_t BuiltinsStubCSigns::GetGlobalEnvIndex(ID builtinId)
{
    switch (builtinId) {
        case BuiltinsStubCSigns::ID::NumberConstructor:
            return static_cast<size_t>(GlobalEnvField::NUMBER_FUNCTION_INDEX);
        case BuiltinsStubCSigns::ID::BigIntConstructor:
            return static_cast<size_t>(GlobalEnvField::BIGINT_FUNCTION_INDEX);
        default:
            LOG_COMPILER(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}
}  // namespace panda::ecmascript::kungfu
