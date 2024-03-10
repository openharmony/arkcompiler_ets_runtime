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

#include "ecmascript/method.h"

#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/program_object.h"

namespace panda::ecmascript {
std::string Method::ParseFunctionName() const
{
    const JSPandaFile *jsPandaFile = GetJSPandaFile();
    return MethodLiteral::ParseFunctionName(jsPandaFile, GetMethodId());
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
    if (IsAotWithCallField()) {
        ASSERT(!IsNativeWithCallField());
        const JSPandaFile *jsPandaFile = GetJSPandaFile();
        return jsPandaFile->FindMethodLiteral(GetMethodId().GetOffset());
    }
    return reinterpret_cast<MethodLiteral *>(GetCodeEntryOrLiteral());
}

uint32_t Method::FindCatchBlock(uint32_t pc) const
{
    ASSERT(!IsNativeWithCallField());
    auto *pandaFile = GetJSPandaFile()->GetPandaFile();
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

JSHandle<Method> Method::Create(JSThread *thread, const JSPandaFile *jsPandaFile, MethodLiteral *methodLiteral)
{
    EcmaVM *vm = thread->GetEcmaVM();
    EntityId methodId = methodLiteral->GetMethodId();
    JSTaggedValue patchVal = vm->GetQuickFixManager()->CheckAndGetPatch(thread, jsPandaFile, methodId);
    if (!patchVal.IsHole()) {
        return JSHandle<Method>(thread, patchVal);
    }

    JSHandle<Method> method = vm->GetFactory()->NewMethod(methodLiteral);
    JSHandle<ConstantPool> newConstpool = thread->GetCurrentEcmaContext()->FindOrCreateConstPool(jsPandaFile, methodId);
    method->SetConstantPool(thread, newConstpool);
    return method;
}

const JSTaggedValue Method::GetRecordName() const
{
    JSTaggedValue module = GetModule();
    if (module.IsSourceTextModule()) {
        JSTaggedValue recordName = SourceTextModule::Cast(module.GetTaggedObject())->GetEcmaModuleRecordName();
        if (!recordName.IsString()) {
            LOG_INTERPRETER(DEBUG) << "module record name is undefined";
            return JSTaggedValue::Hole();
        }
        return recordName;
    }
    if (module.IsString()) {
        return module;
    }
    LOG_INTERPRETER(DEBUG) << "record name is undefined";
    return JSTaggedValue::Hole();
}

void Method::SetCompiledFuncEntry(uintptr_t codeEntry, bool isFastCall)
{
    ASSERT(codeEntry != 0);
    SetCodeEntryAndMarkAOTWhenBinding(codeEntry);

    SetIsFastCall(isFastCall);
    MethodLiteral *methodLiteral = GetMethodLiteral();
    methodLiteral->SetAotCodeBit(true);
    methodLiteral->SetIsFastCall(isFastCall);
}

void Method::SetCodeEntryAndMarkAOTWhenBinding(uintptr_t codeEntry)
{
    SetAotCodeBit(true);
    SetNativeBit(false);
    SetCodeEntryOrLiteral(codeEntry);
}

void Method::ClearAOTStatusWhenDeopt()
{
    ClearAOTFlagsWhenInit();
    SetDeoptType(kungfu::DeoptType::NOTCHECK);
    const JSPandaFile *jsPandaFile = GetJSPandaFile();
    MethodLiteral *methodLiteral = jsPandaFile->FindMethodLiteral(GetMethodId().GetOffset());
    SetCodeEntryOrLiteral(reinterpret_cast<uintptr_t>(methodLiteral));
}

void Method::ClearAOTFlagsWhenInit()
{
    SetAotCodeBit(false);
    SetIsFastCall(false);
}
} // namespace panda::ecmascript
