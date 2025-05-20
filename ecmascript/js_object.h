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

#ifndef ECMASCRIPT_JSOBJECT_H
#define ECMASCRIPT_JSOBJECT_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/enum_cache.h"
#include "ecmascript/filter_helper.h"
#include "ecmascript/ic/property_box.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/layout_visitor.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/method.h"
#include "ecmascript/property_attributes.h"
#include "ecmascript/tagged_array.h"

namespace panda {
namespace ecmascript {
class ObjectOperator;
class JSFunction;
class AccessorData;
class JSArray;
class JSForInIterator;
class LexicalEnv;
class GlobalEnv;
class TaggedQueue;
class NumberDictionary;
class DependentInfos;

namespace builtins {
    class BuiltinsArkTools;
}

using SCheckMode = JSShared::SCheckMode;

// Integrity level for objects
enum IntegrityLevel { SEALED, FROZEN };

enum PositionKind { UNKNOWN = 0, INDEXED_PROPERTY = 1, INLINE_NAMED_PROPERTY = 2, OUT_NAMED_PROPERTY = 3 };
enum PropertyKind { KEY = 0, VALUE, KEY_VALUE };

enum class TrackTypeUpdateMode { ENABLE, DISABLE };

// ecma6.0 6.2.4 The Property Descriptor Specification Type
class PropertyDescriptor final {
public:
    PropertyDescriptor() = delete;

    ~PropertyDescriptor() = default;
    DEFAULT_NOEXCEPT_MOVE_SEMANTIC(PropertyDescriptor);
    DEFAULT_COPY_SEMANTIC(PropertyDescriptor);

    explicit PropertyDescriptor(const JSThread *thread) : thread_(thread) {}

    PropertyDescriptor(const JSThread *thread, JSHandle<JSTaggedValue> v) : thread_(thread), value_(v) {}

    PropertyDescriptor(const JSThread *thread, JSHandle<JSTaggedValue> v, bool w, bool e, bool c)
        : thread_(thread),
          writable_(w),
          enumerable_(e),
          configurable_(c),
          hasWritable_(true),
          hasEnumerable_(true),
          hasConfigurable_(true),
          value_(v)
    {
    }

    PropertyDescriptor(const JSThread *thread, bool w, bool e, bool c)
        : PropertyDescriptor(thread, JSHandle<JSTaggedValue>(), w, e, c)
    {
    }

    inline JSHandle<JSTaggedValue> GetValue() const
    {
        if (value_.IsEmpty()) {
            return JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Undefined());
        }
        return value_;
    }

    inline JSHandle<JSTaggedValue> GetKey() const
    {
        if (key_.IsEmpty()) {
            return JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Undefined());
        }
        return key_;
    }

    inline void SetValue(JSHandle<JSTaggedValue> value)
    {
        value_ = value;
    }

    inline void SetKey(JSHandle<JSTaggedValue> key)
    {
        key_ = key;
    }

    inline void SetSharedFieldType(SharedFieldType fieldType)
    {
        fieldType_ = fieldType;
    }

    inline SharedFieldType GetSharedFieldType() const
    {
        return fieldType_;
    }

    inline bool IsWritable() const
    {
        return writable_;
    }

    inline void SetWritable(bool flag)
    {
        writable_ = flag;
        hasWritable_ = true;
    }

    inline bool IsEnumerable() const
    {
        return enumerable_;
    }

    inline void SetEnumerable(bool flag)
    {
        enumerable_ = flag;
        hasEnumerable_ = true;
    }

    inline bool IsConfigurable() const
    {
        return configurable_;
    }

    inline void SetConfigurable(bool flag)
    {
        configurable_ = flag;
        hasConfigurable_ = true;
    }

    inline bool HasValue() const
    {
        return !value_.IsEmpty();
    }

    inline bool HasWritable() const
    {
        return hasWritable_;
    }

    inline bool HasConfigurable() const
    {
        return hasConfigurable_;
    }

    inline bool HasEnumerable() const
    {
        return hasEnumerable_;
    }

    inline bool HasGetter() const
    {
        return !getter_.IsEmpty();
    }

    inline bool HasSetter() const
    {
        return !setter_.IsEmpty();
    }

