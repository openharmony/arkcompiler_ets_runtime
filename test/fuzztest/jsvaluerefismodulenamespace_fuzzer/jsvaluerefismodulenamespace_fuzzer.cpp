/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "jsvaluerefismodulenamespace_fuzzer.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_list.h"
#include "ecmascript/js_api/js_api_queue.h"
#include "ecmascript/js_collator.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_plural_rules.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/tagged_list.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
constexpr uint32_t ERROR_TYPE_LEN = 2;

void IsModuleNamespaceObjectFuzztest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    {
        JsiFastNativeScope scope(vm);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        ObjectFactory *objectFactory = vm->GetFactory();
        JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
        JSHandle<LocalExportEntry> localExportEntry1 = objectFactory->NewLocalExportEntry();
        SourceTextModule::AddLocalExportEntry(vm->GetJSThread(), module, localExportEntry1, 0, ERROR_TYPE_LEN);
        JSHandle<LocalExportEntry> localExportEntry2 = objectFactory->NewLocalExportEntry();
        SourceTextModule::AddLocalExportEntry(vm->GetJSThread(), module, localExportEntry2, 1, ERROR_TYPE_LEN);
        JSHandle<TaggedArray> localExportEntries(vm->GetJSThread(), module->GetLocalExportEntries());
        CString baseFileName = "a.abc";
        module->SetEcmaModuleFilenameString(baseFileName);
        ModuleManager *moduleManager = vm->GetJSThread()->GetModuleManager();
        moduleManager->AddResolveImportedModule(baseFileName, module.GetTaggedValue());
        JSHandle<ModuleNamespace> np = ModuleNamespace::ModuleNamespaceCreate(vm->GetJSThread(),
            JSHandle<JSTaggedValue>::Cast(module), localExportEntries);
        ModuleNamespace::PreventExtensions();
        JSHandle<JSTaggedValue> moduleNamespaceTag = JSHandle<JSTaggedValue>::Cast(np);
        Local<JSValueRef> moduleNamespace = JSNApiHelper::ToLocal<ModuleNamespace>(moduleNamespaceTag);
        moduleNamespace->IsModuleNamespaceObject(vm);
    }
    JSNApi::DestroyJSVM(vm);
}

void IsProxyFuzztest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    {
        JsiFastNativeScope scope(vm);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        auto thread = vm->GetJSThread();
        JSHandle<GlobalEnv> globalEnv = vm->GetGlobalEnv();
        JSHandle<JSTaggedValue> hclass(thread, globalEnv->GetObjectFunction().GetObject<JSFunction>());
        JSHandle<JSTaggedValue> targetHandle(
            thread->GetEcmaVM()->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(hclass), hclass));
        JSHandle<JSTaggedValue> handlerHandle(
            thread->GetEcmaVM()->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(hclass), hclass));
        JSHandle<JSProxy> proxyHandle = JSProxy::ProxyCreate(thread, targetHandle, handlerHandle);
        Local<JSValueRef> proxy = JSNApiHelper::ToLocal<JSProxy>(JSHandle<JSTaggedValue>(proxyHandle));
        proxy->IsProxy(vm);
    }
    JSNApi::DestroyJSVM(vm);
}

void IsJSCollatorFuzztest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    {
        JsiFastNativeScope scope(vm);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        auto thread = vm->GetJSThread();
        ObjectFactory *factory = vm->GetFactory();
        JSHandle<JSTaggedValue> ctor = vm->GetGlobalEnv()->GetCollatorFunction();
        JSHandle<JSCollator> collator =
            JSHandle<JSCollator>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));
        JSHandle<JSTaggedValue> localeStr = thread->GlobalConstants()->GetHandledEnUsString();
        JSHandle<JSTaggedValue> undefinedHandle(thread, JSTaggedValue::Undefined());
        JSHandle<JSCollator> initCollator =
            JSCollator::InitializeCollator(thread, collator, localeStr, undefinedHandle);
        JSHandle<JSTaggedValue> collatorTagHandleVal = JSHandle<JSTaggedValue>::Cast(initCollator);
        Local<JSValueRef> object = JSNApiHelper::ToLocal<JSValueRef>(collatorTagHandleVal);
        object->IsJSCollator(vm);
    }
    JSNApi::DestroyJSVM(vm);
}

void IsJSPluralRulesFuzztest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    {
        JsiFastNativeScope scope(vm);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        auto thread = vm->GetJSThread();
        ObjectFactory *factory = vm->GetFactory();
        JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
        JSHandle<JSTaggedValue> optionHandle(thread, JSTaggedValue::Undefined());
        JSHandle<JSTaggedValue> ctor = env->GetPluralRulesFunction();
        JSHandle<JSPluralRules> pluralRules =
            JSHandle<JSPluralRules>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));
        JSHandle<JSTaggedValue> localeStr(factory->NewFromASCII("en-GB"));
        JSHandle<JSPluralRules> initPluralRules =
            JSPluralRules::InitializePluralRules(thread, pluralRules, localeStr, optionHandle);
        JSHandle<JSTaggedValue> tagPlureRules = JSHandle<JSTaggedValue>::Cast(initPluralRules);
        Local<JSValueRef> object = JSNApiHelper::ToLocal<JSValueRef>(tagPlureRules);
        object->IsJSPluralRules(vm);
    }
    JSNApi::DestroyJSVM(vm);
}

