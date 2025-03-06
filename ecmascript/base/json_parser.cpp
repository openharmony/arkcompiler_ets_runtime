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

#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/base/json_parser.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/ecma_string_table.h"

namespace panda::ecmascript::base {

template<typename T>
JSHandle<JSTaggedValue> JsonParser<T>::Launch(Text begin, Text end)
{
    // check empty
    if (UNLIKELY(begin == end)) {
        return JSHandle<JSTaggedValue>(thread_, [&]() -> JSTaggedValue {
            THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Text in JSON: Empty Text", JSTaggedValue::Exception());
        }());
    }
    end_ = end - 1;
    current_ = begin;
    SkipEndWhiteSpace();
    range_ = end_;

    auto vm = thread_->GetEcmaVM();
    factory_ = vm->GetFactory();
    env_ = *vm->GetGlobalEnv();

    // For Json, we do not support ElementsKind
    auto index = static_cast<size_t>(ConstantIndex::ELEMENT_HOLE_TAGGED_HCLASS_INDEX);
    auto globalConstant = const_cast<GlobalEnvConstants *>(thread_->GlobalConstants());
    auto hclassVal = globalConstant->GetGlobalConstantObject(index);
    initialJSArrayClass_ = JSHandle<JSHClass>(thread_, hclassVal);
    JSHandle<JSFunction> objectFunc(env_->GetObjectFunction());
    initialJSObjectClass_ =
        JSHandle<JSHClass>(thread_, JSFunction::GetOrCreateInitialJSHClass(thread_, objectFunc));

    JSTaggedValue result = ParseJSONText();
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
    return JSHandle<JSTaggedValue>(thread_, result);
}

template<typename T>
JSTaggedValue JsonParser<T>::ParseJSONText()
{
    JSHandle<JSTaggedValue> parseValue;
    std::vector<JsonContinuation> continuationList;
    std::vector<JSHandle<JSTaggedValue>> elementsList;
    std::vector<JSHandle<JSTaggedValue>> propertyList;
    continuationList.reserve(16); // 16: initial capacity
    elementsList.reserve(16); // 16: initial capacity
    propertyList.reserve(16); // 16: initial capacity
    JsonContinuation continuation(ContType::RETURN, 0);
    while (true) {
        while (true) {
            SkipStartWhiteSpace();
            Tokens token = ParseToken();
            if (current_ > range_) {
                THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected end in JSON", JSTaggedValue::Exception());
            }
            switch (token) {
                case Tokens::OBJECT:
                    if (EmptyObjectCheck()) {
                        if (transformType_ == TransformType::SENDABLE) {
                            JSHandle<JSHClass> sHclass;
                            JSHandle<JSTaggedValue> sJsonPrototype = GetSJsonPrototype();
                            JSHandle<LayoutInfo> sLayout = factory_->CreateSLayoutInfo(0);
                            sHclass = factory_->NewSEcmaHClass(JSSharedObject::SIZE, 0, JSType::JS_SHARED_OBJECT,
                                                               JSHandle<JSTaggedValue>(sJsonPrototype),
                                                               JSHandle<JSTaggedValue>(sLayout));
                            parseValue = JSHandle<JSTaggedValue>(factory_->NewSharedOldSpaceJSObject(sHclass));
                        } else {
                            parseValue = JSHandle<JSTaggedValue>(factory_->NewJSObject(initialJSObjectClass_));
                        }
                        GetNextNonSpaceChar();
                        break;
                    }
                    continuationList.emplace_back(std::move(continuation));
                    continuation = JsonContinuation(ContType::OBJECT, propertyList.size());

                    SkipStartWhiteSpace();
                    if (UNLIKELY(*current_ != '"')) {
                        THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Object Prop in JSON",
                                                      JSTaggedValue::Exception());
                    }
                    propertyList.emplace_back(ParseString(true));
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    SkipStartWhiteSpace();
                    if (UNLIKELY(*current_ != ':')) {
                        THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Object in JSON",
                                                      JSTaggedValue::Exception());
                    }
                    Advance();
                    continue;
                case Tokens::MAP:
                    if (EmptyObjectCheck()) {
                        if (transformType_ == TransformType::SENDABLE) {
                            parseValue = JSHandle<JSTaggedValue>(CreateSharedMap());
                        } else {
                            parseValue = JSHandle<JSTaggedValue>(CreateMap());
                        }
                        GetNextNonSpaceChar();
                        break;
                    }
                    continuationList.emplace_back(std::move(continuation));
                    continuation = JsonContinuation(ContType::MAP, propertyList.size());

                    SkipStartWhiteSpace();
                    if (UNLIKELY(*current_ != '"')) {
                        THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected MAP Prop in JSON",
                                                      JSTaggedValue::Exception());
                    }
                    propertyList.emplace_back(ParseString(true));
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    SkipStartWhiteSpace();
                    if (UNLIKELY(*current_ != ':')) {
                        THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected MAP in JSON",
                                                      JSTaggedValue::Exception());
                    }
                    Advance();
                    continue;
                case Tokens::ARRAY:
                    if (EmptyArrayCheck()) {
                        if (transformType_ == TransformType::SENDABLE) {
                            parseValue = JSHandle<JSTaggedValue>(factory_->NewJSSArray());
                        } else {
                            parseValue = JSHandle<JSTaggedValue>(factory_->NewJSArray(0, initialJSArrayClass_));
                        }
                        GetNextNonSpaceChar();
                        break;
                    }
                    continuationList.emplace_back(std::move(continuation));
                    continuation = JsonContinuation(ContType::ARRAY, elementsList.size());
                    continue;
                case Tokens::LITERAL_TRUE:
                    parseValue = JSHandle<JSTaggedValue>(thread_, ParseLiteralTrue());
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    break;
                case Tokens::LITERAL_FALSE:
                    parseValue = JSHandle<JSTaggedValue>(thread_, ParseLiteralFalse());
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    break;
                case Tokens::LITERAL_NULL:
                    parseValue = JSHandle<JSTaggedValue>(thread_, ParseLiteralNull());
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    break;
                case Tokens::NUMBER:
                    parseValue = JSHandle<JSTaggedValue>(thread_,
                                                         ParseNumber(IsInObjOrArrayOrMap(continuation.type_)));
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    break;
                case Tokens::STRING:
                    parseValue = ParseString(IsInObjOrArrayOrMap(continuation.type_));
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    break;
                default:
                    THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Text in JSON: Invalid Token",
                                                  JSTaggedValue::Exception());
            }
            break;
        }

        while (true) {
            switch (continuation.type_) {
                case ContType::RETURN:
                    ASSERT(continuationList.empty());
                    ASSERT(elementsList.empty());
                    ASSERT(propertyList.empty());
                    if (current_ <= range_) {
                        THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Text in JSON: Remaining Text Before Return",
                                                      JSTaggedValue::Exception());
                    }
                    return parseValue.GetTaggedValue();
                case ContType::ARRAY: {
                    elementsList.emplace_back(parseValue);
                    SkipStartWhiteSpace();
                    if (*current_ == ',') {
                        Advance();
                        break;
                    }
                    if (transformType_ == TransformType::SENDABLE) {
                        parseValue = CreateSJsonArray(continuation, elementsList);
                    } else {
                        parseValue = CreateJsonArray(continuation, elementsList);
                    }
                    if (*current_ != ']') {
                        THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Array in JSON",
                                                      JSTaggedValue::Exception());
                    }
                    Advance();
                    elementsList.resize(continuation.index_);
                    continuation = std::move(continuationList.back());
                    continuationList.pop_back();
                    continue;
                }
                case ContType::OBJECT: {
                    propertyList.emplace_back(parseValue);
                    SkipStartWhiteSpace();
                    if (*current_ == ',') {
                        GetNextNonSpaceChar();
                        if (UNLIKELY(*current_ != '"')) {
                            THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Object Prop in JSON",
                                                          JSTaggedValue::Exception());
                        }
                        propertyList.emplace_back(ParseString(true));
                        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                        SkipStartWhiteSpace();
                        if (UNLIKELY(*current_ != ':')) {
                            THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Object in JSON",
                                                          JSTaggedValue::Exception());
                        }
                        Advance();
                        break;
                    }
                    if (UNLIKELY(transformType_ == TransformType::SENDABLE)) {
                        parseValue = CreateSJsonObject(continuation, propertyList);
                    } else {
                        parseValue = CreateJsonObject(continuation, propertyList);
                    }
                    if (UNLIKELY(*current_ != '}')) {
                        THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Object in JSON",
                                                      JSTaggedValue::Exception());
                    }
                    Advance();
                    propertyList.resize(continuation.index_);
                    continuation = std::move(continuationList.back());
                    continuationList.pop_back();
                    continue;
                }
                case ContType::MAP: {
                    propertyList.emplace_back(parseValue);
                    SkipStartWhiteSpace();
                    if (*current_ == ',') {
                        GetNextNonSpaceChar();
                        if (UNLIKELY(*current_ != '"')) {
                            THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected MAP Prop in JSON",
                                                          JSTaggedValue::Exception());
                        }
                        propertyList.emplace_back(ParseString(true));
                        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                        SkipStartWhiteSpace();
                        if (UNLIKELY(*current_ != ':')) {
                            THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected MAP in JSON",
                                                          JSTaggedValue::Exception());
                        }
                        Advance();
                        break;
                    }
                    if (UNLIKELY(transformType_ == TransformType::SENDABLE)) {
                        parseValue = CreateSJsonMap(continuation, propertyList);
                    } else {
                        parseValue = CreateJsonMap(continuation, propertyList);
                    }
                    if (UNLIKELY(*current_ != '}')) {
                        THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected MAP in JSON",
                                                      JSTaggedValue::Exception());
                    }
                    Advance();
                    propertyList.resize(continuation.index_);
                    continuation = std::move(continuationList.back());
                    continuationList.pop_back();
                    continue;
                }
            }
            break;
        }
    }
}

