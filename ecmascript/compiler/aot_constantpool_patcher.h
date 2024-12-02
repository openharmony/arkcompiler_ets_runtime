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
#ifndef ECMASCRIPT_COMPILER_AOT_CONSTANTPOOL_PATCHER_H
#define ECMASCRIPT_COMPILER_AOT_CONSTANTPOOL_PATCHER_H

#include "ecmascript/js_handle.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/transitions_dictionary.h"

namespace panda::ecmascript {
class ConstantPool;
class AotConstantpoolPatcher {
public:
    static void SetPrototypeForTransitions(JSThread *thread, JSHClass *hclass, JSTaggedValue proto)
    {
        std::queue<JSHClass *> backHClass;
        backHClass.push(hclass);
        while (!backHClass.empty()) {
            JSHClass *current = backHClass.front();
            backHClass.pop();
            hclass->SetProto(thread, proto);

            auto transitions = current->GetTransitions();
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
            dict->IterateEntryValue([&backHClass] (JSHClass *cache) {
                backHClass.push(JSHClass::Cast(cache));
            });
        }
    }

    static void SetObjectFunctionFromConstPool(JSThread *thread, JSHandle<ConstantPool> newConstPool)
    {
        auto aotHCInfo = newConstPool->GetAotHClassInfo();
        if (!aotHCInfo.IsTaggedArray()) {
            return;
        }
        auto aotHCInfoArray = TaggedArray::Cast(aotHCInfo);
        if (aotHCInfoArray->GetLength() <= 0) {
            return;
        }
        JSTaggedValue ihc = aotHCInfoArray->Get(0);
        if (!ihc.IsJSHClass()) {
            return;
        }
        JSHandle<JSHClass> objectFunctionIHC(thread, ihc);
        if (!objectFunctionIHC->IsOnlyJSObject()) {
            LOG_ECMA(FATAL) << "Napi Object must be JSObject.";
            UNREACHABLE();
            return;
        }
        auto env = thread->GetCurrentEcmaContext()->GetGlobalEnv();
        JSHandle<JSHClass> objectFunctionNapiClass(env->GetObjectFunctionNapiClass());
        objectFunctionIHC->SetPrototype(thread, objectFunctionNapiClass->GetProto());
        JSHClass::EnableProtoChangeMarker(thread, objectFunctionIHC);
        SetPrototypeForTransitions(thread, *objectFunctionIHC, objectFunctionNapiClass->GetProto());
        env->SetObjectFunctionTsNapiClass(thread, objectFunctionIHC.GetTaggedValue());
    }
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_AOT_CONSTANTPOOL_PATCHER_H
