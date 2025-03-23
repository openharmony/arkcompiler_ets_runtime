/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_OBJECT_OPERATOR_H
#define ECMASCRIPT_OBJECT_OPERATOR_H

#include "ecmascript/js_handle.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/property_attributes.h"

#include "ecmascript/ecma_string.h"
#include "libpandabase/utils/bit_field.h"

namespace panda::ecmascript {
class PropertyDescriptor;
class JSObject;
using SCheckMode = JSShared::SCheckMode;

enum class OperatorType : uint8_t {
    PROTOTYPE_CHAIN,
    OWN,
};

class ObjectOperator final {
public:
    ObjectOperator() = default;

    ObjectOperator(JSThread *thread, const JSHandle<JSTaggedValue> &key,
                   OperatorType type = OperatorType::PROTOTYPE_CHAIN);

    ObjectOperator(JSThread *thread, const JSHandle<JSObject> &holder, const JSHandle<JSTaggedValue> &key,
                   OperatorType type = OperatorType::PROTOTYPE_CHAIN);

    ObjectOperator(JSThread *thread, const JSHandle<JSTaggedValue> &holder, const JSHandle<JSTaggedValue> &key,
                   OperatorType type = OperatorType::PROTOTYPE_CHAIN);

    ObjectOperator(JSThread *thread, const JSHandle<JSTaggedValue> &holder,
                   const JSHandle<JSTaggedValue> &receiver, const JSHandle<JSTaggedValue> &key,
                   OperatorType type = OperatorType::PROTOTYPE_CHAIN);

    ObjectOperator(JSThread *thread, const JSHandle<JSTaggedValue> &holder, uint32_t index,
                   OperatorType type = OperatorType::PROTOTYPE_CHAIN);
    // op for fast path, name can only string and symbol, and can't be number.
    ObjectOperator(JSThread *thread, const JSTaggedValue &receiver, const JSTaggedValue &name,
                   OperatorType type = OperatorType::PROTOTYPE_CHAIN);
    // op for fast add
    ObjectOperator(JSThread *thread, const JSTaggedValue &receiver, const JSTaggedValue &name,
                   const PropertyAttributes &attr);

    static void FastAdd(JSThread *thread, const JSTaggedValue &receiver, const JSTaggedValue &name,
                        const JSHandle<JSTaggedValue> &value, const PropertyAttributes &attr);

    void UpdateDetector();
    static void PUBLIC_API UpdateDetector(const JSThread *thread, JSTaggedValue receiver, JSTaggedValue key);
    static void UpdateDetectorOnSetPrototype(const JSThread *thread, JSTaggedValue receiver);
    static bool IsDetectorName(const JSThread *thread, JSTaggedValue key);

    NO_COPY_SEMANTIC(ObjectOperator);
    DEFAULT_NOEXCEPT_MOVE_SEMANTIC(ObjectOperator);
    ~ObjectOperator() = default;

    /**
     * Create ObjectOperator instance by new operator is forbidden, for the member holder is a JSHandle type. it must
     * be created and destroyed on stack
     */
    void *operator new([[maybe_unused]] size_t t) = delete;
    void operator delete([[maybe_unused]] void *ptr) = delete;

    inline bool IsFound() const
    {
        return index_ != NOT_FOUND_INDEX;
    }

    inline bool IsFastMode() const
    {
        return IsFastModeField::Get(metaData_);
    }

    inline void SetFastMode(bool flag)
    {
        IsFastModeField::Set(flag, &metaData_);
    }

    inline void SetFoundDict(bool flag)
    {
        IsFoundDictField::Set(flag, &metaData_);
    }

    inline bool IsFoundDict() const
    {
        return IsFoundDictField::Get(metaData_);
    }

    inline bool IsElement() const
    {
        return key_.IsEmpty();
    }

    inline bool CheckValidIndexOrKeyIsLength() const
    {
        // if property key is element, check element index < strLength
        if (key_.IsEmpty()) {
            uint32_t len = EcmaStringAccessor(holder_->GetTaggedObject()).GetLength();
            return elementIndex_ < len;
        }
        // if property key is string, check key is 'length' of str
        if (key_->IsString() && receiver_->IsString()) {
            JSTaggedValue lenKey = thread_->GlobalConstants()->GetLengthString();
            EcmaString *proKey = EcmaString::Cast(key_->GetTaggedObject());
            return EcmaStringAccessor::StringsAreEqual(proKey, EcmaString::Cast(lenKey.GetTaggedObject()));
        }
        return false;
    }

