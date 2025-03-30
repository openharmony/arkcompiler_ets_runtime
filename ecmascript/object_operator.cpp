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

#include "ecmascript/object_operator.h"

#include "ecmascript/global_dictionary-inl.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/object_fast_operator-inl.h"
#include "ecmascript/property_detector-inl.h"

namespace panda::ecmascript {
bool ObjectOperator::TryFastHandleStringKey(const JSHandle<JSTaggedValue> &key)
{
    if (!EcmaStringAccessor(key->GetTaggedObject()).IsInternString()) {
        return false;
    }
    if (EcmaStringAccessor(key->GetTaggedObject()).IsInteger()) {
        elementIndex_ = EcmaStringAccessor(key->GetTaggedObject()).GetIntegerCode();
        return true;
    }
    if (EcmaStringAccessor(key->GetTaggedObject()).GetLength() <= EcmaString::MAX_CACHED_INTEGER_SIZE) {
        // Since the range of hash values is of the int32 type,
        // the IsInteger() function can only accurately determine
        // a string whose length is less than or equal to MAX_CACHED_INTEGER_SIZE is not a number.
        key_ = key;
        return true;
    }
    return false;
}

void ObjectOperator::HandleKey(const JSHandle<JSTaggedValue> &key)
{
    if (key->IsInt()) {
        int32_t keyInt = key->GetInt();
        if (keyInt >= 0) {
            elementIndex_ = static_cast<uint32_t>(keyInt);
            return;
        }
        key_ = JSHandle<JSTaggedValue>::Cast(base::NumberHelper::NumberToString(thread_, JSTaggedValue(keyInt)));
        return;
    }

    if (key->IsString()) {
        keyFromStringType_ = true;
#ifdef ENABLE_NEXT_OPTIMIZATION
        if (TryFastHandleStringKey(key)) {
            return;
        }
#endif
        uint32_t index = 0;
        if (JSTaggedValue::StringToElementIndex(key.GetTaggedValue(), &index)) {
            ASSERT(index < JSObject::MAX_ELEMENT_INDEX);
            elementIndex_ = index;
            return;
        }
        if (EcmaStringAccessor(key->GetTaggedObject()).IsInternString()) {
            key_ = key;
            return;
        }
        key_ = JSHandle<JSTaggedValue>(thread_, thread_->GetEcmaVM()->GetFactory()->InternString(key));
        return;
    }

    if (key->IsDouble()) {
        double number = key->GetDouble();
        if (number >= 0 && number < JSObject::MAX_ELEMENT_INDEX) {
            auto integer = static_cast<uint32_t>(number);
            if (integer == number) {
                elementIndex_ = static_cast<uint32_t>(number);
                return;
            }
        }
        key_ = JSHandle<JSTaggedValue>::Cast(base::NumberHelper::NumberToString(thread_, key.GetTaggedValue()));
        if (!EcmaStringAccessor(key_.GetTaggedValue()).IsInternString()) {
                key_ = JSHandle<JSTaggedValue>(thread_, thread_->GetEcmaVM()->GetFactory()->InternString(key_));
        }
        return;
    }

    if (key->IsSymbol()) {
        key_ = key;
        return;
    }

    JSHandle<JSTaggedValue> keyHandle(thread_, JSTaggedValue::ToPrimitive(thread_, key, PREFER_STRING));
    RETURN_IF_ABRUPT_COMPLETION(thread_);
    if (key->IsSymbol()) {
        key_ = keyHandle;
        return;
    }
    key_ = JSHandle<JSTaggedValue>(thread_,
                                   thread_->GetEcmaVM()->GetFactory()->InternString(
                                       JSHandle<JSTaggedValue>::Cast(JSTaggedValue::ToString(thread_, keyHandle))));
}

void ObjectOperator::UpdateHolder()
{
#ifdef ENABLE_NEXT_OPTIMIZATION
    if (holder_->IsECMAObject()) {
        return;
    }
#endif
    if (holder_->IsString()) {
        if (CheckValidIndexOrKeyIsLength()) {
            // key is 'length' of string or key is index and this index < strLength
            JSHandle<JSTaggedValue> undefined = thread_->GlobalConstants()->GetHandledUndefined();
            holder_.Update(JSPrimitiveRef::StringCreate(thread_, holder_, undefined).GetTaggedValue());
            // holder updated
            return;
        } else {
            SetIsOnPrototype(true);
        }
    } else if (holder_->IsNumber()) {
        SetIsOnPrototype(true);
    }
    holder_.Update(JSTaggedValue::ToPrototypeOrObj(thread_, holder_).GetTaggedValue());
}

void ObjectOperator::StartLookUp(OperatorType type)
{
    UpdateHolder();

    if (type == OperatorType::OWN) {
        LookupPropertyInHolder();
    } else {
        // try find property in prototype chain
        LookupProperty();
    }
}

void ObjectOperator::StartGlobalLookUp(OperatorType type)
{
    UpdateHolder();

    if (type == OperatorType::OWN) {
        GlobalLookupPropertyInHolder();
    } else {
        GlobalLookupProperty();
    }
}

ObjectOperator::ObjectOperator(JSThread *thread, const JSHandle<JSTaggedValue> &key, OperatorType type)
    : thread_(thread),
      holder_(thread, thread->GetEcmaVM()->GetGlobalEnv()->GetGlobalObject()),
      receiver_(thread, thread->GetEcmaVM()->GetGlobalEnv()->GetGlobalObject())
{
    HandleKey(key);
    StartGlobalLookUp(type);
}

ObjectOperator::ObjectOperator(JSThread *thread, const JSHandle<JSObject> &holder, const JSHandle<JSTaggedValue> &key,
                               OperatorType type)
    : thread_(thread), holder_(thread, holder.GetTaggedValue()), receiver_(thread, holder.GetTaggedValue())
{
    HandleKey(key);
    StartLookUp(type);
}

ObjectOperator::ObjectOperator(JSThread *thread, const JSHandle<JSTaggedValue> &holder,
                               const JSHandle<JSTaggedValue> &key, OperatorType type)
    : thread_(thread), holder_(thread, holder.GetTaggedValue()), receiver_(thread, holder.GetTaggedValue())
{
    HandleKey(key);
    StartLookUp(type);
}

ObjectOperator::ObjectOperator(JSThread *thread, const JSHandle<JSTaggedValue> &holder, uint32_t index,
                               OperatorType type)
    : thread_(thread),
      holder_(thread, holder.GetTaggedValue()),
      receiver_(thread, holder.GetTaggedValue()),
      elementIndex_(index)
{
    StartLookUp(type);
}

ObjectOperator::ObjectOperator(JSThread *thread, const JSHandle<JSTaggedValue> &holder,
                               const JSHandle<JSTaggedValue> &receiver, const JSHandle<JSTaggedValue> &key,
                               OperatorType type)
    : thread_(thread), holder_(thread, holder.GetTaggedValue()), receiver_(thread, receiver.GetTaggedValue())
{
    SetHasReceiver(true);
    HandleKey(key);
    StartLookUp(type);
}

// op for fast path
ObjectOperator::ObjectOperator(JSThread *thread, const JSTaggedValue &receiver, const JSTaggedValue &name,
                               OperatorType type)
    : thread_(thread), holder_(thread, receiver), receiver_(thread, receiver), key_(thread, name)
{
    ASSERT(name.IsStringOrSymbol());
    StartLookUp(type);
}
JSHandle<JSTaggedValue> ObjectOperator::FastGetValue()
{
    ASSERT(IsFound() && !value_.IsEmpty());
    if (value_->IsPropertyBox()) {
        value_.Update(PropertyBox::Cast(value_->GetTaggedObject())->GetValue());
    }
    if (!IsAccessorDescriptor()) {
        return value_;
    }
    AccessorData *accessor = AccessorData::Cast(value_->GetTaggedObject());
    ASSERT(!accessor->IsInternal());
    // 8. Return Call(getter, Receiver).
    return JSHandle<JSTaggedValue>(thread_, JSObject::CallGetter(thread_, accessor, receiver_));
}
ObjectOperator::ObjectOperator(JSThread *thread, const JSTaggedValue &receiver, const JSTaggedValue &name,
                               const PropertyAttributes &attr)
    : thread_(thread), receiver_(thread, receiver), key_(thread, name)
{
    SetAttr(attr);
}
void ObjectOperator::FastAdd(JSThread *thread, const JSTaggedValue &receiver, const JSTaggedValue &name,
                             const JSHandle<JSTaggedValue> &value, const PropertyAttributes &attr)
{
    ObjectOperator op(thread, receiver, name, attr);
    op.AddPropertyInternal(value);
}

// static
void ObjectOperator::UpdateDetectorOnSetPrototype(const JSThread *thread, JSTaggedValue receiver)
{
    // skip env prepare
    if (!thread->IsReadyToUpdateDetector()) {
        return;
    }
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHClass *hclass = receiver.GetTaggedObject()->GetClass();
    JSType type = hclass->GetObjectType();
    switch (type) {
        case JSType::JS_REG_EXP: {
            if (PropertyDetector::IsRegExpReplaceDetectorValid(env)) {
                PropertyDetector::InvalidateRegExpReplaceDetector(env);
            }
            if (PropertyDetector::IsRegExpFlagsDetectorValid(env)) {
                PropertyDetector::InvalidateRegExpFlagsDetector(env);
            }
            return;
        }
        case JSType::JS_MAP: {
            if (PropertyDetector::IsMapIteratorDetectorValid(env)) {
                PropertyDetector::InvalidateMapIteratorDetector(env);
            }
            return;
        }
        case JSType::JS_SET: {
            if (PropertyDetector::IsSetIteratorDetectorValid(env)) {
                PropertyDetector::InvalidateSetIteratorDetector(env);
            }
            return;
        }
        case JSType::JS_PRIMITIVE_REF: {
            if (JSPrimitiveRef::Cast(receiver.GetTaggedObject())->IsString() &&
                PropertyDetector::IsStringIteratorDetectorValid(env)) {
                PropertyDetector::InvalidateStringIteratorDetector(env);
            }
            return;
        }
        case JSType::JS_ARRAY: {
            if (PropertyDetector::IsArrayIteratorDetectorValid(env)) {
                PropertyDetector::InvalidateArrayIteratorDetector(env);
            }
            return;
        }
        case JSType::JS_INT8_ARRAY:
        case JSType::JS_UINT8_ARRAY:
        case JSType::JS_UINT8_CLAMPED_ARRAY:
        case JSType::JS_INT16_ARRAY:
        case JSType::JS_UINT16_ARRAY:
        case JSType::JS_INT32_ARRAY:
        case JSType::JS_UINT32_ARRAY:
        case JSType::JS_FLOAT32_ARRAY:
        case JSType::JS_FLOAT64_ARRAY:
        case JSType::JS_BIGINT64_ARRAY:
        case JSType::JS_BIGUINT64_ARRAY: {
            if (PropertyDetector::IsTypedArrayIteratorDetectorValid(env)) {
                PropertyDetector::InvalidateTypedArrayIteratorDetector(env);
            }
            if (PropertyDetector::IsTypedArraySpeciesProtectDetectorValid(env)) {
                PropertyDetector::InvalidateTypedArraySpeciesProtectDetector(env);
            }
            return;
        }
        default:
            break;
    }

    if (hclass->IsPrototype() &&
        (receiver == env->GetTaggedInt8ArrayFunctionPrototype() ||
         receiver == env->GetTaggedUint8ArrayFunctionPrototype() ||
         receiver == env->GetTaggedUint8ClampedArrayFunctionPrototype() ||
         receiver == env->GetTaggedInt16ArrayFunctionPrototype() ||
         receiver == env->GetTaggedUint16ArrayFunctionPrototype() ||
         receiver == env->GetTaggedInt32ArrayFunctionPrototype() ||
         receiver == env->GetTaggedUint32ArrayFunctionPrototype() ||
         receiver == env->GetTaggedFloat32ArrayFunctionPrototype() ||
         receiver == env->GetTaggedFloat64ArrayFunctionPrototype() ||
         receiver == env->GetTaggedBigInt64ArrayFunctionPrototype() ||
         receiver == env->GetTaggedBigUint64ArrayFunctionPrototype()) &&
         PropertyDetector::IsTypedArrayIteratorDetectorValid(env)) {
        PropertyDetector::InvalidateTypedArrayIteratorDetector(env);
        return;
    }
    if ((PropertyDetector::IsNumberStringNotRegexpLikeDetectorValid(env) &&
        JSObject::Cast(receiver)->GetJSHClass()->IsPrototype() && receiver.IsJSPrimitive())) {
        PropertyDetector::InvalidateNumberStringNotRegexpLikeDetector(env);
        return;
    }
}

void ObjectOperator::UpdateDetector()
{
    if (IsElement()) {
        return;
    }
    ObjectOperator::UpdateDetector(thread_, holder_.GetTaggedValue(), key_.GetTaggedValue());
}

// static
void ObjectOperator::UpdateDetector(const JSThread *thread, JSTaggedValue receiver, JSTaggedValue key)
{
    // skip env prepare
    if (!thread->IsReadyToUpdateDetector()) {
        return;
    }
    bool maybeDetector = IsDetectorName(thread, key);
    if (!maybeDetector) {
        return;
    }
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    auto globalConst = thread->GlobalConstants();
    if (key == env->GetTaggedReplaceSymbol() &&
        (receiver.IsJSRegExp() || receiver == env->GetTaggedRegExpPrototype())) {
        if (!PropertyDetector::IsRegExpReplaceDetectorValid(env)) {
            return;
        }
        PropertyDetector::InvalidateRegExpReplaceDetector(env);
    } else if (key == env->GetTaggedIteratorSymbol()) {
        if (receiver.IsJSMap() || receiver == env->GetTaggedMapPrototype()) {
            if (!PropertyDetector::IsMapIteratorDetectorValid(env)) {
                return;
            }
            PropertyDetector::InvalidateMapIteratorDetector(env);
        } else if (receiver.IsJSSet() || receiver == env->GetTaggedSetPrototype()) {
            if (!PropertyDetector::IsSetIteratorDetectorValid(env)) {
                return;
            }
            PropertyDetector::InvalidateSetIteratorDetector(env);
        } else if ((receiver.IsJSPrimitiveRef() && JSPrimitiveRef::Cast(receiver.GetTaggedObject())->IsString()) ||
                   receiver == env->GetTaggedStringPrototype()) {
            if (!PropertyDetector::IsStringIteratorDetectorValid(env)) {
                return;
            }
            PropertyDetector::InvalidateStringIteratorDetector(env);
        } else if (receiver.IsJSArray() || receiver == env->GetTaggedArrayPrototype()) {
            if (!PropertyDetector::IsArrayIteratorDetectorValid(env)) {
                return;
            }
            PropertyDetector::InvalidateArrayIteratorDetector(env);
        } else if (receiver.IsTypedArray() ||
                   receiver == env->GetTaggedArrayPrototype() ||
                   receiver == env->GetTaggedInt8ArrayFunctionPrototype() ||
                   receiver == env->GetTaggedUint8ArrayFunctionPrototype() ||
                   receiver == env->GetTaggedUint8ClampedArrayFunctionPrototype() ||
                   receiver == env->GetTaggedInt16ArrayFunctionPrototype() ||
                   receiver == env->GetTaggedUint16ArrayFunctionPrototype() ||
                   receiver == env->GetTaggedInt32ArrayFunctionPrototype() ||
                   receiver == env->GetTaggedUint32ArrayFunctionPrototype() ||
                   receiver == env->GetTaggedFloat32ArrayFunctionPrototype() ||
                   receiver == env->GetTaggedFloat64ArrayFunctionPrototype() ||
                   receiver == env->GetTaggedBigInt64ArrayFunctionPrototype() ||
                   receiver == env->GetTaggedBigUint64ArrayFunctionPrototype()) {
            if (!PropertyDetector::IsTypedArrayIteratorDetectorValid(env)) {
                return;
            }
            PropertyDetector::InvalidateTypedArrayIteratorDetector(env);
        }
    } else if (key == env->GetTaggedSpeciesSymbol()) {
        if (receiver == env->GetTypedArrayFunction().GetTaggedValue()) {
            if (!PropertyDetector::IsTypedArraySpeciesProtectDetectorValid(env)) {
                return;
            }
            PropertyDetector::InvalidateTypedArraySpeciesProtectDetector(env);
        }
        if (receiver == env->GetRegExpFunction().GetTaggedValue()) {
            if (!PropertyDetector::IsRegExpSpeciesDetectorValid(env)) {
                return;
            }
            PropertyDetector::InvalidateRegExpSpeciesDetector(env);
        }
    } else if ((key == env->GetTaggedReplaceSymbol()) ||
        (key == env->GetTaggedSplitSymbol()) ||
        (key == env->GetTaggedMatchAllSymbol())) {
        if (!PropertyDetector::IsNumberStringNotRegexpLikeDetectorValid(env)) {
            return;
        }
        // check String.prototype or Number.prototype or Object.prototype
        if ((JSObject::Cast(receiver)->GetJSHClass()->IsPrototype() &&
            (receiver.IsJSPrimitive() || receiver == env->GetTaggedObjectFunctionPrototype()))) {
            PropertyDetector::InvalidateNumberStringNotRegexpLikeDetector(env);
        }
    } else if (key == globalConst->GetHandledFlagsString().GetTaggedValue() &&
        (receiver.IsJSRegExp() || receiver == env->GetTaggedRegExpPrototype())) {
        if (!PropertyDetector::IsRegExpFlagsDetectorValid(env)) {
            return;
        }
        PropertyDetector::InvalidateRegExpFlagsDetector(env);
    }
}

// static
bool ObjectOperator::IsDetectorName(const JSThread *thread, JSTaggedValue key)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    uintptr_t start = GlobalEnv::GetFirstDetectorSymbolAddr(*env);
    uintptr_t end = GlobalEnv::GetLastDetectorSymbolAddr(*env);
    uintptr_t addr = key.GetRawData();
    if ((start <= addr) && (addr <= end)) {
        return true;
    }
    if (key == thread->GlobalConstants()->GetHandledFlagsString().GetTaggedValue()) {
        return true;
    }
    return false;
}