    inline JSHandle<JSTaggedValue> GetGetter() const
    {
        if (getter_->IsNull()) {
            return JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Undefined());
        }
        return getter_;
    }

    inline JSHandle<JSTaggedValue> GetSetter() const
    {
        if (setter_->IsNull()) {
            return JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Undefined());
        }
        return setter_;
    }

    inline void SetGetter(JSHandle<JSTaggedValue> value)
    {
        getter_ = value;
    }

    inline void SetSetter(JSHandle<JSTaggedValue> value)
    {
        setter_ = value;
    }

    // 6.2.4.1
    inline bool IsAccessorDescriptor() const
    {
        // 2. If both Desc.[[Get]] and Desc.[[Set]] are absent, return false.
        return !(getter_.IsEmpty() && setter_.IsEmpty());
    }

    inline bool IsDataDescriptor() const
    {
        // 2. If both Desc.[[Value]] and Desc.[[Writable]] are absent, return false.
        return !(value_.IsEmpty() && !hasWritable_);
    }

    inline bool IsGenericDescriptor() const
    {
        // 2. If IsAccessorDescriptor(Desc) and IsDataDescriptor(Desc) are both false, return true
        return !IsAccessorDescriptor() && !IsDataDescriptor();
    }

    inline bool IsEmpty() const
    {
        return !hasWritable_ && !hasEnumerable_ && !hasConfigurable_ && !HasValue() && !HasGetter() && !HasSetter();
    }

    static void CompletePropertyDescriptor(const JSThread *thread, PropertyDescriptor &desc);

private:
    const JSThread *thread_{nullptr};

    bool writable_ {false};
    bool enumerable_ {false};
    bool configurable_ {false};
    bool hasWritable_ {false};
    bool hasEnumerable_ {false};
    bool hasConfigurable_ {false};
    SharedFieldType fieldType_ {SharedFieldType::NONE};

    JSHandle<JSTaggedValue> value_ {};
    JSHandle<JSTaggedValue> getter_ {};
    JSHandle<JSTaggedValue> setter_ {};
    JSHandle<JSTaggedValue> key_ {};
};

enum class ElementTypes { ALLTYPES, STRING_AND_SYMBOL };

class PropertyMetaData {
public:
    using IsFoundField = BitField<bool, 0, 1>;
    using IsInlinedPropsField = IsFoundField::NextFlag;
    // 3: The bit field that represents the "Representation" of the property
    using RepresentationField = IsInlinedPropsField::NextField<Representation, 3>;
    using OffsetField = RepresentationField::NextField<uint32_t, PropertyAttributes::MAX_FAST_PROPS_CAPACITY_LOG2>;

    explicit PropertyMetaData(uint32_t metaData) : metaData_(metaData) {}

    ~PropertyMetaData() = default;
    DEFAULT_NOEXCEPT_MOVE_SEMANTIC(PropertyMetaData);
    DEFAULT_COPY_SEMANTIC(PropertyMetaData);

    explicit PropertyMetaData(bool isFound)
    {
        SetFound(isFound);
    }

    inline bool IsFound() const
    {
        return IsFoundField::Get(metaData_);
    }

    inline void SetFound(bool flag)
    {
        IsFoundField::Set(flag, &metaData_);
    }

    inline bool GetIsInlinedProps() const
    {
        return IsInlinedPropsField::Get(metaData_);
    }

    inline void SetIsInlinedProps(bool flag)
    {
        IsInlinedPropsField::Set(flag, &metaData_);
    }

    inline Representation GetRepresentation() const
    {
        return RepresentationField::Get(metaData_);
    }

    inline void SetRepresentation(Representation representation)
    {
        RepresentationField::Set<uint32_t>(representation, &metaData_);
    }

    inline void SetOffset(uint32_t offset)
    {
        OffsetField::Set<uint32_t>(offset, &metaData_);
    }

    inline uint32_t GetOffset() const
    {
        return OffsetField::Get(metaData_);
    }

private:
    uint32_t metaData_{0};
};

class OperationResult {
public:
    OperationResult(const JSThread *thread, JSTaggedValue value, PropertyMetaData metaData)
        : metaData_(metaData)
    {
        thread_ = thread;
        value_ = JSHandle<JSTaggedValue>(thread_, value);
    }

    ~OperationResult() = default;
    DEFAULT_NOEXCEPT_MOVE_SEMANTIC(OperationResult);
    DEFAULT_COPY_SEMANTIC(OperationResult);

    JSHandle<JSTaggedValue> GetValue() const
    {
        if (value_->IsPropertyBox()) {
            return JSHandle<JSTaggedValue>(thread_,
                                           PropertyBox::Cast(value_.GetTaggedValue().GetTaggedObject())->GetValue());
        }
        return value_;
    }

    JSHandle<JSTaggedValue> GetRawValue() const
    {
        return value_;
    }

    const PropertyMetaData &GetPropertyMetaData() const
    {
        return metaData_;
    }

private:
    const JSThread *thread_ {nullptr};
    JSHandle<JSTaggedValue> value_ {};
    PropertyMetaData metaData_ {0U};
};


