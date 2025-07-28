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

#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/ecma_global_storage.h"
#include "ecmascript/global_env.h"
#include "ecmascript/module/napi_module_loader.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/napi/include/jsnapi_expo.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/napi/jsnapi_helper.h"

namespace panda {
using ecmascript::JSTaggedValue;
template <typename T>
using JSHandle = ecmascript::JSHandle<T>;

using ModulePathHelper = ecmascript::ModulePathHelper;
using ecmascript::ObjectFactory;

Local<StringRef> StringRef::GetProxyNapiWrapperString(const EcmaVM *vm)
{
    // Omit exception check because ark calls here may not
    // cause side effect even pending exception exists.
    CROSS_THREAD_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(thread);
    JSHandle<JSTaggedValue> proxyNapiWapperString = thread->GlobalConstants()->GetHandledProxyNapiWrapperString();
    return JSNApiHelper::ToLocal<StringRef>(proxyNapiWapperString);
}

Local<ObjectRef> ObjectRef::NewJSXRefObject(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(thread);
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> object(factory->NewJSXRefObject());
    return JSNApiHelper::ToLocal<ObjectRef>(object);
}

void JSNApi::SetStackInfo(const EcmaVM *vm, const panda::StackInfo &info)
{
    JSThread *thread = vm->GetJSThread();
    thread->SetStackStart(info.stackStart);
    thread->SetStackLimit(info.stackStart - info.stackSize);
}

panda::StackInfo JSNApi::GetStackInfo(const EcmaVM *vm)
{
    JSThread *thread = vm->GetJSThread();
    size_t stackStart = thread->GetStackStart();
    size_t stackSize = stackStart - thread->GetStackLimit();
    return {stackStart, stackSize};
}

uintptr_t JSNApi::GetXRefGlobalHandleAddr(const EcmaVM *vm, uintptr_t localAddress)
{
    CROSS_THREAD_CHECK(vm);
    if (localAddress == 0) {
        return 0;
    }
    ecmascript::ThreadManagedScope scope(thread);
    JSTaggedType value = *(reinterpret_cast<JSTaggedType *>(localAddress));
    return thread->NewXRefGlobalHandle(value);
}


void JSNApi::DisposeXRefGlobalHandleAddr(const EcmaVM *vm, uintptr_t addr)
{
    CROSS_THREAD_CHECK(vm);
    if (addr == 0 || !reinterpret_cast<ecmascript::Node *>(addr)->IsUsing()) {
        return;
    }
    thread->DisposeXRefGlobalHandle(addr);
}

#ifdef PANDA_JS_ETS_HYBRID_MODE
void JSNApi::MarkFromObject(const EcmaVM *vm, uintptr_t addr)
{
    if (addr == 0 || !reinterpret_cast<ecmascript::Node *>(addr)->IsUsing()) {
        return;
    }
    JSTaggedType value = *(reinterpret_cast<JSTaggedType *>(addr));
    vm->GetCrossVMOperator()->MarkFromObject(value);
}

bool JSNApi::IsObjectAlive(const EcmaVM *vm, uintptr_t addr)
{
    if (addr == 0 || !reinterpret_cast<ecmascript::Node *>(addr)->IsUsing()) {
        return false;
    }
    JSTaggedType value = *(reinterpret_cast<JSTaggedType *>(addr));
    return vm->GetCrossVMOperator()->IsObjectAlive(value);
}

bool JSNApi::IsValidHeapObject(const EcmaVM *vm, uintptr_t addr)
{
    if (addr == 0 || !reinterpret_cast<ecmascript::Node *>(addr)->IsUsing()) {
        return false;
    }
    JSTaggedType value = *(reinterpret_cast<JSTaggedType *>(addr));
    return vm->GetCrossVMOperator()->IsValidHeapObject(value);
}
#endif // PANDA_JS_ETS_HYBRID_MODE

Local<ObjectRef> JSNApi::GetModuleNameSpaceWithPath(const EcmaVM *vm, const char *path)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    auto [filePath, recordName] = ModulePathHelper::ResolvePath(path);
    ecmascript::ThreadManagedScope managedScope(thread);
    JSHandle<JSTaggedValue> moduleNamespace =
        ecmascript::NapiModuleLoader::LoadModuleNameSpaceFromFile(thread, recordName, filePath);
    return JSNApiHelper::ToLocal<ObjectRef>(moduleNamespace);
}

std::pair<std::string, std::string> JSNApi::ResolveOhmUrl(std::string ohmUrl)
{
    return ModulePathHelper::ResolveOhmUrl(ohmUrl);
}

#ifdef PANDA_JS_ETS_HYBRID_MODE
void HandshakeHelper::DoHandshake([[maybe_unused]] EcmaVM *vm, void *stsIface, void **ecmaIface)
{
    ecmascript::CrossVMOperator::DoHandshake(vm, stsIface, ecmaIface);
}
#endif  // PANDA_JS_ETS_HYBRID_MODE

void JSNApi::InitHybridVMEnv(EcmaVM *vm)
{
    auto instance = ecmascript::Runtime::GetInstance();
    ASSERT(instance != nullptr);

    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(thread);

    instance->SetHybridVm(true);
    ecmascript::ObjectFactory *factory = vm->GetFactory();
    JSHandle<ecmascript::GlobalEnv> env = vm->GetGlobalEnv();
    if (env->GetInterfaceTypeSymbol().GetTaggedValue().IsUndefined()) {
        JSHandle<JSTaggedValue> interfaceTypeSymbol(factory->NewPrivateNameSymbolWithChar("interfaceType"));
        env->SetInterfaceTypeSymbol(thread, interfaceTypeSymbol.GetTaggedValue());
    }
    vm->CreateHybridParam();
}

}  // namespace panda
