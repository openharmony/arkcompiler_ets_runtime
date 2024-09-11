/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_JS_DISPLAYNAMES_H
#define ECMASCRIPT_JS_DISPLAYNAMES_H

#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_intl.h"
#include "ecmascript/js_locale.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"

#include "unicode/locdspnm.h"

namespace panda::ecmascript {
enum class TypednsOption : uint8_t {
    LANGUAGE = 0x01,
    CALENDAR,
    CURRENCY,
    DATETIMEFIELD,
    REGION,
    SCRIPT,
    UNDEFINED,
    EXCEPTION
};

enum class StyOption : uint8_t {
    LONG = 0x01,
    SHORT,
    NARROW,
    EXCEPTION
};

enum class FallbackOption : uint8_t {
    CODE = 0x01,
    NONE,
    EXCEPTION
};

enum class LanguageDisplayOption : uint8_t {
    DIALECT = 0x01,
    STANDARD,
    EXCEPTION
};

class JSDisplayNames : public JSObject {
public:
    static const std::vector<LocaleMatcherOption> LOCALE_MATCHER_OPTION;
    static const std::vector<std::string> LOCALE_MATCHER_OPTION_NAME;

    static const std::vector<StyOption> STY_OPTION;
    static const std::vector<std::string> STY_OPTION_NAME;

    static const std::vector<TypednsOption> TYPED_NS_OPTION;
    static const std::vector<std::string> TYPED_NS_OPTION_NAME;

    static const std::vector<FallbackOption> FALLBACK_OPTION;
    static const std::vector<std::string> FALLBACK_OPTION_OPTION_NAME;

    static const std::vector<LanguageDisplayOption> LANGUAGE_DISPLAY_OPTION;
    static const std::vector<std::string> LANGUAGE_DISPLAY_OPTION_NAME;
    CAST_CHECK(JSDisplayNames, IsJSDisplayNames);

    static constexpr size_t LOCALE_OFFSET = JSObject::SIZE;
    ACCESSORS(Locale, LOCALE_OFFSET, ICU_LOCALE_DISPPLAY_NAMES)
    ACCESSORS(IcuLDN, ICU_LOCALE_DISPPLAY_NAMES, BIT_FIELD_OFFSET)
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    // define BitField
    static constexpr size_t TYPE_BITS = 4;
    static constexpr size_t STYLE_BITS = 3;
    static constexpr size_t FALL_BACK_BITS = 2;
    static constexpr size_t LANGUAGE_DISPLAY_BITS = 2;
    FIRST_BIT_FIELD(BitField, Type, TypednsOption, TYPE_BITS)
    NEXT_BIT_FIELD(BitField, Style, StyOption, STYLE_BITS, Type)
    NEXT_BIT_FIELD(BitField, Fallback, FallbackOption, FALL_BACK_BITS, Style)
    NEXT_BIT_FIELD(BitField, LanguageDisplay, LanguageDisplayOption, LANGUAGE_DISPLAY_BITS, Fallback)

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, LOCALE_OFFSET, BIT_FIELD_OFFSET)
    DECL_DUMP()

    icu::LocaleDisplayNames *GetIcuLocaleDisplayNames() const;

    static void FreeIcuLocaleDisplayNames(void *env, void *pointer, void* hint);

    static JSHandle<TaggedArray> GetAvailableLocales(JSThread *thread);
    
    static void SetIcuLocaleDisplayNames(JSThread *thread, const JSHandle<JSDisplayNames> &displayNames,
                                         icu::LocaleDisplayNames* icuPointer, const NativePointerCallback &callback);

    // 12.1.1  CanonicalCodeForDisplayNames ( type, code )
    static JSHandle<EcmaString> CanonicalCodeForDisplayNames(JSThread *thread,
                                                             const JSHandle<JSDisplayNames> &displayNames,
                                                             const TypednsOption &typeOpt,
                                                             const JSHandle<EcmaString> &code);

    // 12.1.2 InitializeDisplayNames ( displaynames, locales, options )
    static JSHandle<JSDisplayNames> InitializeDisplayNames(JSThread *thread,
                                                             const JSHandle<JSDisplayNames> &displayNames,
                                                             const JSHandle<JSTaggedValue> &locales,
                                                             const JSHandle<JSTaggedValue> &options);

    static void ResolvedOptions(JSThread *thread, const JSHandle<JSDisplayNames> &displayNames,
                                const JSHandle<JSObject> &options);

private:
    static UDateTimePatternField StringToUDateTimePatternField(const char* code);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JS_DISPLAYNAMES_H