    inline bool IsOnPrototype() const
    {
        return IsOnPrototypeField::Get(metaData_);
    }

    inline void SetIsOnPrototype(bool flag)
    {
        IsOnPrototypeField::Set(flag, &metaData_);
    }

    inline bool HasReceiver() const
    {
        return HasReceiverField::Get(metaData_);
    }

    inline void SetHasReceiver(bool flag)
    {
        HasReceiverField::Set(flag, &metaData_);
    }

    inline bool IsTransition() const
    {
        return IsTransitionField::Get(metaData_);
    }

    inline void SetIsTransition(bool flag)
    {
        IsTransitionField::Set(flag, &metaData_);
    }

    inline PropertyAttributes GetAttr() const
    {
        return attributes_;
    }

    inline void SetAttr(uint64_t attr)
    {
        attributes_ = PropertyAttributes(attr);
    }

    inline void SetAttr(const PropertyAttributes &attr)
    {
        attributes_ = attr;
    }

    inline bool IsPrimitiveAttr() const
    {
        return !attributes_.GetValue();
    }

    inline bool IsWritable() const
    {
        return GetAttr().IsWritable();
    }

    inline bool IsEnumerable() const
    {
        return GetAttr().IsEnumerable();
    }

    inline bool IsConfigurable() const
    {
        return GetAttr().IsConfigurable();
    }

    inline bool IsAccessorDescriptor() const
    {
        return GetAttr().IsAccessor();
    }

    inline bool IsInlinedProps() const
    {
        return GetAttr().IsInlinedProps();
    }

    inline void SetIsInlinedProps(bool flag)
    {
        attributes_.SetIsInlinedProps(flag);
    }

    inline Representation GetRepresentation() const
    {
        return GetAttr().GetRepresentation();
    }

    inline JSTaggedValue GetValue() const
    {
        if (value_.IsEmpty()) {
            return JSTaggedValue::Undefined();
        }
        return value_.GetTaggedValue();
    }

    JSHandle<JSTaggedValue> FastGetValue();
    inline void SetValue(JSTaggedValue value)
    {
        if (value_.IsEmpty()) {
            value_ = JSMutableHandle<JSTaggedValue>(thread_, value);
        }
        value_.Update(value);
    }

    inline void SetIndex(uint32_t index)
    {
        index_ = index;
    }

    inline uint32_t GetIndex() const
    {
        return index_;
    }

    inline void SetElementOutOfBounds(bool val)
    {
        elementsOutOfBounds_ = val;
    }

    inline bool GetElementOutOfBounds() const
    {
        return elementsOutOfBounds_;
    }

    inline bool HasHolder() const
    {
        return !holder_.IsEmpty();
    }

    inline JSHandle<JSTaggedValue> GetHolder() const
    {
        return holder_;
    }

    inline JSHandle<JSTaggedValue> GetReceiver() const
    {
        return receiver_;
    }

    inline JSHandle<JSTaggedValue> GetKey() const
    {
        if (key_.IsEmpty()) {
            return JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Undefined());
        }
        return key_;
    }

    inline bool KeyFromStringType() const
    {
        return keyFromStringType_;
    }

    inline uint32_t GetElementIndex() const
    {
        return elementIndex_;
    }

    inline JSThread *GetThread() const
    {
        return thread_;
    }