template<typename T>
JSHandle<JSTaggedValue> JsonParser<T>::CreateJsonArray(JsonContinuation continuation,
    std::vector<JSHandle<JSTaggedValue>> &elementsList)
{
    size_t start = continuation.index_;
    size_t size = elementsList.size() - start;
    JSHandle<JSArray> array = factory_->NewJSArray(size, initialJSArrayClass_);
    JSHandle<TaggedArray> elements = factory_->NewJsonFixedArray(start, size, elementsList);
    JSHandle<JSObject> obj(array);
    obj->SetElements(thread_, elements);
    return JSHandle<JSTaggedValue>(array);
}

template<typename T>
JSHandle<JSTaggedValue> JsonParser<T>::CreateSJsonArray([[maybe_unused]] JsonContinuation continuation,
    [[maybe_unused]] std::vector<JSHandle<JSTaggedValue>> &elementsList)
{
    size_t start = continuation.index_;
    size_t size = elementsList.size() - start;
    JSHandle<JSSharedArray> array = factory_->NewJSSArray();
    array->SetArrayLength(thread_, size);
    JSHandle<TaggedArray> elements = factory_->NewSJsonFixedArray(start, size, elementsList);
    JSHandle<JSObject> obj(array);
    obj->SetElements(thread_, elements);
    return JSHandle<JSTaggedValue>(array);
}

template<typename T>
JSHandle<JSTaggedValue> JsonParser<T>::CreateJsonObject(JsonContinuation continuation,
    std::vector<JSHandle<JSTaggedValue>> &propertyList)
{
    size_t start = continuation.index_;
    size_t size = propertyList.size() - start;
    auto obj = JSHandle<JSTaggedValue>(factory_->NewJSObject(initialJSObjectClass_));
    for (size_t i = 0; i < size; i += 2) { // 2: prop name and value
        auto &keyHandle = propertyList[start + i];
        auto &valueHandle = propertyList[start + i + 1];
        auto res = SetPropertyByValue(obj, keyHandle, valueHandle);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
        if (res.IsHole()) {
            // slow path
            JSTaggedValue::SetProperty(thread_, obj, keyHandle, valueHandle, true);
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
        }
    }
    return obj;
}

