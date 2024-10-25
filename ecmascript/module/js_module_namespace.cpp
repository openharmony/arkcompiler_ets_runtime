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

#include "ecmascript/module/js_module_namespace.h"

#include "ecmascript/global_env.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/module/module_manager_helper.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/module/js_module_deregister.h"
#include "ecmascript/module/js_shared_module_manager.h"
#include "ecmascript/shared_objects/js_shared_array.h"

namespace panda::ecmascript {
JSHandle<JSTaggedValue> ModuleNamespace::CreateSortedExports(JSThread *thread, const JSHandle<TaggedArray> &exports)
{
    auto globalConst = thread->GlobalConstants();
    // 7. Let sortedExports be a new List containing the same values as the list exports where the values
    // are ordered as if an Array of the same values had been sorted using
    // Array.prototype.sort using undefined as comparefn.
    JSHandle<JSArray> exportsArray = JSArray::CreateArrayFromList(thread, exports);
    JSHandle<JSTaggedValue> sortedExports = JSHandle<JSTaggedValue>::Cast(exportsArray);
    JSHandle<JSTaggedValue> fn = globalConst->GetHandledUndefined();
    JSArray::Sort(thread, sortedExports, fn);
    return sortedExports;
}

JSHandle<ModuleNamespace> ModuleNamespace::ModuleNamespaceCreate(JSThread *thread,
                                                                 const JSHandle<JSTaggedValue> &module,
                                                                 const JSHandle<TaggedArray> &exports)
{
    auto globalConst = thread->GlobalConstants();
    // 1. Assert: module is a Module Record.
    ASSERT(module->IsModuleRecord());
    if (SourceTextModule::IsSharedModule(JSHandle<SourceTextModule>::Cast(module))) {
        return SharedModuleManager::GetInstance()->SModuleNamespaceCreate(thread, module, exports);
    }

    // 2. Assert: module.[[Namespace]] is undefined.
    // 3. Assert: exports is a List of String values.
    // 4. Let M be a newly created object.
    // 5. Set M's essential internal methods to the definitions specified in 9.4.6.
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<ModuleNamespace> mNp = factory->NewModuleNamespace();
    // 6. Set M.[[Module]] to module.
    mNp->SetModule(thread, module);

    JSHandle<JSTaggedValue> sortedExports = CreateSortedExports(thread, exports);
    // 8. Set M.[[Exports]] to sortedExports.
    mNp->SetExports(thread, sortedExports);
    // 9. Create own properties of M corresponding to the definitions in 26.3.

    JSHandle<JSTaggedValue> toStringTag = thread->GetEcmaVM()->GetGlobalEnv()->GetToStringTagSymbol();
    JSHandle<JSTaggedValue> moduleString = globalConst->GetHandledModuleString();
    PropertyDescriptor des(thread, moduleString, false, false, false);
    JSHandle<JSObject> mNpObj = JSHandle<JSObject>::Cast(mNp);
    JSObject::DefineOwnProperty(thread, mNpObj, toStringTag, des);
    // 10. Set module.[[Namespace]] to M.
    SetModuleDeregisterProcession(thread, mNp, ModuleDeregister::FreeModuleRecord);
    JSHandle<ModuleRecord> moduleRecord = JSHandle<ModuleRecord>::Cast(module);
    SourceTextModule::Cast(moduleRecord.GetTaggedValue().GetTaggedObject())->SetNamespace(thread,
                                                                                          mNp.GetTaggedValue());
    mNp->GetJSHClass()->SetExtensible(false);
    return mNp;
}

OperationResult ModuleNamespace::GetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                             const JSHandle<JSTaggedValue> &key)
{
    // 1. Assert: IsPropertyKey(P) is true.
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
    // 2. If Type(P) is Symbol, then
    //   a. Return ? OrdinaryGet(O, P, Receiver).
    if (key->IsSymbol()) {
        return JSObject::GetProperty(thread, obj, key);
    }
    JSHandle<ModuleNamespace> moduleNamespace = JSHandle<ModuleNamespace>::Cast(obj);
    // 3. Let exports be O.[[Exports]].
    JSHandle<JSTaggedValue> exports(thread, moduleNamespace->GetExports());
    // 4. If P is not an element of exports, return undefined.
    if (exports->IsUndefined()) {
        return OperationResult(thread, thread->GlobalConstants()->GetUndefined(), PropertyMetaData(false));
    }
    if (exports->IsJSArray()) {
        if (!JSArray::IncludeInSortedValue(thread, exports, key))
        return OperationResult(thread, thread->GlobalConstants()->GetUndefined(), PropertyMetaData(false));
    } else if (exports->IsJSSharedArray() && !JSSharedArray::IncludeInSortedValue(thread, exports, key)) {
        return OperationResult(thread, thread->GlobalConstants()->GetUndefined(), PropertyMetaData(false));
    }
    // 5. Let m be O.[[Module]].
    JSHandle<SourceTextModule> mm(thread, moduleNamespace->GetModule());
    // 6. Let binding be ! m.ResolveExport(P, « »).
    CVector<std::pair<JSHandle<SourceTextModule>, JSHandle<JSTaggedValue>>> resolveSet;
    JSHandle<JSTaggedValue> binding = SourceTextModule::ResolveExport(thread, mm, key, resolveSet);
    // 7. Assert: binding is a ResolvedBinding Record.
    // If resolution is null or "ambiguous", throw a SyntaxError exception.
    if (binding->IsNull() || binding->IsString()) {
        CString requestMod = ModulePathHelper::ReformatPath(mm->GetEcmaModuleFilenameString());
        LOG_FULL(FATAL) << "Module: '" << requestMod << SourceTextModule::GetResolveErrorReason(binding) <<
            ConvertToString(key.GetTaggedValue()) << ".";
    }
    JSTaggedValue result;
    // 8. Let targetModule be binding.[[Module]].
    JSType type = binding->GetTaggedObject()->GetClass()->GetObjectType();
    switch (type) {
        case JSType::RESOLVEDBINDING_RECORD: {
            JSHandle<ResolvedBinding> resolvedBind = JSHandle<ResolvedBinding>::Cast(binding);
            JSTaggedValue targetModule = resolvedBind->GetModule();
            // 9. Assert: targetModule is not undefined.
            ASSERT(!targetModule.IsUndefined());
            JSHandle<SourceTextModule> module(thread, targetModule);
            // DFX: make sure lazy module is already evaluated.
            if (module->GetStatus() == ModuleStatus::INSTANTIATED) {
                LOG_FULL(ERROR) << "Module is not evaluated, module is :" << module->GetEcmaModuleRecordNameString();
            }
            ModuleTypes moduleType = module->GetTypes();
            if (UNLIKELY(SourceTextModule::IsNativeModule(moduleType))) {
                result = ModuleManagerHelper::GetModuleValue(thread, module, resolvedBind->GetBindingName());
                RETURN_VALUE_IF_ABRUPT_COMPLETION(
                    thread, OperationResult(thread, JSTaggedValue::Exception(), PropertyMetaData(false)));
            } else {
                result = module->GetModuleValue(thread, resolvedBind->GetBindingName(), true);
            }
            break;
        }
        case JSType::RESOLVEDINDEXBINDING_RECORD: {
            JSHandle<ResolvedIndexBinding> resolvedBind = JSHandle<ResolvedIndexBinding>::Cast(binding);
            JSTaggedValue targetModule = resolvedBind->GetModule();
            // 9. Assert: targetModule is not undefined.
            ASSERT(!targetModule.IsUndefined());
            JSHandle<SourceTextModule> module(thread, targetModule);
            // DFX: make sure lazy module is already evaluated.
            if (module->GetStatus() == ModuleStatus::INSTANTIATED) {
                LOG_FULL(ERROR) << "Module is not evaluated, module is :" << module->GetEcmaModuleRecordNameString();
            }
            ModuleTypes moduleType = module->GetTypes();
            if (UNLIKELY(SourceTextModule::IsNativeModule(moduleType))) {
                result = ModuleManagerHelper::GetNativeOrCjsModuleValue(
                    thread, targetModule, resolvedBind->GetIndex());
                RETURN_VALUE_IF_ABRUPT_COMPLETION(
                    thread, OperationResult(thread, JSTaggedValue::Exception(), PropertyMetaData(false)));
            } else {
                result = module->GetModuleValue(thread, resolvedBind->GetIndex(), true);
            }
            break;
        }
        case JSType::RESOLVEDRECORDINDEXBINDING_RECORD:
            LOG_FULL(INFO) << "RESOLVEDRECORDINDEXBINDING_RECORD";
            break;
        case JSType::RESOLVEDRECORDBINDING_RECORD:
            LOG_FULL(INFO) << "RESOLVEDRECORDINDEXBINDING_RECORD";
            break;
        default:
            LOG_FULL(FATAL) << "UNREACHABLE";
    }
    return OperationResult(thread, result, PropertyMetaData(true));
}