SharedFieldType ObjectOperator::GetSharedFieldType() const
{
    return JSObject::Cast(holder_->GetTaggedObject())->GetJSHClass()->IsDictionaryMode()
                              ? attributes_.GetDictSharedFieldType()
                              : attributes_.GetSharedFieldType();
}

void ObjectOperator::ToPropertyDescriptor(PropertyDescriptor &desc) const
{
    if (!IsFound()) {
        return;
    }

    if (!IsAccessorDescriptor()) {
        desc.SetWritable(IsWritable());
        JSTaggedValue val = GetValue();
        desc.SetValue(JSHandle<JSTaggedValue>(thread_, val));
        desc.SetSharedFieldType(GetSharedFieldType());
    } else {
        auto result = GetValue();
        bool isPropertyBox = result.IsPropertyBox();
        if (isPropertyBox) {
            result = PropertyBox::Cast(result.GetTaggedObject())->GetValue();
        }
        AccessorData *accessor = AccessorData::Cast(result.GetTaggedObject());

        if (UNLIKELY(accessor->IsInternal())) {
            desc.SetWritable(IsWritable());
            auto val = accessor->CallInternalGet(thread_, JSHandle<JSObject>::Cast(GetHolder()));
            JSMutableHandle<JSTaggedValue> value(thread_, val);
            if (isPropertyBox) {
                JSHandle<PropertyBox> cell(value_);
                cell->SetValue(thread_, val);
                value.Update(cell);
            }
            desc.SetValue(value);
        } else {
            desc.SetGetter(JSHandle<JSTaggedValue>(thread_, accessor->GetGetter()));
            desc.SetSetter(JSHandle<JSTaggedValue>(thread_, accessor->GetSetter()));
        }
    }

    desc.SetEnumerable(IsEnumerable());
    desc.SetConfigurable(IsConfigurable());
}

