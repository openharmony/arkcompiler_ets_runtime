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

#include "ecmascript/serializer/inter_op_value_serializer.h"

namespace panda::ecmascript {
void InterOpValueSerializer::SerializeObjectImpl(TaggedObject *object, bool isWeak)
{
    if (TrySerializeInterOpObject(object, isWeak)) {
        return;
    }
    ValueSerializer::SerializeObjectImpl(object, isWeak);
}

bool InterOpValueSerializer::TrySerializeInterOpObject(TaggedObject *object, bool isWeak)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread_);
    JSType type = object->GetClass()->GetObjectType();
    JSHandle<JSTaggedValue> target(thread_, object);
    switch (type) {
        case JSType::JS_OBJECT:
        case JSType::JS_FUNCTION:
            break;
        case JSType::JS_PROXY:
            target = JSProxy::Cast(object)->GetSourceTarget(thread_);
            break;
        default:
            return false;
    }
    JSHandle<JSObject> objHandle(target);
    JSHandle<JSTaggedValue> keyHandle = thread_->GlobalConstants()->GetHandledProxyNapiWrapperString();
    // Otherwise, before lookup key in JSObject will intern string, which may trigger GC
    ASSERT(EcmaStringAccessor(keyHandle->GetTaggedObject()).IsInternString());
    JSTaggedValue xRef = JSObject::GetProperty(thread_, objHandle, keyHandle).GetValue().GetTaggedValue();
    if (xRef.IsUndefined()) {
        return false;
    }

    SerializeInterOpObjectImpl(object, JSObject::Cast(xRef.GetTaggedObject()), isWeak);
    return true;
}

void InterOpValueSerializer::SerializeInterOpObjectImpl(TaggedObject *proxyObject, JSObject *xRefObject, bool isWeak)
{
    if (isWeak) {
        data_->WriteEncodeFlag(EncodeFlag::WEAK);
    }
    if (SerializeReference(proxyObject)) {
        return;
    }

    JSTaggedValue nativePointer = xRefObject->GetNativePointerByIndex(thread_, 0);
    if (UNLIKELY(nativePointer.IsUndefined())) {
        LOG_ECMA(ERROR) << "InterOpValueSerializer: SerializeInterOpObject XRefObject field is not JSNativePointer";
        notSupport_ = true;
        return;
    }

    ASSERT(nativePointer.IsNativePointer());
    panda::JSNApi::XRefBindingInfo *info = reinterpret_cast<panda::JSNApi::XRefBindingInfo *>(
        JSNativePointer::Cast(nativePointer.GetTaggedObject())->GetData());
    if (info == nullptr) {
        LOG_ECMA(ERROR) << "InterOpValueSerializer: SerializeInterOpObject XRefBindingInfo is nullptr";
        notSupport_ = true;
        return;
    }

    AttachXRefFunc attachXRefFunc = reinterpret_cast<AttachXRefFunc>(info->attachXRefFunc);
    void *attachXRefData = info->attachXRefData;
    data_->WriteEncodeFlag(EncodeFlag::XREF_BINDING_OBJECT);
    data_->WriteJSTaggedType(reinterpret_cast<JSTaggedType>(attachXRefFunc));
    data_->WriteJSTaggedType(reinterpret_cast<JSTaggedType>(attachXRefData));
}
}  // namespace panda::ecmascript