    void ToPropertyDescriptor(PropertyDescriptor &desc) const;
    SharedFieldType GetSharedFieldType() const;
    void LookupProperty();
    void GlobalLookupProperty();
    inline void ReLookupPropertyInReceiver()
    {
        ResetState();
        if (IsElement()) {
            LookupElementInlinedProps(JSHandle<JSObject>(receiver_));
        } else {
            LookupPropertyInlinedProps(JSHandle<JSObject>(receiver_));
        }
    }
    inline void SetAsDefaultAttr()
    {
        SetFound(NOT_FOUND_INDEX, JSTaggedValue::Undefined(), PropertyAttributes::GetDefaultAttributes(), false, false);
    }
    bool UpdateDataValue(const JSHandle<JSObject> &receiver, const JSHandle<JSTaggedValue> &value,
                         bool isInternalAccessor, bool mayThrow = false);
    bool WriteDataPropertyInHolder(const PropertyDescriptor &desc)
    {
        JSHandle<JSObject> receiver(holder_);
        return WriteDataProperty(receiver, desc);
    }
    bool WriteDataProperty(const JSHandle<JSObject> &receiver, const PropertyDescriptor &desc);
    bool AddProperty(const JSHandle<JSObject> &receiver, const JSHandle<JSTaggedValue> &value, PropertyAttributes attr);
    inline bool AddPropertyInHolder(const JSHandle<JSTaggedValue> &value, PropertyAttributes attr)
    {
        JSHandle<JSObject> obj(holder_);
        return AddProperty(obj, value, attr);
    }
    void DeletePropertyInHolder();
    static constexpr uint32_t NOT_FOUND_INDEX = std::numeric_limits<uint32_t>::max();
    static JSTaggedValue ToHolder(const JSHandle<JSTaggedValue> &holder);
    void AddPropertyInternal(const JSHandle<JSTaggedValue> &value);
    void DefineSetter(const JSHandle<JSTaggedValue> &value);
    void DefineGetter(const JSHandle<JSTaggedValue> &value);

private:
    using IsFastModeField = BitField<bool, 0, 1>;
    using IsOnPrototypeField = IsFastModeField::NextFlag;  // 1: on prototype
    using HasReceiverField = IsOnPrototypeField::NextFlag;
    using IsTransitionField = HasReceiverField::NextFlag;
    // found dictionary obj between receriver and holder
    using IsFoundDictField = IsTransitionField::NextFlag;

    void UpdateHolder();
    void StartLookUp(OperatorType type);
    void StartGlobalLookUp(OperatorType type);
    bool TryFastHandleStringKey(const JSHandle<JSTaggedValue> &key);
    void HandleKey(const JSHandle<JSTaggedValue> &key);
    uint32_t ComputeElementCapacity(uint32_t oldCapacity);
    void SetFound(uint32_t index, JSTaggedValue value, uint64_t attr, bool mode, bool transition = false);
    void UpdateFound(uint32_t index, uint64_t attr, bool mode, bool transition);
    void ResetState();
    void ResetStateForAddProperty();
    inline void LookupPropertyInHolder()
    {
        JSHandle<JSObject> obj(holder_);
        IsElement()
            ? LookupElementInlinedProps(obj)
            : LookupPropertyInlinedProps(obj);
    }
    inline void GlobalLookupPropertyInHolder()
    {
        JSHandle<JSObject> obj(holder_);
        LookupGlobal(obj);
    }
    template<bool isElement> void TryLookupInProtoChain();
    void LookupGlobal(const JSHandle<JSObject> &obj);
    void LookupPropertyInlinedProps(const JSHandle<JSObject> &obj);
    void LookupElementInlinedProps(const JSHandle<JSObject> &obj);
    void WriteElement(const JSHandle<JSObject> &receiver, const PropertyDescriptor &desc);
    void WriteElement(const JSHandle<JSObject> &receiver, JSHandle<JSTaggedValue> value) const;
    void DeleteElementInHolder() const;
    bool UpdateValueAndDetails(const JSHandle<JSObject> &receiver, const JSHandle<JSTaggedValue> &value,
                               PropertyAttributes attr, bool attrChanged);
    void TransitionForAttributeChanged(const JSHandle<JSObject> &receiver, PropertyAttributes attr);
    bool SetTypedArrayPropByIndex(const JSHandle<JSObject> &receiver, const JSHandle<JSTaggedValue> &value);
    JSThread *thread_{nullptr};
    JSMutableHandle<JSTaggedValue> value_{};
    JSMutableHandle<JSTaggedValue> holder_{};
    JSMutableHandle<JSTaggedValue> receiver_{};
    JSHandle<JSTaggedValue> key_{};
    uint32_t elementIndex_{NOT_FOUND_INDEX};
    uint32_t index_{NOT_FOUND_INDEX};
    PropertyAttributes attributes_;
    uint32_t metaData_{0};
    int receiverHoleEntry_{-1};
    bool keyFromStringType_{false};
    bool elementsOutOfBounds_{false};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_OBJECT_OPERATOR_H