void ObjectOperator::GlobalLookupProperty()
{
    GlobalLookupPropertyInHolder();
    if (IsFound()) {
        return;
    }
    IsElement()
        ? TryLookupInProtoChain<true>()
        : TryLookupInProtoChain<false>();
}

template<bool isElement>
void ObjectOperator::TryLookupInProtoChain()
{
    do {
        JSTaggedValue proto = JSTaggedValue::GetPrototype(thread_, holder_);
        RETURN_IF_ABRUPT_COMPLETION(thread_);
        if (!proto.IsHeapObject()) {
            return;
        }

        holder_.Update(proto);
        SetIsOnPrototype(true);
        if (holder_->IsJSProxy()) {
            return;
        }
        JSHandle<JSObject> obj(holder_);
        if constexpr (isElement) {
            LookupElementInlinedProps(obj);
        } else {
            LookupPropertyInlinedProps(obj);
        }
    } while (!IsFound());
}

void ObjectOperator::LookupProperty()
{
    if (holder_->IsJSProxy()) {
        return;
    }
    LookupPropertyInHolder();
    if (!IsFound()) {
        IsElement()
            ? TryLookupInProtoChain<true>()
            : TryLookupInProtoChain<false>();
    }
}

void ObjectOperator::LookupGlobal(const JSHandle<JSObject> &obj)
{
    ASSERT(obj->IsJSGlobalObject());
    if (IsElement()) {
        LookupElementInlinedProps(obj);
        return;
    }
    TaggedArray *array = TaggedArray::Cast(obj->GetProperties().GetTaggedObject());
    if (array->GetLength() == 0) {
        return;
    }
    GlobalDictionary *dict = GlobalDictionary::Cast(array);
    int entry = dict->FindEntry(key_.GetTaggedValue());
    if (entry == -1) {
        return;
    }
    JSTaggedValue value(dict->GetBox(entry));
    auto attr = dict->GetAttributes(entry).GetValue();
    SetFound(entry, value, attr, true);
}

