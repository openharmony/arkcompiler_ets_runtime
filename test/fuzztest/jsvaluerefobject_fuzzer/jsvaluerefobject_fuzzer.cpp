/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include <fuzzer/FuzzedDataProvider.h>
#include "jsvaluerefobject_fuzzer.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/js_function.h"
#include "ecmascript/napi/include/dfx_jsnapi.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "ecmascript/object_factory.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
    void JSValueRefTypesFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        FuzzedDataProvider fdp(data, size);
        std::string str = fdp.ConsumeRandomLengthString(1024);
        Local<StringRef> description = StringRef::NewFromUtf8(vm, str.data());
        Local<SymbolRef> symbol = SymbolRef::New(vm, description);
        symbol->IsSymbol(vm);
        const int input = fdp.ConsumeIntegral<int>();
        Local<IntegerRef> intValue = IntegerRef::New(vm, input);
        intValue->IsBigInt(vm);
        intValue->IsObject(vm);
        intValue->IsMapIterator(vm);
        ObjectFactory *factory = vm->GetFactory();
        JSHandle<JSArguments> obj = factory->NewJSArguments();
        JSHandle<JSTaggedValue> argumentTag = JSHandle<JSTaggedValue>::Cast(obj);
        JSNApiHelper::ToLocal<ObjectRef>(argumentTag)->IsArgumentsObject(vm);
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::JSValueRefTypesFuzzTest(data, size);
    return 0;
}