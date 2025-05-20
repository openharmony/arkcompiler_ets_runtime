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

#include "ecmascript/global_dictionary.h"
#include "ecmascript/symbol_table.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/template_map.h"

namespace panda::ecmascript {
void GlobalEnv::Init(JSThread *thread)
{
    SetRegisterSymbols(thread, SymbolTable::Create(thread));
    SetGlobalRecord(thread, GlobalDictionary::Create(thread));
    JSTaggedValue emptyStr = thread->GlobalConstants()->GetEmptyString();
    EcmaStringTable *stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    stringTable->GetOrInternFlattenString(thread->GetEcmaVM(), EcmaString::Cast(emptyStr.GetTaggedObject()));
    SetTemplateMap(thread, TemplateMap::Create(thread));
    SetObjectLiteralHClassCache(thread, JSTaggedValue::Hole());
    SetJSThread(thread);
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
}  // namespace panda::ecmascript
