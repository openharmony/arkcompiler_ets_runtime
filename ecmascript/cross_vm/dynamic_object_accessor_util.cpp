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

#include "objects/dynamic_object_accessor_util.h"
#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/debugger/js_debugger_manager.h"
#include "ecmascript/dfx/stackinfo/js_stackinfo.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/interpreter/interpreter_assembly.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/napi/include/jsnapi_expo.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/object_operator.h"


namespace common {

using panda::ecmascript::EcmaHandleScope;
using panda::ecmascript::EcmaVM;
using panda::ecmascript::EcmaRuntimeCallInfo;
using panda::ecmascript::JSFunction;
using panda::ecmascript::JSHandle;
using panda::ecmascript::JSTaggedValue;
using panda::ecmascript::JSThread;
using panda::ecmascript::ObjectFactory;
using panda::ecmascript::TaggedObject;
using panda::FunctionCallScope;
using panda::EscapeLocalScope;
using panda::JSValueRef;
using panda::JSNApi;


TaggedType* DynamicObjectAccessorUtil::GetProperty(const BaseObject *obj, const char *name)
{
    JSThread *jsThread = JSThread::GetCurrent();
    panda::ecmascript::ThreadManagedScope managedScope(jsThread);
    ObjectFactory *factory = jsThread->GetEcmaVM()->GetFactory();
    panda::EscapeLocalScope scope(jsThread->GetEcmaVM());
    JSHandle<JSTaggedValue> holderHandle(jsThread, TaggedObject::Cast(obj));
    JSHandle<JSTaggedValue> keyHandle(factory->NewFromUtf8(name));
    auto resultValue = JSTaggedValue::GetProperty(jsThread, holderHandle, keyHandle).GetValue();
    auto ret = scope.Escape(panda::JSNApiHelper::ToLocal<panda::JSValueRef>(resultValue));
    return reinterpret_cast<TaggedType *>(*ret);
}

bool DynamicObjectAccessorUtil::SetProperty(const BaseObject *obj, const char *name, TaggedType value)
{
    JSThread *jsThread = JSThread::GetCurrent();
    panda::ecmascript::ThreadManagedScope managedScope(jsThread);
    ObjectFactory *factory = jsThread->GetEcmaVM()->GetFactory();
    panda::EscapeLocalScope scope(jsThread->GetEcmaVM());
    JSHandle<JSTaggedValue> holderHandle(jsThread, TaggedObject::Cast(obj));
    JSHandle<JSTaggedValue> keyHandle(factory->NewFromUtf8(name));
    JSTaggedValue taggedValue(value);
    JSHandle<JSTaggedValue> valueHandle(jsThread, taggedValue);
    return JSTaggedValue::SetProperty(jsThread, holderHandle, keyHandle, valueHandle);
}

TaggedType* DynamicObjectAccessorUtil::CallFunction(TaggedType jsThis, TaggedType function, int32_t argc,
                                                    TaggedType *argv)
{
    auto vm = JSThread::GetCurrent()->GetEcmaVM();
    EscapeLocalScope scope(vm);
    TaggedType *undefindType = reinterpret_cast<TaggedType *>(*(JSValueRef::Undefined(vm)));
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, undefindType);
    panda::ecmascript::ThreadManagedScope managedScope(thread);
    FunctionCallScope callScope(EcmaVM::ConstCast(vm));
    JSTaggedValue funcValue(function);
    if (!funcValue.IsCallable()) {
        return undefindType;
    }
    vm->GetJsDebuggerManager()->ClearSingleStepper();
    JSHandle<JSTaggedValue> func(thread, funcValue);
    LOG_IF_SPECIAL(func, ERROR);
    JSTaggedValue thisObj(jsThis);
    JSHandle<JSTaggedValue> thisValue(thread, thisObj);
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info =
        panda::ecmascript::EcmaInterpreter::NewRuntimeCallInfo(thread, func, thisValue, undefined, argc);
    RETURN_VALUE_IF_ABRUPT(thread, undefindType);
    info->SetCallArg(argc, argv);
    JSTaggedValue result = JSFunction::Call(info);
    if (thread->HasPendingException()) {
        panda::ecmascript::JsStackInfo::BuildCrashInfo(thread);
    }
    RETURN_VALUE_IF_ABRUPT(thread, undefindType);
    EcmaVM::ClearKeptObjects(thread);
    vm->GetJsDebuggerManager()->NotifyReturnNative();
    JSHandle<JSTaggedValue> resultValue(thread, result);
    auto ret = scope.Escape(panda::JSNApiHelper::ToLocal<JSValueRef>(resultValue));
    return reinterpret_cast<TaggedType *>(*ret);
}
}