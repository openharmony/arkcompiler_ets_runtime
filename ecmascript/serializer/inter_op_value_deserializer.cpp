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

#include "ecmascript/serializer/inter_op_value_deserializer.h"

#include "ecmascript/checkpoint/thread_state_transition.h"

namespace panda::ecmascript {
size_t InterOpValueDeserializer::DerivedExtraReadSingleEncodeData(uint8_t encodeFlag, uintptr_t objAddr,
                                                                  size_t fieldOffset)
{
    size_t handledFieldSize = sizeof(JSTaggedType);
    ObjectSlot slot(objAddr + fieldOffset);
    switch (encodeFlag) {
        case (uint8_t)EncodeFlag::XREF_BINDING_OBJECT: {
            slot.Update(JSTaggedValue::Undefined().GetRawData());
            AttachXRefFunc af = reinterpret_cast<AttachXRefFunc>(data_->ReadJSTaggedType(position_));
            void *attachXRefData = reinterpret_cast<void *>(data_->ReadJSTaggedType(position_));
            JSHandle<JSTaggedValue> obj(thread_, JSTaggedValue(objAddr));
            // defer new xref binding object until deserialize finish
            xRefBindingAttachInfos_.emplace_back(af, attachXRefData, obj, fieldOffset);
            break;
        }
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable " << static_cast<int>(encodeFlag);
            UNREACHABLE();
    }
    return handledFieldSize;
}

void InterOpValueDeserializer::DeserializeSpecialRecordedObjects()
{
    for (auto &info : xRefBindingAttachInfos_) {
        DeserializeXRefBindingObject(&info);
    }
}

void InterOpValueDeserializer::DeserializeXRefBindingObject(XRefBindingAttachInfo *info)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    AttachXRefFunc af = info->af_;
    void *attachXRefData = info->attachXRefData_;
    Local<JSValueRef> attachVal;
    {
        ThreadNativeScope nativeScope(thread_);
        attachVal = af(engine_, attachXRefData);
    }
    if (attachVal.IsEmpty()) {
        LOG_ECMA(ERROR) << "NativeBindingObject is empty";
        attachVal = JSValueRef::Undefined(thread_->GetEcmaVM());
    }
    JSTaggedType res = JSNApiHelper::ToJSHandle(attachVal).GetTaggedType();
    ObjectSlot slot = info->GetSlot();
    slot.Update(res);
    if (!JSTaggedValue(res).IsInvalidValue()) {
        WriteBarrier(thread_, reinterpret_cast<void *>(info->GetObjAddr()), info->GetFieldOffset(), res);
    }
}
}  // namespace panda::ecmascript

