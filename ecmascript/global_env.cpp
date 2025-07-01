/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/global_env.h"

#include "ecmascript/builtins/builtins_number.h"
#include "ecmascript/builtins/builtins_regexp.h"
#include "ecmascript/builtins/builtins_string.h"
#include "ecmascript/dependent_infos.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/global_dictionary.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/template_map.h"

namespace panda::ecmascript {
void GlobalEnv::Init(JSThread *thread)
{
    SetGlobalEnv(thread, JSTaggedValue(this));
    SetGlobalRecord(thread, GlobalDictionary::Create(thread));
    auto* vm = thread->GetEcmaVM();
    JSTaggedValue emptyStr = thread->GlobalConstants()->GetEmptyString();
    EcmaStringTable *stringTable = vm->GetEcmaStringTable();
    stringTable->GetOrInternFlattenString(vm, EcmaString::Cast(emptyStr.GetTaggedObject()));
    SetTemplateMap(thread, TemplateMap::Create(thread));
    SetObjectLiteralHClassCache(thread, JSTaggedValue::Hole());
    SetArrayJoinStack(thread, vm->GetFactory()->NewTaggedArray(ArrayJoinStack::MIN_JOIN_STACK_SIZE));
    SetNumberToStringResultCache(thread, builtins::NumberToStringResultCache::CreateCacheTable(thread));
    SetStringSplitResultCache(thread, builtins::StringSplitResultCache::CreateCacheTable(thread));
    SetStringToListResultCache(thread, builtins::StringToListResultCache::CreateCacheTable(thread));
    SetRegExpCache(thread, builtins::RegExpExecResultCache::CreateCacheTable(thread));
    SetRegExpGlobalResult(thread, builtins::RegExpGlobalResult::CreateGlobalResultTable(thread));
#define INIT_JSAPI_CONTAINER(Type, Name, INDEX) Set##Name(thread, JSTaggedValue::Undefined());
    GLOBAL_ENV_CONTAINER_ITERATORS(INIT_JSAPI_CONTAINER)
#undef INIT_JSAPI_CONTAINER
    SetModuleManagerNativePointer(thread, ModuleManager::CreateModuleManagerNativePointer(thread));
    ClearBitField();
    SetJSThread(thread);

    auto array = vm->GetFactory()->NewTaggedArray(LastBitFieldBits::START_BIT, JSTaggedValue::Undefined());
    SetDetectorDependentInfos(thread, array);
}

void GlobalEnv::Iterate(RootVisitor &v)
{
    for (uint16_t i = 0; i < FINAL_INDEX; i++) {
        size_t offset = HEADER_SIZE + i * sizeof(JSTaggedType);
        uintptr_t slotAddress = reinterpret_cast<uintptr_t>(this) + offset;
        ObjectSlot slot(slotAddress);
        v.VisitRoot(Root::ROOT_VM, slot);
    }
}

JSHandle<JSTaggedValue> GlobalEnv::GetSymbol(JSThread *thread, const JSHandle<JSTaggedValue> &string)
{
    JSHandle<JSTaggedValue> symbolFunction(GetSymbolFunction());
    return JSObject::GetProperty(thread, symbolFunction, string).GetValue();
}

JSHandle<JSTaggedValue> GlobalEnv::GetStringPrototypeFunctionByName(JSThread *thread, const char *name)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> stringFuncPrototype(
        thread, JSObject::GetPrototype(thread, JSHandle<JSObject>(GetStringFunction())));
    JSHandle<JSTaggedValue> nameKey(factory->NewFromUtf8(name));
    return JSObject::GetProperty(thread, stringFuncPrototype, nameKey).GetValue();
}

JSHandle<JSTaggedValue> GlobalEnv::GetStringFunctionByName(JSThread *thread, const char *name)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> stringFuncObj = GetStringFunction();
    JSHandle<JSTaggedValue> nameKey(factory->NewFromUtf8(name));
    return JSObject::GetProperty(thread, stringFuncObj, nameKey).GetValue();
}

void GlobalEnv::NotifyDetectorDeoptimize(JSThread *thread, uint32_t detectorID)
{
    JSHandle<JSTaggedValue> dependentInfos = GetDependentInfos(thread, detectorID);
    if (!dependentInfos->IsHeapObject()) {
        return;
    }
    JSThread *initThread = GetJSThread();
    DependentInfos::TriggerLazyDeoptimization(
        JSHandle<DependentInfos>::Cast(dependentInfos), initThread,
        DependentInfos::DependentState::DETECTOR_CHECK);
    SetDependentInfos(detectorID, initThread->GlobalConstants()->GetHandledUndefined());
}

