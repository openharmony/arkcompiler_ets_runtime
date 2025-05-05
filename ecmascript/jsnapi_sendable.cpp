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

#include "ecmascript/jsnapi_sendable.h"
#include "ecmascript/global_env.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "js_function.h"

namespace panda::ecmascript {

JSNapiSendable::JSNapiSendable(JSThread *thread, FunctionRef::SendablePropertiesInfos &infos, Local<StringRef> &name)
{
    InitStaticDescription(thread, staticDescs_, name);
    InitNonStaticDescription(thread, nonStaticDescs_);
    InitInstanceDescription(thread, instanceDescs_);
    InitWithPropertiesInfo(thread, infos.staticPropertiesInfo, staticDescs_);
    InitWithPropertiesInfo(thread, infos.nonStaticPropertiesInfo, nonStaticDescs_);
    InitWithPropertiesInfo(thread, infos.instancePropertiesInfo, instanceDescs_);
}

void JSNapiSendable::SetSConstructor(JSHandle<JSFunction> constructor)
{
    nonStaticDescs_[0].SetValue(JSHandle<JSTaggedValue>::Cast(constructor));
}

void JSNapiSendable::InitStaticDescription(JSThread *thread,
                                           std::vector<PropertyDescriptor> &descs,
                                           Local<StringRef> &name)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    JSHandle<JSTaggedValue> nameKey = globalConst->GetHandledNameString();
    PropertyDescriptor nameDesc(thread, false, false, false);
    nameDesc.SetKey(nameKey);
    nameDesc.SetSharedFieldType(SharedFieldType::STRING);
    nameDesc.SetValue(JSNApiHelper::ToJSHandle(name));
    descs.push_back(nameDesc);

    JSHandle<JSTaggedValue> prototypeKey = globalConst->GetHandledPrototypeString();
    PropertyDescriptor prototypeDesc(thread, false, false, false);
    prototypeDesc.SetKey(prototypeKey);
    prototypeDesc.SetSharedFieldType(SharedFieldType::SENDABLE);
    prototypeDesc.SetValue(globalConst->GetHandledNull());
    descs.push_back(prototypeDesc);

    JSHandle<JSTaggedValue> sendableElementsKey = globalConst->GetHandledSendableElementsSymbol();
    PropertyDescriptor sendableElementsDesc(thread, false, false, false);
    sendableElementsDesc.SetKey(sendableElementsKey);
    sendableElementsDesc.SetSharedFieldType(SharedFieldType::SENDABLE);
    sendableElementsDesc.SetValue(globalConst->GetHandledUndefined());
    descs.push_back(sendableElementsDesc);
}

void JSNapiSendable::InitNonStaticDescription(JSThread *thread, std::vector<PropertyDescriptor> &descs)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    PropertyDescriptor constructorDesc(thread, false, false, false);
    constructorDesc.SetKey(constructorKey);
    constructorDesc.SetSharedFieldType(SharedFieldType::SENDABLE);
    constructorDesc.SetValue(globalConst->GetHandledNull());
    descs.push_back(constructorDesc);
}

void JSNapiSendable::InitInstanceDescription(JSThread *thread, std::vector<PropertyDescriptor> &descs)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    JSHandle<JSTaggedValue> napiWrapperKey = globalConst->GetHandledNapiWrapperString();
    PropertyDescriptor napiWrapperDesc(thread, true, false, false);
    napiWrapperDesc.SetKey(napiWrapperKey);
    napiWrapperDesc.SetSharedFieldType(SharedFieldType::SENDABLE);
    napiWrapperDesc.SetValue(globalConst->GetHandledNull());
    descs.push_back(napiWrapperDesc);
}

void JSNapiSendable::InitWithPropertiesInfo(JSThread *thread,
                                            FunctionRef::SendablePropertiesInfo &info,
                                            std::vector<PropertyDescriptor> &descs)
{
    EcmaVM *vm = thread->GetEcmaVM();
    auto length = info.keys.size();

    for (size_t i = 0; i < length; ++i) {
        PropertyDescriptor desc(thread,
                                info.attributes[i].IsWritable(),
                                info.attributes[i].IsEnumerable(),
                                info.attributes[i].IsConfigurable());
        desc.SetKey(JSNApiHelper::ToJSHandle(info.keys[i]));
        auto type = GetSharedFieldType(thread, info.types[i], info.attributes[i].GetValue(vm));
        desc.SetSharedFieldType(type);
        desc.SetValue(JSNApiHelper::ToJSHandle(info.attributes[i].GetValue(vm)));
        descs.push_back(desc);
    }
    InitInstanceDescription(thread, descs);
}

SharedFieldType JSNapiSendable::GetSharedFieldType(JSThread *thread,
                                                   FunctionRef::SendableType type,
                                                   Local<JSValueRef> value)
{
    switch (type) {
        case FunctionRef::SendableType::OBJECT:
            return SharedFieldType::SENDABLE;
        case FunctionRef::SendableType::GENERIC:
            return SharedFieldType::GENERIC;
        case FunctionRef::SendableType::NONE: {
            auto valueHandle = JSNApiHelper::ToJSHandle(value);
            if (valueHandle->IsUndefined()) {
                return SharedFieldType::NONE;
            }
            if (valueHandle->IsNull()) {
                // fixme: SharedFieldType::NULL ?
                return SharedFieldType::SENDABLE;
            }
            if (valueHandle->IsNumber()) {
                return SharedFieldType::NUMBER;
            }
            if (valueHandle->IsString()) {
                return SharedFieldType::STRING;
            }
            if (valueHandle->IsBoolean()) {
                return SharedFieldType::BOOLEAN;
            }
            if (valueHandle->IsJSShared()) {
                return SharedFieldType::SENDABLE;
            }
            if (valueHandle->IsBigInt()) {
                return SharedFieldType::BIG_INT;
            }
            THROW_TYPE_ERROR_AND_RETURN(thread, "Unknown SharedFieldType", SharedFieldType::NONE);
        }
        default:
            THROW_TYPE_ERROR_AND_RETURN(thread, "Unknown SharedFieldType", SharedFieldType::NONE);
    }
}

}  // namespace panda::ecmascript
