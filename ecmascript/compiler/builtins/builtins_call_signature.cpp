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

void BuiltinsStubCSigns::Initialize()
{
#define INIT_SIGNATURES(name)                                            \
    BuiltinsCallSignature::Initialize(&callSigns_[name]);                \
    callSigns_[name].SetID(name);                                        \
    callSigns_[name].SetName(#name);                                     \
    callSigns_[name].SetConstructor(                                     \
    [](void* env) {                                                   \
        return static_cast<void*>(                                       \
            new name##StubBuilder(&callSigns_[name],                     \
                static_cast<Environment*>(env)));                        \
    });
    BUILTINS_STUB_LIST(INIT_SIGNATURES)
#undef INIT_SIGNATURES
    BuiltinsCallSignature::Initialize(&builtinsCSign_);
}

void BuiltinsStubCSigns::GetCSigns(std::vector<const CallSignature*>& outCSigns)
{
    for (size_t i = 0; i < NUM_OF_BUILTINS_STUBS; i++) {
        outCSigns.push_back(&callSigns_[i]);
    }
}
}  // namespace panda::ecmascript::kungfu
