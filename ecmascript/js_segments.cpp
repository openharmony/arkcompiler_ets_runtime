/*
 * Copyright (c) 2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
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

#include "ecmascript/js_segments.h"

#include <cstring>

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/intl/locale_helper.h"
#include "ecmascript/object_factory-inl.h"

namespace panda::ecmascript {

void JSSegments::SetIcuBreakIterator(JSThread *thread, const JSHandle<JSSegments> &segments,
                                     icu::BreakIterator* icuBreakIterator, const NativePointerCallback &callback)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();

    ASSERT(icuBreakIterator != nullptr);
    JSTaggedValue data = segments->GetIcuField();
    if (data.IsJSNativePointer()) {
        JSNativePointer *native = JSNativePointer::Cast(data.GetTaggedObject());
        native->ResetExternalPointer(thread, icuBreakIterator);
        return;
    }
    JSHandle<JSNativePointer> pointer = factory->NewJSNativePointer(icuBreakIterator, callback);
    segments->SetIcuField(thread, pointer.GetTaggedValue());
}

void JSSegments::SetUString(JSThread *thread, const JSHandle<JSSegments> &segments,
                            icu::UnicodeString* icuUnicodeString, const NativePointerCallback &callback)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();

    ASSERT(icuUnicodeString != nullptr);
    JSTaggedValue data = segments->GetUnicodeString();
    if (data.IsJSNativePointer()) {
        JSNativePointer *native = JSNativePointer::Cast(data.GetTaggedObject());
        native->ResetExternalPointer(thread, icuUnicodeString);
        return;
    }
    JSHandle<JSNativePointer> pointer = factory->NewJSNativePointer(icuUnicodeString, callback);
    segments->SetUnicodeString(thread, pointer.GetTaggedValue());
}

void SetTextToBreakIterator(JSThread *thread, const JSHandle<JSSegments> &segments,
                            JSHandle<EcmaString> text, icu::BreakIterator* breakIterator)
{
    std::u16string u16str = EcmaStringAccessor(text).ToU16String();
    icu::UnicodeString src(u16str.data(), u16str.size());
    icu::UnicodeString* uText = static_cast<icu::UnicodeString*>(src.clone());
    breakIterator->setText(*uText);
    JSSegments::SetUString(thread, segments, uText, JSSegments::FreeUString);
}

JSHandle<JSSegments> JSSegments::CreateSegmentsObject(JSThread *thread,
                                                      const JSHandle<JSSegmenter> &segmenter,
                                                      const JSHandle<EcmaString> &string)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. Let internalSlotsList be « [[SegmentsSegmenter]], [[SegmentsString]] ».
    // 2. Let segments be OrdinaryObjectCreate(%SegmentsPrototype%, internalSlotsList).
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> segmentsCtor(env->GetSegmentsFunction());
    JSHandle<JSSegments> segments(factory->NewJSObjectByConstructor(segmentsCtor));
    // 3. Set segments.[[SegmentsSegmenter]] to segmenter.
    icu::BreakIterator* icuBreakIterator = segmenter->GetIcuBreakIterator()->clone();
    SetIcuBreakIterator(thread, segments, icuBreakIterator, JSSegments::FreeIcuBreakIterator);
    segments->SetGranularity(segmenter->GetGranularity());
    // 4. Set segments.[[SegmentsString]] to string.
    segments->SetSegmentsString(thread, string);
    SetTextToBreakIterator(thread, segments, string, icuBreakIterator);
    return segments;
}

JSTaggedValue JSSegments::Containing(JSThread *thread, const JSHandle<JSSegments> &segments, double index)
{
    icu::UnicodeString* unicodeString = segments->GetUString();
    // 5. Let len be the length of string.
    int32_t len = unicodeString->length();
    // 7. If n < 0 or n ≥ len, return undefined.
    if (index < 0 || index >= len) {
        return JSTaggedValue::Undefined();
    }
    int32_t n = static_cast<int32_t>(index);
    // n may point to the surrogate tail- adjust it back to the lead.
    n = unicodeString->getChar32Start(n);
    icu::BreakIterator* breakIterator = segments->GetIcuBreakIterator();
    // 8. Let startIndex be ! FindBoundary(segmenter, string, n, before).
    int32_t startIndex = breakIterator->isBoundary(n) ? n : breakIterator->preceding(n);
    // 9. Let endIndex be ! FindBoundary(segmenter, string, n, after).
    int32_t endIndex = breakIterator->following(n);
    // 10. Return ! CreateSegmentDataObject(segmenter, string, startIndex, endIndex).
    return CreateSegmentDataObject(thread, segments->GetGranularity(), breakIterator,
                                   JSHandle<EcmaString>(thread, segments->GetSegmentsString()),
                                   *unicodeString, startIndex, endIndex).GetTaggedValue();
}

bool CurrentSegmentIsWordLike(icu::BreakIterator* breakIterator)
{
    int32_t rule_status = breakIterator->getRuleStatus();
    return (rule_status >= UBRK_WORD_NUMBER &&
            rule_status < UBRK_WORD_NUMBER_LIMIT) ||
           (rule_status >= UBRK_WORD_LETTER &&
            rule_status < UBRK_WORD_LETTER_LIMIT) ||
           (rule_status >= UBRK_WORD_KANA &&
            rule_status < UBRK_WORD_KANA_LIMIT) ||
           (rule_status >= UBRK_WORD_IDEO && rule_status < UBRK_WORD_IDEO_LIMIT);
}

// 18.7.1 CreateSegmentDataObject ( segmenter, string, startIndex, endIndex )
JSHandle<JSObject> JSSegments::CreateSegmentDataObject(JSThread *thread, GranularityOption granularity,
    icu::BreakIterator* breakIterator, const JSHandle<EcmaString> &inputString,
    const icu::UnicodeString& unicodeString, int32_t startIndex, int32_t endIndex)
{
    // 1. Let len be the length of string.
    // 2. Assert: startIndex ≥ 0.
    ASSERT(startIndex >= 0);
    // 3. Assert: endIndex ≤ len.
    ASSERT(endIndex <= unicodeString.length());
    // 4. Assert: startIndex < endIndex.
    ASSERT(startIndex < endIndex);
    // 5. Let result be OrdinaryObjectCreate(%Object.prototype%).
    auto ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSFunction> ctor(env->GetObjectFunction());
    JSHandle<JSObject> result(factory->NewJSObjectByConstructor(ctor));
    // 6. Let segment be the substring of string from startIndex to endIndex.
    JSHandle<EcmaString> segment =
        intl::LocaleHelper::UStringToString(thread, unicodeString, startIndex, endIndex);
    auto globalConst = thread->GlobalConstants();
    // 7. Perform ! CreateDataPropertyOrThrow(result, "segment", segment).
    JSHandle<JSTaggedValue> segmentKey = globalConst->GetHandledSegmentString();
    JSObject::CreateDataPropertyOrThrow(thread, result, segmentKey, JSHandle<JSTaggedValue>::Cast(segment));
    // 8. Perform ! CreateDataPropertyOrThrow(result, "index", 𝔽(startIndex)).
    JSHandle<JSTaggedValue> indexKey = globalConst->GetHandledIndexString();
    JSObject::CreateDataPropertyOrThrow(thread, result, indexKey, JSHandle<JSTaggedValue>(thread,
                                        base::BuiltinsBase::GetTaggedInt(startIndex)));
    // 9. Perform ! CreateDataPropertyOrThrow(result, "input", string).
    JSHandle<JSTaggedValue> inputKey = globalConst->GetHandledInputString();
    JSObject::CreateDataPropertyOrThrow(thread, result, inputKey, JSHandle<JSTaggedValue>::Cast(inputString));
    // 10. Let granularity be segmenter.[[SegmenterGranularity]].
    // 11. If granularity is "word", then
    //     a. Let isWordLike be a Boolean value indicating whether the segment in string is "word-like"
    //        according to locale segmenter.[[Locale]].
    //     b. Perform ! CreateDataPropertyOrThrow(result, "isWordLike", isWordLike).
    if (granularity == GranularityOption::WORD) {
        bool isWordLike = CurrentSegmentIsWordLike(breakIterator);
        JSHandle<JSTaggedValue> isWordLikeKey = globalConst->GetHandledIsWordLikeString();
        JSObject::CreateDataPropertyOrThrow(thread, result, isWordLikeKey, JSHandle<JSTaggedValue>(thread,
                                            base::BuiltinsBase::GetTaggedBoolean(isWordLike)));
    }
    // 12. Return result.
    return result;
}
}  // namespace panda::ecmascript
