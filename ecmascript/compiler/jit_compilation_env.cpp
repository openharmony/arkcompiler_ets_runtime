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
#include "ecmascript/ecma_context.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/ts_types/ts_manager.h"
#include "ecmascript/pgo_profiler/pgo_profiler.h"

namespace panda::ecmascript {
// jit
JitCompilationEnv::JitCompilationEnv(EcmaVM *jitVm, EcmaVM *jsVm, JSHandle<JSFunction> &jsFunction)
    : CompilationEnv(jitVm), hostThread_(jsVm->GetJSThread()), jsFunction_(jsFunction)
    {
        ptManager_ = hostThread_->GetCurrentEcmaContext()->GetPTManager();
        Method *method = Method::Cast(jsFunction->GetMethod().GetTaggedObject());
        jsPandaFile_ = const_cast<JSPandaFile*>(method->GetJSPandaFile());
        methodLiteral_ = method->GetMethodLiteral();
    }

JSRuntimeOptions &JitCompilationEnv::GetJSOptions()
{
    return hostThread_->GetEcmaVM()->GetJSOptions();
}

const CMap<ElementsKind, ConstantIndex> &JitCompilationEnv::GetArrayHClassIndexMap() const
{
    return hostThread_->GetArrayHClassIndexMap();
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

JSTaggedValue JitCompilationEnv::FindConstpool([[maybe_unused]] const JSPandaFile *jsPandaFile,
    [[maybe_unused]] panda_file::File::EntityId id) const
{
    Method *method = Method::Cast(jsFunction_->GetMethod().GetTaggedObject());
    return method->GetConstantPool();
}

JSTaggedValue JitCompilationEnv::FindConstpool([[maybe_unused]] const JSPandaFile *jsPandaFile,
    [[maybe_unused]] int32_t index) const
{
    Method *method = Method::Cast(jsFunction_->GetMethod().GetTaggedObject());
    return method->GetConstantPool();
}

JSTaggedValue JitCompilationEnv::FindOrCreateUnsharedConstpool([[maybe_unused]] const uint32_t methodOffset) const
{
    Method *method = Method::Cast(jsFunction_->GetMethod().GetTaggedObject());
    JSTaggedValue constpool = method->GetConstantPool();
    if (!ConstantPool::CheckUnsharedConstpool(constpool)) {
        constpool = hostThread_->GetCurrentEcmaContext()->FindUnsharedConstpool(constpool);
        return constpool;
    }
    ASSERT(constpool.IsHole());
    return constpool;
}

JSTaggedValue JitCompilationEnv::FindOrCreateUnsharedConstpool([[maybe_unused]] JSTaggedValue sharedConstpool) const
{
    return FindOrCreateUnsharedConstpool(0);
}

JSHandle<ConstantPool> JitCompilationEnv::FindOrCreateConstPool([[maybe_unused]] const JSPandaFile *jsPandaFile,
    [[maybe_unused]] panda_file::File::EntityId id)
{
    ASSERT_PRINT(0, "jit should unreachable");
    return JSHandle<ConstantPool>();
}

JSTaggedValue JitCompilationEnv::GetConstantPoolByMethodOffset([[maybe_unused]] const uint32_t methodOffset) const
{
    Method *method = Method::Cast(jsFunction_->GetMethod().GetTaggedObject());
    return method->GetConstantPool();
}

JSTaggedValue JitCompilationEnv::GetArrayLiteralFromCache(JSTaggedValue constpool, uint32_t index, CString entry) const
{
    return ConstantPool::GetLiteralFromCache<ConstPoolType::ARRAY_LITERAL>(constpool, index, entry);
}

JSTaggedValue JitCompilationEnv::GetObjectLiteralFromCache(JSTaggedValue constpool, uint32_t index, CString entry) const
{
    return ConstantPool::GetLiteralFromCache<ConstPoolType::OBJECT_LITERAL>(constpool, index, entry);
}

panda_file::File::EntityId JitCompilationEnv::GetIdFromCache(JSTaggedValue constpool, uint32_t index) const
{
    return ConstantPool::GetIdFromCache(constpool, index);
}

JSHandle<GlobalEnv> JitCompilationEnv::GetGlobalEnv() const
{
    return hostThread_->GetEcmaVM()->GetGlobalEnv();
}

const GlobalEnvConstants *JitCompilationEnv::GlobalConstants() const
{
    return hostThread_->GlobalConstants();
}

JSTaggedValue JitCompilationEnv::GetStringFromConstantPool([[maybe_unused]] const uint32_t methodOffset,
    [[maybe_unused]] const uint16_t cpIdx) const
{
    Method *method = Method::Cast(jsFunction_->GetMethod().GetTaggedObject());
    JSTaggedValue constpool = method->GetConstantPool();
    return ConstantPool::GetStringFromCacheForJit(GetJSThread(), constpool, cpIdx);
}
} // namespace panda::ecmascript