void ObjectOperator::LookupPropertyInlinedProps(const JSHandle<JSObject> &obj)
{
    ASSERT(!IsElement());
    if (!obj.GetTaggedValue().IsJSObject()) {
        return;
    }

    if (obj->IsJSGlobalObject()) {
        DISALLOW_GARBAGE_COLLECTION;
        TaggedArray *array = TaggedArray::Cast(obj->GetProperties().GetTaggedObject());
        if (array->GetLength() == 0) {
            return;
        }

        GlobalDictionary *dict = GlobalDictionary::Cast(array);
        int entry = dict->FindEntry(key_.GetTaggedValue());
        if (entry == -1) {
            return;
        }

        JSTaggedValue value(dict->GetBox(entry));
        auto attr = dict->GetAttributes(entry).GetValue();
        SetFound(entry, value, attr, !IsFoundDict());
        return;
    }

    TaggedArray *array = TaggedArray::Cast(obj->GetProperties().GetTaggedObject());
    if (!array->IsDictionaryMode()) {
        JSHClass *jshclass = obj->GetJSHClass();
        int entry = JSHClass::FindPropertyEntry(thread_, jshclass, key_.GetTaggedValue());
        if (entry == -1) {
            return;
        }
        JSTaggedValue attrs = jshclass->GetLayout();
        LayoutInfo *layoutInfo = LayoutInfo::Cast(attrs.GetTaggedObject());
        PropertyAttributes attr(layoutInfo->GetAttr(entry));
        ASSERT(entry == static_cast<int>(attr.GetOffset()));
        JSTaggedValue value;
        if (attr.IsInlinedProps()) {
            value = obj->GetPropertyInlinedPropsWithRep(entry, attr);
            if (value.IsHole()) {
                if (receiverHoleEntry_ == -1 && receiver_ == holder_) {
                    receiverHoleEntry_ = entry;
                }
                return;
            }
        } else {
            entry -= static_cast<int>(jshclass->GetInlinedProperties());
            value = array->Get(entry);
        }

        SetFound(entry, value, attr.GetValue(), !IsFoundDict());
        return;
    }
    SetFoundDict(true);
    NameDictionary *dict = NameDictionary::Cast(array);
    int entry = dict->FindEntry(key_.GetTaggedValue());
    if (entry == -1) {
        return;
    }

    JSTaggedValue value = dict->GetValue(entry);
    auto attr = dict->GetAttributes(entry).GetValue();
    SetFound(entry, value, attr, false);
}

