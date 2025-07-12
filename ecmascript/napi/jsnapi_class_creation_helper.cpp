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

#include "ecmascript/global_env.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/layout_info.h"
#include "ecmascript/napi/include/jsnapi_expo.h"
#include "ecmascript/napi/jsnapi_class_creation_helper.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript {
void JSNApiClassCreationHelper::ConstructDescByAttr(const JSThread *thread, const PropertyAttribute &attr,
                                                    PropertyDescriptor *desc)
{
    new (desc) PropertyDescriptor(thread, JSNApiHelper::ToJSHandle(attr.GetValue(thread->GetEcmaVM())),
                                  attr.IsWritable(), attr.IsEnumerable(), attr.IsConfigurable());
}

void JSNApiClassCreationHelper::DestructDesc(PropertyDescriptor *desc)
{
    desc->~PropertyDescriptor();
}

void JSNApiClassCreationHelper::DestructAttr(PropertyAttribute *attr)
{
    // Call ~PropertyAttribute directly for attrs from napi_define_class, which were construct by using placement new.
    attr->~PropertyAttribute();
}

bool JSNApiClassCreationHelper::TryAddOriKeyAndOriAttrToHClass(const JSThread *thread, const Local<JSValueRef> &oriKey,
                                                               const PropertyAttribute &oriAttr,
                                                               PropertyDescriptor &desc, size_t &attrOffset,
                                                               JSHandle<JSHClass> &hClass)
{
    // If return false, it means that property must be add by slow path(DefinePropertyOrThrow).
    JSMutableHandle<JSTaggedValue> key(JSNApiHelper::ToJSMutableHandle(oriKey));
    JSTaggedValue keyValue = key.GetTaggedValue();
    EcmaVM *vm = thread->GetEcmaVM();
    if (key->IsString() && !EcmaStringAccessor(keyValue).IsInternString()) {
        // update string stable
        key.Update(JSTaggedValue(vm->GetFactory()->InternString(key)));
    }
    ASSERT(EcmaStringAccessor(key->GetTaggedObject()).IsInternString());
    ConstructDescByAttr(thread, oriAttr, &desc);
    // If property key is element index, property must be add by slow path(DefinePropertyOrThrow).
    if (UNLIKELY(!JSTaggedValue::IsPureString(const_cast<JSThread *>(thread), keyValue))) {
        return false;
    }
    // If property key is repeat key, property must be add by slow path.
    JSHandle<ecmascript::LayoutInfo> layoutInfoHandle(thread, hClass->GetLayout(thread));
    if (UNLIKELY(layoutInfoHandle->CheckIsDuplicateKey(thread, attrOffset, key->GetKeyHashCode(thread), keyValue))) {
        return false;
    }
    ASSERT(static_cast<int32_t>(attrOffset) == hClass->GetNextInlinedPropsIndex());
    JSHClass::AddInlinedPropToHClass(thread, desc, attrOffset++, key, hClass);
    return true;
}