JSHandle<TaggedArray> ModuleNamespace::OwnPropertyKeys(JSThread *thread, const JSHandle<JSTaggedValue> &obj)
{
    ASSERT(obj->IsModuleNamespace());
    // 1. Let exports be a copy of O.[[Exports]].
    JSHandle<ModuleNamespace> moduleNamespace = JSHandle<ModuleNamespace>::Cast(obj);
    JSHandle<JSTaggedValue> exports(thread, moduleNamespace->GetExports());
    JSHandle<TaggedArray> exportsArray = JSArray::ToTaggedArray(thread, exports);
    if (!moduleNamespace->ValidateKeysAvailable(thread, exportsArray)) {
        return exportsArray;
    }

    // 2. Let symbolKeys be ! OrdinaryOwnPropertyKeys(O).
    JSHandle<TaggedArray> symbolKeys = JSObject::GetOwnPropertyKeys(thread, JSHandle<JSObject>(obj));
    // 3. Append all the entries of symbolKeys to the end of exports.
    JSHandle<TaggedArray> result = TaggedArray::Append(thread, exportsArray, symbolKeys);
    // 4. Return exports.
    return result;
}

JSHandle<TaggedArray> ModuleNamespace::OwnEnumPropertyKeys(JSThread *thread, const JSHandle<JSTaggedValue> &obj)
{
    ASSERT(obj->IsModuleNamespace());
    // 1. Let exports be a copy of O.[[Exports]].
    JSHandle<ModuleNamespace> moduleNamespace = JSHandle<ModuleNamespace>::Cast(obj);
    JSHandle<JSTaggedValue> exports(thread, moduleNamespace->GetExports());
    JSHandle<TaggedArray> exportsArray = JSArray::ToTaggedArray(thread, exports);
    if (!moduleNamespace->ValidateKeysAvailable(thread, exportsArray)) {
        return exportsArray;
    }

    // 2. Let symbolKeys be ! OrdinaryOwnPropertyKeys(O).
    JSHandle<TaggedArray> symbolKeys = JSObject::GetOwnEnumPropertyKeys(thread, JSHandle<JSObject>(obj));
    // 3. Append all the entries of symbolKeys to the end of exports.
    JSHandle<TaggedArray> result = TaggedArray::Append(thread, exportsArray, symbolKeys);
    // 4. Return exports.
    return result;
}

