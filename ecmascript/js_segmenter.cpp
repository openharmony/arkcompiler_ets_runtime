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

#include "ecmascript/js_segmenter.h"

#include <cstring>

#include "ecmascript/intl/locale_helper.h"
#include "ecmascript/object_factory-inl.h"

namespace panda::ecmascript {

void JSSegmenter::SetIcuBreakIterator(JSThread *thread, const JSHandle<JSSegmenter> &segmenter,
                                      icu::BreakIterator* icuBreakIterator, const NativePointerCallback &callback)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();

    ASSERT(icuBreakIterator != nullptr);
    JSTaggedValue data = segmenter->GetIcuField();
    if (data.IsJSNativePointer()) {
        JSNativePointer *native = JSNativePointer::Cast(data.GetTaggedObject());
        native->ResetExternalPointer(thread, icuBreakIterator);
        return;
    }
    JSHandle<JSNativePointer> pointer = factory->NewJSNativePointer(icuBreakIterator, callback);
    segmenter->SetIcuField(thread, pointer.GetTaggedValue());
}

JSHandle<TaggedArray> JSSegmenter::GetAvailableLocales(JSThread *thread)
{
    std::vector<std::string> availableStringLocales = intl::LocaleHelper::GetAvailableLocales(thread, nullptr, nullptr);
    JSHandle<TaggedArray> availableLocales = JSLocale::ConstructLocaleList(thread, availableStringLocales);
    return availableLocales;
}

void JSSegmenter::InitializeIcuBreakIterator(JSThread *thread, const JSHandle<JSSegmenter> &segmenter,
                                             const icu::Locale &icuLocale, GranularityOption granularity)
{
    UErrorCode status = U_ZERO_ERROR;
    std::unique_ptr<icu::BreakIterator> icuBreakIterator;

    switch (granularity) {
        case GranularityOption::GRAPHEME:
            icuBreakIterator.reset(icu::BreakIterator::createCharacterInstance(icuLocale, status));
            break;
        case GranularityOption::WORD:
            icuBreakIterator.reset(icu::BreakIterator::createWordInstance(icuLocale, status));
            break;
        case GranularityOption::SENTENCE:
            icuBreakIterator.reset(icu::BreakIterator::createSentenceInstance(icuLocale, status));
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    if (U_FAILURE(status) || icuBreakIterator == nullptr) {
        if (status == UErrorCode::U_MISSING_RESOURCE_ERROR) {
            THROW_ERROR(thread, ErrorType::REFERENCE_ERROR, "can not find icu data resources");
        }
        THROW_ERROR(thread, ErrorType::RANGE_ERROR, "create icu::BreakIterator failed");
    }

    SetIcuBreakIterator(thread, segmenter, icuBreakIterator.release(), JSSegmenter::FreeIcuBreakIterator);
}

JSHandle<JSSegmenter> JSSegmenter::InitializeSegmenter(JSThread *thread,
                                                       const JSHandle<JSSegmenter> &segmenter,
                                                       const JSHandle<JSTaggedValue> &locales,
                                                       const JSHandle<JSTaggedValue> &options)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto globalConst = thread->GlobalConstants();
    // 4. Let requestedLocales be ? CanonicalizeLocaleList(locales).
    JSHandle<TaggedArray> requestedLocales = intl::LocaleHelper::CanonicalizeLocaleList(thread, locales);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSSegmenter, thread);

    // 5. Let options be ? GetOptionsObject(options).
    JSHandle<JSObject> optionsObject;
    if (options->IsUndefined()) {
        optionsObject = factory->CreateNullJSObject();
    } else if (!options->IsJSObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "options is not Object", segmenter);
    } else {
        optionsObject = JSTaggedValue::ToObject(thread, options);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSSegmenter, thread);
    }

    // 6. Let opt be a new Record.
    // 7. Let matcher be ? GetOption(options, "localeMatcher", string, « "lookup", "best fit" », "best fit").
    JSHandle<JSTaggedValue> property = globalConst->GetHandledLocaleMatcherString();
    auto matcher = JSLocale::GetOptionOfString<LocaleMatcherOption>(
        thread, optionsObject, property, {LocaleMatcherOption::LOOKUP, LocaleMatcherOption::BEST_FIT},
        {"lookup", "best fit"}, LocaleMatcherOption::BEST_FIT);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSSegmenter, thread);

    // 9. Let localeData be %Segmenter%.[[LocaleData]].
    // 10. Let r be ResolveLocale(%Segmenter%.[[AvailableLocales]], requestedLocales, opt,
    // %Segmenter%.[[RelevantExtensionKeys]], localeData).
    JSHandle<TaggedArray> availableLocales;
    if (requestedLocales->GetLength() == 0) {
        availableLocales = factory->EmptyArray();
    } else {
        availableLocales = JSSegmenter::GetAvailableLocales(thread);
    }
    std::set<std::string> relevantExtensionKeys {""};
    ResolvedLocale r =
        JSLocale::ResolveLocale(thread, availableLocales, requestedLocales, matcher, relevantExtensionKeys);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSSegmenter, thread);

    // 11. Set segmenter.[[Locale]] to r.[[locale]].
    icu::Locale icuLocale = r.localeData;
    JSHandle<EcmaString> localeStr = intl::LocaleHelper::ToLanguageTag(thread, icuLocale);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSSegmenter, thread);
    segmenter->SetLocale(thread, localeStr.GetTaggedValue());

    // 12. Let granularity be ? GetOption(options, "granularity", string, « "grapheme", "word", "sentence" »,
    //     "grapheme").
    property = globalConst->GetHandledGranularityString();
    auto granularity = JSLocale::GetOptionOfString<GranularityOption>(thread, optionsObject, property,
                                                                      {GranularityOption::GRAPHEME,
                                                                      GranularityOption::WORD,
                                                                      GranularityOption::SENTENCE},
                                                                      {"grapheme", "word", "sentence"},
                                                                      GranularityOption::GRAPHEME);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSSegmenter, thread);

    // 13. Set segmenter.[[SegmenterGranularity]] to granularity.
    segmenter->SetGranularity(granularity);
    InitializeIcuBreakIterator(thread, segmenter, icuLocale, granularity);
    // 14. Return segmenter.
    return segmenter;
}

