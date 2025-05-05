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

#include "jsvaluerefisplainarray_fuzzer.h"
#include "ecmascript/containers/containers_list.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_global_object.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/containers/containers_hashset.h"
#include "ecmascript/common.h"
#include "ecmascript/frames.h"
#include "ecmascript/tagged_list.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/js_api/js_api_list.h"
#include "ecmascript/js_api/js_api_linked_list.h"
#include "ecmascript/js_api/js_api_linked_list_iterator.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/containers/containers_arraylist.h"
#include "ecmascript/js_api/js_api_arraylist.h"
#include "ecmascript/builtins/builtins_function.h"
#include "ecmascript/builtins/builtins.h"
#include "ecmascript/ecma_global_storage.h"
#include "ecmascript/js_bigint.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_string_iterator.h"
#include "ecmascript/js_date_time_format.h"
#include "ecmascript/js_tagged_number.h"
#include "ecmascript/js_api/js_api_hashmap.h"
#include "ecmascript/builtins/builtins_regexp.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/tagged_hash_array.h"
#include "ecmascript/containers/containers_lightweightmap.h"
#include "ecmascript/containers/containers_lightweightset.h"
#include "ecmascript/js_api/js_api_plain_array.h"

using namespace panda;
using namespace panda::test;
using namespace panda::ecmascript;
using namespace panda::ecmascript::containers;

namespace OHOS {
EcmaRuntimeCallInfo *CreateEcmaRuntimeCallInfo(JSThread *thread, JSTaggedValue newTgt, uint32_t argvLength)
{
    const uint8_t testDecodedSize = 2;
    int32_t numActualArgs = argvLength / testDecodedSize + 1;
    JSTaggedType *sp = const_cast<JSTaggedType *>(thread->GetCurrentSPFrame());

    size_t frameSize = 0;
    if (thread->IsAsmInterpreter()) {
        frameSize = InterpretedEntryFrame::NumOfMembers() + numActualArgs;
    } else {
        frameSize = InterpretedFrame::NumOfMembers() + numActualArgs;
    }
    JSTaggedType *newSp = sp - frameSize; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    for (int i = numActualArgs; i > 0; i--) {
        newSp[i - 1] = JSTaggedValue::Undefined().GetRawData();
    }
    EcmaRuntimeCallInfo *ecmaRuntimeCallInfo = reinterpret_cast<EcmaRuntimeCallInfo *>(newSp - 2);
    *(--newSp) = numActualArgs;
    *(--newSp) = panda::ecmascript::ToUintPtr(thread);
    ecmaRuntimeCallInfo->SetNewTarget(newTgt);
    return ecmaRuntimeCallInfo;
}

static JSTaggedType *SetupFrame(JSThread *thread, EcmaRuntimeCallInfo *info)
{
    JSTaggedType *sp = const_cast<JSTaggedType *>(thread->GetCurrentSPFrame());
    size_t frameSize = 0;
    const int num = 2;
    // 2 means thread and numArgs
    if (thread->IsAsmInterpreter()) {
        frameSize = InterpretedEntryFrame::NumOfMembers() + info->GetArgsNumber() + NUM_MANDATORY_JSFUNC_ARGS + num;
    } else {
        frameSize = InterpretedFrame::NumOfMembers() + info->GetArgsNumber() + NUM_MANDATORY_JSFUNC_ARGS + num;
    }
    JSTaggedType *newSp = sp - frameSize; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

    InterpretedEntryFrame *state = reinterpret_cast<InterpretedEntryFrame *>(newSp) - 1;
    state->base.type = ecmascript::FrameType::INTERPRETER_ENTRY_FRAME;
    state->base.prev = sp;
    state->pc = nullptr;
    thread->SetCurrentSPFrame(newSp);
    return sp;
}

void TearDownFrame(JSThread *thread, JSTaggedType *prev)
{
    thread->SetCurrentSPFrame(prev);
}

void JSValueRefIsPlainArrayFuzzTest([[maybe_unused]] const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    {
        JsiFastNativeScope scope(vm);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "Parameter out of range..";
            return;
        }
        auto thread = vm->GetAssociatedJSThread();
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
        JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
        JSHandle<JSTaggedValue> value =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();
        auto objCallInfo = CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(value.GetTaggedValue());
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(containers::ContainerTag::PlainArray)));
        [[maybe_unused]] auto prev = SetupFrame(thread, objCallInfo);
        JSTaggedValue result = containers::ContainersPrivate::Load(objCallInfo);
        TearDownFrame(thread, prev);
        JSHandle<JSTaggedValue> constructor(thread, result);
        JSHandle<JSAPIPlainArray> plainArray(
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        constexpr int nodeNumbers = 8;
        JSHandle<JSTaggedValue> keyArray = JSHandle<JSTaggedValue>(factory->NewTaggedArray(nodeNumbers));
        JSHandle<JSTaggedValue> valueArray = JSHandle<JSTaggedValue>(factory->NewTaggedArray(nodeNumbers));
        plainArray->SetKeys(thread, keyArray);
        plainArray->SetValues(thread, valueArray);
        JSHandle<JSTaggedValue> plainarraytag = JSHandle<JSTaggedValue>::Cast(plainArray);
        Local<JSValueRef> object = JSNApiHelper::ToLocal<JSValueRef>(plainarraytag);
        object->IsPlainArray(vm);
    }
    JSNApi::DestroyJSVM(vm);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::JSValueRefIsPlainArrayFuzzTest(data, size);
    return 0;
}