// HashField possible layout:
// [ hashValue ] | [extraInfo] | [ hashValue, extraInfo, nativePointer, ... ]
// nativePointer number depends on the extraLength of taggedArray
class ECMAObject : public TaggedObject {
public:
    static constexpr int HASH_INDEX = 0;
    static constexpr int FUNCTION_EXTRA_INDEX = 1;
    static constexpr int RESOLVED_MAX_SIZE = 2;

    CAST_CHECK(ECMAObject, IsECMAObject);

    void SetCallable(bool flag);
    bool IsCallable() const;
    Method *GetCallTarget() const;
    void *GetNativePointer() const;

    static constexpr size_t HASH_OFFSET = TaggedObjectSize();
    static constexpr size_t SIZE = HASH_OFFSET + sizeof(JSTaggedType);

    static void SetHash(const JSThread *thread, int32_t hash, const JSHandle<ECMAObject> &obj);
    int32_t GetHash() const;
    bool HasHash() const;

    void InitializeHash()
    {
        Barriers::SetPrimitive<JSTaggedType>(this, ECMAObject::HASH_OFFSET, JSTaggedValue(0).GetRawData());
    }

    void* GetNativePointerField(int32_t index) const;
    static void SetNativePointerField(const JSThread *thread, const JSHandle<JSObject> &obj, int32_t index,
                                      void *nativePointer, const NativePointerCallback &callBack, void *data,
                                      size_t nativeBindingsize = 0, Concurrent isConcurrent = Concurrent::NO);
    int32_t GetNativePointerFieldCount() const;
    static void SetNativePointerFieldCount(const JSThread *thread, const JSHandle<JSObject> &obj, int32_t count);

    DECL_VISIT_OBJECT(HASH_OFFSET, SIZE);

    template <VisitType visitType, class DerivedVisitor>
    void VisitObjects(BaseObjectVisitor<DerivedVisitor> &visitor)
    {
        // no field in this object
        VisitRangeSlot<visitType>(visitor);
    }
};

class JSObject : public ECMAObject {
public:
    static constexpr int MIN_ELEMENTS_LENGTH = 3;
    static constexpr int MIN_PROPERTIES_LENGTH = JSHClass::DEFAULT_CAPACITY_OF_IN_OBJECTS;
    static constexpr int FAST_ELEMENTS_FACTOR = 3;
    static constexpr int MIN_GAP = 256;
    static constexpr int MAX_GAP = 1_KB;
    static constexpr uint32_t MAX_ELEMENT_INDEX = std::numeric_limits<uint32_t>::max();
    static constexpr int MIN_ELEMENTS_HINT_LENGTH = 1_KB;
    static constexpr int MAX_ELEMENTS_HINT_LENGTH = 2_MB;
    static constexpr int ELEMENTS_HINT_FACTOR = 8;
    static constexpr int SHOULD_TRANS_TO_FAST_ELEMENTS_FACTOR = 2;
    static constexpr size_t PAIR_SIZE = 2;
    static constexpr size_t VALUE_OFFSET = 1;

    CAST_CHECK(JSObject, IsECMAObject);

    // ecma6.0 6.2.4.4
    static JSHandle<JSTaggedValue> FromPropertyDescriptor(JSThread *thread, const PropertyDescriptor &desc);

    // ecma6.0 6.2.4.5 ToPropertyDescriptor ( Obj )
    static void ToPropertyDescriptor(JSThread *thread, const JSHandle<JSTaggedValue> &obj, PropertyDescriptor &desc);
    static bool ToPropertyDescriptorFast(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                         PropertyDescriptor &desc);

    static JSHandle<JSTaggedValue> CallFunction(JSThread *thread, const JSHandle<JSTaggedValue> &func);

    // ecma6 7.3 Operations on Objects
    static JSHandle<JSTaggedValue> GetMethod(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                             const JSHandle<JSTaggedValue> &key);

    static JSHandle<JSTaggedValue> FastGetMethod(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                 const JSHandle<JSTaggedValue> &key);

    static bool CreateDataProperty(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                                   const JSHandle<JSTaggedValue> &value, SCheckMode sCheckMode = SCheckMode::CHECK);

    static bool CreateDataProperty(JSThread *thread, const JSHandle<JSObject> &obj, uint32_t index,
                                   const JSHandle<JSTaggedValue> &value, SCheckMode sCheckMode = SCheckMode::CHECK);

    static bool CreateMethodProperty(JSThread *thread, const JSHandle<JSObject> &obj,
                                     const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value);

    static bool CreateDataPropertyOrThrow(JSThread *thread, const JSHandle<JSObject> &obj,
                                          const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value,
                                          SCheckMode sCheckMode = SCheckMode::CHECK);

    static bool CreateDataPropertyOrThrow(JSThread *thread, const JSHandle<JSObject> &obj, uint32_t index,
                                          const JSHandle<JSTaggedValue> &value,
                                          SCheckMode sCheckMode = SCheckMode::CHECK);