void ObjectOperator::TransitionForAttributeChanged(const JSHandle<JSObject> &receiver, PropertyAttributes attr)
{
    if (IsElement()) {
        uint32_t index = GetIndex();
        if (!receiver->GetJSHClass()->IsDictionaryElement()) {
            JSObject::ElementsToDictionary(thread_, receiver);
            RETURN_IF_ABRUPT_COMPLETION(thread_);
            auto dict = NumberDictionary::Cast(receiver->GetElements().GetTaggedObject());
            index = static_cast<uint32_t>(dict->FindEntry(JSTaggedValue(index)));
            PropertyAttributes origin = dict->GetAttributes(index);
            attr.SetDictionaryOrder(origin.GetDictionaryOrder());
            dict->SetAttributes(thread_, index, attr);
        } else {
            auto dict = NumberDictionary::Cast(receiver->GetElements().GetTaggedObject());
            dict->SetAttributes(thread_, index, attr);
        }
        // update found result
        UpdateFound(index, attr.GetValue(), false, true);
    } else if (receiver->IsJSGlobalObject()) {
        uint32_t index = GetIndex();
        JSHandle<GlobalDictionary> dictHandle(thread_, receiver->GetProperties());
        dictHandle->SetAttributes(thread_, index, attr);
        GlobalDictionary::InvalidatePropertyBox(thread_, dictHandle, index);
    } else {
        uint32_t index = GetIndex();
        if (!receiver->GetJSHClass()->IsDictionaryMode()) {
            JSHandle<NameDictionary> dict(JSObject::TransitionToDictionary(thread_, receiver));
            RETURN_IF_ABRUPT_COMPLETION(thread_);
            index = static_cast<uint32_t>(dict->FindEntry(key_.GetTaggedValue()));
            PropertyAttributes origin = dict->GetAttributes(index);
            attr.SetDictSharedFieldType(attr.GetSharedFieldType());
            attr.SetDictionaryOrder(origin.GetDictionaryOrder());
            dict->SetAttributes(thread_, index, attr);
        } else {
            auto dict = NameDictionary::Cast(receiver->GetProperties().GetTaggedObject());
            dict->SetAttributes(thread_, index, attr);
        }
        // update found result
        UpdateFound(index, attr.GetValue(), false, true);
    }
}

bool ObjectOperator::UpdateValueAndDetails(const JSHandle<JSObject> &receiver, const JSHandle<JSTaggedValue> &value,
                                           PropertyAttributes attr, bool attrChanged)
{
    auto valueAccessor = GetValue();
    if (valueAccessor.IsPropertyBox()) {
        valueAccessor = PropertyBox::Cast(valueAccessor.GetTaggedObject())->GetValue();
    }
    bool isInternalAccessor = IsAccessorDescriptor()
        && AccessorData::Cast(valueAccessor.GetTaggedObject())->IsInternal();
    if (!attrChanged) {
        return UpdateDataValue(receiver, value, isInternalAccessor);
    }
    if (attr.IsWritable()) {
        TransitionForAttributeChanged(receiver, attr);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        return UpdateDataValue(receiver, value, isInternalAccessor);
    }
    bool res = UpdateDataValue(receiver, value, isInternalAccessor);
    if (res) {
        TransitionForAttributeChanged(receiver, attr);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
    }
    return res;
}

bool ObjectOperator::SetTypedArrayPropByIndex(const JSHandle<JSObject> &receiver, const JSHandle<JSTaggedValue> &value)
{
    JSTaggedValue holder = receiver.GetTaggedValue();
    JSType jsType = holder.GetTaggedObject()->GetClass()->GetObjectType();
    JSTaggedValue typedArrayProperty =
        JSTypedArray::FastSetPropertyByIndex(thread_, receiver.GetTaggedValue(),
                                             GetIndex(), value.GetTaggedValue(), jsType);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
    if (typedArrayProperty.IsHole()) {
        return false;
    }
    return true;
}