template<typename T>
JSHandle<JSTaggedValue> JsonParser<T>::CreateSJsonObject(JsonContinuation continuation,
    std::vector<JSHandle<JSTaggedValue>> &propertyList)
{
    size_t start = continuation.index_;
    size_t size = propertyList.size() - start;
    uint32_t fieldNum = size / 2; // 2: key-value pair
    JSHandle<JSHClass> hclass;
    JSHandle<LayoutInfo> layout;
    JSHandle<JSTaggedValue> jsonPrototype = GetSJsonPrototype();
    if (fieldNum == 0) {
        layout = factory_->CreateSLayoutInfo(fieldNum);
        hclass = factory_->NewSEcmaHClass(JSSharedObject::SIZE, fieldNum, JSType::JS_SHARED_OBJECT,
                                          JSHandle<JSTaggedValue>(jsonPrototype), JSHandle<JSTaggedValue>(layout));
        JSHandle<JSObject> obj = factory_->NewSharedOldSpaceJSObject(hclass);
        return JSHandle<JSTaggedValue>(obj);
    } else if (LIKELY(fieldNum <= JSSharedObject::MAX_INLINE)) {
        layout = factory_->CreateSLayoutInfo(fieldNum);
        JSHandle<TaggedArray> propertyArray = factory_->NewSTaggedArray(size);
        for (size_t i = 0; i < size; i += 2) { // 2: prop name and value
            JSHandle<JSTaggedValue> keyHandle = propertyList[start + i];
            auto newKey = keyHandle.GetTaggedValue();
            auto stringAccessor = EcmaStringAccessor(newKey);
            if (!stringAccessor.IsInternString()) {
                newKey = JSTaggedValue(thread_->GetEcmaVM()->GetFactory()->InternString(keyHandle));
            }
            propertyArray->Set(thread_, i, newKey);
            propertyArray->Set(thread_, i + 1, JSTaggedValue(int(FieldType::NONE)));
        }
        hclass = factory_->NewSEcmaHClass(JSSharedObject::SIZE, fieldNum, JSType::JS_SHARED_OBJECT,
            JSHandle<JSTaggedValue>(jsonPrototype), JSHandle<JSTaggedValue>(layout));
        JSHandle<NumberDictionary> elementsDic = NumberDictionary::CreateInSharedHeap(thread_);
        bool hasElement = false;
        SendableClassDefiner::AddFieldTypeToHClass(thread_, propertyArray, size, layout, hclass, elementsDic,
                                                   hasElement, start, std::move(propertyList));
        JSHandle<JSObject> obj = factory_->NewSharedOldSpaceJSObject(hclass);
        uint32_t index = 0;
        size = (hclass->GetInlinedProperties() << 1);
        for (size_t i = 0; i < size; i += 2) { // 2: prop name and value
            obj->SetPropertyInlinedProps(thread_, index++, propertyList[start + i + 1].GetTaggedValue());
        }
        if (hasElement) {
            JSHandle<TaggedArray> elementsDicHdl(elementsDic);
            JSHandle<TaggedArray> elements =
                factory_->NewAndCopySNameDictionary(elementsDicHdl, elementsDicHdl->GetLength());
            obj->SetElements(thread_, elements);
            hclass->SetIsDictionaryElement(true);
        }
        return JSHandle<JSTaggedValue>(obj);
    }
    // build in dict mode
    JSMutableHandle<NameDictionary> dict(
        thread_, NameDictionary::CreateInSharedHeap(thread_, NameDictionary::ComputeHashTableSize(fieldNum)));
    JSMutableHandle<JSTaggedValue> propKey(thread_, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> propValue(thread_, JSTaggedValue::Undefined());
    // create dict and set key value
    for (size_t i = 0; i < size; i += 2) { // 2: prop name and value
        PropertyAttributes attributes = PropertyAttributes::Default(false, false, false);
        propKey.Update(propertyList[start + i]);
        propValue.Update(propertyList[start + i + 1]);
        JSHandle<NameDictionary> newDict = NameDictionary::PutIfAbsent(thread_, dict, propKey,
                                                                       propValue, attributes);
        dict.Update(newDict);
    }
    hclass = factory_->NewSEcmaHClassDictMode(JSSharedObject::SIZE, fieldNum, JSType::JS_SHARED_OBJECT,
                                              JSHandle<JSTaggedValue>(jsonPrototype));
    JSHandle<JSObject> obj = factory_->NewSharedOldSpaceJSObject(hclass);
    obj->SetProperties(thread_, dict);
    return JSHandle<JSTaggedValue>(obj);
}

template<typename T>
JSHandle<JSSharedMap> JsonParser<T>::CreateSharedMap()
{
    JSHandle<JSTaggedValue> proto = GetSMapPrototype();
    auto emptySLayout = thread_->GlobalConstants()->GetHandledEmptySLayoutInfo();
    JSHandle<JSHClass> mapClass = factory_->NewSEcmaHClass(JSSharedMap::SIZE, 0,
                                                           JSType::JS_SHARED_MAP, proto,
                                                           emptySLayout);
    JSHandle<JSObject> obj = factory_->NewSharedOldSpaceJSObjectWithInit(mapClass);
    JSHandle<JSSharedMap> jsMap = JSHandle<JSSharedMap>::Cast(obj);
    JSHandle<LinkedHashMap> linkedMap(
        LinkedHashMap::Create(thread_, LinkedHashMap::MIN_CAPACITY, MemSpaceKind::SHARED));
    jsMap->SetLinkedMap(thread_, linkedMap);
    jsMap->SetModRecord(0);
    return jsMap;
}

template<typename T>
JSHandle<JSMap> JsonParser<T>::CreateMap()
{
    JSHandle<JSTaggedValue> constructor = env_->GetBuiltinsMapFunction();
    JSHandle<JSMap> map =
        JSHandle<JSMap>::Cast(factory_->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    JSHandle<LinkedHashMap> linkedMap = LinkedHashMap::Create(thread_);
    map->SetLinkedMap(thread_, linkedMap);
    return JSHandle<JSMap>(thread_, *map);
}

template<typename T>
JSHandle<JSTaggedValue> JsonParser<T>::CreateJsonMap(JsonContinuation continuation,
                                                     std::vector<JSHandle<JSTaggedValue>> &propertyList)
{
    size_t start = continuation.index_;
    size_t size = propertyList.size() - start;
    uint32_t fieldNum = size / 2;
    JSHandle<JSMap> map = CreateMap();
    if (fieldNum == 0) {
        return JSHandle<JSTaggedValue>(map);
    }
    for (size_t i = 0; i < size; i += 2) { // 2: prop name and value
        JSMap::Set(thread_, map, propertyList[start + i], propertyList[start + i + 1]);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
    }
    return JSHandle<JSTaggedValue>(map);
}

template<typename T>
JSHandle<JSTaggedValue> JsonParser<T>::CreateSJsonMap(JsonContinuation continuation,
                                                      std::vector<JSHandle<JSTaggedValue>> &propertyList)
{
    size_t start = continuation.index_;
    size_t size = propertyList.size() - start;
    uint32_t fieldNum = size / 2; // 2: key-value pair
    JSHandle<JSSharedMap> jsMap = CreateSharedMap();
    if (fieldNum == 0) {
        return JSHandle<JSTaggedValue>(jsMap);
    } else if (LIKELY(fieldNum <= JSSharedMap::MAX_INLINE)) {
        for (size_t i = 0; i < size; i += 2) { // 2: prop name and value
            JSSharedMap::Set(thread_, jsMap, propertyList[start + i], propertyList[start + i + 1]);
        }
        return JSHandle<JSTaggedValue>(jsMap);
    }
    // build in dict mode
    JSMutableHandle<NameDictionary> dict(
        thread_, NameDictionary::CreateInSharedHeap(thread_, NameDictionary::ComputeHashTableSize(fieldNum)));
    JSMutableHandle<JSTaggedValue> propKey(thread_, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> propValue(thread_, JSTaggedValue::Undefined());
    // create dict and set key value
    for (size_t i = 0; i < size; i += 2) { // 2: prop name and value
        PropertyAttributes attributes = PropertyAttributes::Default(false, false, false);
        propKey.Update(propertyList[start + i]);
        propValue.Update(propertyList[start + i + 1]);
        JSHandle<NameDictionary> newDict = NameDictionary::PutIfAbsent(thread_, dict, propKey,
                                                                       propValue, attributes);
        dict.Update(newDict);
    }
    jsMap->SetProperties(thread_, dict);
    return JSHandle<JSTaggedValue>(jsMap);
}

template<typename T>
JSTaggedValue JsonParser<T>::SetPropertyByValue(const JSHandle<JSTaggedValue> &receiver,
    const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value)
{
    ASSERT(key->IsString());
    auto newKey = key.GetTaggedValue();
    auto stringAccessor = EcmaStringAccessor(newKey);
    if (!stringAccessor.IsLineString() || (stringAccessor.IsUtf8() &&
        IsNumberCharacter(*stringAccessor.GetDataUtf8()))) {
        uint32_t index = 0;
        if (stringAccessor.ToElementIndex(&index)) {
            return ObjectFastOperator::SetPropertyByIndex<ObjectFastOperator::Status::UseOwn>(thread_,
                receiver.GetTaggedValue(), index, value.GetTaggedValue());
        }
    }
    if (!stringAccessor.IsInternString()) {
        newKey = JSTaggedValue(thread_->GetEcmaVM()->GetFactory()->InternString(key));
    }
    return ObjectFastOperator::SetPropertyByName<ObjectFastOperator::Status::UseOwn>(thread_,
        receiver.GetTaggedValue(), newKey, value.GetTaggedValue());
}

template<typename T>
JSTaggedValue JsonParser<T>::ParseNumber(bool inObjorArr)
{
    if (inObjorArr) {
        bool isFast = true;
        int32_t fastInteger = 0;
        bool isNumber = ReadNumberRange(isFast, fastInteger);
        if (!isNumber) {
            THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Number in JSON Array Or Object",
                                          JSTaggedValue::Exception());
        }
        if (isFast) {
            return parseOptions_.bigIntMode == BigIntMode::ALWAYS_PARSE_AS_BIGINT ?
                BigInt::Int32ToBigInt(thread_, fastInteger).GetTaggedValue() : JSTaggedValue(fastInteger);
        }
    }

    Text current = current_;
    bool negative = false;
    bool hasExponent = false;
    bool hasDecimal = false;
    if (*current_ == '-') {
        if (UNLIKELY(current_++ == end_)) {
            THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Number in JSON", JSTaggedValue::Exception());
        }
        negative = true;
    }
    if (*current_ == '0') {
        if (!CheckZeroBeginNumber(hasExponent, hasDecimal)) {
            THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Number in JSON", JSTaggedValue::Exception());
        }
    } else if (*current_ >= '1' && *current_ <= '9') {
        if (!CheckNonZeroBeginNumber(hasExponent, hasDecimal)) {
            THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Number in JSON", JSTaggedValue::Exception());
        }
    } else {
        THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Number in JSON", JSTaggedValue::Exception());
    }

    std::string strNum(current, end_ + 1);
    current_ = end_ + 1;
    return ConvertToNumber(strNum, negative, hasExponent, hasDecimal);
}

template<typename T>
JSTaggedValue JsonParser<T>::ConvertToNumber(const std::string &str, bool negative, bool hasExponent, bool hasDecimal)
{
    errno = 0; // reset errno to 0 to avoid errno has been changed
    double v = std::strtod(str.c_str(), nullptr);
    if (errno == ERANGE) {
        // errno is ERANGE mean double value greater than DBL_MAX(1.79e+308) or less than DBL_MIN(2.22e-308),
        // by compromising precision, std::strtod support representation allows even smaller
        // values up to about 5e-324(Number.MIN_VALUE).
        errno = 0;
        return JSTaggedValue(v);
    }
    errno = 0;
    if (negative && v == 0) {
        return JSTaggedValue(-0.0);
    }
    if (parseOptions_.bigIntMode == BigIntMode::DEFAULT) {
        return JSTaggedValue::TryCastDoubleToInt32(v);
    }
    if (NumberHelper::IsSafeIntegerNumber(v)) {
        if (parseOptions_.bigIntMode == BigIntMode::ALWAYS_PARSE_AS_BIGINT) {
            if (v == 0.0) {
                return BigInt::Int32ToBigInt(thread_, 0).GetTaggedValue();
            }
            JSTaggedValue value = BigInt::DoubleToBigInt(thread_, v);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
            if (value.IsBigInt()) {
                return value;
            }
            THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Text in JSON: ConvertToNumber Fail",
                                          JSTaggedValue::Exception());
        }
        return JSTaggedValue::TryCastDoubleToInt32(v);
    } else {
        return (hasExponent || hasDecimal) ? JSTaggedValue::TryCastDoubleToInt32(v) :
            NumberHelper::StringToBigInt(thread_, JSHandle<JSTaggedValue>::Cast(factory_->NewFromStdString(str)));
    }
}

template<typename T>
bool JsonParser<T>::ParseStringLength(size_t &length, bool &isAscii, bool inObjOrArrOrMap)
{
    Text last = inObjOrArrOrMap ? range_ : end_;
    for (Text current = current_; current < last; ++current) {
        T c = *current;
        if (inObjOrArrOrMap && c == '"') {
            end_ = current;
            return true;
        } else if (c == '\\') {
            if (UNLIKELY(!CheckBackslash(current, last, isAscii))) {
                return false;
            }
        } else if (UNLIKELY(c < CODE_SPACE)) {
            return false;
        } else if (c > ASCII_END) {
            ASSERT(sizeof(T) == sizeof(uint16_t));
            isAscii = false;
        }
        ++length;
    }
    return !inObjOrArrOrMap;
}

template<typename T>
bool JsonParser<T>::CheckBackslash(Text &text, Text last, bool &isAscii)
{
    ASSERT(*text == '\\');
    ++text;
    if (text >= last) {
        return false;
    }
    switch (*text) {
        case '\"':
        case '\\':
        case '/':
        case 'b':
        case 'f':
        case 'n':
        case 'r':
        case 't':
            break;
        case 'u': {
            if (text + UNICODE_DIGIT_LENGTH >= last) {
                return false;
            };
            T ucharFirst = *++text;
            if (ucharFirst == '0') {
                // do nothing
            } else if ((ucharFirst >= '1' && ucharFirst <= '9') ||
                        (ucharFirst >= 'A' && ucharFirst <= 'F') || (ucharFirst >= 'a' && ucharFirst <= 'f')) {
                isAscii = false;  // >= \u1000
            } else {
                return false;
            }
            T ucharSecond = *++text;
            if (ucharSecond == '0') {
                // do nothing
            } else if ((ucharSecond >= '1' && ucharSecond <= '9') ||
                        (ucharSecond >= 'A' && ucharSecond <= 'F') || (ucharSecond >= 'a' && ucharSecond <= 'f')) {
                isAscii = false;  // >= \u0100
            } else {
                return false;
            }
            bool thirdZero = false;
            T ucharThird = *++text;
            if (ucharThird == '0') {
                thirdZero = true;
            } else if (ucharThird >= '1' && ucharThird <= '7') {
                // do nothing
            } else if ((ucharThird >= '8' && ucharThird <= '9') ||
                        (ucharThird >= 'A' && ucharThird <= 'F') || (ucharThird >= 'a' && ucharThird <= 'f')) {
                isAscii = false;  // >= \u0080
            } else {
                return false;
            }
            T ucharFourth = *++text;
            if (thirdZero && ucharFourth == '0') {
                isAscii = false;  // \uxx00
            } else if ((ucharFourth >= '0' && ucharFourth <= '9') ||
                        (ucharFourth >= 'A' && ucharFourth <= 'F') || (ucharFourth >= 'a' && ucharFourth <= 'f')) {
                // do nothing
            } else {
                return false;
            }
            break;
        }
        default:
            return false;
    }
    return true;
}

template<typename T>
template<typename Char>
void JsonParser<T>::ParseBackslash(Char *&p)
{
    ASSERT(current_ < end_);
    Advance();
    switch (*current_) {
        case '\"':
            *p++ = '\"';
            break;
        case '\\':
            *p++ = '\\';
            break;
        case '/':
            *p++ = '/';
            break;
        case 'b':
            *p++ = '\b';
            break;
        case 'f':
            *p++ = '\f';
            break;
        case 'n':
            *p++ = '\n';
            break;
        case 'r':
            *p++ = '\r';
            break;
        case 't':
            *p++ = '\t';
            break;
        case 'u': {
            ASSERT(end_ - current_ >= UNICODE_DIGIT_LENGTH);
            uint16_t res = 0;
            for (size_t pos = 0; pos < UNICODE_DIGIT_LENGTH; pos++) {
                Advance();
                T uchar = *current_;
                if (uchar >= '0' && uchar <= '9') {
                    res *= NUMBER_SIXTEEN;
                    res += (uchar - '0');
                } else if (uchar >= 'a' && uchar <= 'f') {
                    res *= NUMBER_SIXTEEN;
                    res += (uchar - 'a' + NUMBER_TEN);
                } else if (uchar >= 'A' && uchar <= 'F') {
                    res *= NUMBER_SIXTEEN;
                    res += (uchar - 'A' + NUMBER_TEN);
                } else {
                    UNREACHABLE();
                }
            }
            ASSERT(sizeof(Char) == sizeof(uint16_t) || res <= ASCII_END);
            *p++ = static_cast<Char>(res);
            break;
        }
        default:
            UNREACHABLE();
    }
}

template<typename T>
template<typename Char>
void JsonParser<T>::CopyCharWithBackslash(Char *&p)
{
    while (current_ <= end_) {
        T c = *current_;
        ASSERT(c >= CODE_SPACE);
        ASSERT(sizeof(Char) == sizeof(uint16_t) || c <= ASCII_END);
        if (c == '\\') {
            ParseBackslash(p);
        } else {
            *p++ = c;
        }
        Advance();
    }
}

template<typename T>
JSHandle<JSTaggedValue> JsonParser<T>::ParseStringWithBackslash(bool inObjOrArrOrMap)
{
    size_t length = 0;
    bool isAscii = true;
    if (UNLIKELY(!ParseStringLength(length, isAscii, inObjOrArrOrMap))) {
        THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected string in JSON",
            JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Exception()));
    }
    end_--;
    if (isAscii) {
        EcmaString *str = EcmaStringAccessor::CreateLineString(thread_->GetEcmaVM(), length, true);
        uint8_t *data = const_cast<uint8_t *>(EcmaStringAccessor(str).GetDataUtf8());
        uint8_t *p = data;
        CopyCharWithBackslash(p);
        ASSERT(p - data == length);
        Advance();
        return JSHandle<JSTaggedValue>(thread_, str);
    } else {
        EcmaString *str = EcmaStringAccessor::CreateLineString(thread_->GetEcmaVM(), length, false);
        uint16_t *data = const_cast<uint16_t *>(EcmaStringAccessor(str).GetDataUtf16());
        uint16_t *p = data;
        CopyCharWithBackslash(p);
        ASSERT(p - data == length);
        Advance();
        return JSHandle<JSTaggedValue>(thread_, str);
    }
}

