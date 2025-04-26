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
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/global_dictionary.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/symbol_table.h"
#include "ecmascript/template_map.h"

namespace panda::ecmascript {
void GlobalEnv::Init(JSThread *thread)
{
    SetRegisterSymbols(thread, SymbolTable::Create(thread));
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
    SetJSThread(thread);
}

JSHandle<JSTaggedValue> GlobalEnv::GetSymbol(JSThread *thread, const JSHandle<JSTaggedValue> &string)
{
    JSHandle<JSTaggedValue> symbolFunction(GetSymbolFunction());
    return JSObject::GetProperty(thread, symbolFunction, string).GetValue();
}

JSHandle<JSTaggedValue> GlobalEnv::GetStringPrototypeFunctionByName(JSThread *thread, const char *name)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> stringFuncPrototype(thread,
        JSObject::GetPrototype(JSHandle<JSObject>(GetStringFunction())));
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

void GlobalEnv::NotifyArrayPrototypeChangedGuardians(JSHandle<JSObject> receiver)
{
    if (!GetArrayPrototypeChangedGuardians()) {
        return;
    }
    if (!receiver->GetJSHClass()->IsPrototype() && !receiver->IsJSArray()) {
        return;
    }
    if (receiver.GetTaggedValue() == GetObjectFunctionPrototype().GetTaggedValue() ||
        receiver.GetTaggedValue() == GetArrayPrototype().GetTaggedValue()) {
        SetArrayPrototypeChangedGuardians(false);
        return;
    }
}

}  // namespace panda::ecmascript
