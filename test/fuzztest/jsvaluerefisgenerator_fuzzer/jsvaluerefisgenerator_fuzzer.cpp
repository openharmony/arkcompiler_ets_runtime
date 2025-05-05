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

#include "jsvaluerefisgenerator_fuzzer.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_function_kind.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/napi/jsnapi_helper.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
void IsGeneratorObjectFuzzTest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    {
        JsiFastNativeScope scope(vm);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        ObjectFactory *factory = vm->GetFactory();
        auto env = vm->GetGlobalEnv();
        JSHandle<JSTaggedValue> genFunc = env->GetGeneratorFunctionFunction();
        JSHandle<JSGeneratorObject> genObjHandleVal = factory->NewJSGeneratorObject(genFunc);
        JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetGeneratorFunctionClass());
        JSHandle<JSFunction> generatorFunc = JSHandle<JSFunction>::Cast(factory->NewJSObject(hclass));
        JSFunction::InitializeJSFunction(vm->GetJSThread(), generatorFunc, FunctionKind::GENERATOR_FUNCTION);
        JSHandle<JSTaggedValue> genObjTagHandleVal = JSHandle<JSTaggedValue>::Cast(genObjHandleVal);
        Local<JSValueRef> genObjectRef = JSNApiHelper::ToLocal<GeneratorObjectRef>(genObjTagHandleVal);
        genObjectRef->IsGeneratorObject(vm);
    }
    JSNApi::DestroyJSVM(vm);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::IsGeneratorObjectFuzzTest(data, size);
    return 0;
}