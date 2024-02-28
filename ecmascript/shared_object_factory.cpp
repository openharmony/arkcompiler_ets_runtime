/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "ecmascript/object_factory.h"

#include "ecmascript/accessor_data.h"
#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/ecma_context.h"
#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/js_function.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/layout_info-inl.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/symbol_table.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/module/js_shared_module.h"

namespace panda::ecmascript {
void ObjectFactory::NewSObjectHook() const
{
#ifndef NDEBUG
    static std::atomic<uint32_t> count = 0;
    static uint32_t frequency = vm_->GetJSOptions().GetForceSharedGCFrequency();
    if (frequency == 0 || !vm_->GetJSOptions().EnableForceGC() || !vm_->IsInitialized() ||
        !thread_->IsAllContextsInitialized()) {
        return;
    }
    if (count++ % frequency == 0) {
        sHeap_->CollectGarbage(thread_, TriggerGCType::SHARED_GC, GCReason::OTHER);
    }
#endif
}

JSHandle<JSHClass> ObjectFactory::CreateSFunctionClass(uint32_t size, JSType type,
                                                       const JSHandle<JSTaggedValue> &prototype, bool isAccessor)
{
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    uint32_t fieldOrder = 0;
    ASSERT(JSFunction::LENGTH_INLINE_PROPERTY_INDEX == fieldOrder);
    PropertyAttributes attributes = PropertyAttributes::Default(false, false, false);
    attributes.SetIsAccessor(isAccessor);
    attributes.SetIsInlinedProps(true);
    attributes.SetRepresentation(Representation::TAGGED);
    JSHandle<LayoutInfo> layoutInfoHandle = CreateSLayoutInfo(JSFunction::LENGTH_OF_INLINE_PROPERTIES);
    {
        attributes.SetOffset(fieldOrder);
        layoutInfoHandle->AddKey(thread_, fieldOrder, globalConst->GetLengthString(), attributes);
        fieldOrder++;
    }

    ASSERT(JSFunction::NAME_INLINE_PROPERTY_INDEX == fieldOrder);
    {
        attributes.SetOffset(fieldOrder);
        layoutInfoHandle->AddKey(thread_, fieldOrder,
                                 globalConst->GetHandledNameString().GetTaggedValue(), attributes);
        fieldOrder++;
    }
    JSHandle<JSHClass> functionClass = NewSEcmaHClass(size, fieldOrder, type, prototype,
        JSHandle<JSTaggedValue>(layoutInfoHandle));
    functionClass->SetCallable(true);
    return functionClass;
}

JSHandle<JSHClass> ObjectFactory::NewSEcmaHClass(uint32_t size, JSType type, uint32_t inlinedProps)
{
    return NewSEcmaHClass(JSHClass::Cast(thread_->GlobalConstants()->GetHClassClass().GetTaggedObject()),
                          size, type, inlinedProps);
}

JSHandle<JSHClass> ObjectFactory::NewSEcmaHClass(JSHClass *hclass, uint32_t size, JSType type, uint32_t inlinedProps)
{
    NewSObjectHook();
    uint32_t classSize = JSHClass::SIZE;
    auto *newClass = static_cast<JSHClass *>(sHeap_->AllocateNonMovableOrHugeObject(thread_, hclass, classSize));
    newClass->Initialize(thread_, size, type, inlinedProps, thread_->GlobalConstants()->GetHandledEmptySLayoutInfo());
    return JSHandle<JSHClass>(thread_, newClass);
}

// This function don't UpdateProtoClass
JSHandle<JSHClass> ObjectFactory::NewSEcmaHClass(uint32_t size, uint32_t inlinedProps, JSType type,
    const JSHandle<JSTaggedValue> &prototype, const JSHandle<JSTaggedValue> &layout)
{
    NewSObjectHook();
    uint32_t classSize = JSHClass::SIZE;
    auto *newClass = static_cast<JSHClass *>(sHeap_->AllocateNonMovableOrHugeObject(
        thread_, JSHClass::Cast(thread_->GlobalConstants()->GetHClassClass().GetTaggedObject()), classSize));
    newClass->Initialize(thread_, size, type, inlinedProps, layout);
    JSHandle<JSHClass> hclass(thread_, newClass);
    if (prototype->IsJSObject()) {
        prototype->GetTaggedObject()->GetClass()->SetIsPrototype(true);
    }
    hclass->SetProto(thread_, prototype.GetTaggedValue());
    hclass->SetExtensible(false);
    hclass->SetNumberOfProps(inlinedProps);
    return hclass;
}

JSHandle<JSHClass> ObjectFactory::NewSEcmaHClassClass(JSHClass *hclass, uint32_t size, JSType type)
{
    NewSObjectHook();
    uint32_t classSize = JSHClass::SIZE;
    auto *newClass = static_cast<JSHClass *>(sHeap_->AllocateClassClass(hclass, classSize));
    newClass->Initialize(thread_, size, type, 0, thread_->GlobalConstants()->GetHandledEmptySLayoutInfo());
    return JSHandle<JSHClass>(thread_, newClass);
}

JSHandle<JSHClass> ObjectFactory::NewSEcmaReadOnlyHClass(JSHClass *hclass, uint32_t size, JSType type,
                                                         uint32_t inlinedProps)
{
    NewSObjectHook();
    uint32_t classSize = JSHClass::SIZE;
    auto *newClass = static_cast<JSHClass *>(sHeap_->AllocateReadOnlyOrHugeObject(thread_, hclass, classSize));
    newClass->Initialize(thread_, size, type, inlinedProps, thread_->GlobalConstants()->GetHandledEmptySLayoutInfo());
    return JSHandle<JSHClass>(thread_, newClass);
}

JSHandle<JSHClass> ObjectFactory::InitSClassClass()
{
    JSHandle<JSHClass> hClassHandle = NewSEcmaHClassClass(nullptr, JSHClass::SIZE, JSType::HCLASS);
    JSHClass *hclass = reinterpret_cast<JSHClass *>(hClassHandle.GetTaggedValue().GetTaggedObject());
    hclass->SetClass(thread_, hclass);
    return hClassHandle;
}

JSHandle<AccessorData> ObjectFactory::NewSAccessorData()
{
    NewSObjectHook();
    TaggedObject *header = sHeap_->AllocateOldOrHugeObject(
        thread_, JSHClass::Cast(thread_->GlobalConstants()->GetAccessorDataClass().GetTaggedObject()));
    JSHandle<AccessorData> acc(thread_, AccessorData::Cast(header));
    acc->SetGetter(thread_, JSTaggedValue::Undefined());
    acc->SetSetter(thread_, JSTaggedValue::Undefined());
    return acc;
}

JSHandle<Method> ObjectFactory::NewSMethod(const JSPandaFile *jsPandaFile, MethodLiteral *methodLiteral,
                                           JSHandle<ConstantPool> constpool, uint32_t entryIndex,
                                           bool needSetAotFlag, bool *canFastCall)
{
    JSHandle<Method> method;
    if (jsPandaFile->IsNewVersion()) {
        method = Method::Create(thread_, jsPandaFile, methodLiteral);
    } else {
        method = NewSMethod(methodLiteral);
        method->SetConstantPool(thread_, constpool);
    }
    if (needSetAotFlag) {
        auto aotFileManager = thread_->GetEcmaVM()->GetAOTFileManager();
        aotFileManager->SetAOTFuncEntry(jsPandaFile, nullptr, *method, entryIndex, canFastCall);
    } else {
        method->ClearAOTFlagsWhenInit();
    }
    return method;
}

JSHandle<Method> ObjectFactory::NewSMethod(const MethodLiteral *methodLiteral, MemSpaceType spaceType)
{
    ASSERT(spaceType == SHARED_NON_MOVABLE || spaceType == SHARED_OLD_SPACE);
    NewSObjectHook();
    TaggedObject *header = nullptr;
    if (spaceType == SHARED_NON_MOVABLE) {
        header = sHeap_->AllocateNonMovableOrHugeObject(thread_,
            JSHClass::Cast(thread_->GlobalConstants()->GetMethodClass().GetTaggedObject()));
    } else {
        header = sHeap_->AllocateOldOrHugeObject(thread_,
            JSHClass::Cast(thread_->GlobalConstants()->GetMethodClass().GetTaggedObject()));
    }
    JSHandle<Method> method(thread_, header);
    InitializeMethod(methodLiteral, method);
    return method;
}

JSHandle<Method> ObjectFactory::NewSMethodForNativeFunction(const void *func, FunctionKind kind,
                                                            kungfu::BuiltinsStubCSigns::ID builtinId,
                                                            MemSpaceType spaceType)
{
    uint32_t numArgs = 2;  // function object and this
    auto method = NewSMethod(nullptr, spaceType);
    method->SetNativePointer(const_cast<void *>(func));
    method->SetNativeBit(true);
    if (builtinId != kungfu::BuiltinsStubCSigns::INVALID) {
        bool isFast = kungfu::BuiltinsStubCSigns::IsFastBuiltin(builtinId);
        method->SetFastBuiltinBit(isFast);
        method->SetBuiltinId(static_cast<uint8_t>(builtinId));
    }
    method->SetNumArgsWithCallField(numArgs);
    method->SetFunctionKind(kind);
    return method;
}

JSHandle<JSFunction> ObjectFactory::NewSFunctionByHClass(const JSHandle<Method> &method,
                                                         const JSHandle<JSHClass> &hclass)
{
    JSHandle<JSFunction> function(NewSharedOldSpaceJSObject(hclass));
    hclass->SetCallable(true);
    JSFunction::InitializeSFunction(thread_, function, method->GetFunctionKind());
    function->SetMethod(thread_, method);
    if (method->IsAotWithCallField()) {
        thread_->GetEcmaVM()->GetAOTFileManager()->
            SetAOTFuncEntry(method->GetJSPandaFile(), *function, *method);
    }
    return function;
}

// new function with name/length accessor
JSHandle<JSFunction> ObjectFactory::NewSFunctionWithAccessor(const void *func, const JSHandle<JSHClass> &hclass,
    FunctionKind kind, kungfu::BuiltinsStubCSigns::ID builtinId, MemSpaceType spaceType)
{
    ASSERT(spaceType == SHARED_NON_MOVABLE || spaceType == SHARED_OLD_SPACE);
    JSHandle<Method> method = NewSMethodForNativeFunction(func, kind, builtinId, spaceType);
    return NewSFunctionByHClass(method, hclass);
}

// new function without name/length accessor
JSHandle<JSFunction> ObjectFactory::NewSFunctionByHClass(const void *func, const JSHandle<JSHClass> &hclass,
    FunctionKind kind, kungfu::BuiltinsStubCSigns::ID builtinId, MemSpaceType spaceType)
{
    ASSERT(spaceType == SHARED_NON_MOVABLE || spaceType == SHARED_OLD_SPACE);
    JSHandle<Method> method = NewSMethodForNativeFunction(func, kind, builtinId, spaceType);
    JSHandle<JSFunction> function(NewSharedOldSpaceJSObject(hclass));
    hclass->SetCallable(true);
    JSFunction::InitializeWithDefaultValue(thread_, function);
    function->SetMethod(thread_, method);
    return function;
}

TaggedObject *ObjectFactory::NewSharedOldSpaceObject(const JSHandle<JSHClass> &hclass)
{
    NewSObjectHook();
    TaggedObject *header = sHeap_->AllocateOldOrHugeObject(thread_, *hclass);
    uint32_t inobjPropCount = hclass->GetInlinedProperties();
    if (inobjPropCount > 0) {
        InitializeExtraProperties(hclass, header, inobjPropCount);
    }
    return header;
}

JSHandle<JSObject> ObjectFactory::NewSharedOldSpaceJSObjectWithInit(const JSHandle<JSHClass> &jshclass)
{
    auto obj = NewSharedOldSpaceJSObject(jshclass);
    InitializeJSObject(obj, jshclass);
    return obj;
}

JSHandle<JSObject> ObjectFactory::NewSharedOldSpaceJSObject(const JSHandle<JSHClass> &jshclass)
{
    JSHandle<JSObject> obj(thread_, JSObject::Cast(NewSharedOldSpaceObject(jshclass)));
    JSHandle<TaggedArray> emptyArray = SharedEmptyArray();
    obj->InitializeHash();
    obj->SetElements(thread_, emptyArray);
    obj->SetProperties(thread_, emptyArray);
    return obj;
}

JSHandle<TaggedArray> ObjectFactory::SharedEmptyArray() const
{
    return JSHandle<TaggedArray>(thread_->GlobalConstants()->GetHandledEmptyArray());
}

JSHandle<TaggedArray> ObjectFactory::NewSTaggedArrayWithoutInit(uint32_t length)
{
    NewSObjectHook();
    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), length);
    auto arrayClass = JSHClass::Cast(thread_->GlobalConstants()->GetArrayClass().GetTaggedObject());
    TaggedObject *header = sHeap_->AllocateOldOrHugeObject(thread_, arrayClass, size);
    JSHandle<TaggedArray> array(thread_, header);
    array->SetLength(length);
    return array;
}

