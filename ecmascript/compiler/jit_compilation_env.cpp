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
#include "ecmascript/compiler/jit_compilation_env.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/ic/ic_handler.h"

namespace panda::ecmascript {
// jit
JitCompilationEnv::JitCompilationEnv(EcmaVM *jitVm, EcmaVM *jsVm, JSHandle<JSFunction> &jsFunction)
    : CompilationEnv(jitVm), hostThread_(jsVm->GetJSThreadNoCheck()), jsFunction_(jsFunction)
{
    if (hostThread_ != nullptr && hostThread_->GetCurrentEcmaContext() != nullptr &&
        hostThread_->GetCurrentEcmaContext()->GetPTManager() != nullptr) {
        ptManager_ = hostThread_->GetCurrentEcmaContext()->GetPTManager();
    }
    Method *method = Method::Cast(jsFunction->GetMethod().GetTaggedObject());
    jsPandaFile_ = const_cast<JSPandaFile*>(method->GetJSPandaFile());
    methodLiteral_ = method->GetMethodLiteral();
    pcStart_ = method->GetBytecodeArray();
    abcId_ = PGOProfiler::GetMethodAbcId(*jsFunction);
    if (method->GetFunctionKind() == FunctionKind::CLASS_CONSTRUCTOR) {
        methodLiteral_->SetFunctionKind(FunctionKind::CLASS_CONSTRUCTOR);
    }
}

JSRuntimeOptions &JitCompilationEnv::GetJSOptions()
{
    return hostThread_->GetEcmaVM()->GetJSOptions();
}

ConstantIndex JitCompilationEnv::GetArrayHClassIndex(ElementsKind kind, bool isProtoType) const
{
    return hostThread_->GetArrayInstanceHClassIndex(kind, isProtoType);
}

const BuiltinHClassEntries &JitCompilationEnv::GetBuiltinHClassEntries() const
{
    return hostThread_->GetBuiltinHClassEntries();
}

JSHClass *JitCompilationEnv::GetBuiltinPrototypeHClass(BuiltinTypeId type) const
{
    return hostThread_->GetBuiltinPrototypeHClass(type);
}

void JitCompilationEnv::SetTsManagerCompilationEnv()
{
    auto pt = hostThread_->GetCurrentEcmaContext()->GetPTManager();
    ptManager_ = pt;
}

std::shared_ptr<pgo::PGOProfiler> JitCompilationEnv::GetPGOProfiler() const
{
    return hostThread_->GetEcmaVM()->GetPGOProfiler();
}

JSTaggedValue JitCompilationEnv::FindConstpool([[maybe_unused]] const JSPandaFile *jsPandaFile,
    [[maybe_unused]] panda_file::File::EntityId id) const
{
    ASSERT(thread_->IsInRunningState());
    Method *method = Method::Cast(jsFunction_->GetMethod().GetTaggedObject());
    JSTaggedValue constpool = method->GetConstantPool();
    [[maybe_unused]] const ConstantPool *taggedPool = ConstantPool::Cast(constpool.GetTaggedObject());
    ASSERT(taggedPool->GetJSPandaFile() == jsPandaFile);
    ASSERT(method->GetMethodId() == id);
    return constpool;
}

JSTaggedValue JitCompilationEnv::FindConstpool([[maybe_unused]] const JSPandaFile *jsPandaFile,
    [[maybe_unused]] int32_t index) const
{
    ASSERT(thread_->IsInRunningState());
    Method *method = Method::Cast(jsFunction_->GetMethod().GetTaggedObject());
    JSTaggedValue constpool = method->GetConstantPool();
    [[maybe_unused]] const ConstantPool *taggedPool = ConstantPool::Cast(constpool.GetTaggedObject());
    ASSERT(taggedPool->GetJSPandaFile() == jsPandaFile);
    ASSERT(taggedPool->GetSharedConstpoolId().GetInt() == index);
    return constpool;
}

JSTaggedValue JitCompilationEnv::FindOrCreateUnsharedConstpool([[maybe_unused]] const uint32_t methodOffset) const
{
    JSTaggedValue constpool = GetConstantPoolByMethodOffset(methodOffset);
    if (constpool.IsUndefined()) {
        return JSTaggedValue::Undefined();
    }
    ASSERT(!ConstantPool::CheckUnsharedConstpool(constpool));
    JSTaggedValue unSharedConstpool = hostThread_->GetCurrentEcmaContext()->FindUnsharedConstpool(constpool);
    return unSharedConstpool;
}

JSTaggedValue JitCompilationEnv::FindOrCreateUnsharedConstpool([[maybe_unused]] JSTaggedValue sharedConstpool) const
{
    Method *method = Method::Cast(jsFunction_->GetMethod().GetTaggedObject());
    [[maybe_unused]] JSTaggedValue constpool = method->GetConstantPool();
    ASSERT(constpool == sharedConstpool);
    uint32_t methodOffset = method->GetMethodId().GetOffset();
    return FindOrCreateUnsharedConstpool(methodOffset);
}

JSHandle<ConstantPool> JitCompilationEnv::FindOrCreateConstPool([[maybe_unused]] const JSPandaFile *jsPandaFile,
    [[maybe_unused]] panda_file::File::EntityId id)
{
    ASSERT_PRINT(0, "jit should unreachable");
    return JSHandle<ConstantPool>();
}

JSTaggedValue JitCompilationEnv::GetConstantPoolByMethodOffset([[maybe_unused]] const uint32_t methodOffset) const
{
    ASSERT(thread_->IsInRunningState());
    JSTaggedValue constpool;
    Method *currMethod = Method::Cast(jsFunction_->GetMethod().GetTaggedObject());
    if (methodOffset != currMethod->GetMethodId().GetOffset()) {
        auto calleeFunc = GetJsFunctionByMethodOffset(methodOffset);
        if (!calleeFunc) {
            return JSTaggedValue::Undefined();
        }
        constpool = Method::Cast(calleeFunc->GetMethod())->GetConstantPool();
    } else {
        constpool = currMethod->GetConstantPool();
    }
    return constpool;
}

JSTaggedValue JitCompilationEnv::GetArrayLiteralFromCache(JSTaggedValue constpool, uint32_t index, CString entry) const
{
    ASSERT(thread_->IsInRunningState());
    return ConstantPool::GetLiteralFromCache<ConstPoolType::ARRAY_LITERAL>(constpool, index, entry);
}

JSTaggedValue JitCompilationEnv::GetObjectLiteralFromCache(JSTaggedValue constpool, uint32_t index, CString entry) const
{
    ASSERT(thread_->IsInRunningState());
    return ConstantPool::GetLiteralFromCache<ConstPoolType::OBJECT_LITERAL>(constpool, index, entry);
}

JSTaggedValue JitCompilationEnv::GetMethodFromCache(JSTaggedValue constpool, uint32_t index) const
{
    return ConstantPool::GetMethodFromCache(constpool, index);
}

panda_file::File::EntityId JitCompilationEnv::GetIdFromCache(JSTaggedValue constpool, uint32_t index) const
{
    ASSERT(thread_->IsInRunningState());
    return ConstantPool::GetIdFromCache(constpool, index);
}

JSHandle<GlobalEnv> JitCompilationEnv::GetGlobalEnv() const
{
    ASSERT(thread_->IsInRunningState());
    return hostThread_->GetEcmaVM()->GetGlobalEnv();
}

const GlobalEnvConstants *JitCompilationEnv::GlobalConstants() const
{
    ASSERT(thread_->IsInRunningState());
    return hostThread_->GlobalConstants();
}

// The caller should add assessment for undefined constpool.
// When slotValue in profileTypeInfo is changed in main thread, constpool may be undefined.
JSTaggedValue JitCompilationEnv::GetStringFromConstantPool([[maybe_unused]] const uint32_t methodOffset,
    const uint16_t cpIdx, bool allowAlloc) const
{
    JSTaggedValue constpool = GetConstantPoolByMethodOffset(methodOffset);
    if (constpool.IsUndefined()) {
        return JSTaggedValue::Undefined();
    }
    return ConstantPool::GetStringFromCacheForJit(GetJSThread(), constpool, cpIdx, allowAlloc);
}

JSFunction *JitCompilationEnv::GetJsFunctionByMethodOffset(uint32_t methodOffset) const
{
    ASSERT(thread_->IsInRunningState());
    Method *currMethod = Method::Cast(jsFunction_->GetMethod().GetTaggedObject());
    auto currMethodOffset = currMethod->GetMethodId().GetOffset();
    if (methodOffset == currMethodOffset) {
        return *jsFunction_;
    }
    std::vector<std::pair<uint32_t, uint32_t>> funcSlotChain;
    uint32_t calleeOffset = methodOffset;
    do {
        if (functionSlotIdMap_.find(calleeOffset) == functionSlotIdMap_.end()) {
            return nullptr;
        }
        funcSlotChain.push_back({functionSlotIdMap_.at(calleeOffset), callee2CallerMap_.at(calleeOffset)});
        calleeOffset = callee2CallerMap_.at(calleeOffset);
    } while (calleeOffset != currMethodOffset);
    JSFunction *currFunc = *jsFunction_;
    ProfileTypeInfo *currFuncPTI = *profileTypeInfo_;
    for (int i = static_cast<int>(funcSlotChain.size()) - 1; i >= 0; --i) {
        uint32_t slotId = funcSlotChain[i].first;
        uint32_t callerOffset = funcSlotChain[i].second;
        if (Method::Cast(currFunc->GetMethod())->GetMethodId().GetOffset() != callerOffset) {
            return nullptr;
        }
        auto slotValue = currFuncPTI->Get(slotId);
        if (slotValue.IsJSFunction()) {
            currFunc = JSFunction::Cast(currFuncPTI->Get(slotId).GetTaggedObject());
        } else if (slotValue.IsPrototypeHandler()) {
            auto prototypeHandler = PrototypeHandler::Cast(slotValue.GetTaggedObject());
            auto accessorFunction = prototypeHandler->GetAccessorJSFunction();
            if (!accessorFunction.IsJSFunction()) {
                return nullptr;
            }
            currFunc = JSFunction::Cast(accessorFunction.GetTaggedObject());
        } else {
            return nullptr;
        }
        auto profileTypeInfoVal = currFunc->GetProfileTypeInfo();
        if (profileTypeInfoVal.IsUndefined()) {
            return nullptr;
        }
        currFuncPTI = ProfileTypeInfo::Cast(profileTypeInfoVal.GetTaggedObject());
    }
    if (Method::Cast(currFunc->GetMethod())->GetMethodId().GetOffset() != methodOffset) {
        return nullptr;
    }
    return currFunc;
}
} // namespace panda::ecmascript
