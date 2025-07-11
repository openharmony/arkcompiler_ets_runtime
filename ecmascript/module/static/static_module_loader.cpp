/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "ecmascript/module/static/static_module_loader.h"
#include "ecmascript/global_env.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/module/js_module_deregister.h"
#include "ecmascript/module/js_shared_module_manager.h"
#include "ecmascript/shared_objects/js_shared_array.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/module/module_data_extractor.h"
#include "ecmascript/builtins/builtins_promise_job.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/module/static/static_module_proxy_handler.h"

namespace panda::ecmascript {
bool StaticModuleLoader::CanTryLoadStaticModulePath(const CString &requestPath)
{
    // Filters the 1.0 ohmurl that can be determined
    if (ModulePathHelper::IsOhmUrl(requestPath) || ModulePathHelper::IsImportFile(requestPath)
        || StringHelper::StringStartWith(requestPath, ModulePathHelper::PREFIX_MODULE)
        || StringHelper::StringStartWith(requestPath, ModulePathHelper::RAW_ARKUIX_PREFIX)
        || StringHelper::StringStartWith(requestPath, ModulePathHelper::PREFIX_NORMALIZED)) {
        return false;
    }
    return true;
}

Local<JSValueRef> StaticModuleLoader::GetStaticModuleLoadFunc(EcmaVM *vm)
{
    auto thread = vm->GetJSThread();
    auto globalConstants = thread->GlobalConstants();
    Local<ObjectRef> globalObject = JSNApi::GetGlobalObject(vm);
    Local<JSValueRef> pandaObject = globalObject->Get(vm,
        JSNApiHelper::ToLocal<StringRef>(globalConstants->GetHandledPandaString()));
    if (pandaObject->IsUndefined()) {
        LOG_ECMA(DEBUG) << "the current env is not 1.2 vm";
        return JSValueRef::Undefined(vm);
    }
    JSHandle<JSTaggedValue> getModule = JSTaggedValue::GetProperty(thread, JSNApiHelper::ToJSHandle(pandaObject),
        globalConstants->GetHandledGetModuleString()).GetValue();
    Local<JSValueRef> getModuleRef = JSNApiHelper::ToLocal<JSValueRef>(getModule);
    if (!getModuleRef->IsFunction(vm)) {
        return JSValueRef::Undefined(vm);
    }
    return getModuleRef;
}

JSHandle<JSTaggedValue> StaticModuleLoader::LoadStaticModule(JSThread *thread,
    Local<FunctionRef> getEsModuleFunc, const CString &key)
{
    auto moduleManager = thread->GetModuleManager();
    if (moduleManager->IsModuleLoaded(key)) {
        LOG_ECMA(INFO) << "load static module from cache.";
        JSHandle<SourceTextModule> moduleRecord = moduleManager->HostGetImportedModule(key);
        return JSHandle<JSTaggedValue>(thread, moduleRecord->GetModuleValue(thread, 0, false));
    }

    auto vm = thread->GetEcmaVM();
    auto globalConstants = thread->GlobalConstants();

    JSHandle<JSTaggedValue> moduleRecord = ModuleDataExtractor::ParseNativeModule(thread, key,
                                                                                  "", ModuleTypes::STATIC_MODULE);
    JSHandle<SourceTextModule> nativeModule = JSHandle<SourceTextModule>::Cast(moduleRecord);
    std::vector<Local<JSValueRef>> moduleArgs;
    moduleArgs.emplace_back(StringRef::NewFromUtf8(vm, key.c_str()));
    Local<JSValueRef> moduleExport = getEsModuleFunc->Call(vm, JSValueRef::Undefined(vm),
                                                      moduleArgs.data(), moduleArgs.size());
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    JSHandle<JSProxy> proxyExport = StaticModuleProxyHandler::CreateStaticModuleProxyHandler(thread,
        JSNApiHelper::ToJSHandle(moduleExport));
    JSHandle<JSTaggedValue> value(proxyExport);
    SourceTextModule::RecordEvaluatedOrError(thread, nativeModule);
    nativeModule->SetLoadingTypes(LoadingTypes::STABLE_MODULE);
    SourceTextModule::StoreModuleValue(thread, nativeModule, 0, value);
    moduleManager->AddResolveImportedModule(key, moduleRecord.GetTaggedValue());
    return value;
}

}  // namespace panda::ecmascript