JSHandle<LayoutInfo> ObjectFactory::CreateSLayoutInfo(uint32_t properties)
{
    uint32_t arrayLength = LayoutInfo::ComputeArrayLength(properties);
    JSHandle<LayoutInfo> layoutInfoHandle = JSHandle<LayoutInfo>::Cast(NewSTaggedArrayWithoutInit(arrayLength));
    layoutInfoHandle->Initialize(thread_);
    return layoutInfoHandle;
}

JSHandle<LayoutInfo> ObjectFactory::CopyAndReSortSLayoutInfo(const JSHandle<LayoutInfo> &old, int end, int capacity)
{
    ASSERT(capacity >= end);
    JSHandle<LayoutInfo> newArr = CreateSLayoutInfo(capacity);
    Span<struct Properties> sp(old->GetProperties(), end);
    for (int i = 0; i < end; i++) {
        newArr->AddKey(thread_, i, sp[i].key_, PropertyAttributes(sp[i].attr_));
    }
    return newArr;
}

JSHandle<TaggedArray> ObjectFactory::NewSDictionaryArray(uint32_t length)
{
    NewSObjectHook();
    ASSERT(length > 0);
    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), length);
    auto header = sHeap_->AllocateOldOrHugeObject(
        thread_, JSHClass::Cast(thread_->GlobalConstants()->GetDictionaryClass().GetTaggedObject()), size);
    JSHandle<TaggedArray> array(thread_, header);
    array->InitializeWithSpecialValue(JSTaggedValue::Undefined(), length);
    return array;
}

