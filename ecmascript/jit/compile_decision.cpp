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

#include "ecmascript/jit/compile_decision.h"
#include "ecmascript/jit/jit.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/ic/profile_type_info.h"

namespace panda::ecmascript {
CompileDecision::CompileDecision(EcmaVM *vm, JSHandle<JSFunction> &jsFunction, CompilerTier tier,
    int32_t osrOffset, JitCompileMode mode) : vm_(vm), jsFunction_(jsFunction),
    tier_(tier), osrOffset_(osrOffset), compileMode_(mode) { }

CString CompileDecision::GetMethodInfo() const
{
    uint32_t codeSize = GetCodeSize();
    return GetMethodName() + ", bytecode size:" + ToCString(codeSize);
}

CString CompileDecision::GetMethodName() const
{
    Method *method = Method::Cast(jsFunction_->GetMethod().GetTaggedObject());
    ASSERT(method != nullptr);
    auto jSPandaFile = method->GetJSPandaFile();
    CString fileDesc;
    if (jSPandaFile != nullptr) {
        fileDesc = jSPandaFile->GetJSPandaFileDesc();
    }
    return fileDesc + ":" + method->GetRecordNameStr() + "." + CString(method->GetMethodName());
}

uint32_t CompileDecision::GetCodeSize() const
{
    Method *method = Method::Cast(jsFunction_->GetMethod().GetTaggedObject());
    return method->GetCodeSize();
}

bool CompileDecision::Decision()
{
    return IsGoodCompilationRequest();
}

bool CompileDecision::IsGoodCompilationRequest() const
{
    if (!CheckJsFunctionStatus()) {
        return false;
    }

    if (!IsJsFunctionSupportCompile()) {
        DisableJitCompile();
        return false;
    }

    if (!CheckVmState()) {
        return false;
    }
    return true;
}

bool CompileDecision::IsJsFunctionSupportCompile() const
{
    if (!IsSupportFunctionKind()) {
        return false;
    }

    uint32_t maxSize = 9000;
    if (vm_->GetJSOptions().IsEnableJitFastCompile()) {
        maxSize = 15; // 15 is method codesize threshold during fast compiling
    }
    if (GetCodeSize() > maxSize && !(vm_->GetJSOptions().IsEnableForceJitCompileMain() && compileMode_.IsSync())) {
        LOG_JIT(DEBUG) << tier_ << "skip jit task, as too large:" << GetMethodInfo();
        return false;
    }
    Method *method = Method::Cast(jsFunction_->GetMethod().GetTaggedObject());
    if (vm_->IsEnableOsr() && osrOffset_ != MachineCode::INVALID_OSR_OFFSET && method->HasCatchBlock()) {
        LOG_JIT(DEBUG) << "skip jit task, as osr does not support catch blocks: " << GetMethodInfo();
        return false;
    }
    return true;
}

bool CompileDecision::IsSupportFunctionKind() const
{
    Method *method = Method::Cast(jsFunction_->GetMethod().GetTaggedObject());
    if (jsFunction_.GetTaggedValue().IsJSSharedFunction()) {
        LOG_JIT(DEBUG) << tier_ << "method does not support compile shared function:" << GetMethodInfo();
        return false;
    }

    FunctionKind kind = method->GetFunctionKind();
    switch (kind) {
        case FunctionKind::NORMAL_FUNCTION:
        case FunctionKind::GETTER_FUNCTION:
        case FunctionKind::SETTER_FUNCTION:
        case FunctionKind::ARROW_FUNCTION:
        case FunctionKind::BASE_CONSTRUCTOR:
        case FunctionKind::CLASS_CONSTRUCTOR:
        case FunctionKind::DERIVED_CONSTRUCTOR:
        case FunctionKind::NONE_FUNCTION:
            return true;
        default:
            break;
    }
    LOG_JIT(DEBUG) << tier_ << "method does not support jit:" << GetMethodInfo() << ", kind:" << static_cast<int>(kind);
    return false;
}

bool CompileDecision::CheckJsFunctionStatus() const
{
    if (tier_.IsFast() && jsFunction_->IsJitCompiling()) {
        return false;
    }

    if (tier_.IsBaseLine() && jsFunction_->IsBaselinejitCompiling()) {
        return false;
    }

    if (tier_.IsFast() && jsFunction_->IsCompiledCode()) {
        JSTaggedValue machineCode = jsFunction_->GetMachineCode();
        if (machineCode.IsMachineCodeObject() &&
            MachineCode::Cast(machineCode.GetTaggedObject())->GetOSROffset() == MachineCode::INVALID_OSR_OFFSET) {
            return false;
        }
        return true;
    }

    if (tier_.IsBaseLine() && !jsFunction_->GetBaselineCode().IsUndefined()) {
        return false;
    }
    return true;
}

void CompileDecision::DisableJitCompile() const
{
    jsFunction_->SetJitHotnessCnt(ProfileTypeInfo::JIT_DISABLE_FLAG);
}

bool CompileDecision::CheckVmState() const
{
    if (vm_->GetJSThread()->IsMachineCodeLowMemory()) {
        LOG_JIT(DEBUG) << tier_ << "skip jit task, as low code memory:" << GetMethodInfo();
        return false;
    }
    return true;
}
}  // namespace panda::ecmascript