bool ObjectOperator::UpdateDataValue(const JSHandle<JSObject> &receiver, const JSHandle<JSTaggedValue> &value,
                                     bool isInternalAccessor, bool mayThrow)
{
    if (IsElement()) {
        TaggedArray *elements = TaggedArray::Cast(receiver->GetElements().GetTaggedObject());
        if (!elements->IsDictionaryMode()) {
            if (receiver.GetTaggedValue().IsJSCOWArray()) {
                JSArray::CheckAndCopyArray(thread_, JSHandle<JSArray>(receiver));
            } else if (receiver->IsTypedArray()) {
                return SetTypedArrayPropByIndex(receiver, value);
            }
            ElementsKind oldKind = receiver->GetClass()->GetElementsKind();
            if (JSHClass::TransitToElementsKind(thread_, receiver, value)) {
                SetIsTransition(true);
                ElementsKind newKind = receiver->GetClass()->GetElementsKind();
                // newKind != currentKind, we need to convert the whole array to the newKind.
                Elements::MigrateArrayWithKind(thread_, receiver, oldKind, newKind);
            }
            ElementAccessor::Set(thread_, receiver, GetIndex(), value, false);
            return true;
        }

        NumberDictionary *dict = NumberDictionary::Cast(elements);
        dict->UpdateValue(thread_, GetIndex(), value.GetTaggedValue());
        return true;
    }

    if (receiver->IsJSGlobalObject()) {
        // need update cell type ?
        auto *dict = GlobalDictionary::Cast(receiver->GetProperties().GetTaggedObject());
        if (isInternalAccessor && !value->IsAccessor()) {
            PropertyAttributes attr = dict->GetAttributes(GetIndex());
            attr.SetIsAccessor(false);
            dict->SetAttributes(thread_, GetIndex(), attr);
        }
        PropertyBox *cell = dict->GetBox(GetIndex());
        cell->SetValue(thread_, value.GetTaggedValue());
        return true;
    }

    if (isInternalAccessor) {
        auto accessor = AccessorData::Cast(GetValue().GetTaggedObject());
        if (accessor->HasSetter()) {
            bool res = accessor->CallInternalSet(thread_, JSHandle<JSObject>(receiver), value, mayThrow);
            if (receiver->GetJSHClass()->IsDictionaryMode()) {
                SetIsInlinedProps(false);
                SetFastMode(false);
            }
            return res;
        }
    }

    JSMutableHandle<TaggedArray> properties(thread_, TaggedArray::Cast(receiver->GetProperties().GetTaggedObject()));
    if (!properties->IsDictionaryMode()) {
        PropertyAttributes attr = GetAttr();
        uint32_t offset = index_;
        if (!attr.IsInlinedProps()) {
            auto *hclass = receiver_->GetTaggedObject()->GetClass();
            offset += hclass->GetInlinedProperties();
        }
        attr.SetOffset(offset);
        JSHandle<JSObject> objHandle(receiver_);
        ElementsKind oldKind = objHandle->GetJSHClass()->GetElementsKind();
        auto actualValue =
            JSHClass::ConvertOrTransitionWithRep(thread_, objHandle, key_, value, attr);
        JSObject::TryMigrateToGenericKindForJSObject(thread_, objHandle, oldKind);
        if (actualValue.isTransition) {
            SetIsTransition(true);
        }
        attributes_.SetRepresentation(attr.GetRepresentation());

        if (attr.IsInlinedProps()) {
            receiver->SetPropertyInlinedPropsWithRep(thread_, GetIndex(), actualValue.value);
        } else {
            if (receiver.GetTaggedValue().IsJSCOWArray()) {
                JSArray::CheckAndCopyArray(thread_, JSHandle<JSArray>(receiver));
                properties.Update(JSHandle<JSArray>(receiver)->GetProperties());
            }
            if (actualValue.isTagged) {
                properties->Set<true>(thread_, GetIndex(), value.GetTaggedValue());
            } else {
                properties->Set<false>(thread_, GetIndex(), actualValue.value);
            }
        }
    } else {
        properties.GetObject<NameDictionary>()->UpdateValue(thread_, GetIndex(), value.GetTaggedValue());
    }
    return true;
}

bool ObjectOperator::WriteDataProperty(const JSHandle<JSObject> &receiver, const PropertyDescriptor &desc)
{
    PropertyAttributes attr = GetAttr();
    bool attrChanged = false;

    // composed new attribute from desc
    if (desc.HasConfigurable() && attr.IsConfigurable() != desc.IsConfigurable()) {
        attr.SetConfigurable(desc.IsConfigurable());
        attrChanged = true;
    }
    if (desc.HasEnumerable() && attr.IsEnumerable() != desc.IsEnumerable()) {
        attr.SetEnumerable(desc.IsEnumerable());
        attrChanged = true;
    }

    if (!desc.IsAccessorDescriptor()) {
        if (desc.HasWritable() && attr.IsWritable() != desc.IsWritable()) {
            attr.SetWritable(desc.IsWritable());
            attrChanged = true;
        }
        if (!desc.HasValue()) {
            if (attrChanged) {
                TransitionForAttributeChanged(receiver, attr);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            }
            return true;
        }

        if (IsAccessorDescriptor()) {
            TaggedObject *obj = GetValue().GetTaggedObject();
            if (receiver->IsJSGlobalObject()) {
                JSTaggedValue val = GetValue();
                if (val.IsPropertyBox()) {
                    PropertyBox *cell = PropertyBox::Cast(val.GetTaggedObject());
                    obj = cell->GetValue().GetTaggedObject();
                }
            }
            auto accessor = AccessorData::Cast(obj);
            if (!accessor->IsInternal() || !accessor->HasSetter()) {
                attr.SetIsAccessor(false);
                attrChanged = true;
            }
        }

        return UpdateValueAndDetails(receiver, desc.GetValue(), attr, attrChanged);
    } else {
        auto valueAccessor = GetValue();
        if (valueAccessor.IsPropertyBox()) {
            valueAccessor = PropertyBox::Cast(valueAccessor.GetTaggedObject())->GetValue();
        }
        bool isNotInternalAccessor = IsAccessorDescriptor()
                && !AccessorData::Cast(valueAccessor.GetTaggedObject())->IsInternal();
        if (isNotInternalAccessor && !IsElement()) {
            TaggedArray *properties = TaggedArray::Cast(receiver->GetProperties().GetTaggedObject());
            if (attrChanged && !properties->IsDictionaryMode()) {
                // as some accessorData is in globalEnv, we need to new accessorData.
                JSHandle<AccessorData> accessor = thread_->GetEcmaVM()->GetFactory()->NewAccessorData();

                if (desc.HasGetter()) {
                    accessor->SetGetter(thread_, desc.GetGetter().GetTaggedValue());
                } else {
                    accessor->SetGetter(thread_, JSHandle<AccessorData>::Cast(value_)->GetGetter());
                }
                if (desc.HasSetter()) {
                    accessor->SetSetter(thread_, desc.GetSetter().GetTaggedValue());
                } else {
                    accessor->SetSetter(thread_, JSHandle<AccessorData>::Cast(value_)->GetSetter());
                }

                JSHandle<NameDictionary> dict(JSObject::TransitionToDictionary(thread_, receiver));
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
                int entry = dict->FindEntry(key_.GetTaggedValue());
                ASSERT(entry != -1);
                dict->UpdateValueAndAttributes(thread_, entry, accessor.GetTaggedValue(), attr);
                return true;
            }
        }

        JSHandle<AccessorData> accessor = isNotInternalAccessor ?
            JSHandle<AccessorData>(thread_, valueAccessor) :
            thread_->GetEcmaVM()->GetFactory()->NewAccessorData();
        if (desc.HasGetter()) {
            accessor->SetGetter(thread_, desc.GetGetter().GetTaggedValue());
        }

        if (desc.HasSetter()) {
            accessor->SetSetter(thread_, desc.GetSetter().GetTaggedValue());
        }

        if (!IsAccessorDescriptor()) {
            attr.SetIsAccessor(true);
            attrChanged = true;
        }

        JSHandle<JSTaggedValue> value = JSHandle<JSTaggedValue>::Cast(accessor);
        bool success = UpdateValueAndDetails(receiver, value, attr, attrChanged);
        if (success) {
            JSHandle<JSObject> obj(receiver);
            if (obj->GetJSHClass()->IsPrototype()) {
                JSHandle<ProtoChangeMarker> markerHandle = thread_->GetEcmaVM()->GetFactory()->NewProtoChangeMarker();
                obj->GetJSHClass()->SetProtoChangeMarker(thread_, markerHandle.GetTaggedValue());
            }
            JSHClass::NotifyAccessorChanged(thread_, JSHandle<JSHClass>(thread_, obj->GetJSHClass()));
        }
        return success;
    }
}