    static JSHandle<TaggedArray> PUBLIC_API EnumerableOwnNames(JSThread *thread, const JSHandle<JSObject> &obj);

    // 7.3.23 EnumerableOwnPropertyNames ( O, kind )
    static JSHandle<TaggedArray> EnumerableOwnPropertyNames(JSThread *thread, const JSHandle<JSObject> &obj,
                                                            PropertyKind kind);
    static void EnumerableOwnPropertyNamesHelper(JSThread *thread, const JSHandle<JSObject> &obj,
        const JSHandle<TaggedArray> &arr, JSHandle<TaggedArray> &properties,
        uint32_t &index, bool &fastMode, PropertyKind kind);

    static JSHandle<GlobalEnv> GetFunctionRealm(JSThread *thread, const JSHandle<JSTaggedValue> &object);

    static bool SetIntegrityLevel(JSThread *thread, const JSHandle<JSObject> &obj, IntegrityLevel level);

    static bool FreezeSharedObject(JSThread *thread, const JSHandle<JSObject> &obj);

    static bool TestIntegrityLevel(JSThread *thread, const JSHandle<JSObject> &obj, IntegrityLevel level);

    static JSHandle<JSTaggedValue> SpeciesConstructor(JSThread *thread, const JSHandle<JSObject> &obj,
                                                      const JSHandle<JSTaggedValue> &defaultConstructor);
    static JSHandle<JSTaggedValue> SlowSpeciesConstructor(JSThread *thread,
                                                          const JSHandle<JSTaggedValue> &objConstructor,
                                                          const JSHandle<JSTaggedValue> &defaultConstructor);
    // 7.3.17
    template<ElementTypes types = ElementTypes::ALLTYPES>
    static JSHandle<JSTaggedValue> CreateListFromArrayLike(JSThread *thread, const JSHandle<JSTaggedValue> &obj);

    // ecma6 9.1
    // [[GetPrototypeOf]]
    static JSTaggedValue GetPrototype(const JSHandle<JSObject> &obj);

    static JSTaggedValue GetPrototype(JSTaggedValue obj);

    // [[SetPrototypeOf]]
    static bool SetPrototype(JSThread *thread, const JSHandle<JSObject> &obj,
                             const JSHandle<JSTaggedValue> &proto,
                             bool isChangeProto = false);

    // [[IsExtensible]]
    bool IsExtensible() const;

    // [[PreventExtensions]]
    static bool PreventExtensions(JSThread *thread, const JSHandle<JSObject> &obj);

    // [[GetOwnProperty]] -> Undefined | Property Descriptor
    static bool GetOwnProperty(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                               PropertyDescriptor &desc);

    static bool GlobalGetOwnProperty(JSThread *thread, const JSHandle<JSTaggedValue> &key, PropertyDescriptor &desc);

    static bool OrdinaryGetOwnProperty(JSThread *thread, const JSHandle<JSObject> &obj,
                                       const JSHandle<JSTaggedValue> &key, PropertyDescriptor &desc);

    // [[DefineOwnProperty]]
    static bool DefineOwnProperty(JSThread *thread, ObjectOperator *op,
                                  const PropertyDescriptor &desc, SCheckMode sCheckMode = SCheckMode::CHECK);

    static bool DefineOwnProperty(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                                  const PropertyDescriptor &desc, SCheckMode sCheckMode = SCheckMode::CHECK);

    static bool DefineOwnProperty(JSThread *thread, const JSHandle<JSObject> &obj, uint32_t index,
                                  const PropertyDescriptor &desc, SCheckMode sCheckMode = SCheckMode::CHECK);

    static bool OrdinaryDefineOwnProperty(JSThread *thread, ObjectOperator *op,
                                          const PropertyDescriptor &desc, SCheckMode sCheckMode = SCheckMode::CHECK);

    static bool OrdinaryDefineOwnProperty(JSThread *thread, const JSHandle<JSObject> &obj,
                                          const JSHandle<JSTaggedValue> &key, const PropertyDescriptor &desc,
                                          SCheckMode sCheckMode = SCheckMode::CHECK);

    static bool OrdinaryDefineOwnProperty(JSThread *thread, const JSHandle<JSObject> &obj, uint32_t index,
                                          const PropertyDescriptor &desc,
                                          SCheckMode sCheckMode = SCheckMode::CHECK);

    static bool IsCompatiblePropertyDescriptor(bool extensible, const PropertyDescriptor &desc,
                                               const PropertyDescriptor &current);

    static bool ValidateAndApplyPropertyDescriptor(ObjectOperator *op, bool extensible, const PropertyDescriptor &desc,
                                                   const PropertyDescriptor &current,
                                                   SCheckMode sCheckMode = SCheckMode::CHECK);