template<typename T>
void JsonParser<T>::SkipEndWhiteSpace()
{
    while (current_ != end_) {
        if (*end_ == ' ' || *end_ == '\r' || *end_ == '\n' || *end_ == '\t') {
            end_--;
        } else {
            break;
        }
    }
}

template<typename T>
void JsonParser<T>::SkipStartWhiteSpace()
{
    while (current_ != end_) {
        if (*current_ == ' ' || *current_ == '\r' || *current_ == '\n' || *current_ == '\t') {
            Advance();
        } else {
            break;
        }
    }
}

template<typename T>
void JsonParser<T>::GetNextNonSpaceChar()
{
    Advance();
    SkipStartWhiteSpace();
}

template<typename T>
Tokens JsonParser<T>::ParseToken()
{
    switch (*current_) {
        case '{':
            return parseOptions_.returnType == ParseReturnType::MAP ? Tokens::MAP : Tokens::OBJECT;
        case '[':
            return Tokens::ARRAY;
        case '"':
            return Tokens::STRING;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '-':
            return Tokens::NUMBER;
        case 't':
            return Tokens::LITERAL_TRUE;
        case 'f':
            return Tokens::LITERAL_FALSE;
        case 'n':
            return Tokens::LITERAL_NULL;
        default:
            return Tokens::TOKEN_ILLEGAL;
    }
}

