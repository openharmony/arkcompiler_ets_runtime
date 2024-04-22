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

#ifndef ECMASCRIPT_JS_SEGMENTER_H
#define ECMASCRIPT_JS_SEGMENTER_H

#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_intl.h"
#include "ecmascript/js_locale.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"

#include "unicode/locdspnm.h"

namespace panda::ecmascript {
enum class GranularityOption : uint8_t {
    GRAPHEME = 0x01,
    WORD,
    SENTENCE,
    EXCEPTION
};

class JSSegmenter : public JSObject {
public:
    CAST_CHECK(JSSegmenter, IsJSSegmenter);

    static constexpr size_t LOCALE_OFFSET = JSObject::SIZE;
    ACCESSORS(Locale, LOCALE_OFFSET, ICU_FIELD_OFFSET)
    ACCESSORS(IcuField, ICU_FIELD_OFFSET, BIT_FIELD_OFFSET)
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    // define BitField
    static constexpr size_t GRANULARITY_BITS = 3;
    FIRST_BIT_FIELD(BitField, Granularity, GranularityOption, GRANULARITY_BITS)

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, LOCALE_OFFSET, BIT_FIELD_OFFSET)
    DECL_DUMP()

    static JSHandle<JSSegmenter> InitializeSegmenter(JSThread *thread,
                                                     const JSHandle<JSSegmenter> &segmenter,
                                                     const JSHandle<JSTaggedValue> &locales,
                                                     const JSHandle<JSTaggedValue> &options);
    // Get icu break iterator from icu field
    icu::BreakIterator *GetIcuBreakIterator() const
    {
        ASSERT(GetIcuField().IsJSNativePointer());
        auto result = JSNativePointer::Cast(GetIcuField().GetTaggedObject())->GetExternalPointer();
        return reinterpret_cast<icu::BreakIterator *>(result);
    }

    static void FreeIcuBreakIterator([[maybe_unused]] void *env, void *pointer, [[maybe_unused]] void* hint)
    {
        if (pointer == nullptr) {
            return;
        }
        auto icuBreakIterator = reinterpret_cast<icu::BreakIterator *>(pointer);
        delete icuBreakIterator;
    }

    static void SetIcuBreakIterator(JSThread *thread, const JSHandle<JSSegmenter> &segmenter,
                                    icu::BreakIterator* icuBreakIterator, const NativePointerCallback &callback);

    static JSHandle<TaggedArray> GetAvailableLocales(JSThread *thread);

    static void ResolvedOptions(JSThread *thread, const JSHandle<JSSegmenter> &segmenter,
                                const JSHandle<JSObject> &options);

    static JSHandle<JSTaggedValue> GranularityOptionToEcmaString(JSThread *thread, GranularityOption granularity);

private:
    static void InitializeIcuBreakIterator(JSThread *thread, const JSHandle<JSSegmenter> &segmenter,
                                           const icu::Locale &icuLocale, GranularityOption granularity);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JS_SEGMENTER_H