JSHandle<JSTaggedValue> JSSegmenter::GranularityOptionToEcmaString(JSThread *thread, GranularityOption granularity)
{
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    auto globalConst = thread->GlobalConstants();
    switch (granularity) {
        case GranularityOption::GRAPHEME:
            result.Update(globalConst->GetHandledGraphemeString().GetTaggedValue());
            break;
        case GranularityOption::WORD:
            result.Update(globalConst->GetHandledWordString().GetTaggedValue());
            break;
        case GranularityOption::SENTENCE:
            result.Update(globalConst->GetHandledSentenceString().GetTaggedValue());
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    return result;
}

void JSSegmenter::ResolvedOptions(JSThread *thread, const JSHandle<JSSegmenter> &segmenter,
                                  const JSHandle<JSObject> &options)
{
    auto globalConst = thread->GlobalConstants();

    // [[Locale]]
    JSHandle<JSTaggedValue> propertyKey = globalConst->GetHandledLocaleString();
    JSHandle<JSTaggedValue> locale(thread, segmenter->GetLocale());
    JSObject::CreateDataPropertyOrThrow(thread, options, propertyKey, locale);
    RETURN_IF_ABRUPT_COMPLETION(thread);

    // [[SegmenterGranularity]]
    GranularityOption granularity = segmenter->GetGranularity();
    propertyKey = globalConst->GetHandledGranularityString();
    JSHandle<JSTaggedValue> granularityString = GranularityOptionToEcmaString(thread, granularity);
    JSObject::CreateDataPropertyOrThrow(thread, options, propertyKey, granularityString);
    RETURN_IF_ABRUPT_COMPLETION(thread);
}
}  // namespace panda::ecmascript