JSHandle<TaggedArray> ObjectFactory::NewSEmptyArray()
{
    NewSObjectHook();
    auto header = sHeap_->AllocateReadOnlyOrHugeObject(thread_,
        JSHClass::Cast(thread_->GlobalConstants()->GetArrayClass().GetTaggedObject()), TaggedArray::SIZE);
    JSHandle<TaggedArray> array(thread_, header);
    array->SetLength(0);
    array->SetExtraLength(0);
    return array;
}

JSHandle<MutantTaggedArray> ObjectFactory::NewSEmptyMutantArray()
{
    NewSObjectHook();
    auto header = sHeap_->AllocateReadOnlyOrHugeObject(thread_,
        JSHClass::Cast(thread_->GlobalConstants()->GetMutantTaggedArrayClass().GetTaggedObject()), TaggedArray::SIZE);
    JSHandle<MutantTaggedArray> array(thread_, header);
    array->SetLength(0);
    array->SetExtraLength(0);
    return array;
}

JSHandle<JSNativePointer> ObjectFactory::NewSJSNativePointer(void *externalPointer,
                                                             const DeleteEntryPoint &callBack,
                                                             void *data,
                                                             bool nonMovable,
                                                             size_t nativeBindingsize,
                                                             NativeFlag flag)
{
    NewSObjectHook();
    TaggedObject *header;
    auto jsNativePointerClass = JSHClass::Cast(thread_->GlobalConstants()->GetJSNativePointerClass().GetTaggedObject());
    if (nonMovable) {
        header = sHeap_->AllocateNonMovableOrHugeObject(thread_, jsNativePointerClass);
    } else {
        header = sHeap_->AllocateOldOrHugeObject(thread_, jsNativePointerClass);
    }
    JSHandle<JSNativePointer> obj(thread_, header);
    obj->SetExternalPointer(externalPointer);
    obj->SetDeleter(callBack);
    obj->SetData(data);
    obj->SetBindingSize(nativeBindingsize);
    obj->SetNativeFlag(flag);

    if (callBack != nullptr) {
        vm_->PushToNativePointerList(static_cast<JSNativePointer *>(header));
        // In some cases, the size of JS/TS object is too small and the native binding size is too large.
        // Check and try trigger concurrent mark here.
    }
    return obj;
}

