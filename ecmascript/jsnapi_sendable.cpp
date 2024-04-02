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

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_dictionary.h"
#include "js_function.h"

namespace panda::ecmascript {

JSNapiSendable::JSNapiSendable(JSThread *thread, FunctionRef::SendablePropertiesInfos &infos, Local<StringRef> &name)
{
    InitSPropertiesInfos(thread, infos, name);
    Init(thread, infos.staticPropertiesInfo, staticDescs_);
    Init(thread, infos.nonStaticPropertiesInfo, nonStaticDescs_);
    Init(thread, infos.instancePropertiesInfo, instanceDescs_);
}

void JSNapiSendable::InitSPropertiesInfos(JSThread *thread,
                                          FunctionRef::SendablePropertiesInfos &infos,
                                          Local<StringRef> &name)
{
    EcmaVM *vm = thread->GetEcmaVM();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> napiWrapperKey = globalConst->GetHandledNapiWrapperString();

    infos.instancePropertiesInfo.keys.insert(infos.instancePropertiesInfo.keys.begin(),
                                             JSNApiHelper::ToLocal<StringRef>(napiWrapperKey));
    infos.instancePropertiesInfo.types.insert(infos.instancePropertiesInfo.types.begin(),
                                              FunctionRef::SendableType::OBJECT);
    infos.instancePropertiesInfo.attributes.insert(infos.instancePropertiesInfo.attributes.begin(),
                                                   PropertyAttribute(JSValueRef::Null(vm), true, false, true));

    JSHandle<JSTaggedValue> nameKey = globalConst->GetHandledNameString();
    infos.staticPropertiesInfo.keys.insert(infos.staticPropertiesInfo.keys.begin(),
                                           JSNApiHelper::ToLocal<StringRef>(nameKey));
    infos.staticPropertiesInfo.types.insert(infos.staticPropertiesInfo.types.begin(), FunctionRef::SendableType::NONE);
    infos.staticPropertiesInfo.attributes.insert(infos.staticPropertiesInfo.attributes.begin(),
                                                 PropertyAttribute(name, false, false, false));

    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    infos.nonStaticPropertiesInfo.keys.insert(infos.nonStaticPropertiesInfo.keys.begin(),
                                              JSNApiHelper::ToLocal<StringRef>(constructorKey));
    infos.nonStaticPropertiesInfo.types.insert(infos.nonStaticPropertiesInfo.types.begin(),
                                               FunctionRef::SendableType::OBJECT);
    infos.nonStaticPropertiesInfo.attributes.insert(infos.nonStaticPropertiesInfo.attributes.begin(),
                                                    PropertyAttribute(JSValueRef::Null(vm), false, false, false));
}

void JSNapiSendable::Init(JSThread *thread,
                          FunctionRef::SendablePropertiesInfo &info,
                          std::vector<PropertyDescriptor> &descs)
{
    EcmaVM *vm = thread->GetEcmaVM();
    auto length = info.keys.size();

    for (size_t i = 0; i < length; ++i) {
        PropertyDescriptor desc(thread, info.attributes[i].IsWritable(), info.attributes[i].IsEnumerable(),
                                info.attributes[i].IsConfigurable());
        desc.SetKey(JSNApiHelper::ToJSHandle(info.keys[i]));
        auto type = GetSharedFieldType(thread, info.types[i], info.attributes[i].GetValue(vm));
        desc.SetSharedFieldType(type);
        desc.SetValue(JSNApiHelper::ToJSHandle(info.attributes[i].GetValue(vm)));
        descs.push_back(desc);
    }
}

SharedFieldType JSNapiSendable::GetSharedFieldType(JSThread *thread,
                                                   FunctionRef::SendableType type,
                                                   Local<JSValueRef> value)
{
    switch (type) {
        case FunctionRef::SendableType::OBJECT:
            return SharedFieldType::SENDABLE;
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