bool ModuleNamespace::PreventExtensions()
{
    return true;
}

bool ModuleNamespace::DefineOwnProperty(JSThread *thread,
                                        const JSHandle<JSTaggedValue> &obj,
                                        const JSHandle<JSTaggedValue> &key,
                                        PropertyDescriptor desc)
{
    ASSERT(obj->IsModuleNamespace());
    // 1. If Type(P) is Symbol, return ! OrdinaryDefineOwnProperty(O, P, Desc).
    if (key->IsSymbol()) {
        bool res = JSObject::OrdinaryDefineOwnProperty(thread, JSHandle<JSObject>(obj), key, desc);
        return res;
    }

    // 2. Let current be ? O.[[GetOwnProperty]](P).
    PropertyDescriptor current(thread);
    // 3. If current is undefined, return false.
    if (!GetOwnProperty(thread, obj, key, current)) {
        return false;
    }
    // 4. If Desc has a [[Configurable]] field and Desc.[[Configurable]] is true, return false.
    // 5. If Desc has an [[Enumerable]] field and Desc.[[Enumerable]] is false, return false.
    // 6. If IsAccessorDescriptor(Desc) is true, return false.
    // 7. If Desc has a [[Writable]] field and Desc.[[Writable]] is false, return false.
    if (desc.IsAccessorDescriptor()) {
        return false;
    }
    if (desc.HasConfigurable() && desc.IsConfigurable()) {
        return false;
    }
    if (desc.HasEnumerable() && !desc.IsEnumerable()) {
        return false;
    }
    if (desc.HasWritable() && !desc.IsWritable()) {
        return false;
    }

    // 8. If Desc has a [[Value]] field, return SameValue(Desc.[[Value]], current.[[Value]]).
    if (desc.HasValue()) {
        JSHandle<JSTaggedValue> descValue = desc.GetValue();
        JSHandle<JSTaggedValue> currentValue = current.GetValue();
        return JSTaggedValue::SameValue(descValue, currentValue);
    }

    // 9. Return true.
    return true;
}