JSHandle<JSObject> JSNApiClassCreationHelper::NewClassFuncProtoWithProperties(
    JSThread *thread, const JSHandle<JSFunction> &func, size_t propertyCount, size_t nonStaticPropCount,
    const Local<JSValueRef> *keys, PropertyAttribute *attrs, PropertyDescriptor *descs)
{
    ASSERT(propertyCount >= nonStaticPropCount);
    JSHandle<GlobalEnv> env = thread->GetGlobalEnv();
    size_t inlNonStaticPropCount =
        std::min(nonStaticPropCount, static_cast<size_t>(ecmascript::PropertyAttributes::MAX_FAST_PROPS_CAPACITY));
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> classPrototypeHClass = factory->CreateClassFuncProtoHClass(thread, inlNonStaticPropCount);
    // Step1: create hClass of protoOrHClass in class function
    // Set "constructor" in prototype
    PropertyDescriptor ctorDesc(thread, JSHandle<JSTaggedValue>::Cast(func), true, false, true);
    ASSERT(classPrototypeHClass->GetNextInlinedPropsIndex() != -1);
    const size_t startInlPropIdx = static_cast<size_t>(classPrototypeHClass->GetNextInlinedPropsIndex());
    JSHandle<JSTaggedValue> ctorKey = thread->GlobalConstants()->GetHandledConstructorString();
    JSHClass::AddInlinedPropToHClass(thread, ctorDesc, startInlPropIdx, ctorKey, classPrototypeHClass);
    // Set inlined property slot of non-static properties
    const size_t nonStaticPropStartIdx = propertyCount - 1; // 1: array length of keys and attrs is propertyCount
    std::bitset<ecmascript::PropertyAttributes::MAX_FAST_PROPS_CAPACITY> propTags;
    if (inlNonStaticPropCount > 0) {
        ASSERT(propertyCount >= inlNonStaticPropCount);
        size_t nextInlinedPropIdx = startInlPropIdx + 1; // 1: constructor attr set first
        // Properties in keys and attrs are stored in the following way:
        // +------------------+ ... + -------------------------------------+ ... +---------------------+
        // | first staticProp | ... | last staticProp | last nonStaticProp | ... | first nonStaticProp |
        // +------------------+ ... + -------------------------------------+ ... +---------------------+
        for (size_t i = 0; i < inlNonStaticPropCount; ++i) {
            size_t curNonStaticPropIdx = nonStaticPropStartIdx - i;
            propTags[curNonStaticPropIdx] =
                TryAddOriKeyAndOriAttrToHClass(thread, keys[curNonStaticPropIdx], attrs[curNonStaticPropIdx],
                                               descs[curNonStaticPropIdx], nextInlinedPropIdx, classPrototypeHClass);
        }
    }
    // Step2: create protoOrHClass object in terms of the created hclass
    JSHandle<JSObject> classPrototype = factory->NewJSObjectWithInit(classPrototypeHClass);
    // Step3: set values of inlined properties
    size_t curInlPropIdx = startInlPropIdx;
    classPrototype->SetPropertyInlinedProps<true>(thread, curInlPropIdx++, ctorDesc.GetValue().GetTaggedValue());
    if (inlNonStaticPropCount > 0) {
        JSHandle<JSTaggedValue> classPrototypeHandle(classPrototype);
        for (size_t i = 0; i < inlNonStaticPropCount; ++i) {
            size_t curNonStaticPropIdx = nonStaticPropStartIdx - i;
            if (propTags[curNonStaticPropIdx]) {
                classPrototype->SetPropertyInlinedProps<true>(thread, curInlPropIdx++,
                                                              descs[curNonStaticPropIdx].GetValue().GetTaggedValue());
            } else {
                JSTaggedValue::DefinePropertyOrThrow(thread, classPrototypeHandle,
                                                     JSNApiHelper::ToJSMutableHandle(keys[curNonStaticPropIdx]),
                                                     descs[curNonStaticPropIdx]);
            }
            // Corresponds to ConstructDescByAttr in TryAddOriKeyAndOriAttrToHClass
            DestructDesc(&descs[curNonStaticPropIdx]);
            DestructAttr(&attrs[curNonStaticPropIdx]);
        }
    }
    // Add non-static properties that out bound of MAX_FAST_PROPS_CAPACITY
    if (nonStaticPropCount > ecmascript::PropertyAttributes::MAX_FAST_PROPS_CAPACITY) {
        JSHandle<JSTaggedValue> classPrototypeHandle(classPrototype);
        for (size_t i = inlNonStaticPropCount; i < nonStaticPropCount; ++i) {
            size_t curNonStaticPropIdx = nonStaticPropStartIdx - i;
            ConstructDescByAttr(thread, attrs[curNonStaticPropIdx], &descs[curNonStaticPropIdx]);
            JSTaggedValue::DefinePropertyOrThrow(thread, classPrototypeHandle,
                                                 JSNApiHelper::ToJSMutableHandle(keys[curNonStaticPropIdx]),
                                                 descs[curNonStaticPropIdx]);
            DestructDesc(&descs[curNonStaticPropIdx]);
            DestructAttr(&attrs[curNonStaticPropIdx]);
        }
    }
    return classPrototype;
}