JSHandle<AccessorData> ObjectFactory::NewSInternalAccessor(void *setter, void *getter)
{
    NewSObjectHook();
    TaggedObject *header = sHeap_->AllocateNonMovableOrHugeObject(thread_,
        JSHClass::Cast(thread_->GlobalConstants()->GetInternalAccessorClass().GetTaggedObject()));
    JSHandle<AccessorData> obj(thread_, AccessorData::Cast(header));
    obj->SetGetter(thread_, JSTaggedValue::Undefined());
    obj->SetSetter(thread_, JSTaggedValue::Undefined());
    if (setter != nullptr) {
        JSHandle<JSNativePointer> setFunc = NewSJSNativePointer(setter, nullptr, nullptr, true);
        obj->SetSetter(thread_, setFunc.GetTaggedValue());
    }
    if (getter != nullptr) {
        JSHandle<JSNativePointer> getFunc = NewSJSNativePointer(getter, nullptr, nullptr, true);
        obj->SetGetter(thread_, getFunc);
    }
    return obj;
}

JSHandle<ConstantPool> ObjectFactory::NewSConstantPool(uint32_t capacity)
{
    NewSObjectHook();
    size_t size = ConstantPool::ComputeSize(capacity);
    auto header = sHeap_->AllocateOldOrHugeObject(
        thread_, JSHClass::Cast(sHeap_->GetGlobalConst()->GetConstantPoolClass().GetTaggedObject()), size);
    JSHandle<ConstantPool> array(thread_, header);
    array->InitializeWithSpecialValue(thread_, JSTaggedValue::Hole(), capacity);
    return array;
}

