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

#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/layout_info-inl.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/symbol_table.h"
#include "ecmascript/jspandafile/program_object.h"

// class Object;
namespace panda::ecmascript {
void ObjectFactory::NewSObjectHook() const
{
    CHECK_NO_HEAP_ALLOC;
#ifndef NDEBUG
    static std::atomic<uint32_t> count = 0;
    static uint32_t frequency = vm_->GetJSOptions().GetForceSharedGCFrequency();
    static constexpr uint32_t CONCURRENT_MARK_FREQUENCY_FACTOR = 2;
    if (frequency == 0 || !vm_->GetJSOptions().EnableForceGC() || !vm_->IsInitialized() ||
        !thread_->IsAllContextsInitialized()) {
        return;
    }
    if (count++ % frequency == 0) {
        if (count % (CONCURRENT_MARK_FREQUENCY_FACTOR * frequency) == 0) {
            sHeap_->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread_);
        } else if (sHeap_->CheckCanTriggerConcurrentMarking(thread_)) {
            sHeap_->TriggerConcurrentMarking<TriggerGCType::SHARED_GC, MarkReason::OTHER>(thread_);
        }
        if (!ecmascript::AnFileDataManager::GetInstance()->IsEnable()) {
            if (count % (CONCURRENT_MARK_FREQUENCY_FACTOR * frequency) == 0) {
                sHeap_->WaitGCFinished(thread_);
                sHeap_->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread_);
            } else if (sHeap_->CheckCanTriggerConcurrentMarking(thread_)) {
                sHeap_->TriggerConcurrentMarking<TriggerGCType::SHARED_PARTIAL_GC, MarkReason::OTHER>(thread_);
            }
        }
    }
#endif
}

JSHandle<JSHClass> ObjectFactory::CreateSFunctionClass(uint32_t size, JSType type,
                                                       const JSHandle<JSTaggedValue> &prototype,
                                                       bool isAccessor, bool setProto)
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
    if (setProto) {
        ASSERT(JSFunction::PROTOTYPE_INLINE_PROPERTY_INDEX == fieldOrder);
        {
            attributes.SetOffset(fieldOrder);
            layoutInfoHandle->AddKey(thread_, fieldOrder,
                                     globalConst->GetPrototypeString(), attributes);
            fieldOrder++;
        }
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
    hclass->SetNumberOfProps(inlinedProps);
    hclass->SetExtensible(false);
    return hclass;
}

JSHandle<JSHClass> ObjectFactory::NewSEcmaHClassDictMode(uint32_t size, uint32_t inlinedProps, JSType type,
                                                         const JSHandle<JSTaggedValue> &prototype)
{
    NewSObjectHook();
    uint32_t classSize = JSHClass::SIZE;
    auto *newClass = static_cast<JSHClass *>(sHeap_->AllocateNonMovableOrHugeObject(thread_,
        JSHClass::Cast(thread_->GlobalConstants()->GetHClassClass().GetTaggedObject()), classSize));
    newClass->Initialize(thread_, size, type, inlinedProps, thread_->GlobalConstants()->GetHandledEmptySLayoutInfo());
    JSHandle<JSHClass> hclass(thread_, newClass);
    if (prototype->IsJSObject()) {
        prototype->GetTaggedObject()->GetClass()->SetIsPrototype(true);
    }
    hclass->SetProto(thread_, prototype.GetTaggedValue());
    hclass->SetNumberOfProps(0);
    hclass->SetIsDictionaryMode(true);
    hclass->SetExtensible(false);
    return hclass;
}

JSHandle<JSHClass> ObjectFactory::NewSEcmaHClassClass(JSHClass *hclass, uint32_t size, JSType type)
{
    NewSObjectHook();
    uint32_t classSize = JSHClass::SIZE;
    auto *newClass = static_cast<JSHClass *>(sHeap_->AllocateClassClass(thread_, hclass, classSize));
    newClass->Initialize(thread_, size, type, 0, thread_->GlobalConstants()->GetHandledEmptySLayoutInfo());
    return JSHandle<JSHClass>(thread_, newClass);
}