    static OperationResult PUBLIC_API GetProperty(JSThread *thread, const JSHandle<JSObject> &obj,
                                                  const JSHandle<JSTaggedValue> &key);

    static OperationResult GetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                       const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &receiver);

    static OperationResult GetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                       const JSHandle<JSTaggedValue> &key, SCheckMode sCheckMode = SCheckMode::CHECK);

    static OperationResult GetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, uint32_t index);

    static OperationResult GetPropertyFromGlobal(JSThread *thread, const JSHandle<JSTaggedValue> &key);

    static bool SetProperty(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                            JSHandle<JSTaggedValue> value, bool mayThrow = false);

    static bool SetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &key,
                            JSHandle<JSTaggedValue> value, bool mayThrow = false,
                            SCheckMode checkMode = SCheckMode::CHECK);

    static bool SetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &key,
                            JSHandle<JSTaggedValue> value, const JSHandle<JSTaggedValue> &receiver,
                            bool mayThrow = false);

    static bool SetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, uint32_t index,
                            JSHandle<JSTaggedValue> value, bool mayThrow = false);

    static bool GlobalSetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &key,
                                  JSHandle<JSTaggedValue> value, bool mayThrow);

    // [[HasProperty]]
    static bool HasProperty(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key);

    static bool HasProperty(JSThread *thread, const JSHandle<JSObject> &obj, uint32_t index);

    // 9.1.10 [[Delete]]
    static bool DeleteProperty(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                               SCheckMode sCheckMode = SCheckMode::CHECK);

    // [[OwnPropertyKeys]]
    static JSHandle<TaggedArray> GetOwnPropertyKeys(JSThread *thread, const JSHandle<JSObject> &obj);

    static JSHandle<TaggedArray> GetAllPropertyKeys(JSThread *thread, const JSHandle<JSObject> &obj, uint32_t filter);

    static void CollectEnumKeys(JSThread *thread, const JSHandle<JSObject> &obj,
                                               JSHandle<TaggedArray> keyArray, uint32_t *keys,
                                               JSHandle<TaggedQueue> shadowQueue, int32_t lastLength = -1);

    static void AppendOwnEnumPropertyKeys(JSThread *thread, const JSHandle<JSObject> &obj,
                                          JSHandle<TaggedArray> keyArray, uint32_t *keys,
                                          JSHandle<TaggedQueue> shadowQueue);

    static JSHandle<TaggedArray> GetOwnEnumPropertyKeys(JSThread *thread, const JSHandle<JSObject> &obj);

    // 9.1.13 ObjectCreate
    static JSHandle<JSObject> ObjectCreate(JSThread *thread, const JSHandle<JSObject> &proto);

    // 12.9.4 Runtime Semantics: InstanceofOperator(O, C)
    static bool InstanceOf(JSThread *thread, const JSHandle<JSTaggedValue> &object,
                           const JSHandle<JSTaggedValue> &target);

    static JSTaggedValue TryGetEnumCache(JSThread *thread, JSTaggedValue obj);

    // 13.7.5.15 EnumerateObjectProperties ( O ); same as [[Enumerate]]
    static JSHandle<JSForInIterator> EnumerateObjectProperties(JSThread *thread, const JSHandle<JSTaggedValue> &obj);
    static JSHandle<JSForInIterator> LoadEnumerateProperties(JSThread *thread, const JSHandle<JSTaggedValue> &object);

    static bool IsRegExp(JSThread *thread, const JSHandle<JSTaggedValue> &argument);

    static JSTaggedValue CallGetter(JSThread *thread, const AccessorData *accessor,
                                    const JSHandle<JSTaggedValue> &receiver);
    static bool PUBLIC_API CallSetter(JSThread *thread, const AccessorData &accessor,
                                      const JSHandle<JSTaggedValue> &receiver,
                                      const JSHandle<JSTaggedValue> &value, bool mayThrow = false);

    static void FillElementsWithHoles(const JSThread *thread, const JSHandle<JSObject> &obj,
                                      uint32_t start, uint32_t end);

    JSHClass *GetJSHClass() const
    {
        return GetClass();
    }
    uint32_t GetNonInlinedFastPropsCapacity() const;
    bool IsJSGlobalObject() const;
    bool IsConstructor() const;
    bool IsECMAObject() const;
    bool IsJSError() const;
    bool IsArguments() const;
    bool IsDate() const;
    bool IsJSArray() const;
    bool IsJSSArray() const;
    bool IsJSShared() const;
    bool IsJSMap() const;
    bool IsJSSet() const;
    bool IsJSRegExp() const;
    bool IsJSFunction() const;
    bool IsBoundFunction() const;
    bool IsJSIntlBoundFunction() const;
    bool IsProxyRevocFunction() const;
    bool IsAccessorData() const;
    bool IsJSGlobalEnv() const;
    bool IsJSProxy() const;
    bool IsGeneratorObject() const;
    bool IsAsyncGeneratorObject() const;
    bool IsForinIterator() const;
    bool IsJSSetIterator() const;
    bool IsJSRegExpIterator() const;
    bool IsJSMapIterator() const;
    bool IsJSArrayIterator() const;
    bool IsJSAPIArrayListIterator() const;
    bool IsJSAPIStackIterator() const;
    bool IsJSAPIVectorIterator() const;
    bool IsJSAPIBitVectorIterator() const;
    bool IsJSAPILinkedListIterator() const;
    bool IsJSAPIListIterator() const;
    bool IsJSPrimitiveRef() const;
    bool IsElementDict() const;
    bool IsPropertiesDict() const;
    bool IsTypedArray() const;
    bool IsSharedTypedArray() const;
    bool PUBLIC_API ElementsAndPropertiesIsEmpty() const;

    static PUBLIC_API void DefinePropertyByLiteral(JSThread *thread, const JSHandle<JSObject> &obj,
                                                   const JSHandle<JSTaggedValue> &key,
                                                   const JSHandle<JSTaggedValue> &value,
                                                   bool useForClass = false);
    static void DefineSetter(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &key,
                             const JSHandle<JSTaggedValue> &value);
    static void DefineGetter(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &key,
                             const JSHandle<JSTaggedValue> &value);
    static PUBLIC_API JSHandle<JSObject> CreateObjectFromProperties(const JSThread *thread,
                                                                    const JSHandle<TaggedArray> &properties,
                                                         JSTaggedValue ihc = JSTaggedValue::Undefined());
    static JSHandle<JSObject> CreateObjectFromPropertiesByIHClass(const JSThread *thread,
                                                                  const JSHandle<TaggedArray> &properties,
                                                                  uint32_t propsLen,
                                                                  const JSHandle<JSHClass> &ihc,
                                                                  TrackTypeUpdateMode trackMode);
    static bool CheckPropertiesForRep(
        const JSHandle<TaggedArray> &properties, uint32_t propsLen, const JSHandle<JSHClass> &ihc);
    static void GetAllKeys(const JSThread *thread, const JSHandle<JSObject> &obj, int offset,
                           const JSHandle<TaggedArray> &keyArray);
    static void GetAllKeysForSerialization(const JSHandle<JSObject> &obj, std::vector<JSTaggedValue> &keyVector);

    static void GetAllKeysByFilter(const JSThread *thread, const JSHandle<JSObject> &obj,
                                   uint32_t &keyArrayEffectivelength,
                                   const JSHandle<TaggedArray> &keyArray,
                                   uint32_t filter);
    static void GetAllElementKeys(JSThread *thread, const JSHandle<JSObject> &obj, int offset,
                                  const JSHandle<TaggedArray> &keyArray);
    static void GetAllElementKeysByFilter(JSThread *thread,
                                          const JSHandle<JSObject> &obj,
                                          const JSHandle<TaggedArray> &keyArray,
                                          uint32_t &keyArrayEffectiveLength,
                                          uint32_t filter);

    static void GetALLElementKeysIntoVector(const JSThread *thread, const JSHandle<JSObject> &obj,
                                            std::vector<JSTaggedValue> &keyVector);
    std::pair<uint32_t, uint32_t> GetNumberOfEnumKeys() const;
    uint32_t GetNumberOfKeys();
    uint32_t GetNumberOfElements(JSThread *thread);

    static JSHandle<TaggedArray> GetEnumElementKeys(JSThread *thread, const JSHandle<JSObject> &obj, int offset,
                                                    uint32_t numOfElements, uint32_t *keys);
    static void CollectEnumElements(JSThread *thread, const JSHandle<JSObject> &obj, int offset,
                                                   JSHandle<TaggedArray> elementArray, uint32_t *keys,
                                                   int32_t lastLength = -1);
    static void GetEnumElementKeys(JSThread *thread, const JSHandle<JSObject> &obj, int offset,
                                   const JSHandle<TaggedArray> &keyArray);
    static JSHandle<TaggedArray> GetAllEnumKeys(JSThread *thread, const JSHandle<JSObject> &obj,
                                                uint32_t numOfKeys, uint32_t *keys);
    static uint32_t GetAllEnumKeys(JSThread *thread, const JSHandle<JSObject> &obj, int offset,
                                   const JSHandle<TaggedArray> &keyArray);

    static void AddAccessor(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &key,
                            const JSHandle<AccessorData> &value, PropertyAttributes attr);

    static constexpr size_t PROPERTIES_OFFSET = ECMAObject::SIZE;

    ACCESSORS(Properties, PROPERTIES_OFFSET, ELEMENTS_OFFSET);
    ACCESSORS(Elements, ELEMENTS_OFFSET, SIZE);

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(ECMAObject, PROPERTIES_OFFSET, SIZE)

    void Dump(std::ostream &os, bool isPrivacy = false) const DUMP_API_ATTR;
    void Dump() const DUMP_API_ATTR
    {
        Dump(std::cout);
    }
    void DumpForSnapshot(std::vector<Reference> &vec) const;
    static const CString ExtractConstructorAndRecordName(JSThread *thread, TaggedObject *obj, bool noAllocate = false,
                                                         bool *isCallGetter = nullptr);

    static const CString ExtractFilePath(JSThread *thread, CString name, CString moduleName,
                                         CString defaultName, CString fileName, int32_t line);

    static JSHandle<NameDictionary> PUBLIC_API TransitionToDictionary(const JSThread *thread,
                                                                      const JSHandle<JSObject> &receiver);

    static inline std::pair<bool, JSTaggedValue> ConvertValueWithRep(PropertyAttributes attr, JSTaggedValue value);

    inline void SetPropertyInlinedPropsWithRep(const JSThread *thread, uint32_t index, JSTaggedValue value);
    template <size_t objectSize, uint32_t index, bool needBarrier = true>
    inline void SetPropertyInlinedPropsWithSize(const JSThread* thread, JSTaggedValue value);
    template <bool needBarrier = true>
    inline void SetPropertyInlinedProps(const JSThread *thread, uint32_t index, JSTaggedValue value);
    template <bool needBarrier = true>
    inline void SetPropertyInlinedProps(const JSThread *thread, const JSHClass *hclass, uint32_t index,
                                        JSTaggedValue value);
    inline JSTaggedValue GetPropertyInlinedPropsWithRep(uint32_t index, PropertyAttributes attr) const;
    inline JSTaggedValue GetPropertyInlinedPropsWithRep(const JSHClass *hclass, uint32_t index,
        PropertyAttributes attr) const;
    template <size_t objectSize, uint32_t index>
    inline JSTaggedValue GetPropertyInlinedPropsWithSize() const;
    inline JSTaggedValue GetPropertyInlinedProps(uint32_t index) const;
    inline JSTaggedValue GetPropertyInlinedProps(const JSHClass *hclass, uint32_t index) const;
    inline JSTaggedValue GetProperty(const JSHClass *hclass, PropertyAttributes attr) const;
    PropertyBox* GetGlobalPropertyBox(JSThread *thread, const std::string& key);
    template <bool needBarrier = true>
    inline void SetProperty(const JSThread *thread, const JSHClass *hclass, PropertyAttributes attr,
                            JSTaggedValue value);

    static bool IsArrayLengthWritable(JSThread *thread, const JSHandle<JSObject> &receiver);
    bool UpdatePropertyInDictionary(const JSThread *thread, JSTaggedValue key, JSTaggedValue value);
    static bool ShouldTransToDict(uint32_t capacity, uint32_t index);
    static bool ShouldTransToFastElements(JSThread *thread, TaggedArray *elements, uint32_t capacity, uint32_t index);
    static bool AttributesUnchanged(const JSThread *thread, const JSHandle<JSObject> &obj);
    static JSHandle<TaggedArray> GrowElementsCapacity(const JSThread *thread, const JSHandle<JSObject> &obj,
                                                      uint32_t capacity, bool highGrowth = false, bool isNew = false);

    static bool IsDepulicateKeys(JSThread *thread, JSHandle<TaggedArray> keys, int32_t lastLength,
                                 JSHandle<TaggedQueue> shadowQueue, JSHandle<JSTaggedValue> key);
    static JSHandle<EnumCache> GetOrCreateEnumCache(JSThread *thread, JSHandle<JSHClass> jsHClass);
    static JSHandle<JSTaggedValue> GetOrCreateDependentInfos(JSThread *thread, JSHandle<JSHClass> jsHClass);
    static inline void SetEnumCacheKind([[maybe_unused]] const JSThread *thread,
                                        JSHandle<EnumCache> enumCache, const EnumCacheKind kind)
    {
        enumCache->SetEnumCacheKind(static_cast<uint32_t>(kind));
    }

    static inline EnumCacheKind GetEnumCacheKind(JSTaggedValue enumCache)
    {
        if (!enumCache.IsEnumCache()) {
            return EnumCacheKind::NONE;
        }
        EnumCacheKind kind = static_cast<EnumCacheKind>(EnumCache::Cast(enumCache)->GetEnumCacheKind());
        ASSERT(kind == EnumCacheKind::NONE ||
               kind == EnumCacheKind::SIMPLE ||
               kind == EnumCacheKind::PROTOCHAIN);
        return kind;
    }

    static void ClearHasDeleteProperty(JSHandle<JSTaggedValue> object);

    static JSHandle<JSTaggedValue> IterableToList(JSThread *thread, const JSHandle<JSTaggedValue> &items,
                                                  JSTaggedValue method = JSTaggedValue::Undefined());

    static void TryOptimizeAsFastElements(const JSThread *thread, JSHandle<JSObject> obj);
    static void OptimizeAsFastProperties(const JSThread *thread, JSHandle<JSObject> obj);

    static void SetSProperties(JSThread *thread, JSHandle<JSObject> obj, const std::vector<PropertyDescriptor> &descs);

    static void PUBLIC_API TryMigrateToGenericKindForJSObject(const JSThread *thread, const JSHandle<JSObject> &obj,
                                                  const ElementsKind oldKind);
    static void ElementsToDictionary(const JSThread *thread, JSHandle<JSObject> obj);