JSHandle<COWTaggedArray> ObjectFactory::NewSCOWTaggedArray(uint32_t length, JSTaggedValue initVal)
{
    NewSObjectHook();
    ASSERT(length > 0);

    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), length);
    auto header = sHeap_->AllocateNonMovableOrHugeObject(
        thread_, JSHClass::Cast(sHeap_->GetGlobalConst()->GetCOWArrayClass().GetTaggedObject()), size);
    JSHandle<COWTaggedArray> cowArray(thread_, header);
    cowArray->InitializeWithSpecialValue(initVal, length);
    return cowArray;
}

JSHandle<ClassLiteral> ObjectFactory::NewSClassLiteral()
{
    NewSObjectHook();
    TaggedObject *header = sHeap_->AllocateOldOrHugeObject(
        thread_, JSHClass::Cast(sHeap_->GetGlobalConst()->GetClassLiteralClass().GetTaggedObject()));
    JSHandle<TaggedArray> emptyArray = EmptyArray();

    JSHandle<ClassLiteral> classLiteral(thread_, header);
    classLiteral->SetArray(thread_, emptyArray);
    classLiteral->SetIsAOTUsed(false);

    return classLiteral;
}

JSHandle<ClassInfoExtractor> ObjectFactory::NewSClassInfoExtractor(
    JSHandle<JSTaggedValue> method)
{
    NewSObjectHook();
    TaggedObject *header = sHeap_->AllocateOldOrHugeObject(
        thread_, JSHClass::Cast(sHeap_->GetGlobalConst()->GetClassInfoExtractorHClass().GetTaggedObject()));
    JSHandle<ClassInfoExtractor> obj(thread_, header);
    obj->ClearBitField();
    obj->SetConstructorMethod(thread_, method.GetTaggedValue());
    JSHandle<TaggedArray> emptyArray = EmptyArray();
    obj->SetNonStaticKeys(thread_, emptyArray, SKIP_BARRIER);
    obj->SetNonStaticProperties(thread_, emptyArray, SKIP_BARRIER);
    obj->SetNonStaticElements(thread_, emptyArray, SKIP_BARRIER);
    obj->SetStaticKeys(thread_, emptyArray, SKIP_BARRIER);
    obj->SetStaticProperties(thread_, emptyArray, SKIP_BARRIER);
    obj->SetStaticElements(thread_, emptyArray, SKIP_BARRIER);
    return obj;
}

JSHandle<TaggedArray> ObjectFactory::NewSOldSpaceTaggedArray(uint32_t length, JSTaggedValue initVal)
{
    return NewSTaggedArray(length, initVal, MemSpaceType::SHARED_OLD_SPACE);
}

