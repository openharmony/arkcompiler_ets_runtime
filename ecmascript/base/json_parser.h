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

#ifndef ECMASCRIPT_BASE_JSON_PARSE_INL_H
#define ECMASCRIPT_BASE_JSON_PARSE_INL_H

#include <cerrno>

#include "ecmascript/base/json_helper.h"
#include "ecmascript/base/builtins_base.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_map.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/message_string.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/object_fast_operator-inl.h"
#include "ecmascript/shared_objects/js_shared_map.h"

namespace panda::ecmascript::base {
constexpr unsigned int UNICODE_DIGIT_LENGTH = 4;
constexpr unsigned int NUMBER_TEN = 10;
constexpr unsigned int NUMBER_SIXTEEN = 16;
constexpr unsigned int INTEGER_MAX_LEN = 9;

constexpr unsigned char ASCII_END = 0X7F;
enum class Tokens : uint8_t {
        // six structural tokens
        OBJECT = 0,
        ARRAY,
        NUMBER,
        STRING,
        LITERAL_TRUE,
        LITERAL_FALSE,
        LITERAL_NULL,
        TOKEN_ILLEGAL,
        MAP,
    };

struct JsonContinuation {
    enum class ContinuationType : uint8_t {
        RETURN = 0,
        ARRAY,
        OBJECT,
        MAP,
    };
    JsonContinuation(ContinuationType type, size_t index) : type_(type), index_(index) {}

    ContinuationType type_ {ContinuationType::RETURN};
    size_t index_ {0};
};

template<typename T>
class JsonParser {
protected:
    using BigIntMode =  base::JsonHelper::BigIntMode;
    using ParseReturnType = base::JsonHelper::ParseReturnType;
    using ParseOptions =  base::JsonHelper::ParseOptions;
    using TransformType = base::JsonHelper::TransformType;
    using Text = const T *;
    using ContType = JsonContinuation::ContinuationType;
    // Instantiation of the class is prohibited
    JsonParser() = default;
    JsonParser(JSThread *thread, TransformType transformType,
               ParseOptions options = ParseOptions())
        : thread_(thread), transformType_(transformType), parseOptions_(options)
    {
    }
    virtual ~JsonParser() = default;
    NO_COPY_SEMANTIC(JsonParser);
    NO_MOVE_SEMANTIC(JsonParser);

    JSHandle<JSTaggedValue> Launch(Text begin, Text end);

    inline bool IsInObjOrArrayOrMap(ContType type)
    {
        return type == ContType::ARRAY || type == ContType::OBJECT || type == ContType::MAP;
    }

    inline bool EmptyArrayCheck()
    {
        GetNextNonSpaceChar();
        return *current_ == ']';
    }

    inline bool EmptyObjectCheck()
    {
        GetNextNonSpaceChar();
        return *current_ == '}';
    }

    JSHandle<JSTaggedValue> GetSJsonPrototype()
    {
        JSHandle<JSFunction> sObjFunction(thread_->GetEcmaVM()->GetGlobalEnv()->GetSObjectFunction());
        JSHandle<JSTaggedValue> jsonPrototype = JSHandle<JSTaggedValue>(thread_,
            sObjFunction->GetFunctionPrototype(thread_));
        return jsonPrototype;
    }

    JSHandle<JSTaggedValue> GetSMapPrototype()
    {
        auto globalEnv = thread_->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSTaggedValue> proto = globalEnv->GetSharedMapPrototype();
        return proto;
    }

    JSTaggedValue ParseJSONText();

    JSHandle<JSTaggedValue> CreateJsonArray(JsonContinuation continuation,
                                            std::vector<JSHandle<JSTaggedValue>> &elementsList);

    JSHandle<JSTaggedValue> CreateSJsonArray([[maybe_unused]] JsonContinuation continuation,
                                             [[maybe_unused]] std::vector<JSHandle<JSTaggedValue>> &elementsList);

    JSHandle<JSTaggedValue> CreateJsonObject(JsonContinuation continuation,
                                             std::vector<JSHandle<JSTaggedValue>> &propertyList);

    JSHandle<JSTaggedValue> CreateSJsonObject(JsonContinuation continuation,
                                              std::vector<JSHandle<JSTaggedValue>> &propertyList);
    
    JSHandle<JSSharedMap> CreateSharedMap();

    JSHandle<JSMap> CreateMap();

    JSHandle<JSTaggedValue> CreateJsonMap(JsonContinuation continuation,
                                          std::vector<JSHandle<JSTaggedValue>> &propertyList);

    JSHandle<JSTaggedValue> CreateSJsonMap(JsonContinuation continuation,
                                           std::vector<JSHandle<JSTaggedValue>> &propertyList);
    
    JSTaggedValue SetPropertyByValue(const JSHandle<JSTaggedValue> &receiver, const JSHandle<JSTaggedValue> &key,
                                     const JSHandle<JSTaggedValue> &value);

    JSTaggedValue ParseNumber(bool inObjorArr = false);

