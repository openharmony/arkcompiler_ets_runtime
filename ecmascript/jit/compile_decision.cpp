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
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/platform/aot_crash_info.h"

namespace panda::ecmascript {
std::string GetFileName(const std::string& fileName, bool isApp)
{
    if (isApp) {
        std::string pathOnMobile;
        std::string sanboxPath = panda::os::file::File::GetExtendedFilePath(AotCrashInfo::GetSandBoxPath());
        if (ecmascript::RealPath(sanboxPath, pathOnMobile, false)) {
            return pathOnMobile + "/method_compiled_by_jit.cfg";
        } else {
            LOG_JIT(ERROR) << "get method_compiled_by_jit.cfg path fail: " << sanboxPath;
            return "";
        }
    }
    return fileName;
}

void MethodNameCollector::Init(EcmaVM *vm)
{
    if (vm == nullptr || isInit_) {
        return;
    }
    if (!(vm->GetJSOptions().IsEnableJitMethodCollect())) {
        return;
    }
    enable_ = true;
    std::string fileName = vm->GetJSOptions().GetJitMethodPath();
    fileName = GetFileName(fileName, vm->GetJSOptions().IsEnableAPPJIT());
    if (fileName.empty()) {
        return;
    }
    file_.open(fileName.c_str(), std::ofstream::out | std::ofstream::trunc);
    if (!file_.is_open()) {
        LOG_JIT(ERROR) << "open method_compiled_by_jit.cfg fail: " << fileName;
        return;
    }
    LOG_JIT(INFO) << "open method_compiled_by_jit.cfg succ: " << fileName;
    isInit_ = true;
}

void MethodNameCollector::Collect(const std::string& methodFullName) const
{
    if (enable_ && isInit_) {
        file_ << methodFullName << std::endl;
    }
}

MethodNameCollector::~MethodNameCollector()
{
    if (enable_ && isInit_) {
        ASSERT(file_.is_open());
        file_.close();
    }
    isInit_ = false;
    enable_ = false;
}

void MethodNameFilter::Init(EcmaVM *vm)
{
    if (vm == nullptr || isInit_) {
        return;
    }
    if (!(vm->GetJSOptions().IsEnableJitMethodFilter())) {
        return;
    }
    enable_ = true;
    std::string fileName = vm->GetJSOptions().GetJitMethodPath();
    fileName = GetFileName(fileName, vm->GetJSOptions().IsEnableAPPJIT());
    if (fileName.empty()) {
        return;
    }
    std::ifstream file(fileName.c_str());
    if (!file.is_open()) {
        LOG_JIT(INFO) << "open method_compiled_by_jit.cfg fail: " << fileName;
        return;
    }
    LOG_JIT(INFO) << "open method_compiled_by_jit.cfg succ: " << fileName;
    std::string methodFullName;
    while (getline(file, methodFullName)) {
        if (methodFullName.empty()) {
            continue;
        }
        methodFullNames.insert(methodFullName);
    }
    if (methodFullNames.empty()) {
        LOG_JIT(INFO) << "the number of method names is 0.";
        return;
    }
    isInit_ = true;
}

bool MethodNameFilter::NeedCompiledByJit(const std::string& methodFullName) const
{
    if (enable_ && isInit_) {
        return methodFullNames.find(methodFullName) != methodFullNames.end();
    }
    // When filtering is not enabled, all JS functions need to be compiled.
    return true;
}

MethodNameFilter::~MethodNameFilter()
{
    methodFullNames.clear();
    enable_ = false;
    isInit_ = false;
}

MethodNameCollector CompileDecision::methodNameCollector;
MethodNameFilter CompileDecision::methodNameFilter;

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
    JSThread *thread = vm_->GetJSThread();
    Method *method = Method::Cast(jsFunction_->GetMethod(thread).GetTaggedObject());
    ASSERT(method != nullptr);
    auto jSPandaFile = method->GetJSPandaFile(thread);
    CString fileDesc;
    if (jSPandaFile != nullptr) {
        fileDesc = jSPandaFile->GetJSPandaFileDesc();
    }
    return fileDesc + ":" + method->GetRecordNameStr(thread) + "." + CString(method->GetMethodName(thread));
}

uint32_t CompileDecision::GetCodeSize() const
{
    JSThread *thread = vm_->GetJSThread();
    Method *method = Method::Cast(jsFunction_->GetMethod(thread).GetTaggedObject());
    return method->GetCodeSize(thread);
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
    Method *method = Method::Cast(jsFunction_->GetMethod(vm_->GetJSThread()).GetTaggedObject());
    if (vm_->IsEnableOsr() && osrOffset_ != MachineCode::INVALID_OSR_OFFSET &&
        method->HasCatchBlock(vm_->GetJSThread())) {
        LOG_JIT(DEBUG) << "skip jit task, as osr does not support catch blocks: " << GetMethodInfo();
        return false;
    }
    methodNameCollector.Collect(std::string(GetMethodName()));
    if (!methodNameFilter.NeedCompiledByJit(std::string(GetMethodName()))) {
        LOG_JIT(DEBUG) << "skip jit task, as not the compilation target:" << GetMethodInfo();
        return false;
    }
    return true;
}

bool CompileDecision::IsSupportFunctionKind() const
{
    Method *method = Method::Cast(jsFunction_->GetMethod(vm_->GetJSThread()).GetTaggedObject());
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
        JSTaggedValue machineCode = jsFunction_->GetMachineCode(vm_->GetJSThread());
        if (machineCode.IsMachineCodeObject() &&
            MachineCode::Cast(machineCode.GetTaggedObject())->GetOSROffset() == MachineCode::INVALID_OSR_OFFSET) {
            return false;
        }
        return true;
    }

    if (tier_.IsBaseLine() && !jsFunction_->GetBaselineCode(vm_->GetJSThread()).IsUndefined()) {
        return false;
    }
    return true;
}

void CompileDecision::DisableJitCompile() const
{
    jsFunction_->SetJitHotnessCnt(vm_->GetJSThread(), ProfileTypeInfo::JIT_DISABLE_FLAG);
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