JSHandle<TaggedArray> ObjectFactory::NewSTaggedArray(uint32_t length, JSTaggedValue initVal, MemSpaceType spaceType)
{
    NewSObjectHook();
    if (length == 0) {
        return EmptyArray();
    }

    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), length);
    TaggedObject *header = nullptr;
    JSHClass *arrayClass = JSHClass::Cast(thread_->GlobalConstants()->GetArrayClass().GetTaggedObject());
    switch (spaceType) {
        case MemSpaceType::SHARED_OLD_SPACE:
            header = sHeap_->AllocateOldOrHugeObject(thread_, arrayClass, size);
            break;
        case MemSpaceType::SHARED_NON_MOVABLE:
            header = sHeap_->AllocateNonMovableOrHugeObject(thread_, arrayClass, size);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }

    JSHandle<TaggedArray> array(thread_, header);
    array->InitializeWithSpecialValue(initVal, length);
    return array;
}

JSHandle<JSSymbol> ObjectFactory::NewSWellKnownSymbol(const JSHandle<JSTaggedValue> &name)
{
    NewSObjectHook();
    TaggedObject *header = sHeap_->AllocateNonMovableOrHugeObject(
        thread_, JSHClass::Cast(thread_->GlobalConstants()->GetSymbolClass().GetTaggedObject()));
    JSHandle<JSSymbol> obj(thread_, JSSymbol::Cast(header));
    obj->SetFlags(0);
    obj->SetWellKnownSymbol();
    obj->SetDescription(thread_, name);
    obj->SetHashField(SymbolTable::Hash(name.GetTaggedValue()));
    return obj;
}

JSHandle<JSSymbol> ObjectFactory::NewSEmptySymbol()
{
    NewObjectHook();
    TaggedObject *header = sHeap_->AllocateNonMovableOrHugeObject(
        thread_, JSHClass::Cast(thread_->GlobalConstants()->GetSymbolClass().GetTaggedObject()));
    JSHandle<JSSymbol> obj(thread_, JSSymbol::Cast(header));
    obj->SetDescription(thread_, JSTaggedValue::Undefined());
    obj->SetFlags(0);
    obj->SetHashField(0);
    return obj;
}

JSHandle<JSSymbol> ObjectFactory::NewSWellKnownSymbolWithChar(std::string_view description)
{
    JSHandle<EcmaString> string = NewFromUtf8(description);
    return NewSWellKnownSymbol(JSHandle<JSTaggedValue>(string));
}

JSHandle<SourceTextModule> ObjectFactory::NewSModule()
{
    NewObjectHook();
    TaggedObject *header = sHeap_->AllocateOldOrHugeObject(thread_,
        JSHClass::Cast(thread_->GlobalConstants()->GetSourceTextModuleClass().GetTaggedObject()));
    JSHandle<SourceTextModule> obj(thread_, header);
    JSTaggedValue undefinedValue = thread_->GlobalConstants()->GetUndefined();
    obj->SetEnvironment(thread_, undefinedValue);
    obj->SetNamespace(thread_, undefinedValue);
    obj->SetEcmaModuleFilename(thread_, undefinedValue);
    obj->SetEcmaModuleRecordName(thread_, undefinedValue);
    obj->SetRequestedModules(thread_, undefinedValue);
    obj->SetImportEntries(thread_, undefinedValue);
    obj->SetLocalExportEntries(thread_, undefinedValue);
    obj->SetIndirectExportEntries(thread_, undefinedValue);
    obj->SetStarExportEntries(thread_, undefinedValue);
    obj->SetNameDictionary(thread_, undefinedValue);
    // [[CycleRoot]]: For a module not in a cycle, this would be the module itself.
    obj->SetCycleRoot(thread_, obj);
    obj->SetTopLevelCapability(thread_, undefinedValue);
    obj->SetAsyncParentModules(thread_, undefinedValue);
    obj->SetHasTLA(false);
    obj->SetAsyncEvaluatingOrdinal(SourceTextModule::NOT_ASYNC_EVALUATED);
    obj->SetPendingAsyncDependencies(SourceTextModule::UNDEFINED_INDEX);
    obj->SetDFSIndex(SourceTextModule::UNDEFINED_INDEX);
    obj->SetDFSAncestorIndex(SourceTextModule::UNDEFINED_INDEX);
    obj->SetEvaluationError(SourceTextModule::UNDEFINED_INDEX);
    obj->SetStatus(ModuleStatus::UNINSTANTIATED);
    obj->SetTypes(ModuleTypes::UNKNOWN);
    obj->SetIsNewBcVersion(false);
    obj->SetRegisterCounts(UINT16_MAX);
    obj->SetSharedType(SharedTypes::UNSENDABLE_MODULE);
    return obj;
}

