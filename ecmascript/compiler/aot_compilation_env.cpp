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
#include "ecmascript/compiler/aot_compilation_env.h"
#include "ecmascript/compiler/pgo_type/pgo_type_manager.h"
#include "ecmascript/ecma_context.h"
#include "ecmascript/jspandafile/program_object.h"

namespace panda::ecmascript {
AOTCompilationEnv::AOTCompilationEnv(EcmaVM *vm) : CompilationEnv(vm)
{
    ptManager_ = thread_->GetCurrentEcmaContext()->GetPTManager();
}

JSRuntimeOptions &AOTCompilationEnv::GetJSOptions()
{
    return vm_->GetJSOptions();
}

JSHandle<GlobalEnv> AOTCompilationEnv::GetGlobalEnv() const
{
    return vm_->GetGlobalEnv();
}

const CMap<ElementsKind, std::pair<ConstantIndex, ConstantIndex>> &AOTCompilationEnv::GetArrayHClassIndexMap() const
{
    return thread_->GetArrayHClassIndexMap();
}

const BuiltinHClassEntries &AOTCompilationEnv::GetBuiltinHClassEntries() const
{
    return thread_->GetBuiltinHClassEntries();
}

JSHClass *AOTCompilationEnv::GetBuiltinPrototypeHClass(BuiltinTypeId type) const
{
    return thread_->GetBuiltinPrototypeHClass(type);
}

JSTaggedValue AOTCompilationEnv::FindConstpool(const JSPandaFile *jsPandaFile, panda_file::File::EntityId id) const
{
    return thread_->GetCurrentEcmaContext()->FindConstpool(jsPandaFile, id);
}

JSTaggedValue AOTCompilationEnv::FindConstpool(const JSPandaFile *jsPandaFile, int32_t index) const
{
    return thread_->GetCurrentEcmaContext()->FindConstpool(jsPandaFile, index);
}

JSTaggedValue AOTCompilationEnv::FindOrCreateUnsharedConstpool(const uint32_t methodOffset) const
{
    JSTaggedValue cp = ptManager_->GetConstantPoolByMethodOffset(methodOffset);
    return thread_->GetCurrentEcmaContext()->FindOrCreateUnsharedConstpool(cp);
}

JSTaggedValue AOTCompilationEnv::FindOrCreateUnsharedConstpool(JSTaggedValue sharedConstpool) const
{
    return thread_->GetCurrentEcmaContext()->FindOrCreateUnsharedConstpool(sharedConstpool);
}

JSHandle<ConstantPool> AOTCompilationEnv::FindOrCreateConstPool(const JSPandaFile *jsPandaFile,
    panda_file::File::EntityId id)
{
    return thread_->GetCurrentEcmaContext()->FindOrCreateConstPool(jsPandaFile, id);
}

JSTaggedValue AOTCompilationEnv::GetConstantPoolByMethodOffset(const uint32_t methodOffset) const
{
    return ptManager_->GetConstantPoolByMethodOffset(methodOffset);
}

const GlobalEnvConstants *AOTCompilationEnv::GlobalConstants() const
{
    return thread_->GlobalConstants();
}

JSTaggedValue AOTCompilationEnv::GetArrayLiteralFromCache(JSTaggedValue constpool, uint32_t index, CString entry) const
{
    return ConstantPool::GetLiteralFromCache<ConstPoolType::ARRAY_LITERAL>(thread_, constpool, index, entry);
}

JSTaggedValue AOTCompilationEnv::GetObjectLiteralFromCache(JSTaggedValue constpool, uint32_t index, CString entry) const
{
    return ConstantPool::GetLiteralFromCache<ConstPoolType::OBJECT_LITERAL>(thread_, constpool, index, entry);
}

JSTaggedValue AOTCompilationEnv::GetMethodFromCache(JSTaggedValue constpool, uint32_t index) const
{
    return ConstantPool::GetMethodFromCache(thread_, constpool, index);
}

panda_file::File::EntityId AOTCompilationEnv::GetIdFromCache(JSTaggedValue constpool, uint32_t index) const
{
    return ConstantPool::GetIdFromCache(constpool, index);
}

JSTaggedValue AOTCompilationEnv::GetStringFromConstantPool(const uint32_t methodOffset, const uint16_t cpIdx,
    [[maybe_unused]] bool allowAlloc) const
{
    return ptManager_->GetStringFromConstantPool(methodOffset, cpIdx);
}
} // namespace panda::ecmascript