JSHandle<JSHClass> ObjectFactory::NewSEcmaReadOnlyHClass(JSHClass *hclass, uint32_t size, JSType type,
                                                         uint32_t inlinedProps)
{
    NewSObjectHook();
    uint32_t classSize = JSHClass::SIZE;
    auto *newClass = static_cast<JSHClass *>(sHeap_->AllocateReadOnlyOrHugeObject(thread_, hclass, classSize));
    ASSERT(newClass != nullptr);
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
        method->InitInterpreterStatusForCompiledMethod(thread_);
    }
    return method;
}

JSHandle<Method> ObjectFactory::NewSMethod(const MethodLiteral *methodLiteral, MemSpaceType spaceType)
{
    ASSERT(spaceType == SHARED_READ_ONLY_SPACE ||
           spaceType == SHARED_NON_MOVABLE ||
           spaceType == SHARED_OLD_SPACE);
    NewSObjectHook();
    TaggedObject *header = nullptr;
    if (spaceType == SHARED_READ_ONLY_SPACE) {
        header = sHeap_->AllocateReadOnlyOrHugeObject(thread_,
            JSHClass::Cast(thread_->GlobalConstants()->GetMethodClass().GetTaggedObject()));
    } else if (spaceType == SHARED_NON_MOVABLE) {
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
    if (builtinId != BUILTINS_STUB_ID(INVALID)) {
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
    function->SetTaskConcurrentFuncFlag(0); // 0 : default value
    if (method->IsNativeWithCallField()) {
        SetNativePointerToFunctionFromMethod(JSHandle<JSFunctionBase>::Cast(function), method);
    } else if (method->IsAotWithCallField()) {
        thread_->GetEcmaVM()->GetAOTFileManager()->
            SetAOTFuncEntry(method->GetJSPandaFile(), *function, *method);
    } else {
        SetCodeEntryToFunctionFromMethod(function, method);
    }
    return function;
}

JSHandle<JSFunction> ObjectFactory::NewNativeSFunctionByHClass(const JSHandle<JSHClass> &hclass,
                                                               const void *nativeFunc,
                                                               FunctionKind kind)
{
    JSHandle<JSFunction> function(NewSharedOldSpaceJSObject(hclass));
    hclass->SetCallable(true);
    JSFunction::InitializeSFunction(thread_, function, kind);
    function->SetMethod(thread_, GetReadOnlyMethodForNativeFunction(kind));
    function->SetNativePointer(const_cast<void *>(nativeFunc));
    function->SetTaskConcurrentFuncFlag(0); // 0 : default value
    return function;
}

// new function with name/length accessor
JSHandle<JSFunction> ObjectFactory::NewSFunctionWithAccessor(const void *func, const JSHandle<JSHClass> &hclass,
    FunctionKind kind, kungfu::BuiltinsStubCSigns::ID builtinId, MemSpaceType spaceType)
{
    ASSERT(spaceType == SHARED_NON_MOVABLE || spaceType == SHARED_OLD_SPACE);
    if (builtinId != BUILTINS_STUB_ID(INVALID)) {
        JSHandle<Method> method = NewSMethodForNativeFunction(func, kind, builtinId, spaceType);
        return NewSFunctionByHClass(method, hclass);
    }
    return NewNativeSFunctionByHClass(hclass, func, kind);
}

// new function without name/length accessor
JSHandle<JSFunction> ObjectFactory::NewSFunctionByHClass(const void *func, const JSHandle<JSHClass> &hclass,
    FunctionKind kind, kungfu::BuiltinsStubCSigns::ID builtinId, MemSpaceType spaceType)
{
    ASSERT(spaceType == SHARED_NON_MOVABLE || spaceType == SHARED_OLD_SPACE);
    JSHandle<JSFunction> function(NewSharedOldSpaceJSObject(hclass));
    hclass->SetCallable(true);
    JSFunction::InitializeWithDefaultValue(thread_, function);
    if (builtinId != BUILTINS_STUB_ID(INVALID)) {
        JSHandle<Method> method = NewSMethodForNativeFunction(func, kind, builtinId, spaceType);
        function->SetMethod(thread_, method);
    } else {
        function->SetMethod(thread_, GetReadOnlyMethodForNativeFunction(kind));
    }
    function->SetNativePointer(const_cast<void *>(func));
    return function;
}

TaggedObject *ObjectFactory::NewSharedOldSpaceObject(const JSHandle<JSHClass> &hclass)
{
    NewSObjectHook();
    TaggedObject *header = sHeap_->AllocateOldOrHugeObject(thread_, *hclass);
    if (header == nullptr) {
        LOG_ECMA(FATAL) << "ObjectFactory::NewSharedOldSpaceObject:header is nullptr";
    }
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

JSHandle<JSTaggedValue> ObjectFactory::CreateSObjectWithProperties(std::vector<PropertyDescriptor> &descs)
{
    JSHandle<JSHClass> hclass = JSHClass::CreateSHClass(thread_, descs);
    JSHandle<JSObject> object = NewSharedOldSpaceJSObject(hclass);
    JSObject::SetSProperties(thread_, object, descs);
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFuncProto = env->GetSObjectFunctionPrototype();
    hclass->SetPrototype(thread_, objFuncProto);
    hclass->SetExtensible(false);
    return JSHandle<JSTaggedValue>(object);
}

JSHandle<TaggedArray> ObjectFactory::SharedEmptyArray() const
{
    return JSHandle<TaggedArray>(thread_->GlobalConstants()->GetHandledEmptyArray());
}

JSHandle<TaggedArray> ObjectFactory::CopySArray(const JSHandle<TaggedArray> &old, uint32_t oldLength,
                                                uint32_t newLength, JSTaggedValue initVal, ElementsKind kind)
{
    if (newLength == 0) {
        return SharedEmptyArray();
    }
    if (newLength > oldLength) {
        return ExtendSArray(old, newLength, initVal, kind);
    }

    NewObjectHook();
    // Shared-array does not support Mutantarray yet.
    ASSERT(!old->GetClass()->IsMutantTaggedArray());

    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), newLength);
    JSHClass *arrayClass = JSHClass::Cast(thread_->GlobalConstants()->GetTaggedArrayClass().GetTaggedObject());
    TaggedObject *header = sHeap_->AllocateOldOrHugeObject(thread_, arrayClass, size);
    JSHandle<TaggedArray> newArray(thread_, header);
    newArray->SetLength(newLength);
    newArray->SetExtraLength(old->GetExtraLength());

    for (uint32_t i = 0; i < newLength; i++) {
        newArray->Set(thread_, i, old->Get(i));
    }

    return newArray;
}

JSHandle<TaggedArray> ObjectFactory::ExtendSArray(const JSHandle<TaggedArray> &old, uint32_t length,
                                                  JSTaggedValue initVal, [[maybe_unused]] ElementsKind kind)
{
    ASSERT(length > old->GetLength());
    NewObjectHook();
    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), length);
    JSHClass *arrayClass = nullptr;
    // Shared-array does not support Mutantarray yet.
    ASSERT(!old->GetClass()->IsMutantTaggedArray());
    arrayClass = JSHClass::Cast(thread_->GlobalConstants()->GetTaggedArrayClass().GetTaggedObject());

    TaggedObject *header = sHeap_->AllocateOldOrHugeObject(thread_, arrayClass, size);
    JSHandle<TaggedArray> newArray(thread_, header);
    newArray->SetLength(length);
    newArray->SetExtraLength(old->GetExtraLength());

    uint32_t oldLength = old->GetLength();
    for (uint32_t i = 0; i < oldLength; i++) {
        newArray->Set(thread_, i, old->Get(i));
    }
    for (uint32_t i = oldLength; i < length; i++) {
        newArray->Set(thread_, i, initVal);
    }
    return newArray;
}

