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
#include "ecmascript/layout_info-inl.h"
#include "ecmascript/mem/heap-inl.h"

namespace panda::ecmascript {
JSHandle<JSHClass> ObjectFactory::CreateSFunctionClass(uint32_t size, JSType type,
                                                       const JSHandle<JSTaggedValue> &prototype, bool isAccessor)
{
    const GlobalEnvConstants *globalConst = sHeap_->GetGlobalConst();
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
    return NewSEcmaHClass(JSHClass::Cast(sHeap_->GetGlobalConst()->GetHClassClass().GetTaggedObject()),
                          size, type, inlinedProps);
}

JSHandle<JSHClass> ObjectFactory::NewSEcmaHClass(JSHClass *hclass, uint32_t size, JSType type, uint32_t inlinedProps)
{
    NewObjectHook();
    uint32_t classSize = JSHClass::SIZE;
    auto *newClass = static_cast<JSHClass *>(sHeap_->AllocateNonMovableOrHugeObject(thread_, hclass, classSize));
    newClass->Initialize(thread_, size, type, inlinedProps, sHeap_->GetGlobalConst()->GetHandledEmptySLayoutInfo());
    return JSHandle<JSHClass>(thread_, newClass);
}

// This function don't UpdateProtoClass
JSHandle<JSHClass> ObjectFactory::NewSEcmaHClass(uint32_t size, uint32_t inlinedProps, JSType type,
    const JSHandle<JSTaggedValue> &prototype, const JSHandle<JSTaggedValue> &layout)
{
    NewObjectHook();
    uint32_t classSize = JSHClass::SIZE;
    auto *newClass = static_cast<JSHClass *>(sHeap_->AllocateNonMovableOrHugeObject(
        thread_, JSHClass::Cast(sHeap_->GetGlobalConst()->GetHClassClass().GetTaggedObject()), classSize));
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

JSHandle<AccessorData> ObjectFactory::NewSAccessorData()
{
    NewObjectHook();
    TaggedObject *header = sHeap_->AllocateOldOrHugeObject(
        thread_, JSHClass::Cast(sHeap_->GetGlobalConst()->GetAccessorDataClass().GetTaggedObject()));
    JSHandle<AccessorData> acc(thread_, AccessorData::Cast(header));
    acc->SetGetter(thread_, JSTaggedValue::Undefined());
    acc->SetSetter(thread_, JSTaggedValue::Undefined());
    return acc;
}

JSHandle<Method> ObjectFactory::NewSMethod(const JSPandaFile *jsPandaFile, MethodLiteral *methodLiteral,
                                           JSHandle<ConstantPool> constpool, JSHandle<JSTaggedValue> module)
{
    JSHandle<Method> method;
    if (jsPandaFile->IsNewVersion()) {
        method = Method::Create(thread_, jsPandaFile, methodLiteral, true);
    } else {
        method = NewSMethod(methodLiteral);
        method->SetConstantPool(thread_, constpool);
    }
    if (method->GetModule().IsUndefined()) {
        method->SetModule(thread_, module);
    }
    return method;
}

JSHandle<Method> ObjectFactory::NewSMethod(const MethodLiteral *methodLiteral, MemSpaceType spaceType)
{
    ASSERT(spaceType == SHARED_NON_MOVABLE || spaceType == SHARED_OLD_SPACE);
    NewObjectHook();
    TaggedObject *header = nullptr;
    if (spaceType == SHARED_NON_MOVABLE) {
        header = sHeap_->AllocateNonMovableOrHugeObject(thread_,
            JSHClass::Cast(sHeap_->GetGlobalConst()->GetMethodClass().GetTaggedObject()));
    } else {
        header = sHeap_->AllocateOldOrHugeObject(thread_,
            JSHClass::Cast(sHeap_->GetGlobalConst()->GetMethodClass().GetTaggedObject()));
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
        thread_->GetCurrentEcmaContext()->GetAOTFileManager()->
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
    NewObjectHook();
    TaggedObject *header = sHeap_->AllocateOldOrHugeObject(thread_, *hclass);
    uint32_t inobjPropCount = hclass->GetInlinedProperties();
    if (inobjPropCount > 0) {
        InitializeExtraProperties(hclass, header, inobjPropCount);
    }
    return header;
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
    return JSHandle<TaggedArray>(sHeap_->GetGlobalConst()->GetHandledEmptyArray());
}

JSHandle<TaggedArray> ObjectFactory::NewSTaggedArrayWithoutInit(uint32_t length)
{
    NewObjectHook();
    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), length);
    auto arrayClass = JSHClass::Cast(sHeap_->GetGlobalConst()->GetArrayClass().GetTaggedObject());
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
    NewObjectHook();
    ASSERT(length > 0);
    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), length);
    auto header = sHeap_->AllocateOldOrHugeObject(
        thread_, JSHClass::Cast(sHeap_->GetGlobalConst()->GetDictionaryClass().GetTaggedObject()), size);
    JSHandle<TaggedArray> array(thread_, header);
    array->InitializeWithSpecialValue(JSTaggedValue::Undefined(), length);
    return array;
}
}  // namespace panda::ecmascript