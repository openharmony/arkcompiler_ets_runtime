/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/jspandafile/program_object.h"

namespace panda::ecmascript {
JSHandle<JSTaggedValue> ConstantPool::GetDeserializedConstantPool(EcmaVM *vm, const JSPandaFile *jsPandaFile,
                                                                  int32_t cpID)
{
    auto aotFileManager = vm->GetAOTFileManager();
    auto constantPool = aotFileManager->GetDeserializedConstantPool(jsPandaFile, cpID);
    MergeObjectLiteralHClassCache(vm, constantPool);
    return constantPool;
}

void ConstantPool::MergeObjectLiteralHClassCache(EcmaVM *vm, const JSHandle<JSTaggedValue> &constpool)
{
    if (constpool->IsHole()) {
        return;
    }
    JSHandle<ConstantPool> pool(constpool);
    auto aotHCInfo = pool->GetAotHClassInfo();
    if (!aotHCInfo.IsTaggedArray()) {
        return;
    }
    auto aotHCInfoArray = TaggedArray::Cast(aotHCInfo);
    auto last = aotHCInfoArray->Get(aotHCInfoArray->GetLength() - 1);
    if (!last.IsTaggedArray()) {
        return;
    }
    auto snapshotCachedArray = TaggedArray::Cast(last);
    auto curCached = vm->GetGlobalEnv()->GetObjectLiteralHClassCache();
    auto length = snapshotCachedArray->GetLength();
    auto prototype = vm->GetGlobalEnv()->GetObjectFunctionPrototype();
    JSThread *thread = vm->GetJSThread();
    if (curCached->IsHole()) {
        for (uint32_t i = 0; i < length; i++) {
            auto newValue = snapshotCachedArray->Get(i);
            if (newValue.IsJSHClass()) {
                JSHClass::Cast(newValue.GetTaggedObject())->SetPrototype(thread, prototype);
            }
        }
        vm->GetGlobalEnv()->SetObjectLiteralHClassCache(thread, last);
        return;
    }
    auto curCachedArray = TaggedArray::Cast(curCached.GetTaggedValue());
    for (uint32_t i = 0; i < length; i++) {
        auto newValue = snapshotCachedArray->Get(i);
        if (newValue.IsHole()) {
            continue;
        }
        auto curValue = curCachedArray->Get(i);
        // If already merged, stop to merge.
        if (curValue.IsJSHClass() && JSHClass::Cast(curValue.GetTaggedObject())->IsTS()) {
            break;
        }
        JSHClass::Cast(newValue.GetTaggedObject())->SetPrototype(thread, prototype);
        curCachedArray->Set(thread, i, newValue);
    }
}

JSTaggedValue ConstantPool::GetMethodFromCache(JSTaggedValue constpool, uint32_t index)
{
    const ConstantPool *taggedPool = ConstantPool::Cast(constpool.GetTaggedObject());
    auto val = taggedPool->GetObjectFromCache(index);
    JSPandaFile *jsPandaFile = taggedPool->GetJSPandaFile();

    if (IsLoadedMethodInfoFromAOT(jsPandaFile, val)) {
        val = JSTaggedValue::Hole();
    }

    return val.IsHole() ? JSTaggedValue::Undefined() : val;
}

bool ConstantPool::IsAotMethodLiteralInfo(JSTaggedValue literalInfo)
{
    return literalInfo.IsAOTLiteralInfo() && (AOTLiteralInfo::Cast(literalInfo.GetTaggedObject())->
        GetLiteralType() == AOTLiteralInfo::METHOD_LITERAL_TYPE);
}

void ConstantPool::UpdateConstpoolWhenDeserialAI(EcmaVM *vm, JSHandle<ConstantPool> aiCP,
                                                 JSHandle<ConstantPool> sharedCP, JSHandle<ConstantPool> unsharedCP)
{
    uint32_t constpoolLen = aiCP->GetCacheLength();
    auto aiCPLength = aiCP->GetLength();
    JSMutableHandle<JSTaggedValue> valHandle(vm->GetJSThread(), JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < constpoolLen; i++) {
        // We need preserve unshared constantPool index and shared constantPool id instead of fetching from ai.
        // Because framework abc's ai does not contain those infos.
        if (i == (aiCPLength - ConstantPool::UNSHARED_CONSTPOOL_INDEX) ||
            i == (aiCPLength - ConstantPool::SHARED_CONSTPOOL_ID)) {
            continue;
        }
        JSThread *thread = vm->GetJSThread();
        JSTaggedValue val = aiCP->GetObjectFromCache(i);
        valHandle.Update(val);
        if (IsAotMethodLiteralInfo(valHandle.GetTaggedValue())) {
            JSHandle<AOTLiteralInfo> value(thread, val);
            JSHandle<AOTLiteralInfo> methodLiteral = CopySharedMethodAOTLiteralInfo(vm, value);
            sharedCP->SetObjectToCache(thread, i, methodLiteral.GetTaggedValue());
        } else if (valHandle->IsInt()) {
            // For MethodInfo which does not have ihc infos, we store codeEntry directly.
            sharedCP->SetObjectToCache(thread, i, valHandle.GetTaggedValue());
            unsharedCP->SetObjectToCache(thread, i, valHandle.GetTaggedValue());
        }
        // update method, class and object aotliteralinfo
        if (valHandle->IsAOTLiteralInfo()) {
            unsharedCP->SetObjectToCache(thread, i, valHandle.GetTaggedValue());
        }
    }
    unsharedCP->InitConstantPoolTail(vm->GetJSThread(), aiCP);
}
}