void ObjectOperator::DeletePropertyInHolder()
{
    if (IsElement()) {
        return DeleteElementInHolder();
    }
    ObjectOperator::UpdateDetector(thread_, holder_.GetTaggedValue(), key_.GetTaggedValue());
    JSObject::DeletePropertyInternal(thread_, JSHandle<JSObject>(holder_), key_, GetIndex());
}

bool ObjectOperator::AddProperty(const JSHandle<JSObject> &receiver, const JSHandle<JSTaggedValue> &value,
                                 PropertyAttributes attr)
{
    if (IsElement()) {
        ElementsKind oldKind = receiver->GetClass()->GetElementsKind();
        uint32_t oldLen = receiver.GetTaggedValue().IsJSArray() ?
            JSArray::Cast(*receiver)->GetArrayLength() : 0;
        bool ret = JSObject::AddElementInternal(thread_, receiver, elementIndex_, value, attr);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        ElementsKind newKind = receiver->GetClass()->GetElementsKind();
        uint32_t newLen = receiver.GetTaggedValue().IsJSArray() ?
            JSArray::Cast(*receiver)->GetArrayLength() : 0;
        SetElementOutOfBounds(newLen > oldLen);
        bool isTransited = false;
        if (receiver.GetTaggedValue().IsJSArray() && (newKind != oldKind)) {
            isTransited = true;
        }
        bool isDict = receiver->GetJSHClass()->IsDictionaryElement();
        SetFound(elementIndex_, value.GetTaggedValue(), attr.GetValue(), !isDict, isTransited);
        return ret;
    }

    ResetStateForAddProperty();
    receiver_.Update(receiver.GetTaggedValue());
    SetAttr(attr.GetValue());
    AddPropertyInternal(value);
    return true;
}

void ObjectOperator::WriteElement(const JSHandle<JSObject> &receiver, JSHandle<JSTaggedValue> value) const
{
    ASSERT(IsElement() && GetIndex() < JSObject::MAX_ELEMENT_INDEX);

    if (!ElementAccessor::IsDictionaryMode(receiver)) {
        ElementAccessor::Set(thread_, receiver, index_, value, true);
        return;
    }

    TaggedArray *elements = TaggedArray::Cast(receiver->GetElements().GetTaggedObject());
    NumberDictionary *dictionary = NumberDictionary::Cast(elements);
    dictionary->UpdateValue(thread_, GetIndex(), value.GetTaggedValue());
}

void ObjectOperator::DeleteElementInHolder() const
{
    JSHandle<JSObject> obj(holder_);
    if (obj->IsJSSArray()) {
        auto arrayHandler = JSHandle<JSSharedArray>::Cast(obj);
        JSSharedArray::DeleteInElementMode(thread_, arrayHandler);
        return;
    }
    JSHandle<JSTaggedValue> holeHandle(thread_, JSTaggedValue::Hole());
    if (!ElementAccessor::IsDictionaryMode(obj)) {
        if (obj.GetTaggedValue().IsJSCOWArray()) {
            JSArray::CheckAndCopyArray(thread_, JSHandle<JSArray>(obj));
        }
        ElementAccessor::Set(thread_, obj, index_, holeHandle, true, ElementsKind::HOLE);
    } else {
        TaggedArray *elements = TaggedArray::Cast(obj->GetElements().GetTaggedObject());
        JSHandle<NumberDictionary> dictHandle(thread_, elements);
        JSHandle<NumberDictionary> newDict = NumberDictionary::Remove(thread_, dictHandle, GetIndex());
        obj->SetElements(thread_, newDict);
    }
}

void ObjectOperator::SetFound(uint32_t index, JSTaggedValue value, uint64_t attr, bool mode, bool transition)
{
    SetIndex(index);
    SetValue(value);
    SetFastMode(mode);
    SetIsTransition(transition);
    SetAttr(attr);
}

void ObjectOperator::UpdateFound(uint32_t index, uint64_t attr, bool mode, bool transition)
{
    SetIndex(index);
    SetFastMode(mode);
    SetIsTransition(transition);
    SetAttr(attr);
}

void ObjectOperator::ResetState()
{
    // index may used by element
    SetIndex(NOT_FOUND_INDEX);
    SetValue(JSTaggedValue::Undefined());
    SetFastMode(false);
    SetAttr(0);
    SetIsOnPrototype(false);
    SetHasReceiver(false);
}

void ObjectOperator::ResetStateForAddProperty()
{
    bool isOnPrototype = IsOnPrototype();
    ResetState();
    SetIsOnPrototype(isOnPrototype);
}