void GlobalEnv::NotifyArrayPrototypeChangedGuardians(JSThread *thread, JSHandle<JSObject> receiver)
{
    if (!GetArrayPrototypeChangedGuardians()) {
        return;
    }
    if (!receiver->GetJSHClass()->IsPrototype() && !receiver->IsJSArray()) {
        return;
    }
    if (receiver.GetTaggedValue() == GetObjectFunctionPrototype().GetTaggedValue() ||
        receiver.GetTaggedValue() == GetArrayPrototype().GetTaggedValue()) {
        NotifyDetectorDeoptimize(thread, ArrayPrototypeChangedGuardiansBits::START_BIT);
        SetArrayPrototypeChangedGuardians(false);
        return;
    }
}

void GlobalEnv::ClearCache(JSThread *thread) const
{
    builtins::StringSplitResultCache::ClearCache(thread, GetStringSplitResultCache());
}

GlobalEnvField GetBuildinTypedArrayHClassOnHeapIndex(JSType jSType)
{
    switch (jSType) {
        case JSType::JS_INT8_ARRAY:
            return GlobalEnvField::INT8_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX;
        case JSType::JS_UINT8_ARRAY:
            return GlobalEnvField::UINT8_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX;
        case JSType::JS_UINT8_CLAMPED_ARRAY:
            return GlobalEnvField::UINT8_CLAMPED_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX;
        case JSType::JS_INT16_ARRAY:
            return GlobalEnvField::INT16_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX;
        case JSType::JS_UINT16_ARRAY:
            return GlobalEnvField::UINT16_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX;
        case JSType::JS_INT32_ARRAY:
            return GlobalEnvField::INT32_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX;
        case JSType::JS_UINT32_ARRAY:
            return GlobalEnvField::UINT32_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX;
        case JSType::JS_FLOAT32_ARRAY:
            return GlobalEnvField::FLOAT32_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX;
        case JSType::JS_FLOAT64_ARRAY:
            return GlobalEnvField::FLOAT64_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX;
        case JSType::JS_BIGINT64_ARRAY:
            return GlobalEnvField::BIGINT64_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX;
        case JSType::JS_BIGUINT64_ARRAY:
            return GlobalEnvField::BIGUINT64_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX;
        default:
            return GlobalEnvField::INVALID;
    }
    return GlobalEnvField::INVALID;
}

GlobalEnvField GetBuildinTypedArrayHClassIndex(JSType jSType)
{
    switch (jSType) {
        case JSType::JS_INT8_ARRAY:
            return GlobalEnvField::INT8_ARRAY_ROOT_HCLASS_INDEX;
        case JSType::JS_UINT8_ARRAY:
            return GlobalEnvField::UINT8_ARRAY_ROOT_HCLASS_INDEX;
        case JSType::JS_UINT8_CLAMPED_ARRAY:
            return GlobalEnvField::UINT8_CLAMPED_ARRAY_ROOT_HCLASS_INDEX;
        case JSType::JS_INT16_ARRAY:
            return GlobalEnvField::INT16_ARRAY_ROOT_HCLASS_INDEX;
        case JSType::JS_UINT16_ARRAY:
            return GlobalEnvField::UINT16_ARRAY_ROOT_HCLASS_INDEX;
        case JSType::JS_INT32_ARRAY:
            return GlobalEnvField::INT32_ARRAY_ROOT_HCLASS_INDEX;
        case JSType::JS_UINT32_ARRAY:
            return GlobalEnvField::UINT32_ARRAY_ROOT_HCLASS_INDEX;
        case JSType::JS_FLOAT32_ARRAY:
            return GlobalEnvField::FLOAT32_ARRAY_ROOT_HCLASS_INDEX;
        case JSType::JS_FLOAT64_ARRAY:
            return GlobalEnvField::FLOAT64_ARRAY_ROOT_HCLASS_INDEX;
        case JSType::JS_BIGINT64_ARRAY:
            return GlobalEnvField::BIGINT64_ARRAY_ROOT_HCLASS_INDEX;
        case JSType::JS_BIGUINT64_ARRAY:
            return GlobalEnvField::BIGUINT64_ARRAY_ROOT_HCLASS_INDEX;
        default:
            return GlobalEnvField::INVALID;
    }
    return GlobalEnvField::INVALID;
}

GlobalEnvField GlobalEnv::GetBuildinTypedArrayHClassIndex(JSType jSType, OnHeapMode mode)
{
    if (OnHeap::IsOnHeap(mode)) {
        return GetBuildinTypedArrayHClassOnHeapIndex(jSType);
    } else if (OnHeap::IsNotOnHeap(mode)) {
        return ::panda::ecmascript::GetBuildinTypedArrayHClassIndex(jSType);
    } else {
        return GlobalEnvField::INVALID;
    }
}
}  // namespace panda::ecmascript
