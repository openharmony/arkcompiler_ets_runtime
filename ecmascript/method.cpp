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

#include "ecmascript/method.h"

#include "ecmascript/jspandafile/program_object.h"

namespace panda::ecmascript {
std::string Method::ParseFunctionName() const
{
    const JSPandaFile *jsPandaFile = GetJSPandaFile();
    return MethodLiteral::ParseFunctionName(jsPandaFile, GetMethodId());
}

std::pair<std::string_view, bool> Method::ParseFunctionNameView() const
{
    const JSPandaFile *jsPandaFile = GetJSPandaFile();
    return MethodLiteral::ParseFunctionNameView(jsPandaFile, GetMethodId());
}

const char *Method::GetMethodName() const
{
    const JSPandaFile *jsPandaFile = GetJSPandaFile();
    return MethodLiteral::GetMethodName(jsPandaFile, GetMethodId());
}

const char *Method::GetMethodName(const JSPandaFile *file) const
{
    return MethodLiteral::GetMethodName(file, GetMethodId());
}

const CString Method::GetRecordNameStr() const
{
    const JSPandaFile *jsPandaFile = GetJSPandaFile();
    return MethodLiteral::GetRecordName(jsPandaFile, GetMethodId());
}

uint32_t Method::GetCodeSize() const
{
    const JSPandaFile *jsPandaFile = GetJSPandaFile();
    return MethodLiteral::GetCodeSize(jsPandaFile, GetMethodId());
}

const JSPandaFile *Method::GetJSPandaFile() const
{
    JSTaggedValue constpool = GetConstantPool();
    if (constpool.IsUndefined()) {
        return nullptr;
    }

    const ConstantPool *taggedPool = ConstantPool::Cast(constpool.GetTaggedObject());
    return taggedPool->GetJSPandaFile();
}

MethodLiteral *Method::GetMethodLiteral() const
{
    if (IsAotWithCallField() || IsDeoptimized()) {
        ASSERT(!IsNativeWithCallField());
        const JSPandaFile *jsPandaFile = GetJSPandaFile();
        ASSERT(jsPandaFile != nullptr);
        return jsPandaFile->FindMethodLiteral(GetMethodId().GetOffset());
    }
    return reinterpret_cast<MethodLiteral *>(GetCodeEntryOrLiteral());
}

bool Method::IsDeoptimized() const
{
    return GetDeoptType() != kungfu::DeoptType::NONE;
}

uint32_t Method::FindCatchBlock(uint32_t pc) const
{
    ASSERT(!IsNativeWithCallField());
    ASSERT(GetJSPandaFile() != nullptr);
    auto *pandaFile = GetJSPandaFile()->GetPandaFile();
    ASSERT(pandaFile != nullptr);
    panda_file::MethodDataAccessor mda(*pandaFile, GetMethodId());
    panda_file::CodeDataAccessor cda(*pandaFile, mda.GetCodeId().value());

    uint32_t pcOffset = INVALID_INDEX;
    cda.EnumerateTryBlocks([&pcOffset, pc](panda_file::CodeDataAccessor::TryBlock &tryBlock) {
        if ((tryBlock.GetStartPc() <= pc) && ((tryBlock.GetStartPc() + tryBlock.GetLength()) > pc)) {
            tryBlock.EnumerateCatchBlocks([&](panda_file::CodeDataAccessor::CatchBlock &catchBlock) {
                pcOffset = catchBlock.GetHandlerPc();
                return false;
            });
        }
        return pcOffset == INVALID_INDEX;
    });
    return pcOffset;
}

bool Method::HasCatchBlock() const
{
    ASSERT(GetJSPandaFile() != nullptr);
    auto *pandaFile = GetJSPandaFile()->GetPandaFile();
    ASSERT(pandaFile != nullptr);
    panda_file::MethodDataAccessor mda(*pandaFile, GetMethodId());
    panda_file::CodeDataAccessor cda(*pandaFile, mda.GetCodeId().value());
    return cda.GetTriesSize() != 0;
}

JSHandle<Method> Method::Create(JSThread *thread, const JSPandaFile *jsPandaFile, MethodLiteral *methodLiteral)
{
    EcmaVM *vm = thread->GetEcmaVM();
    EntityId methodId = methodLiteral->GetMethodId();
    JSTaggedValue patchVal = vm->GetQuickFixManager()->CheckAndGetPatch(thread, jsPandaFile, methodId);
    if (!patchVal.IsHole()) {
        return JSHandle<Method>(thread, patchVal);
    }

    JSHandle<Method> method;
    method = vm->GetFactory()->NewSMethod(methodLiteral);
    JSHandle<ConstantPool> newConstpool = vm->FindOrCreateConstPool(jsPandaFile, methodId);
    method->SetConstantPool(thread, newConstpool);
    return method;
}

void Method::SetCodeEntryAndMarkAOTWhenBinding(uintptr_t codeEntry)
{
    SetAotCodeBit(true);
    SetNativeBit(false);
    SetCodeEntryOrLiteral(codeEntry);
}

void Method::ClearAOTStatusWhenDeopt(uintptr_t entry)
{
    ClearAOTFlagsWhenInit();
    // Do not clear deopt type, which records a method has deoptimized before
    SetCodeEntryOrLiteral(entry);
}

void Method::ClearAOTFlagsWhenInit()
{
    SetAotCodeBit(false);
    SetIsFastCall(false);
}

void Method::InitInterpreterStatusForCompiledMethod(const JSThread *thread)
{
    if (!IsAotWithCallField()) {
        return;
    }
    bool isFastCall = IsFastCall();
    uintptr_t entry =
        isFastCall ? thread->GetRTInterface(kungfu::RuntimeStubCSigns::ID_FastCallToAsmInterBridge)
                   : thread->GetRTInterface(kungfu::RuntimeStubCSigns::ID_AOTCallToAsmInterBridge);
    SetCodeEntryOrLiteral(entry);
    ClearAOTFlagsWhenInit();
    SetDeoptType(kungfu::DeoptType::INIT_AOT_FAILED);
}
} // namespace panda::ecmascript