JSHandle<JSFunction> JSNApiClassCreationHelper::CreateClassFuncWithProperties(
    JSThread *thread, const char *name, InternalFunctionCallback nativeFunc, bool callNapi, size_t propertyCount,
    size_t staticPropCount, const Local<JSValueRef> *keys, PropertyAttribute *attrs, PropertyDescriptor *descs)
{
    ASSERT(propertyCount >= staticPropCount);
    size_t inlinedStaticPropCount =
        std::min(staticPropCount, static_cast<size_t>(ecmascript::PropertyAttributes::MAX_FAST_PROPS_CAPACITY));
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> functionClass = factory->CreateClassFuncHClass(thread, inlinedStaticPropCount);
    // Step1: create hClass of class function
    // Set name of class function
    JSHandle<JSTaggedValue> functionName(factory->NewFromUtf8(name));
    PropertyDescriptor nameDesc(thread, functionName, false, false, true);
    ASSERT(functionClass->GetNextInlinedPropsIndex() != -1);
    const size_t startInlPropIdx = static_cast<size_t>(functionClass->GetNextInlinedPropsIndex());
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();
    JSHClass::AddInlinedPropToHClass(thread, nameDesc, startInlPropIdx, nameKey, functionClass);
    // Set static properties of class function
    std::bitset<ecmascript::PropertyAttributes::MAX_FAST_PROPS_CAPACITY> propTags;
    size_t nextInlinedPropIdx = startInlPropIdx + 1; // 1: first static property was set after class function name
    // Properties in keys and attrs are stored in the following way:
    // +------------------+ ... + -------------------------------------+ ... +---------------------+
    // | first staticProp | ... | last staticProp | last nonStaticProp | ... | first nonStaticProp |
    // +------------------+ ... + -------------------------------------+ ... +---------------------+
    for (size_t i = 0; i < inlinedStaticPropCount; ++i) {
        propTags[i] =
            TryAddOriKeyAndOriAttrToHClass(thread, keys[i], attrs[i], descs[i], nextInlinedPropIdx, functionClass);
    }
    // Step2: create object of class function
    JSHandle<JSFunction> classFunc = factory->NewJSFunctionByHClass(reinterpret_cast<void *>(nativeFunc), functionClass,
                                                                    ecmascript::FunctionKind::CLASS_CONSTRUCTOR);
    // Step3: set values of inlined properties
    size_t curInlPropIdx = startInlPropIdx;
    classFunc->SetPropertyInlinedProps<true>(thread, curInlPropIdx++, nameDesc.GetValue().GetTaggedValue());
    JSHandle<JSTaggedValue> classFuncHandle(classFunc);
    for (size_t i = 0; i < inlinedStaticPropCount; ++i) {
        if (propTags[i]) {
            classFunc->SetPropertyInlinedProps<true>(thread, curInlPropIdx++,
                                                     descs[i].GetValue().GetTaggedValue());
        } else {
            JSTaggedValue::DefinePropertyOrThrow(thread, classFuncHandle, JSNApiHelper::ToJSMutableHandle(keys[i]),
                                                 descs[i]);
        }
        DestructDesc(&descs[i]); // Corresponds to ConstructDescByAttr in TryAddOriKeyAndOriAttrToHClass
        DestructAttr(&attrs[i]);
    }
    // Add static properties that out bound of MAX_FAST_PROPS_CAPACITY
    for (size_t i = inlinedStaticPropCount; i < staticPropCount; ++i) {
        ConstructDescByAttr(thread, attrs[i], &descs[i]);
        JSTaggedValue::DefinePropertyOrThrow(thread, classFuncHandle, JSNApiHelper::ToJSMutableHandle(keys[i]),
                                             descs[i]);
        DestructDesc(&descs[i]);
        DestructAttr(&attrs[i]);
    }

    JSHandle<JSObject> clsPrototype = NewClassFuncProtoWithProperties(
        thread, classFunc, propertyCount, propertyCount - staticPropCount, keys, attrs, descs);
    JSFunction::InitClassFunctionWithClsPrototype(thread, classFunc, callNapi, clsPrototype);

    return classFunc;
}
} // namespace panda::ecmascript