JSHandle<TaggedArray> ObjectFactory::NewSTaggedArrayWithoutInit(uint32_t length, MemSpaceType spaceType)
{
    NewSObjectHook();
    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), length);
    TaggedObject *header;
    auto arrayClass = JSHClass::Cast(thread_->GlobalConstants()->GetTaggedArrayClass().GetTaggedObject());
    switch (spaceType) {
        case MemSpaceType::SHARED_OLD_SPACE:
            header = sHeap_->AllocateOldOrHugeObject(thread_, arrayClass, size);
            break;
        case MemSpaceType::SHARED_READ_ONLY_SPACE:
            header = sHeap_->AllocateReadOnlyOrHugeObject(thread_, arrayClass, size);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
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

JSHandle<LayoutInfo> ObjectFactory::NewSEmptyLayoutInfo()
{
    JSHandle<LayoutInfo> layoutInfoHandle = JSHandle<LayoutInfo>::Cast(
        NewSTaggedArrayWithoutInit(0, MemSpaceType::SHARED_READ_ONLY_SPACE));
    layoutInfoHandle->Initialize(thread_);
    return layoutInfoHandle;
}

JSHandle<JSSharedArray> ObjectFactory::NewJSSArray()
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSFunction> function(env->GetSharedArrayFunction());
    return JSHandle<JSSharedArray>(NewJSObjectByConstructor(function));
}