JSHandle<ResolvedIndexBinding> ObjectFactory::NewSResolvedIndexBindingRecord()
{
    JSHandle<JSTaggedValue> undefinedValue = thread_->GlobalConstants()->GetHandledUndefined();
    JSHandle<SourceTextModule> ecmaModule(undefinedValue);
    int32_t index = 0;
    return NewSResolvedIndexBindingRecord(ecmaModule, index);
}

JSHandle<ResolvedIndexBinding> ObjectFactory::NewSResolvedIndexBindingRecord(const JSHandle<SourceTextModule> &module,
                                                                             int32_t index)
{
    NewObjectHook();
    TaggedObject *header = sHeap_->AllocateOldOrHugeObject(thread_,
        JSHClass::Cast(thread_->GlobalConstants()->GetResolvedIndexBindingClass().GetTaggedObject()));
    JSHandle<ResolvedIndexBinding> obj(thread_, header);
    obj->SetModule(thread_, module);
    obj->SetIndex(index);
    return obj;
}

JSHandle<ResolvedBinding> ObjectFactory::NewSResolvedBindingRecord()
{
    JSHandle<JSTaggedValue> undefinedValue = thread_->GlobalConstants()->GetHandledUndefined();
    JSHandle<SourceTextModule> ecmaModule(undefinedValue);
    JSHandle<JSTaggedValue> bindingName(undefinedValue);
    return NewSResolvedBindingRecord(ecmaModule, bindingName);
}

JSHandle<ResolvedBinding> ObjectFactory::NewSResolvedBindingRecord(const JSHandle<SourceTextModule> &module,
                                                                   const JSHandle<JSTaggedValue> &bindingName)
{
    NewObjectHook();
    TaggedObject *header = sHeap_->AllocateOldOrHugeObject(thread_,
        JSHClass::Cast(thread_->GlobalConstants()->GetResolvedBindingClass().GetTaggedObject()));
    JSHandle<ResolvedBinding> obj(thread_, header);
    obj->SetModule(thread_, module);
    obj->SetBindingName(thread_, bindingName);
    return obj;
}

JSHandle<ResolvedRecordBinding> ObjectFactory::NewSResolvedRecordBindingRecord()
{
    JSHandle<JSTaggedValue> undefinedValue = thread_->GlobalConstants()->GetHandledUndefined();
    JSHandle<EcmaString> ecmaModule(undefinedValue);
    int32_t index = 0;
    return NewSResolvedRecordBindingRecord(ecmaModule, index);
}

JSHandle<ResolvedRecordBinding> ObjectFactory::NewSResolvedRecordBindingRecord(
    const JSHandle<EcmaString> &moduleRecord, int32_t index)
{
    NewObjectHook();
    TaggedObject *header = sHeap_->AllocateOldOrHugeObject(thread_,
        JSHClass::Cast(thread_->GlobalConstants()->GetResolvedRecordBindingClass().GetTaggedObject()));
    JSHandle<ResolvedRecordBinding> obj(thread_, header);
    obj->SetModuleRecord(thread_, moduleRecord);
    obj->SetIndex(index);
    return obj;
}

JSHandle<AOTLiteralInfo> ObjectFactory::NewSAOTLiteralInfo(uint32_t length, JSTaggedValue initVal)
{
    NewObjectHook();
    size_t size = AOTLiteralInfo::ComputeSize(length);
    auto header = sHeap_->AllocateOldOrHugeObject(thread_,
        JSHClass::Cast(sHeap_->GetGlobalConst()->GetAOTLiteralInfoClass().GetTaggedObject()), size);

    JSHandle<AOTLiteralInfo> aotLiteralInfo(thread_, header);
    aotLiteralInfo->InitializeWithSpecialValue(initVal, length);
    return aotLiteralInfo;
}
}  // namespace panda::ecmascript