template<typename T>
JSTaggedValue JsonParser<T>::ParseLiteralTrue()
{
    static const char literalTrue[] = "true";
    uint32_t remainingLength = range_ - current_;
    if (UNLIKELY(remainingLength < 3)) { // 3: literalTrue length - 1
        THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Text in JSON: ParseLiteralTrue Fail",
                                      JSTaggedValue::Exception());
    }
    bool isMatch = MatchText(literalTrue, 4); // 4: literalTrue length
    if (LIKELY(isMatch)) {
        return JSTaggedValue::True();
    }
    THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Text in JSON: ParseLiteralTrue Fail",
                                  JSTaggedValue::Exception());
}

template<typename T>
JSTaggedValue JsonParser<T>::ParseLiteralFalse()
{
    static const char literalFalse[] = "false";
    uint32_t remainingLength = range_ - current_;
    if (UNLIKELY(remainingLength < 4)) { // 4: literalFalse length - 1
        THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Text in JSON: ParseLiteralFalse Fail",
                                      JSTaggedValue::Exception());
    }
    bool isMatch = MatchText(literalFalse, 5); // 5: literalFalse length
    if (LIKELY(isMatch)) {
        return JSTaggedValue::False();
    }
    THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Text in JSON: ParseLiteralFalse Fail",
                                  JSTaggedValue::Exception());
}