private:
    friend class ObjectOperator;
    friend class LoadICRuntime;
    friend class StoreICRuntime;
    friend class ObjectFastOperator;
    friend class ICRuntimeStub;
    friend class RuntimeStubs;
    friend class JSSharedArray;
    friend class builtins::BuiltinsArkTools;

    static bool HasMutantTaggedArrayElements(const JSHandle<JSObject> &obj);
    PropertyBox* GetGlobalPropertyBox(JSTaggedValue key);
    static bool CheckAndUpdateArrayLength(JSThread *thread, const JSHandle<JSObject> &receiver,
                                          uint32_t index, ElementsKind &kind);
    static bool PUBLIC_API AddElementInternal(
        JSThread *thread, const JSHandle<JSObject> &receiver, uint32_t index, const JSHandle<JSTaggedValue> &value,
        PropertyAttributes attr = PropertyAttributes(PropertyAttributes::GetDefaultAttributes()));

    static JSTaggedValue GetProperty(JSThread *thread, ObjectOperator *op);
    static bool SetProperty(ObjectOperator *op, JSHandle<JSTaggedValue> value, bool mayThrow);
    static bool SetPropertyForData(ObjectOperator *op, const JSHandle<JSTaggedValue> &value, bool *isAccessor);
    static bool SetPropertyForAccessor(ObjectOperator *op, const JSHandle<JSTaggedValue> &value);
    static void DeletePropertyInternal(JSThread *thread, const JSHandle<JSObject> &obj,
                                       const JSHandle<JSTaggedValue> &key, uint32_t index);
    int FindProperty(const JSHandle<JSTaggedValue> &key);

    static uint32_t ComputeElementCapacity(uint32_t oldCapacity, bool isNew = false);
    static uint32_t ComputeElementCapacityHighGrowth(uint32_t oldCapacity);
    static uint32_t ComputeElementCapacityWithHint(uint32_t oldCapacity, uint32_t hint);
    static uint32_t ComputeNonInlinedFastPropsCapacity(JSThread *thread, uint32_t oldCapacity,
                                                       uint32_t maxNonInlinedFastPropsCapacity);

    static JSTaggedValue ShouldGetValueFromBox(ObjectOperator *op);
    static std::pair<JSHandle<TaggedArray>, JSHandle<TaggedArray>> GetOwnEnumerableNamesInFastMode(
        JSThread *thread, const JSHandle<JSObject> &obj, uint32_t *copyLengthOfKeys, uint32_t *copyLengthOfElements);
    static bool CheckHClassHit(const JSHandle<JSObject> &obj, const JSHandle<JSHClass> &cls);
    static uint32_t SetValuesOrEntries(JSThread *thread, const JSHandle<TaggedArray> &prop, uint32_t index,
                                       const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value,
                                       PropertyKind kind);
    static bool IsSimpleEnumCacheValid(JSThread *thread, JSTaggedValue receiver);
    static bool IsProtoChainCacheValid(JSThread *thread, JSTaggedValue receiver);
    static void TrimInlinePropsSpace(const JSThread *thread, const JSHandle<JSObject> &object,
                                     uint32_t numberInlinedProps);
    static bool ValidateDataDescriptorWhenConfigurable(ObjectOperator *op, const PropertyDescriptor &desc,
                                                       const PropertyDescriptor &current, SCheckMode sCheckMode);
    static bool SetPropertyForDataDescriptor(ObjectOperator *op, JSHandle<JSTaggedValue> value,
                                             JSHandle<JSTaggedValue> &receiver, bool mayThrow, bool isInternalAccessor);
    static bool SetPropertyForDataDescriptorProxy(JSThread *thread, ObjectOperator *op,
                                                  const JSHandle<JSTaggedValue> &value,
                                                  JSHandle<JSTaggedValue> &receiver);
};
}  // namespace ecmascript
}  // namespace panda

#endif