void ObjectOperator::LookupElementInlinedProps(const JSHandle<JSObject> &obj)
{
    // if is js string, do special.
    if (obj->IsJSPrimitiveRef() && JSPrimitiveRef::Cast(obj.GetTaggedValue().GetTaggedObject())->IsString()) {
        PropertyDescriptor desc(thread_);
        bool status = JSPrimitiveRef::StringGetIndexProperty(thread_, obj, elementIndex_, &desc);
        if (status) {
            PropertyAttributes attr(desc);
            SetFound(elementIndex_, desc.GetValue().GetTaggedValue(), attr.GetValue(), !IsFoundDict());
            return;
        }
    }
    {
        DISALLOW_GARBAGE_COLLECTION;
        if (obj->IsTypedArray()) {
            JSTaggedValue val = JSTypedArray::FastElementGet(thread_,
                JSHandle<JSTaggedValue>::Cast(obj), elementIndex_).GetValue().GetTaggedValue();
            RETURN_IF_ABRUPT_COMPLETION(thread_);
            if (!val.IsHole()) {
                SetFound(elementIndex_, val, PropertyAttributes::GetDefaultAttributes(), !IsFoundDict());
            }
            return;
        }
        TaggedArray *elements = TaggedArray::Cast(obj->GetElements().GetTaggedObject());
        if (elements->GetLength() == 0) {
            return;  // Empty Array
        }

        if (!elements->IsDictionaryMode()) {
            if (elements->GetLength() <= elementIndex_) {
                return;
            }

            JSTaggedValue value = ElementAccessor::Get(thread_, obj, elementIndex_);
            if (value.IsHole()) {
                return;
            }
            SetFound(elementIndex_, value, PropertyAttributes::GetDefaultAttributes(), !IsFoundDict());
        } else {
            SetFoundDict(true);
            NumberDictionary *dictionary = NumberDictionary::Cast(obj->GetElements().GetTaggedObject());
            JSTaggedValue key(static_cast<int>(elementIndex_));
            int entry = dictionary->FindEntry(key);
            if (entry == -1) {
                return;
            }

            auto attr = dictionary->GetAttributes(entry).GetValue();
            SetFound(entry, dictionary->GetValue(entry), attr, false);
        }
    }
}

void ObjectOperator::AddPropertyInternal(const JSHandle<JSTaggedValue> &value)
{
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> obj(GetReceiver());
    PropertyAttributes attr = GetAttr();
    if (obj->IsJSGlobalObject()) {
        JSMutableHandle<GlobalDictionary> dict(thread_, obj->GetProperties());
        if (dict->GetLength() == 0) {
            dict.Update(GlobalDictionary::Create(thread_));
        }

        // Add PropertyBox to global dictionary
        JSHandle<PropertyBox> cellHandle = factory->NewPropertyBox(key_);
        cellHandle->SetValue(thread_, value.GetTaggedValue());
        PropertyBoxType cellType = value->IsUndefined() ? PropertyBoxType::UNDEFINED : PropertyBoxType::CONSTANT;
        attr.SetBoxType(cellType);

        JSHandle<GlobalDictionary> properties =
            GlobalDictionary::PutIfAbsent(thread_, dict, key_, JSHandle<JSTaggedValue>(cellHandle), attr);
        obj->SetProperties(thread_, properties);
        // index and fastMode is not essential for global obj;
        SetFound(0, cellHandle.GetTaggedValue(), attr.GetValue(), true);
        return;
    }

    // The property has already existed whose value is hole, initialized by speculative hclass.
    // Not need AddProperty,just SetProperty
    if (receiverHoleEntry_ != -1) {
        attr.SetOffset(receiverHoleEntry_);
        JSHandle<JSObject> objHandle(receiver_);
        ElementsKind oldKind = objHandle->GetJSHClass()->GetElementsKind();
        auto actualValue =
            JSHClass::ConvertOrTransitionWithRep(thread_, objHandle, key_, value, attr);
        JSObject::TryMigrateToGenericKindForJSObject(thread_, objHandle, oldKind);
        if (actualValue.isTransition) {
            SetIsTransition(true);
        }
        attributes_.SetRepresentation(attr.GetRepresentation());
        auto *hclass = receiver_->GetTaggedObject()->GetClass();
        if (actualValue.isTagged) {
            JSObject::Cast(receiver_.GetTaggedValue())->SetProperty<true>(thread_, hclass,
                                                                          attr, value.GetTaggedValue());
        } else {
            JSObject::Cast(receiver_.GetTaggedValue())->SetProperty<false>(thread_, hclass, attr, actualValue.value);
        }
        uint32_t index = attr.IsInlinedProps() ? attr.GetOffset() :
                attr.GetOffset() - obj->GetJSHClass()->GetInlinedProperties();
        SetIsOnPrototype(false);
        SetFound(index, value.GetTaggedValue(), attr.GetValue(), true);
        return;
    }

    attr = ObjectFastOperator::AddPropertyByName(thread_, obj, key_, value, attr);
    RETURN_IF_ABRUPT_COMPLETION(thread_);
    if (obj->GetJSHClass()->IsDictionaryMode()) {
        SetFound(0, value.GetTaggedValue(), attr.GetValue(), false);
    } else {
        uint32_t index = attr.IsInlinedProps() ? attr.GetOffset() :
                attr.GetOffset() - obj->GetJSHClass()->GetInlinedProperties();
        SetFound(index, value.GetTaggedValue(), attr.GetValue(), true, true);
    }
}

void ObjectOperator::DefineSetter(const JSHandle<JSTaggedValue> &value)
{
    ASSERT(IsAccessorDescriptor());
    JSHandle<AccessorData> accessor = JSHandle<AccessorData>::Cast(value_);
    accessor->SetSetter(thread_, value.GetTaggedValue());
    UpdateDataValue(JSHandle<JSObject>::Cast(receiver_), JSHandle<JSTaggedValue>::Cast(accessor), false);
}

void ObjectOperator::DefineGetter(const JSHandle<JSTaggedValue> &value)
{
    ASSERT(IsAccessorDescriptor());
    JSHandle<AccessorData> accessor = JSHandle<AccessorData>::Cast(value_);
    accessor->SetGetter(thread_, value.GetTaggedValue());
    UpdateDataValue(JSHandle<JSObject>::Cast(receiver_), JSHandle<JSTaggedValue>::Cast(accessor), false);
}
}  // namespace panda::ecmascript
