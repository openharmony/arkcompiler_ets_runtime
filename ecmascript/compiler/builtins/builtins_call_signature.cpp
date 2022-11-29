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
#include "ecmascript/compiler/builtins/builtins_call_signature.h"
#include "ecmascript/compiler/builtins/builtins_stubs.h"
#include "ecmascript/compiler/call_signature.h"
#include "ecmascript/stubs/runtime_stubs.h"

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

    BUILTINS_METHOD_STUB_LIST(INIT_BUILTINS_METHOD)
#undef INIT_BUILTINS_METHOD

#define INIT_BUILTINS_CONSTRUCTOR(name)                              \
    BuiltinsWithArgvCallSignature::Initialize(&callSigns_[name]);    \
    COMMON_INIT(name)

    BUILTINS_CONSTRUCTOR_STUB_LIST(INIT_BUILTINS_CONSTRUCTOR)
#undef INIT_BUILTINS_CONSTRUCTOR
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
}  // namespace panda::ecmascript::kungfu
