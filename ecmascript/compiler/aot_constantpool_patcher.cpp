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

#include "ecmascript/compiler/aot_constantpool_patcher.h"

namespace panda::ecmascript {
void AotConstantpoolPatcher::SetPrototypeForTransitions(JSThread *thread, JSHClass *hclass, JSTaggedValue proto)
{
    std::queue<JSHClass *> backHClass;
    backHClass.push(hclass);
    while (!backHClass.empty()) {
        JSHClass *current = backHClass.front();
        backHClass.pop();
        hclass->SetProto(thread, proto);

        auto transitions = current->GetTransitions(thread);
        if (transitions.IsUndefined()) {
            continue;
        }
        if (transitions.IsWeak()) {
            auto cache = transitions.GetTaggedWeakRef();
            backHClass.push(JSHClass::Cast(cache));
            continue;
        }
        ASSERT(transitions.IsTaggedArray());
        TransitionsDictionary *dict = TransitionsDictionary::Cast(transitions.GetTaggedObject());
        dict->IterateEntryValue(thread, [&backHClass] (JSHClass *cache) {
            backHClass.push(JSHClass::Cast(cache));
        });
    }
}

void AotConstantpoolPatcher::SetObjectFunctionFromConstPool(JSThread *thread, JSHandle<ConstantPool> newConstPool)
{
    auto aotHCInfo = newConstPool->GetAotHClassInfo(thread);
    if (!aotHCInfo.IsTaggedArray()) {
        return;
    }
    auto aotHCInfoArray = TaggedArray::Cast(aotHCInfo);
    if (aotHCInfoArray->GetLength() <= 0) {
        return;
    }
    JSTaggedValue ihc = aotHCInfoArray->Get(thread, 0);
    if (!ihc.IsJSHClass()) {
        return;
    }
    JSHandle<JSHClass> objectFunctionIHC(thread, ihc);
    if (!objectFunctionIHC->IsOnlyJSObject()) {
        LOG_ECMA(FATAL) << "Napi Object must be JSObject.";
        UNREACHABLE();
        return;
    }
    // not be set before
    auto env = thread->GetGlobalEnv();
    if (env->GetObjectFunctionTsNapiClass() == env->GetObjectFunctionNapiClass()) {
        JSHandle<JSHClass> objectFunctionNapiClass(env->GetObjectFunctionNapiClass());
        objectFunctionIHC->SetPrototype(thread, objectFunctionNapiClass->GetProto(thread));
        JSHClass::EnableProtoChangeMarker(thread, objectFunctionIHC);
        SetPrototypeForTransitions(thread, *objectFunctionIHC, objectFunctionNapiClass->GetProto(thread));
        env->SetObjectFunctionTsNapiClass(thread, objectFunctionIHC.GetTaggedValue());
    }
}
}  // namespace panda::ecmascript::kungfu
