/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_REGEXP_INL_H
#define ECMASCRIPT_BUILTINS_BUILTINS_REGEXP_INL_H

#include "ecmascript/global_env.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/js_array.h"

namespace panda::ecmascript::builtins {

/* static */
template <int N>
JSTaggedValue RegExpGlobalResult::GetCapture(JSThread *thread)
{
    JSHandle<JSTaggedValue> table = JSHandle<JSTaggedValue>(thread, RegExpExecResultCache::GetGlobalTable(thread));
    JSHandle<RegExpGlobalResult> globalTable = JSHandle<RegExpGlobalResult>::Cast(table);
    JSTaggedValue res = globalTable->Get(CAPTURE_START_INDEX + N - 1);
    int captureNum = globalTable->GetTotalCaptureCounts().GetInt();
    if (res.IsHole() && (N < captureNum)) {
        int startIndex = globalTable->GetStartOfCaptureIndex(N).GetInt();
        int endIndex = globalTable->GetEndOfCaptureIndex(N).GetInt();
        int len = endIndex - startIndex;
        if (len < 0) {
            res = JSTaggedValue::Undefined();
        } else {
            res = JSTaggedValue(EcmaStringAccessor::FastSubString(thread->GetEcmaVM(),
                JSHandle<EcmaString>(thread, EcmaString::Cast(globalTable->GetInputString())),
                static_cast<uint32_t>(startIndex), static_cast<uint32_t>(len)));
        }
        globalTable->Set(thread, CAPTURE_START_INDEX + N - 1, res);
    } else if (res.IsHole()) {
        res = thread->GetEcmaVM()->GetFactory()->GetEmptyString().GetTaggedValue();
        globalTable->Set(thread, CAPTURE_START_INDEX + N - 1, res);
    }
    return res;
}

template <RBMode mode>
JSTaggedValue RegExpExecResultCache::FindCachedResult(JSThread *thread,
                                                      const JSHandle<JSTaggedValue> input, CacheType type,
                                                      const JSHandle<JSTaggedValue> regexp,
                                                      JSTaggedValue lastIndexInput, JSHandle<JSTaggedValue> extend,
                                                      bool isIntermediateResult)
{
    JSHandle<JSRegExp> regexpObj(regexp);
    JSTaggedValue pattern = regexpObj->GetOriginalSource<mode>();
    JSTaggedValue flags = regexpObj->GetOriginalFlags<mode>();
    JSTaggedValue inputValue = input.GetTaggedValue();
    JSTaggedValue extendValue = extend.GetTaggedValue();
    if (!pattern.IsString() || !flags.IsInt() || !input->IsString() || !lastIndexInput.IsInt()) {
        return JSTaggedValue::Undefined();
    }
    uint32_t hash = pattern.GetKeyHashCode() + static_cast<uint32_t>(flags.GetInt()) +
                    input->GetKeyHashCode() + static_cast<uint32_t>(lastIndexInput.GetInt());
    uint32_t entry = hash & static_cast<uint32_t>(GetCacheLength() - 1);
    if (!Match<mode>(entry, pattern, flags, inputValue, lastIndexInput, extendValue, type)) {
        uint32_t entry2 = (entry + 1) & static_cast<uint32_t>(GetCacheLength() - 1);
        if (!Match<mode>(entry2, pattern, flags, inputValue, lastIndexInput, extendValue, type)) {
            return JSTaggedValue::Undefined();
        }
        entry = entry2;
    }
    ASSERT((static_cast<size_t>(CACHE_TABLE_HEADER_SIZE) +
        static_cast<size_t>(entry) * static_cast<size_t>(ENTRY_SIZE)) <= static_cast<size_t>(UINT32_MAX));
    uint32_t index = CACHE_TABLE_HEADER_SIZE + entry * ENTRY_SIZE;
    // update cached value if input value is changed
    JSTaggedValue cachedStr = Get<mode>(index + INPUT_STRING_INDEX);
    if (!cachedStr.IsUndefined() && cachedStr != inputValue) {
        Set(thread, index + INPUT_STRING_INDEX, inputValue);
    }
    JSTaggedValue result;
    switch (type) {
        case REPLACE_TYPE:
            result = Get<mode>(index + RESULT_REPLACE_INDEX);
            break;
        case SPLIT_TYPE:
            result = Get<mode>(index + RESULT_SPLIT_INDEX);
            break;
        case MATCH_TYPE:
            result = Get<mode>(index + RESULT_MATCH_INDEX);
            break;
        case EXEC_TYPE:
            result = Get<mode>(index + RESULT_EXEC_INDEX);
            break;
        case INTERMEDIATE_REPLACE_TYPE:
            result = Get<mode>(index + RESULT_INTERMEDIATE_REPLACE_INDEX);
            break;
        case TEST_TYPE:
            result = Get<mode>(index + RESULT_TEST_INDEX);
            break;
        case SEARCH_TYPE:
            result = Get<mode>(index + RESULT_SEARCH_INDEX);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
            break;
    }
    SetLastMatchGlobalTableIndex(thread, index);
    SetUseLastMatch(thread, true);
    SetNeedUpdateGlobal(thread, true);
    SetHitCount(thread, GetHitCount() + 1);
    if (type != SEARCH_TYPE && type != SPLIT_TYPE) {
        BuiltinsRegExp::SetLastIndex(thread, regexp, Get<mode>(index + LAST_INDEX_INDEX), true);
    }
    if (!isIntermediateResult && result.IsJSArray()) {
        JSHandle<JSArray> resultHandle(thread, JSArray::Cast(result));
        JSHandle<JSArray> copyArray = thread->GetEcmaVM()->GetFactory()->CloneArrayLiteral(resultHandle);
        return copyArray.GetTaggedValue();
    }
    return result;
}

template <RBMode mode>
bool RegExpExecResultCache::Match(int entry, JSTaggedValue &pattern, JSTaggedValue &flags, JSTaggedValue &input,
                                  JSTaggedValue &lastIndexInputValue, JSTaggedValue &extend, CacheType type)
{
    ASSERT((static_cast<size_t>(CACHE_TABLE_HEADER_SIZE) +
            static_cast<size_t>(entry) * static_cast<size_t>(ENTRY_SIZE)) <= static_cast<size_t>(INT_MAX));
    int index = CACHE_TABLE_HEADER_SIZE + entry * ENTRY_SIZE;

    JSTaggedValue typeKey = Get<mode>(index + RESULT_REPLACE_INDEX + type);
    if (typeKey.IsUndefined()) {
        return false;
    }

    JSTaggedValue keyPattern = Get<mode>(index + PATTERN_INDEX);
    if (keyPattern.IsUndefined()) {
        return false;
    }

    uint8_t flagsBits = static_cast<uint8_t>(flags.GetInt());
    JSTaggedValue keyFlags = Get<mode>(index + FLAG_INDEX);
    uint8_t keyFlagsBits = static_cast<uint8_t>(keyFlags.GetInt());
    if (flagsBits != keyFlagsBits) {
        return false;
    }

    uint32_t lastIndexInputInt = static_cast<uint32_t>(lastIndexInputValue.GetInt());
    JSTaggedValue keyLastIndexInput = Get<mode>(index + LAST_INDEX_INPUT_INDEX);
    uint32_t keyLastIndexInputInt = static_cast<uint32_t>(keyLastIndexInput.GetInt());
    if (lastIndexInputInt != keyLastIndexInputInt) {
        return false;
    }

    JSTaggedValue keyInput = Get<mode>(index + INPUT_STRING_INDEX);
    JSTaggedValue keyExtend = Get<mode>(index + EXTEND_INDEX);
    EcmaString *patternStr = EcmaString::Cast(pattern.GetTaggedObject());
    EcmaString *inputStr = EcmaString::Cast(input.GetTaggedObject());
    EcmaString *keyPatternStr = EcmaString::Cast(keyPattern.GetTaggedObject());
    EcmaString *keyInputStr = EcmaString::Cast(keyInput.GetTaggedObject());
    bool extendEqual = false;
    if (extend.IsString() && keyExtend.IsString()) {
        EcmaString *extendStr = EcmaString::Cast(extend.GetTaggedObject());
        EcmaString *keyExtendStr = EcmaString::Cast(keyExtend.GetTaggedObject());
        extendEqual = EcmaStringAccessor::StringsAreEqual<mode>(extendStr, keyExtendStr);
    } else if (extend.IsUndefined() && keyExtend.IsUndefined()) {
        extendEqual = true;
    } else {
        return false;
    }
    return extendEqual &&
           EcmaStringAccessor::StringsAreEqual<mode>(patternStr, keyPatternStr) &&
           EcmaStringAccessor::StringsAreEqual<mode>(inputStr, keyInputStr);
}
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_REGEXP_INL_H
