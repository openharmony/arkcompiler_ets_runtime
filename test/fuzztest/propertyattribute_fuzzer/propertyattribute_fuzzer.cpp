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

#include "ecmascript/global_env.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "propertyattribute_fuzzer.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
void PropertyAttributeFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    uint8_t *ptr = nullptr;
    ptr = const_cast<uint8_t *>(data);
    PropertyAttribute::Default();
    JSNApi::DestroyJSVM(vm);
}

void PropertyAttributeSetIsFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    uint8_t *ptr = nullptr;
    ptr = const_cast<uint8_t *>(data);
    bool w = false, e = true, c = true;
    constexpr int num = 2; //判断奇偶性
    if (size % num == 1) {
        w = true;
        e = false;
        c = false;
    }
    Local<JSValueRef> jsvalueref;
    PropertyAttribute propertyattribute(jsvalueref, w, e, c);
    propertyattribute.SetWritable(e);
    propertyattribute.IsWritable();
    propertyattribute.SetEnumerable(e);
    propertyattribute.IsEnumerable();
    propertyattribute.SetConfigurable(e);
    propertyattribute.IsConfigurable();
    propertyattribute.HasWritable();
    propertyattribute.HasEnumerable();
    propertyattribute.HasConfigurable();
    propertyattribute.SetValue(jsvalueref);
    propertyattribute.GetValue(vm);
    propertyattribute.HasValue();
    propertyattribute.SetSetter(jsvalueref);
    propertyattribute.GetSetter(vm);
    propertyattribute.HasSetter();
    JSNApi::DestroyJSVM(vm);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::PropertyAttributeFuzzTest(data, size);
    OHOS::PropertyAttributeSetIsFuzzTest(data, size);
    return 0;
}