template<typename T>
JSTaggedValue JsonParser<T>::ParseLiteralNull()
{
    static const char literalNull[] = "null";
    uint32_t remainingLength = range_ - current_;
    if (UNLIKELY(remainingLength < 3)) { // 3: literalNull length - 1
        THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Text in JSON: ParseLiteralNull Fail",
                                      JSTaggedValue::Exception());
    }
    bool isMatch = MatchText(literalNull, 4); // 4: literalNull length
    if (LIKELY(isMatch)) {
        return JSTaggedValue::Null();
    }
    THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Text in JSON: ParseLiteralNull Fail",
                                  JSTaggedValue::Exception());
}

template<typename T>
bool JsonParser<T>::MatchText(const char *str, uint32_t matchLen)
{
    // first char is already matched
    for (uint32_t pos = 1; pos < matchLen; ++pos) {
        if (current_[pos] != str[pos]) {
            return false;
        }
    }
    current_ += matchLen;
    return true;
}

template<typename T>
bool JsonParser<T>::ReadNumberRange(bool &isFast, int32_t &fastInteger)
{
    Text current = current_;
    int32_t sign = 1;
    if (*current == '-') {
        current++;
        sign = -1;
    }

    if (*current == '0') {
        isFast = false;
        current++;
    } else {
        Text advance = AdvanceLastNumberCharacter(current);
        if (UNLIKELY(current == advance)) {
            return false;
        }
        size_t numberLength = advance - current;
        int32_t i = 0;
        if (numberLength <= INTEGER_MAX_LEN && (*advance == ',' || *advance == ']' || *advance == '}')) {
            for (; current != advance; current++) {
                i = (i * 10U) + ((*current) - '0');
            }
            fastInteger = i * sign;
            current_ = advance;
            return true;
        }
        isFast = false;
    }

    while (current != range_) {
        if (IsNumberCharacter(*current)) {
            current++;
            continue;
        } else if (IsNumberSignalCharacter(*current)) {
            isFast = false;
            current++;
            continue;
        }
        Text end = current;
        while (current != range_) {
            if (*current == ' ' || *current == '\r' || *current == '\n' || *current == '\t') {
                current++;
            } else if (*current == ',' || *current == ']' || *current == '}') {
                end_ = end - 1;
                return true;
            } else {
                return false;
            }
        }
        if (*current == ']' || *current == '}') {
            end_ = end - 1;
            return true;
        }
        return false;
    }
    end_ = range_ - 1;
    return true;
}

template<typename T>
typename JsonParser<T>::Text JsonParser<T>::AdvanceLastNumberCharacter(Text current)
{
    return std::find_if(current, range_, [this](T c) { return !IsNumberCharacter(c); });
}

template<typename T>
bool JsonParser<T>::IsNumberCharacter(T ch)
{
    if (ch >= '0' && ch <= '9') {
        return true;
    }
    return false;
}

template<typename T>
bool JsonParser<T>::IsNumberSignalCharacter(T ch)
{
    return ch == '.' || ch == 'e' || ch == 'E' || ch == '+' || ch == '-';
}

template<typename T>
bool JsonParser<T>::IsExponentNumber()
{
    if (IsNumberCharacter(*current_)) {
        return true;
    } else if (*current_ == '-' || *current_ == '+') {
        if (current_ == end_) {
            return false;
        }
        Advance();
        if (IsNumberCharacter(*current_)) {
            return true;
        }
    }
    return false;
}

template<typename T>
bool JsonParser<T>::IsDecimalsLegal(bool &hasExponent)
{
    if (current_ == end_ && !IsNumberCharacter(*++current_)) {
        return false;
    }

    while (current_ != end_) {
        Advance();
        if (IsNumberCharacter(*current_)) {
            continue;
        } else if (*current_ == 'e' || *current_ == 'E') {
            if (hasExponent || current_ == end_) {
                return false;
            }
            Advance();
            if (!IsExponentNumber()) {
                return false;
            }
            hasExponent = true;
        } else {
            return false;
        }
    }
    return true;
}

template<typename T>
bool JsonParser<T>::IsExponentLegal(bool &hasExponent)
{
    if (hasExponent || current_ == end_) {
        return false;
    }
    Advance();
    if (!IsExponentNumber()) {
        return false;
    }
    while (current_ != end_) {
        if (!IsNumberCharacter(*current_)) {
            return false;
        }
        Advance();
    }
    return true;
}

template<typename T>
bool JsonParser<T>::CheckZeroBeginNumber(bool &hasExponent, bool &hasDecimal)
{
    if (current_++ != end_) {
        if (*current_ == '.') {
            hasDecimal = true;
            if (!IsDecimalsLegal(hasExponent)) {
                return false;
            }
        } else if (*current_ == 'e' || *current_ == 'E') {
            if (!IsExponentLegal(hasExponent)) {
                return false;
            }
        } else {
            return false;
        }
    }
    return true;
}

template<typename T>
bool JsonParser<T>::CheckNonZeroBeginNumber(bool &hasExponent, bool &hasDecimal)
{
    while (current_ != end_) {
        Advance();
        if (IsNumberCharacter(*current_)) {
            continue;
        } else if (*current_ == '.') {
            hasDecimal = true;
            if (!IsDecimalsLegal(hasExponent)) {
                return false;
            }
        } else if (*current_ == 'e' || *current_ == 'E') {
            if (!IsExponentLegal(hasExponent)) {
                return false;
            }
        } else {
            return false;
        }
    }
    return true;
}

