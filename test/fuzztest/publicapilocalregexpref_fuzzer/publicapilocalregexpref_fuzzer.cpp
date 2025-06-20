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

#include <fuzzer/FuzzedDataProvider.h>
#include "publicapilocalregexpref_fuzzer.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/regexp/regexp_parser.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/napi/jsnapi_helper.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
void LocalRegExpAllFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    {
        JsiFastNativeScope scope(vm);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        FuzzedDataProvider fdp(data, size);
        const uint32_t flags = fdp.ConsumeIntegralInRange<uint32_t>(0, (1 << RegExpParser::FLAG_NUM) - 1);
        const uint32_t len = fdp.ConsumeIntegralInRange<uint32_t>(0, 1024);
        JSThread *thread = vm->GetJSThread();
        ObjectFactory *factory = vm->GetFactory();
        auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSTaggedValue> proto = globalEnv->GetObjectFunctionPrototype();
        JSHandle<JSHClass> jSRegExpClass = factory->NewEcmaHClass(JSRegExp::SIZE, JSType::JS_REG_EXP, proto);
        JSHandle<JSRegExp> jSRegExp = JSHandle<JSRegExp>::Cast(factory->NewJSObject(jSRegExpClass));
        jSRegExp->SetByteCodeBuffer(thread, JSTaggedValue::Undefined());
        jSRegExp->SetOriginalSource(thread, JSTaggedValue::Undefined());
        jSRegExp->SetGroupName(thread, JSTaggedValue::Undefined());
        jSRegExp->SetOriginalFlags(thread, JSTaggedValue(flags));
        jSRegExp->SetLength(len);
        JSHandle<JSTaggedValue> jsregtag = JSHandle<JSTaggedValue>::Cast(jSRegExp);
        Local<RegExpRef> object = JSNApiHelper::ToLocal<RegExpRef>(jsregtag);
        object->GetOriginalSource(vm);
        object->GetOriginalFlags(vm);
        object->IsGlobal(vm);
        object->IsIgnoreCase(vm);
        object->IsMultiline(vm);
        object->IsDotAll(vm);
        object->IsUtf16(vm);
        object->IsStick(vm);
    }
    JSNApi::DestroyJSVM(vm);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::LocalRegExpAllFuzzTest(data, size);
    return 0;
}