    JSTaggedValue ConvertToNumber(const std::string &str, bool negative, bool hasExponent, bool hasDecimal);

    bool ParseStringLength(size_t &length, bool &isAscii, bool inObjorArr);

    bool CheckBackslash(Text &text, Text last, bool &isAscii);

    template<typename Char>
    void ParseBackslash(Char *&p);

    template<typename Char>
    void CopyCharWithBackslash(Char *&p);

    JSHandle<JSTaggedValue> ParseStringWithBackslash(bool inObjorArr);
    virtual void ParticalParseString(std::string& str, Text current, Text nextCurrent) = 0;

    virtual JSHandle<JSTaggedValue> ParseString(bool inObjorArr = false) = 0;

    void SkipEndWhiteSpace();

    void SkipStartWhiteSpace();

    void GetNextNonSpaceChar();

    Tokens ParseToken();

    JSTaggedValue ParseLiteralTrue();

    JSTaggedValue ParseLiteralFalse();

    JSTaggedValue ParseLiteralNull();

    bool MatchText(const char *str, uint32_t matchLen);

    bool ReadNumberRange(bool &isFast, int32_t &fastInteger);

    Text AdvanceLastNumberCharacter(Text current);

    bool IsNumberCharacter(T ch);

    bool IsNumberSignalCharacter(T ch);

    bool IsExponentNumber();

    bool IsDecimalsLegal(bool &hasExponent);

    bool IsExponentLegal(bool &hasExponent);

    bool CheckZeroBeginNumber(bool &hasExponent, bool &hasDecimal);

    bool CheckNonZeroBeginNumber(bool &hasExponent, bool &hasDecimal);

    inline void Advance()
    {
        ++current_;
    }

    inline void AdvanceMultiStep(int step)
    {
        current_ += step;
    }

    Text end_ {nullptr};
    Text current_ {nullptr};
    Text range_ {nullptr};
    JSThread *thread_ {nullptr};
    ObjectFactory *factory_ {nullptr};
    GlobalEnv *env_ {nullptr};
    TransformType transformType_ {TransformType::NORMAL};
    ParseOptions parseOptions_;
    JSHandle<JSHClass> initialJSArrayClass_;
    JSHandle<JSHClass> initialJSObjectClass_;
};

class PUBLIC_API Utf8JsonParser final : public JsonParser<uint8_t> {
public:
    Utf8JsonParser() = default;
    Utf8JsonParser(JSThread *thread, TransformType transformType,
                   ParseOptions options = ParseOptions())
        : JsonParser(thread, transformType, options) {}
    ~Utf8JsonParser() = default;
    NO_COPY_SEMANTIC(Utf8JsonParser);
    NO_MOVE_SEMANTIC(Utf8JsonParser);

    JSHandle<JSTaggedValue> PUBLIC_API Parse(const JSHandle<EcmaString> &strHandle);

private:
    void ParticalParseString(std::string& str, Text current, Text nextCurrent) override;

    static void UpdatePointersListener(void *utf8Parser);

    JSHandle<JSTaggedValue> ParseString(bool inObjOrArrOrMap  = false) override;

    bool ReadJsonStringRange(bool &isFastString);

    bool IsFastParseJsonString(bool &isFastString);

    const uint8_t *begin_ {nullptr};
    JSHandle<EcmaString> sourceString_;
};

class Utf16JsonParser final : public JsonParser<uint16_t> {
public:
    Utf16JsonParser() = default;
    Utf16JsonParser(JSThread *thread, TransformType transformType,
                    ParseOptions options = ParseOptions())
        : JsonParser(thread, transformType, options) {}
    ~Utf16JsonParser() = default;
    NO_COPY_SEMANTIC(Utf16JsonParser);
    NO_MOVE_SEMANTIC(Utf16JsonParser);

    JSHandle<JSTaggedValue> Parse(EcmaString *str);

private:
    void ParticalParseString(std::string& str, Text current, Text nextCurrent) override;
    JSHandle<JSTaggedValue> ParseString(bool inObjOrArrOrMap = false) override;

    bool ReadJsonStringRange(bool &isFastString, bool &isAscii);

    bool IsFastParseJsonString(bool &isFastString, bool &isAscii);

    bool IsLegalAsciiCharacter(uint16_t c, bool &isAscii);
};

class Internalize {
public:
    using TransformType = base::JsonHelper::TransformType;
    static JSHandle<JSTaggedValue> InternalizeJsonProperty(JSThread *thread, const JSHandle<JSObject> &holder,
                                                           const JSHandle<JSTaggedValue> &name,
                                                           const JSHandle<JSTaggedValue> &receiver,
                                                           TransformType transformType);
private:
    static bool RecurseAndApply(JSThread *thread, const JSHandle<JSObject> &holder, const JSHandle<JSTaggedValue> &name,
                                const JSHandle<JSTaggedValue> &receiver, TransformType transformType);
};
}  // namespace panda::ecmascript::base

#endif  // ECMASCRIPT_BASE_JSON_PARSE_INL_H