JSHandle<TaggedArray> ObjectFactory::NewSJsonFixedArray(size_t start, size_t length,
                                                        const std::vector<JSHandle<JSTaggedValue>> &vec)
{
    if (length == 0) {
        return SharedEmptyArray();
    }

    JSHandle<TaggedArray> array = NewTaggedArrayWithoutInit(length, MemSpaceType::SHARED_OLD_SPACE);
    array->SetExtraLength(0);
    for (size_t i = 0; i < length; i++) {
        array->Set(thread_, i, vec[start + i]);
    }
    return array;
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

JSHandle<ProfileTypeInfoCell> ObjectFactory::NewSEmptyProfileTypeInfoCell()
{
    NewSObjectHook();
    auto header = sHeap_->AllocateReadOnlyOrHugeObject(thread_,
        JSHClass::Cast(thread_->GlobalConstants()->GetProfileTypeInfoCell0Class().GetTaggedObject()));
    JSHandle<ProfileTypeInfoCell> profileTypeInfoCell(thread_, header);
    profileTypeInfoCell->SetValue(thread_, JSTaggedValue::Undefined());
    profileTypeInfoCell->SetMachineCode(thread_, JSTaggedValue::Hole());
    profileTypeInfoCell->SetBaselineCode(thread_, JSTaggedValue::Hole());
    profileTypeInfoCell->SetHandle(thread_, JSTaggedValue::Undefined());
    return profileTypeInfoCell;
}

JSHandle<Method> ObjectFactory::NewSEmptyNativeFunctionMethod(FunctionKind kind)
{
    uint32_t numArgs = 2;  // function object and this
    auto method = NewSMethod(nullptr, MemSpaceType::SHARED_READ_ONLY_SPACE);
    method->SetNativeBit(true);
    method->SetNumArgsWithCallField(numArgs);
    method->SetFunctionKind(kind);
    return method;
}

JSHandle<FunctionTemplate> ObjectFactory::NewSFunctionTemplate(
    const JSHandle<Method> &method, const JSHandle<JSTaggedValue> &module, int32_t length)
{
    NewSObjectHook();
    auto globalConstants = thread_->GlobalConstants();
    auto header = sHeap_->AllocateOldOrHugeObject(thread_,
        JSHClass::Cast(globalConstants->GetFunctionTemplateClass().GetTaggedObject()));
    JSHandle<FunctionTemplate> functionTemplate(thread_, header);
    functionTemplate->SetMethod(thread_, method);
    functionTemplate->SetModule(thread_, module);
    functionTemplate->SetRawProfileTypeInfo(thread_, globalConstants->GetEmptyProfileTypeInfoCell(), SKIP_BARRIER);
    functionTemplate->SetLength(length);
    return functionTemplate;
}

JSHandle<TaggedArray> ObjectFactory::NewSEmptyArray()
{
    NewSObjectHook();
    auto header = sHeap_->AllocateReadOnlyOrHugeObject(thread_,
        JSHClass::Cast(thread_->GlobalConstants()->GetTaggedArrayClass().GetTaggedObject()), TaggedArray::SIZE);
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
                                                             const NativePointerCallback &callBack,
                                                             void *data,
                                                             bool nonMovable,
                                                             size_t nativeBindingsize,
                                                             NativeFlag flag)
{
    NewSObjectHook();
    TaggedObject *header;
    auto jsNativePointerClass =
        JSHClass::Cast(thread_->GlobalConstants()->GetSJSNativePointerClass().GetTaggedObject());
    jsNativePointerClass->SetIsJSShared(true);
    if (nonMovable) {
        header = sHeap_->AllocateNonMovableOrHugeObject(thread_, jsNativePointerClass);
    } else {
        header = sHeap_->AllocateOldOrHugeObject(thread_, jsNativePointerClass);
    }
    JSHandle<JSNativePointer> obj(thread_, header);
    obj->SetExternalPointer(externalPointer);
    obj->SetDeleter(callBack);
    obj->SetData(data);
    uint32_t fixedNativeBindingsize = nativeBindingsize < UINT32_MAX ? nativeBindingsize
                                                                     : UINT32_MAX;
    obj->SetBindingSize(fixedNativeBindingsize);
    obj->SetNativeFlag(flag);

    if (callBack != nullptr) {
        sHeap_->IncNativeSizeAfterLastGC(fixedNativeBindingsize);
        Runtime::GetInstance()->PushToSharedNativePointerList(static_cast<JSNativePointer *>(header));
        // In some cases, the size of JS/TS object is too small and the native binding size is too large.
        // Check and try trigger concurrent mark here.
        size_t nativeSizeAfterLastGC = sHeap_->GetNativeSizeAfterLastGC();
        if (nativeSizeAfterLastGC > sHeap_->GetNativeSizeTriggerSharedGC()) {
            sHeap_->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::NATIVE_LIMIT>(thread_);
        } else if (sHeap_->CheckCanTriggerConcurrentMarking(thread_) &&
            nativeSizeAfterLastGC > sHeap_->GetNativeSizeTriggerSharedCM()) {
            sHeap_->TriggerConcurrentMarking<TriggerGCType::SHARED_GC, MarkReason::NATIVE_LIMIT>(thread_);
        }
    }
    return obj;
}

JSHandle<JSNativePointer> ObjectFactory::NewSReadOnlyJSNativePointer(void* externalPointer)
{
    NewSObjectHook();
    auto jsNativePointerClass =
        JSHClass::Cast(thread_->GlobalConstants()->GetSJSNativePointerClass().GetTaggedObject());
    jsNativePointerClass->SetIsJSShared(true);
    TaggedObject* header = sHeap_->AllocateReadOnlyOrHugeObject(thread_, jsNativePointerClass);
    JSHandle<JSNativePointer> obj(thread_, header);
    obj->SetExternalPointer(externalPointer);
    obj->SetDeleter(nullptr);
    obj->SetData(nullptr);
    obj->SetBindingSize(0U);
    obj->SetNativeFlag(NativeFlag::NO_DIV);
    return obj;
}

JSHandle<AccessorData> ObjectFactory::NewSInternalAccessor(void *setter, void *getter)
{
    NewSObjectHook();
    TaggedObject *header = sHeap_->AllocateReadOnlyOrHugeObject(thread_,
        JSHClass::Cast(thread_->GlobalConstants()->GetInternalAccessorClass().GetTaggedObject()));
    JSHandle<InternalAccessor> obj(thread_, InternalAccessor::Cast(header));

    obj->SetSetter(reinterpret_cast<InternalAccessor::InternalSetFunc>(setter));
    obj->SetGetter(reinterpret_cast<InternalAccessor::InternalGetFunc>(getter));
    return JSHandle<AccessorData>::Cast(obj);
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
    JSHClass *arrayClass = JSHClass::Cast(thread_->GlobalConstants()->GetTaggedArrayClass().GetTaggedObject());
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

JSHandle<JSSymbol> ObjectFactory::NewSPublicSymbol(const JSHandle<JSTaggedValue> &name)
{
    NewObjectHook();
    TaggedObject *header = sHeap_->AllocateNonMovableOrHugeObject(
        thread_, JSHClass::Cast(thread_->GlobalConstants()->GetSymbolClass().GetTaggedObject()));
    JSHandle<JSSymbol> obj(thread_, JSSymbol::Cast(header));
    obj->SetFlags(0);
    obj->SetDescription(thread_, name);
    obj->SetHashField(SymbolTable::Hash(name.GetTaggedValue()));
    return obj;
}

JSHandle<JSSymbol> ObjectFactory::NewSConstantPrivateSymbol()
{
    NewObjectHook();
    TaggedObject *header = sHeap_->AllocateReadOnlyOrHugeObject(
        thread_, JSHClass::Cast(thread_->GlobalConstants()->GetSymbolClass().GetTaggedObject()));
    JSHandle<JSSymbol> obj(thread_, JSSymbol::Cast(header));
    obj->SetDescription(thread_, JSTaggedValue::Undefined());
    obj->SetFlags(0);
    obj->SetHashField(SymbolTable::Hash(obj.GetTaggedValue()));
    obj->SetPrivate();
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

JSHandle<JSSymbol> ObjectFactory::NewSPublicSymbolWithChar(std::string_view description)
{
    JSHandle<EcmaString> string = NewFromUtf8(description);
    return NewSPublicSymbol(JSHandle<JSTaggedValue>(string));
}

JSHandle<SourceTextModule> ObjectFactory::NewSSourceTextModule()
{
    NewObjectHook();
    TaggedObject *header = sHeap_->AllocateOldOrHugeObject(thread_,
        JSHClass::Cast(thread_->GlobalConstants()->GetSourceTextModuleClass().GetTaggedObject()));
    JSHandle<SourceTextModule> obj(thread_, header);
    JSTaggedValue undefinedValue = thread_->GlobalConstants()->GetUndefined();
    obj->SetEnvironment(thread_, undefinedValue);
    obj->SetNamespace(thread_, undefinedValue);
    obj->SetModuleRequests(thread_, undefinedValue);
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
    obj->SetException(thread_, JSTaggedValue::Hole());
    obj->SetStatus(ModuleStatus::UNINSTANTIATED);
    obj->SetTypes(ModuleTypes::UNKNOWN);
    obj->SetIsNewBcVersion(false);
    obj->SetRegisterCounts(UINT16_MAX);
    obj->SetLazyImportStatus(ToUintPtr(nullptr));
    obj->SetEcmaModuleFilename(ToUintPtr(nullptr));
    obj->SetEcmaModuleRecordName(ToUintPtr(nullptr));
    obj->SetSharedType(SharedTypes::UNSENDABLE_MODULE);
    obj->SetSendableEnv(thread_, undefinedValue);
    return obj;
}

JSHandle<ModuleNamespace> ObjectFactory::NewSModuleNamespace()
{
    NewObjectHook();
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetSharedModuleNamespaceClass());
    JSHandle<JSObject> obj = NewSharedOldSpaceJSObject(hclass);

    JSHandle<ModuleNamespace> moduleNamespace = JSHandle<ModuleNamespace>::Cast(obj);
    moduleNamespace->SetModule(thread_, JSTaggedValue::Undefined());
    moduleNamespace->SetExports(thread_, JSTaggedValue::Undefined());
    moduleNamespace->SetDeregisterProcession(thread_, JSTaggedValue::Undefined());
    return moduleNamespace;
}

JSHandle<ImportEntry> ObjectFactory::NewSImportEntry(const uint32_t moduleRequestIdx,
                                                     const JSHandle<JSTaggedValue> &importName,
                                                     const JSHandle<JSTaggedValue> &localName)
{
    NewObjectHook();
    TaggedObject *header = sHeap_->AllocateOldOrHugeObject(thread_,
        JSHClass::Cast(thread_->GlobalConstants()->GetImportEntryClass().GetTaggedObject()));
    JSHandle<ImportEntry> obj(thread_, header);
    obj->SetModuleRequestIndex(moduleRequestIdx);
    obj->SetImportName(thread_, importName);
    obj->SetLocalName(thread_, localName);
    return obj;
}

JSHandle<LocalExportEntry> ObjectFactory::NewSLocalExportEntry(const JSHandle<JSTaggedValue> &exportName,
    const JSHandle<JSTaggedValue> &localName, const uint32_t index)
{
    NewObjectHook();
    TaggedObject *header = sHeap_->AllocateOldOrHugeObject(thread_,
        JSHClass::Cast(thread_->GlobalConstants()->GetLocalExportEntryClass().GetTaggedObject()));
    JSHandle<LocalExportEntry> obj(thread_, header);
    obj->SetExportName(thread_, exportName);
    obj->SetLocalName(thread_, localName);
    obj->SetLocalIndex(index);
    return obj;
}

JSHandle<IndirectExportEntry> ObjectFactory::NewSIndirectExportEntry(const JSHandle<JSTaggedValue> &exportName,
                                                                     const uint32_t moduleRequestIdx,
                                                                     const JSHandle<JSTaggedValue> &importName)
{
    NewObjectHook();
    TaggedObject *header = sHeap_->AllocateOldOrHugeObject(thread_,
        JSHClass::Cast(thread_->GlobalConstants()->GetIndirectExportEntryClass().GetTaggedObject()));
    JSHandle<IndirectExportEntry> obj(thread_, header);
    obj->SetExportName(thread_, exportName);
    obj->SetModuleRequestIndex(moduleRequestIdx);
    obj->SetImportName(thread_, importName);
    return obj;
}

JSHandle<StarExportEntry> ObjectFactory::NewSStarExportEntry(const uint32_t moduleRequestIdx)
{
    NewObjectHook();
    TaggedObject *header = sHeap_->AllocateOldOrHugeObject(thread_,
        JSHClass::Cast(thread_->GlobalConstants()->GetStarExportEntryClass().GetTaggedObject()));
    JSHandle<StarExportEntry> obj(thread_, header);
    obj->SetModuleRequestIndex(moduleRequestIdx);
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

JSHandle<ResolvedRecordIndexBinding> ObjectFactory::NewSResolvedRecordIndexBindingRecord()
{
    JSHandle<JSTaggedValue> undefinedValue = thread_->GlobalConstants()->GetHandledUndefined();
    JSHandle<EcmaString> ecmaModule(undefinedValue);
    JSHandle<EcmaString> fileName(undefinedValue);
    int32_t index = 0;
    return NewSResolvedRecordIndexBindingRecord(ecmaModule, fileName, index);
}

JSHandle<ResolvedRecordIndexBinding> ObjectFactory::NewSResolvedRecordIndexBindingRecord(
    const JSHandle<EcmaString> &moduleRecord, const JSHandle<EcmaString> &abcFileName, int32_t index)
{
    NewObjectHook();
    TaggedObject *header = sHeap_->AllocateOldOrHugeObject(thread_,
        JSHClass::Cast(thread_->GlobalConstants()->GetResolvedRecordIndexBindingClass().GetTaggedObject()));
    JSHandle<ResolvedRecordIndexBinding> obj(thread_, header);
    obj->SetModuleRecord(thread_, moduleRecord);
    obj->SetAbcFileName(thread_, abcFileName);
    obj->SetIndex(index);
    return obj;
}

JSHandle<ResolvedRecordBinding> ObjectFactory::NewSResolvedRecordBindingRecord()
{
    JSHandle<JSTaggedValue> undefinedValue = thread_->GlobalConstants()->GetHandledUndefined();
    JSHandle<EcmaString> ecmaModule(undefinedValue);
    JSHandle<JSTaggedValue> bindingName(undefinedValue);
    return NewSResolvedRecordBindingRecord(ecmaModule, bindingName);
}

JSHandle<ResolvedRecordBinding> ObjectFactory::NewSResolvedRecordBindingRecord(
    const JSHandle<EcmaString> &moduleRecord, const JSHandle<JSTaggedValue> &bindingName)
{
    NewObjectHook();
    TaggedObject *header = sHeap_->AllocateOldOrHugeObject(thread_,
        JSHClass::Cast(thread_->GlobalConstants()->GetResolvedRecordBindingClass().GetTaggedObject()));
    JSHandle<ResolvedRecordBinding> obj(thread_, header);
    obj->SetModuleRecord(thread_, moduleRecord);
    obj->SetBindingName(thread_, bindingName);
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

JSHandle<SendableEnv> ObjectFactory::NewSendableEnv(int numSlots)
{
    NewObjectHook();
    size_t size = SendableEnv::ComputeSize(numSlots);
    auto header = sHeap_->AllocateOldOrHugeObject(thread_,
        JSHClass::Cast(sHeap_->GetGlobalConst()->GetSendableEnvClass().GetTaggedObject()), size);
    JSHandle<SendableEnv> array(thread_, header);
    array->InitializeWithSpecialValue(JSTaggedValue::Hole(), numSlots + SendableEnv::SENDABLE_RESERVED_ENV_LENGTH);
    return array;
}

JSHandle<JSFunction> ObjectFactory::NewJSSendableFunction(const JSHandle<Method> &methodHandle)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    FunctionKind kind = methodHandle->GetFunctionKind();
    JSHandle<JSHClass> hclass;
    switch (kind) {
        case FunctionKind::ASYNC_FUNCTION: {
            hclass = JSHandle<JSHClass>::Cast(env->GetSAsyncFunctionClass());
            break;
        }
        case FunctionKind::BASE_CONSTRUCTOR: {
            hclass = JSHandle<JSHClass>::Cast(env->GetSFunctionClassWithProto());
            break;
        }
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }

    JSHandle<JSFunction> func = NewSFunctionByHClass(methodHandle, hclass);
    ASSERT_NO_ABRUPT_COMPLETION(thread_);
    return func;
}
}  // namespace panda::ecmascript