JSHandle<JSTaggedValue> Utf8JsonParser::Parse(const JSHandle<EcmaString> &strHandle)
{
    ASSERT(*strHandle != nullptr);
    auto stringAccessor = EcmaStringAccessor(strHandle);
    uint32_t len = stringAccessor.GetLength();
    ASSERT(len != UINT32_MAX);

    uint32_t slicedOffset = 0;
    if (LIKELY(stringAccessor.IsLineOrConstantString())) {
        sourceString_ = strHandle;
    } else if (stringAccessor.IsSlicedString()) {
        auto *sliced = static_cast<SlicedString *>(*strHandle);
        slicedOffset = sliced->GetStartIndex();
        sourceString_ = JSHandle<EcmaString>(thread_, EcmaString::Cast(sliced->GetParent()));
    } else {
        auto *flatten = EcmaStringAccessor::Flatten(thread_->GetEcmaVM(), strHandle);
        sourceString_ = JSHandle<EcmaString>(thread_, flatten);
    }
    begin_ = EcmaStringAccessor(sourceString_).GetDataUtf8();
    auto *heap = const_cast<Heap *>(thread_->GetEcmaVM()->GetHeap());
    auto listenerId = heap->AddGCListener(UpdatePointersListener, this);
    auto res = Launch(begin_ + slicedOffset, begin_ + slicedOffset + len);
    heap->RemoveGCListener(listenerId);
    return res;
}

void Utf8JsonParser::ParticalParseString(std::string& str, Text current, Text nextCurrent)
{
    str += std::string_view(reinterpret_cast<const char *>(current), nextCurrent - current);
}

void Utf8JsonParser::UpdatePointersListener(void *utf8Parser)
{
    auto *parser = reinterpret_cast<Utf8JsonParser *>(utf8Parser);
    auto *begin = EcmaStringAccessor(parser->sourceString_).GetDataUtf8();
    if (parser->begin_ != begin) {
        uint32_t currentOffset = parser->current_ - parser->begin_;
        uint32_t endOffset = parser->end_ - parser->begin_;
        uint32_t rangeOffset = parser->range_ - parser->begin_;
        parser->current_ = reinterpret_cast<uint8_t *>(ToUintPtr(begin) + currentOffset);
        parser->end_ = reinterpret_cast<uint8_t *>(ToUintPtr(begin) + endOffset);
        parser->range_ = reinterpret_cast<uint8_t *>(ToUintPtr(begin) + rangeOffset);
        parser->begin_ = begin;
    }
}

JSHandle<JSTaggedValue> Utf8JsonParser::ParseString(bool inObjOrArrOrMap)
{
    bool isFastString = true;
    if (inObjOrArrOrMap) {
        if (UNLIKELY(!ReadJsonStringRange(isFastString))) {
            THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected end Text in JSON",
                JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Exception()));
        }
        if (isFastString) {
            uint32_t offset = current_ - begin_;
            uint32_t strLength = end_ - current_;
            ASSERT(strLength <= static_cast<size_t>(UINT32_MAX));
            current_ = end_ + 1;
            auto *utf8Data = EcmaStringAccessor(sourceString_).GetDataUtf8() + offset;
            if (strLength == 1 && EcmaStringAccessor::IsASCIICharacter(utf8Data[0])) {
                int32_t ch = static_cast<int32_t>(utf8Data[0]);
                JSHandle<SingleCharTable> singleCharTable(thread_, thread_->GetSingleCharTable());
                return JSHandle<JSTaggedValue>(thread_, singleCharTable->GetStringFromSingleCharTable(ch));
            }
            return JSHandle<JSTaggedValue>::Cast(factory_->NewCompressedUtf8SubString(
                sourceString_, offset, strLength));
        }
    } else {
        if (UNLIKELY(*end_ != '"' || current_ == end_ || !IsFastParseJsonString(isFastString))) {
            THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected end Text in JSON",
                JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Exception()));
        }
        if (LIKELY(isFastString)) {
            uint32_t offset = current_ - begin_;
            uint32_t strLength = end_ - current_;
            ASSERT(strLength <= static_cast<size_t>(UINT32_MAX));
            current_ = end_ + 1;
            return JSHandle<JSTaggedValue>::Cast(factory_->NewFromUtf8LiteralCompressSubString(
                sourceString_, offset, strLength));
        }
    }
    return ParseStringWithBackslash(inObjOrArrOrMap);
}

bool Utf8JsonParser::ReadJsonStringRange(bool &isFastString)
{
    Advance();
    // chars are within Ascii
    for (Text current = current_; current != range_; ++current) {
        uint8_t c = *current;
        if (c == '"') {
            end_ = current;
            return true;
        } else if (UNLIKELY(c == '\\')) {
            isFastString = false;
            // early return for ParseStringWithBackslash
            return true;
        } else if (UNLIKELY(c < CODE_SPACE)) {
            return false;
        }
    }
    return false;
}

bool Utf8JsonParser::IsFastParseJsonString(bool &isFastString)
{
    Advance();
    // chars are within Ascii
    for (Text current = current_; current != end_; ++current) {
        if (*current < CODE_SPACE) {
            return false;
        } else if (*current == '\\') {
            isFastString = false;
            // early return for ParseStringWithBackslash
            return true;
        }
    }
    return true;
}

JSHandle<JSTaggedValue> Utf16JsonParser::Parse(EcmaString *str)
{
    ASSERT(str != nullptr);
    uint32_t len = EcmaStringAccessor(str).GetLength();
    CVector<uint16_t> buf(len + 1, 0);
    EcmaStringAccessor(str).WriteToFlatUtf16(buf.data(), len);
    Text begin = buf.data();
    return Launch(begin, begin + len);
}

void Utf16JsonParser::ParticalParseString(std::string& str, Text current, Text nextCurrent)
{
    str += StringHelper::U16stringToString(std::u16string(current, nextCurrent));
}

