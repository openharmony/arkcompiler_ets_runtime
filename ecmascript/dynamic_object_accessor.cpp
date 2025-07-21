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

#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/dynamic_object_accessor.h"
#include "ecmascript/object_operator.h"
#include "ecmascript/object_factory.h"
#include "common_interfaces/objects/base_object_dispatcher.h"

namespace panda::ecmascript {

DynamicObjectAccessor DynamicObjectAccessor::dynObjectAccessor_;

void DynamicObjectAccessor::Initialize()
{
    common::BaseObjectDispatcher::GetDispatcher().RegisterDynamicObjectAccessor(&dynObjectAccessor_);
}

bool DynamicObjectAccessor::HasProperty(ThreadHolder *thread, const BaseObject *obj, const char *name) const
{
    JSThread *jsThread = thread->GetJSThread();
    ObjectFactory *factory = jsThread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] EcmaHandleScope handleScope(jsThread);
    JSHandle<JSTaggedValue> holderHandle(jsThread, TaggedObject::Cast(obj));
    JSHandle<JSTaggedValue> keyHandle(factory->NewFromUtf8(name));
    return JSTaggedValue::HasProperty(jsThread, holderHandle, keyHandle);
}

JSTaggedValue DynamicObjectAccessor::GetProperty(ThreadHolder *thread, const BaseObject *obj, const char *name) const
{
    JSThread *jsThread = thread->GetJSThread();
    ObjectFactory *factory = jsThread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] EcmaHandleScope handleScope(jsThread);
    JSHandle<JSTaggedValue> holderHandle(jsThread, TaggedObject::Cast(obj));
    JSHandle<JSTaggedValue> keyHandle(factory->NewFromUtf8(name));
    return JSTaggedValue::GetProperty(jsThread, holderHandle, keyHandle).GetValue().GetTaggedValue();
}

bool DynamicObjectAccessor::SetProperty(ThreadHolder *thread, BaseObject *obj, const char *name, JSTaggedValue value)
{
    JSThread *jsThread = thread->GetJSThread();
    ObjectFactory *factory = jsThread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] EcmaHandleScope handleScope(jsThread);
    JSHandle<JSTaggedValue> holderHandle(jsThread, TaggedObject::Cast(obj));
    JSHandle<JSTaggedValue> keyHandle(factory->NewFromUtf8(name));
    JSHandle<JSTaggedValue> valueHandle(jsThread, value);
    return JSTaggedValue::SetProperty(jsThread, holderHandle, keyHandle, valueHandle);
}

bool DynamicObjectAccessor::HasElementByIdx(ThreadHolder *thread, const BaseObject *obj, const uint32_t index) const
{
    JSThread *jsThread = thread->GetJSThread();
    [[maybe_unused]] EcmaHandleScope handleScope(jsThread);
    JSHandle<JSTaggedValue> holderHandle(jsThread, TaggedObject::Cast(obj));
    return JSTaggedValue::HasProperty(jsThread, holderHandle, index);
}

JSTaggedValue DynamicObjectAccessor::GetElementByIdx(ThreadHolder *thread, const BaseObject *obj,
                                                     const uint32_t index) const
{
    JSThread *jsThread = thread->GetJSThread();
    [[maybe_unused]] EcmaHandleScope handleScope(jsThread);
    JSHandle<JSTaggedValue> holderHandle(jsThread, TaggedObject::Cast(obj));
    return JSTaggedValue::GetProperty(jsThread, holderHandle, index).GetValue().GetTaggedValue();
}

bool DynamicObjectAccessor::SetElementByIdx(ThreadHolder *thread, BaseObject *obj, const uint32_t index,
                                            JSTaggedValue value)
{
    JSThread *jsThread = thread->GetJSThread();
    [[maybe_unused]] EcmaHandleScope handleScope(jsThread);
    JSHandle<JSTaggedValue> holderHandle(jsThread, TaggedObject::Cast(obj));
    JSHandle<JSTaggedValue> valueHandle(jsThread, value);
    return JSTaggedValue::SetProperty(jsThread, holderHandle, index, valueHandle);
}
}
