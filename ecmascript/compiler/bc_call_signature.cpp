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

#include "ecmascript/compiler/bc_call_signature.h"
#include "ecmascript/compiler/interpreter_stub.h"

namespace panda::ecmascript::kungfu {
CallSignature BytecodeStubCSigns::callSigns_[BytecodeStubCSigns::NUM_OF_VALID_STUBS];
CallSignature BytecodeStubCSigns::bcHandlerCSign_;
CallSignature BytecodeStubCSigns::bcDebuggerHandlerCSign_;

void BytecodeStubCSigns::Initialize()
{
#define INIT_SIGNATURES(name)                                               \
    BytecodeHandlerCallSignature::Initialize(&callSigns_[name]);            \
    callSigns_[name].SetID(ID_##name);                                      \
    callSigns_[name].SetName(std::string("BCStub_") + #name);               \
    callSigns_[name].SetConstructor(                                        \
    [](void* env) {                                                         \
        return static_cast<void*>(                                          \
            new name##StubBuilder(&callSigns_[name],                        \
                static_cast<Environment*>(env)));                           \
    });

    INTERPRETER_BC_STUB_LIST(INIT_SIGNATURES)
#define INIT_SIGNATURES_DYN(name, ...) \
    INIT_SIGNATURES(name)              \
    callSigns_[name].SetTargetKind(CallSignature::TargetKind::BYTECODE_PROFILE_HANDLER);
    ASM_INTERPRETER_BC_PROFILER_STUB_LIST(INIT_SIGNATURES_DYN)
#undef INIT_SIGNATURES_DYN

#define INIT_SIGNATURES_DYN(name, ...) \
    INIT_SIGNATURES(name)              \
    callSigns_[name].SetTargetKind(CallSignature::TargetKind::BYTECODE_JIT_PROFILE_HANDLER);
    ASM_INTERPRETER_BC_JIT_PROFILER_STUB_LIST(INIT_SIGNATURES_DYN)
#undef INIT_SIGNATURES_DYN

#define INIT_SIGNATURES_DYN(name, ...)              \
    INIT_SIGNATURES(name##StwCopy)                  \
    callSigns_[name##StwCopy].SetStwCopyStub(true); \
    callSigns_[name##StwCopy].SetTargetKind(CallSignature::TargetKind::BYTECODE_STW_COPY_HANDLER);
    ASM_INTERPRETER_BC_STW_COPY_STUB_LIST(INIT_SIGNATURES_DYN)
#undef INIT_SIGNATURES_DYN
#undef INIT_SIGNATURES

#define INIT_HELPER_SIGNATURES(name)                                                        \
    BytecodeHandlerCallSignature::Initialize(&callSigns_[name]);                            \
    callSigns_[name].SetID(ID_##name);                                                      \
    callSigns_[name].SetName(std::string("BCStub_") + #name);                               \
    callSigns_[name].SetTargetKind(CallSignature::TargetKind::BYTECODE_HELPER_HANDLER);     \
    callSigns_[name].SetConstructor(                                                        \
    [](void* env) {                                                                         \
        return static_cast<void*>(                                                          \
            new name##StubBuilder(&callSigns_[name], static_cast<Environment*>(env)));      \
    });

    ASM_INTERPRETER_BC_HELPER_STUB_LIST(INIT_HELPER_SIGNATURES)
#undef INIT_HELPER_SIGNATURES

    BytecodeHandlerCallSignature::Initialize(&bcHandlerCSign_);
    BytecodeDebuggerHandlerCallSignature::Initialize(&bcDebuggerHandlerCSign_);
}

void BytecodeStubCSigns::GetCSigns(std::vector<const CallSignature*>& outCSigns)
{
    for (size_t i = 0; i < NUM_OF_VALID_STUBS; i++) {
        outCSigns.push_back(&callSigns_[i]);
    }
}

void BytecodeStubCSigns::GetNormalCSigns(std::vector<const CallSignature*>& outCSigns)
{
    for (size_t i = VID_NORMAL_START; i < VID_NORMAL_END; i++) {
        outCSigns.push_back(&callSigns_[i]);
    }
}

void BytecodeStubCSigns::GetStwCopyCSigns(std::vector<const CallSignature*>& outCSigns)
{
    for (size_t i = VID_STW_COPY_START; i < VID_STW_COPY_END; i++) {
        outCSigns.push_back(&callSigns_[i]);
    }
}
}  // namespace panda::ecmascript::kungfu