JSHandle<JSTaggedValue> Utf16JsonParser::ParseString(bool inObjOrArrOrMap)
{
    bool isFastString = true;
    bool isAscii = true;
    if (inObjOrArrOrMap) {
        if (UNLIKELY(!ReadJsonStringRange(isFastString, isAscii))) {
            THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected end Text in JSON",
                JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Exception()));
        }
        if (isFastString) {
            if (isAscii) {
                std::string value(current_, end_); // from uint16_t* to std::string, can't use std::string_view
                current_ = end_ + 1;
                ASSERT(value.size() <= static_cast<size_t>(UINT32_MAX));
                return JSHandle<JSTaggedValue>::Cast(factory_->NewFromUtf8LiteralCompress(
                    reinterpret_cast<const uint8_t *>(value.c_str()), value.size()));
            }
            std::u16string_view value(reinterpret_cast<const char16_t *>(current_), end_ - current_);
            current_ = end_ + 1;
            ASSERT(value.size() <= static_cast<size_t>(UINT32_MAX));
            return JSHandle<JSTaggedValue>::Cast(factory_->NewFromUtf16LiteralNotCompress(
                reinterpret_cast<const uint16_t *>(value.data()), value.size()));
        }
    } else {
        if (UNLIKELY(*end_ != '"' || current_ == end_ || !IsFastParseJsonString(isFastString, isAscii))) {
            THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected end Text in JSON",
                JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Exception()));
        }
        if (LIKELY(isFastString)) {
            if (isAscii) {
                std::string value(current_, end_);  // from uint16_t* to std::string, can't use std::string_view
                ASSERT(value.size() <= static_cast<size_t>(UINT32_MAX));
                current_ = end_ + 1;
                return JSHandle<JSTaggedValue>::Cast(factory_->NewFromUtf8LiteralCompress(
                    reinterpret_cast<const uint8_t *>(value.c_str()), value.size()));
            }
            std::u16string_view value(reinterpret_cast<const char16_t *>(current_), end_ - current_);
            ASSERT(value.size() <= static_cast<size_t>(UINT32_MAX));
            current_ = end_ + 1;
            return JSHandle<JSTaggedValue>::Cast(factory_->NewFromUtf16LiteralNotCompress(
                reinterpret_cast<const uint16_t *>(value.data()), value.size()));
        }
    }
    return ParseStringWithBackslash(inObjOrArrOrMap);
}

bool Utf16JsonParser::ReadJsonStringRange(bool &isFastString, bool &isAscii)
{
    Advance();
    for (Text current = current_; current != range_; ++current) {
        uint16_t c = *current;
        if (c == '"') {
            end_ = current;
            return true;
        } else if (UNLIKELY(c == '\\')) {
            isFastString = false;
            // early return for ParseStringWithBackslash
            return true;
        }
        if (!IsLegalAsciiCharacter(c, isAscii)) {
            return false;
        }
    }
    return false;
}

bool Utf16JsonParser::IsFastParseJsonString(bool &isFastString, bool &isAscii)
{
    Advance();
    for (Text current = current_; current != end_; ++current) {
        if (!IsLegalAsciiCharacter(*current, isAscii)) {
            return false;
        }
        if (*current == '\\') {
            isFastString = false;
            // early return for ParseStringWithBackslash
            return true;
        }
    }
    return true;
}

bool Utf16JsonParser::IsLegalAsciiCharacter(uint16_t c, bool &isAscii)
{
    if (c <= ASCII_END) {
        return c >= CODE_SPACE ? true : false;
    }
    isAscii = false;
    return true;
}

JSHandle<JSTaggedValue> Internalize::InternalizeJsonProperty(JSThread *thread, const JSHandle<JSObject> &holder,
                                                             const JSHandle<JSTaggedValue> &name,
                                                             const JSHandle<JSTaggedValue> &receiver,
                                                             TransformType transformType)
{
    JSHandle<JSTaggedValue> objHandle(holder);
    JSHandle<JSTaggedValue> val = JSTaggedValue::GetProperty(thread, objHandle, name).GetValue();
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
    if (val->IsECMAObject()) {
        JSHandle<JSObject> obj = JSTaggedValue::ToObject(thread, val);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
        bool isArray = val->IsArray(thread);
        if (isArray) {
            JSHandle<JSTaggedValue> lenResult = JSTaggedValue::GetProperty(thread, val, lengthKey).GetValue();
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
            JSTaggedNumber lenNumber = JSTaggedValue::ToLength(thread, lenResult);
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
            uint32_t length = lenNumber.ToUint32();
            JSMutableHandle<JSTaggedValue> keyUnknow(thread, JSTaggedValue::Undefined());
            JSMutableHandle<JSTaggedValue> keyName(thread, JSTaggedValue::Undefined());
            for (uint32_t i = 0; i < length; i++) {
                // Let prop be ! ToString((I)).
                keyUnknow.Update(JSTaggedValue(i));
                keyName.Update(JSTaggedValue::ToString(thread, keyUnknow).GetTaggedValue());
                RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
                RecurseAndApply(thread, obj, keyName, receiver, transformType);
                RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
            }
        } else {
            // Let keys be ? EnumerableOwnPropertyNames(val, key).
            JSHandle<TaggedArray> ownerNames(JSObject::EnumerableOwnNames(thread, obj));
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
            uint32_t namesLength = ownerNames->GetLength();
            JSMutableHandle<JSTaggedValue> keyName(thread, JSTaggedValue::Undefined());
            for (uint32_t i = 0; i < namesLength; i++) {
                keyName.Update(ownerNames->Get(i));
                RecurseAndApply(thread, obj, keyName, receiver, transformType);
                RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
            }
        }
    }

    // Return ? Call(receiver, holder, « name, val »).
    const uint32_t argsLength = 2;  // 2: « name, val »
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, receiver, objHandle, undefined, argsLength);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    info->SetCallArg(name.GetTaggedValue(), val.GetTaggedValue());
    JSTaggedValue result = JSFunction::Call(info);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    return JSHandle<JSTaggedValue>(thread, result);
}

bool Internalize::RecurseAndApply(JSThread *thread, const JSHandle<JSObject> &holder,
                                  const JSHandle<JSTaggedValue> &name, const JSHandle<JSTaggedValue> &receiver,
                                  TransformType transformType)
{
    STACK_LIMIT_CHECK(thread, false);
    JSHandle<JSTaggedValue> value = InternalizeJsonProperty(thread, holder, name, receiver, transformType);
    bool changeResult = false;

    // If newElement is undefined, then Perform ? val.[[Delete]](P).
    if (value->IsUndefined()) {
        SCheckMode sCheckMode = transformType == TransformType::SENDABLE ? SCheckMode::SKIP : SCheckMode::CHECK;
        changeResult = JSObject::DeleteProperty(thread, holder, name, sCheckMode);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    } else {
        // Perform ? CreateDataProperty(val, P, newElement)
        changeResult = JSObject::CreateDataProperty(thread, holder, name, value);
    }
    return changeResult;
}
}  // namespace panda::ecmascript::base
