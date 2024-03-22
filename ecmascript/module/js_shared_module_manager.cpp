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
#include "ecmascript/module/js_shared_module_manager.h"

#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/interpreter/slow_runtime_stub.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/js_array.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_executor.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/module/js_module_deregister.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/module/module_data_extractor.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/require/js_cjs_module.h"
#include "ecmascript/tagged_dictionary.h"
#ifdef PANDA_TARGET_WINDOWS
#include <algorithm>
#endif

namespace panda::ecmascript {
using StringHelper = base::StringHelper;
using JSPandaFile = ecmascript::JSPandaFile;
using JSRecordInfo = ecmascript::JSPandaFile::JSRecordInfo;

SharedModuleManager* SharedModuleManager::GetInstance()
{
    static SharedModuleManager* instance = new SharedModuleManager();
    return instance;
}

JSHandle<SourceTextModule> SharedModuleManager::GetImportedSModule(JSThread *thread, JSTaggedValue referencing)
{
    NameDictionary *dict = NameDictionary::Cast(resolvedSharedModules_.GetTaggedObject());
    int entry = dict->FindEntry(referencing);
    LOG_ECMA_IF(entry == -1, FATAL) << "Can not get module: "
                                    << ConvertToString(referencing);
    JSTaggedValue result = dict->GetValue(entry);
    return JSHandle<SourceTextModule>(thread, result);
}

JSTaggedValue SharedModuleManager::GetSendableModuleValue(JSThread *thread, int32_t index, JSTaggedValue jsFunc) const
{
    JSTaggedValue currentModule = JSFunction::Cast(jsFunc.GetTaggedObject())->GetModule();
    return GetSendableModuleValueImpl(thread, index, currentModule);
}

JSTaggedValue SharedModuleManager::GetSendableModuleValueImpl(
    JSThread *thread, int32_t index, JSTaggedValue currentModule) const
{
    if (currentModule.IsUndefined()) {
        LOG_FULL(FATAL) << "GetModuleValueOutter currentModule failed";
        UNREACHABLE();
    }

    SourceTextModule *module = SourceTextModule::Cast(currentModule.GetTaggedObject());
    JSTaggedValue moduleEnvironment = module->GetEnvironment();
    if (moduleEnvironment.IsUndefined()) {
        return thread->GlobalConstants()->GetUndefined();
    }
    ASSERT(moduleEnvironment.IsTaggedArray());
    JSTaggedValue resolvedBinding = TaggedArray::Cast(moduleEnvironment.GetTaggedObject())->Get(index);
    if (resolvedBinding.IsResolvedRecordBinding()) {
        ResolvedRecordBinding *binding = ResolvedRecordBinding::Cast(resolvedBinding.GetTaggedObject());
        JSTaggedValue moduleRecord = binding->GetModuleRecord();
        ASSERT(moduleRecord.IsString());
        // moduleRecord is string, find at current vm
        ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
        JSHandle<SourceTextModule> resolvedModule;
        if (moduleManager->IsImportedModuleLoaded(moduleRecord)) {
            resolvedModule = moduleManager->HostGetImportedModule(moduleRecord);
        } else {
            auto isMergedAbc = !module->GetEcmaModuleRecordName().IsUndefined();
            CString record = ConvertToString(moduleRecord);
            CString fileName = ConvertToString(module->GetEcmaModuleFilename());
            if (JSPandaFileExecutor::LazyExecuteModule(thread, record, fileName, isMergedAbc)) {
                resolvedModule = moduleManager->HostGetImportedModule(moduleRecord);
            }
            LOG_ECMA(FATAL) << "LazyExecuteModule failed";
        }
        // [[todo::DaiHN will consider HotReload]]
        // [[todo::DaiHN should consider json, cjs and native module later]]
        ASSERT(resolvedModule->GetTypes() == ModuleTypes::ECMA_MODULE);
        return resolvedModule->GetModuleValue(thread, binding->GetIndex(), false);
    } else if (resolvedBinding.IsResolvedIndexBinding()) {
        ResolvedIndexBinding *binding = ResolvedIndexBinding::Cast(resolvedBinding.GetTaggedObject());
        SourceTextModule *resolvedModule = SourceTextModule::Cast(binding->GetModule().GetTaggedObject());
        return resolvedModule->GetModuleValue(thread, binding->GetIndex(), false);
    }
    UNREACHABLE();
}
} // namespace panda::ecmascript