void IsStrictEqualsFuzztest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    Local<ObjectRef> object = ObjectRef::New(vm);
    Local<ObjectRef> object1 = ObjectRef::New(vm);
    object->IsStrictEquals(vm, object1);
    JSNApi::DestroyJSVM(vm);
}

void IsJSListFormatFuzztest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    Local<JSValueRef> object = ObjectRef::New(vm);
    object->IsJSListFormat(vm);
    JSNApi::DestroyJSVM(vm);
}

void IsJSPrimitiveRefFuzztest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    {
        JsiFastNativeScope scope(vm);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        auto thread = vm->GetJSThread();
        auto factory = vm->GetFactory();
        JSHandle<JSTaggedValue> nullHandle(thread, JSTaggedValue::Null());
        JSHandle<JSHClass> jsClassHandle = factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_PRIMITIVE_REF, nullHandle);
        TaggedObject *taggedObject = factory->NewObject(jsClassHandle);
        JSHandle<JSTaggedValue> jsTaggedValue(thread, JSTaggedValue(taggedObject));
        Local<JSValueRef> jsValueRef = JSNApiHelper::ToLocal<JSPrimitiveRef>(jsTaggedValue);
        jsValueRef->IsJSPrimitiveRef(vm);
    }
    JSNApi::DestroyJSVM(vm);
}

void IsDequeFuzztest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    {
        JsiFastNativeScope scope(vm);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        auto thread = vm->GetJSThread();
        auto factory = vm->GetFactory();
        JSHandle<JSTaggedValue> proto = thread->GetEcmaVM()->GetGlobalEnv()->GetFunctionPrototype();
        JSHandle<JSHClass> queueClass = factory->NewEcmaHClass(JSAPIQueue::SIZE, JSType::JS_API_QUEUE, proto);
        JSHandle<JSAPIQueue> jsQueue = JSHandle<JSAPIQueue>::Cast(factory->NewJSObjectWithInit(queueClass));
        JSHandle<TaggedArray> newElements = factory->NewTaggedArray(JSAPIQueue::DEFAULT_CAPACITY_LENGTH);
        jsQueue->SetLength(thread, JSTaggedValue(0));
        jsQueue->SetFront(0);
        jsQueue->SetTail(0);
        jsQueue->SetElements(thread, newElements);
        JSHandle<JSTaggedValue> Que = JSHandle<JSTaggedValue>::Cast(jsQueue);
        Local<JSValueRef> jsValueRef = JSNApiHelper::ToLocal<ArrayRef>(Que);
        jsValueRef->IsDeque(vm);
    }
    JSNApi::DestroyJSVM(vm);
}

Local<JSValueRef> CreateJSValueRef(EcmaVM *vm, panda::ecmascript::JSType type)
{
    auto thread = vm->GetJSThread();
    auto factory = vm->GetFactory();
    JSHandle<JSTaggedValue> nullHandle(thread, JSTaggedValue::Null());
    JSHandle<JSHClass> jsClassHandle = factory->NewEcmaHClass(JSObject::SIZE, type, nullHandle);
    TaggedObject *taggedObject = factory->NewObject(jsClassHandle);
    JSHandle<JSTaggedValue> jsTaggedValue(thread, JSTaggedValue(taggedObject));
    return JSNApiHelper::ToLocal<JSValueRef>(jsTaggedValue);
}

void IsJSIntlFuzztest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    {
        JsiFastNativeScope scope(vm);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        Local<JSValueRef> jsInt1 = CreateJSValueRef(vm, JSType::JS_INTL);
        jsInt1->IsJSIntl(vm);
    }
    JSNApi::DestroyJSVM(vm);
}

void IsJSDateTimeFormatFuzztest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    {
        JsiFastNativeScope scope(vm);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        Local<JSValueRef> dateTime = CreateJSValueRef(vm, JSType::JS_DATE_TIME_FORMAT);
        dateTime->IsJSDateTimeFormat(vm);
    }
    JSNApi::DestroyJSVM(vm);
}

void IsJSNumberFormatFuzztest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    {
        JsiFastNativeScope scope(vm);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        Local<JSValueRef> number = CreateJSValueRef(vm, JSType::JS_NUMBER_FORMAT);
        number->IsJSNumberFormat(vm);
    }
    JSNApi::DestroyJSVM(vm);
}

void IsJSRelativeTimeFormatFuzztest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    {
        JsiFastNativeScope scope(vm);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        Local<JSValueRef> relative = CreateJSValueRef(vm, JSType::JS_RELATIVE_TIME_FORMAT);
        relative->IsJSRelativeTimeFormat(vm);
    }
    JSNApi::DestroyJSVM(vm);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::IsModuleNamespaceObjectFuzztest(data, size);
    OHOS::IsProxyFuzztest(data, size);
    OHOS::IsJSCollatorFuzztest(data, size);
    OHOS::IsJSPluralRulesFuzztest(data, size);
    OHOS::IsStrictEqualsFuzztest(data, size);
    OHOS::IsJSListFormatFuzztest(data, size);
    OHOS::IsJSPrimitiveRefFuzztest(data, size);
    OHOS::IsDequeFuzztest(data, size);
    OHOS::IsJSIntlFuzztest(data, size);
    OHOS::IsJSDateTimeFormatFuzztest(data, size);
    OHOS::IsJSNumberFormatFuzztest(data, size);
    OHOS::IsJSRelativeTimeFormatFuzztest(data, size);
    return 0;
}