bool ModuleNamespace::HasProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                  const JSHandle<JSTaggedValue> &key)
{
    ASSERT(obj->IsModuleNamespace());
    // 1. If Type(P) is Symbol, return OrdinaryHasProperty(O, P).
    if (key->IsSymbol()) {
        return JSObject::HasProperty(thread, JSHandle<JSObject>(obj), key);
    }
    // 2. Let exports be O.[[Exports]].
    JSHandle<ModuleNamespace> moduleNamespace = JSHandle<ModuleNamespace>::Cast(obj);
    JSHandle<JSTaggedValue> exports(thread, moduleNamespace->GetExports());
    // 3. If P is an element of exports, return true.
    if (exports->IsUndefined()) {
        return false;
    }
    if (JSArray::IncludeInSortedValue(thread, exports, key)) {
        return true;
    }
    // 4. Return false.
    return false;
}

bool ModuleNamespace::SetPrototype([[maybe_unused]] const JSHandle<JSTaggedValue> &obj,
                                   const JSHandle<JSTaggedValue> &proto)
{
    ASSERT(obj->IsModuleNamespace());
    // 1. Assert: Either Type(V) is Object or Type(V) is Null.
    ASSERT(proto->IsECMAObject() || proto->IsNull());
    return proto->IsNull();
}

bool ModuleNamespace::GetOwnProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                     const JSHandle<JSTaggedValue> &key, PropertyDescriptor &desc)
{
    // 1. If Type(P) is Symbol, return OrdinaryGetOwnProperty(O, P).
    if (key->IsSymbol()) {
        return JSObject::GetOwnProperty(thread, JSHandle<JSObject>(obj), key, desc);
    }
    // 2. Let exports be O.[[Exports]].
    JSHandle<ModuleNamespace> moduleNamespace = JSHandle<ModuleNamespace>::Cast(obj);
    JSHandle<JSTaggedValue> exports(thread, moduleNamespace->GetExports());
    // 3. If P is not an element of exports, return undefined.
    if (exports->IsUndefined()) {
        return false;
    }
    if (!JSArray::IncludeInSortedValue(thread, exports, key)) {
        return false;
    }
    // 4. Let value be ? O.[[Get]](P, O).
    JSHandle<JSTaggedValue> value = ModuleNamespace::GetProperty(thread, obj, key).GetValue();
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    // 5. Return PropertyDescriptor {
    //    [[Value]]: value, [[Writable]]: true, [[Enumerable]]: true, [[Configurable]]: false }.
    desc.SetValue(value);
    desc.SetEnumerable(true);
    desc.SetWritable(true);
    desc.SetConfigurable(false);
    return true;
}

