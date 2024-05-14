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

#ifndef ECMASCRIPT_JS_NAPI_SENDABLE_H
#define ECMASCRIPT_JS_NAPI_SENDABLE_H

#include "ecmascript/layout_info.h"
#include "ecmascript/property_attributes.h"

namespace panda::ecmascript {

class JSNapiSendable {
public:
    JSNapiSendable(JSThread *thread, FunctionRef::SendablePropertiesInfos &infos, Local<StringRef> &name);
    void SetSConstructor(JSHandle<JSFunction> constructor);
    const std::vector<PropertyDescriptor> GetStaticDescs() const
    {
        return staticDescs_;
    }
    const std::vector<PropertyDescriptor> GetNonStaticDescs() const
    {
        return nonStaticDescs_;
    }
    const std::vector<PropertyDescriptor> GetInstanceDescs() const
    {
        return instanceDescs_;
    }
    static void InitWithPropertiesInfo(JSThread *thread,
                                       FunctionRef::SendablePropertiesInfo &info,
                                       std::vector<PropertyDescriptor> &descs);

private:
    std::vector<PropertyDescriptor> staticDescs_;
    std::vector<PropertyDescriptor> nonStaticDescs_;
    std::vector<PropertyDescriptor> instanceDescs_;

    static void InitStaticDescription(JSThread *thread, std::vector<PropertyDescriptor> &descs, Local<StringRef> &name);
    static void InitNonStaticDescription(JSThread *thread, std::vector<PropertyDescriptor> &descs);
    static void InitInstanceDescription(JSThread *thread, std::vector<PropertyDescriptor> &descs);
    static SharedFieldType GetSharedFieldType(JSThread *thread,
                                              FunctionRef::SendableType type,
                                              Local<JSValueRef> value);
};

}  // namespace panda::ecmascript

#endif