bool ModuleNamespace::SetProperty(JSThread *thread, bool mayThrow)
{
    if (mayThrow) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot assign to read only property of Object Module", false);
    }
    return false;
}

bool ModuleNamespace::DeleteProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                     const JSHandle<JSTaggedValue> &key)
{
    ASSERT(obj->IsModuleNamespace());
    // 1. Assert: IsPropertyKey(P) is true.
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
    // 2. If Type(P) is Symbol, then
    //    Return ? OrdinaryDelete(O, P).
    if (key->IsSymbol()) {
        return JSObject::DeleteProperty(thread, JSHandle<JSObject>(obj), key);
    }
    // 3. Let exports be O.[[Exports]].
    JSHandle<ModuleNamespace> moduleNamespace = JSHandle<ModuleNamespace>::Cast(obj);
    JSHandle<JSTaggedValue> exports(thread, moduleNamespace->GetExports());
    // 4. If P is an element of exports, return false.
    if (exports->IsUndefined()) {
        return true;
    }
    if (JSArray::IncludeInSortedValue(thread, exports, key)) {
        return false;
    }
    return true;
}

bool ModuleNamespace::ValidateKeysAvailable(JSThread *thread, const JSHandle<TaggedArray> &exports)
{
    JSHandle<ModuleNamespace> moduleNamespace(thread, this);
    JSHandle<SourceTextModule> mm(thread, moduleNamespace->GetModule());
    uint32_t exportsLength = exports->GetLength();
    for (uint32_t idx = 0; idx < exportsLength; idx++) {
        JSHandle<JSTaggedValue> key(thread, exports->Get(idx));
        CVector<std::pair<JSHandle<SourceTextModule>, JSHandle<JSTaggedValue>>> resolveSet;
        JSHandle<JSTaggedValue> binding = SourceTextModule::ResolveExport(thread, mm, key, resolveSet);
        // Adapter new module
        ASSERT(binding->IsResolvedBinding() || binding->IsResolvedIndexBinding());
        JSTaggedValue targetModule = JSTaggedValue::Undefined();
        if (binding->IsResolvedBinding()) {
            targetModule = JSHandle<ResolvedBinding>::Cast(binding)->GetModule();
        } else if (binding->IsResolvedIndexBinding()) {
            targetModule = JSHandle<ResolvedIndexBinding>::Cast(binding)->GetModule();
        }
        ASSERT(!targetModule.IsUndefined());
        JSTaggedValue dictionary = SourceTextModule::Cast(targetModule.GetTaggedObject())->GetNameDictionary();
        if (dictionary.IsUndefined()) {
            THROW_REFERENCE_ERROR_AND_RETURN(thread, "module environment is undefined", false);
        }
    }
    return true;
}

void ModuleNamespace::SetModuleDeregisterProcession(JSThread *thread, const JSHandle<ModuleNamespace> &nameSpace,
    const NativePointerCallback &callback)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<SourceTextModule> module(thread, nameSpace->GetModule());
    CString moduleStr = SourceTextModule::GetModuleName(module.GetTaggedValue());
    int srcLength = strlen(moduleStr.c_str()) + 1;
    auto moduleNameData = thread->GetEcmaVM()->GetNativeAreaAllocator()->AllocateBuffer(srcLength);
    if (memcpy_s(moduleNameData, srcLength, moduleStr.c_str(), srcLength) != EOK) { // LCOV_EXCL_BR_LINE
        LOG_ECMA(FATAL) << "Failed to copy module name's data.";
        UNREACHABLE();
    }
    char *tmpData = reinterpret_cast<char *>(moduleNameData);
    tmpData[srcLength - 1] = '\0';
    ASSERT(nameSpace->GetDeregisterProcession().IsUndefined());
    JSHandle<JSNativePointer> registerPointer = factory->NewJSNativePointer(
        reinterpret_cast<void *>(moduleNameData), callback, reinterpret_cast<void *>(thread), false, srcLength);
    nameSpace->SetDeregisterProcession(thread, registerPointer.GetTaggedValue());
}
}  // namespace panda::ecmascript
