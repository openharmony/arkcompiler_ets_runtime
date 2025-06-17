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

#include "ecmascript/js_segment_iterator.h"


#include "ecmascript/js_iterator.h"
#include "ecmascript/js_segments.h"
#include "ecmascript/object_factory-inl.h"

namespace panda::ecmascript {

void JSSegmentIterator::SetIcuBreakIterator(JSThread *thread, const JSHandle<JSSegmentIterator> &iterator,
                                            icu::BreakIterator* icuBreakIterator, const NativePointerCallback &callback)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();

    ASSERT(icuBreakIterator != nullptr);
    JSTaggedValue data = iterator->GetIcuField(thread);
    if (data.IsJSNativePointer()) {
        JSNativePointer *native = JSNativePointer::Cast(data.GetTaggedObject());
        native->ResetExternalPointer(thread, icuBreakIterator);
        return;
    }
    JSHandle<JSNativePointer> pointer = factory->NewJSNativePointer(icuBreakIterator, callback);
    iterator->SetIcuField(thread, pointer.GetTaggedValue());
}

void JSSegmentIterator::SetUString(JSThread *thread, const JSHandle<JSSegmentIterator> &iterator,
                                   icu::UnicodeString* icuUnicodeString, const NativePointerCallback &callback)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();

    ASSERT(icuUnicodeString != nullptr);
    JSTaggedValue data = iterator->GetUnicodeString(thread);
    if (data.IsJSNativePointer()) {
        JSNativePointer *native = JSNativePointer::Cast(data.GetTaggedObject());
        native->ResetExternalPointer(thread, icuUnicodeString);
        return;
    }
    JSHandle<JSNativePointer> pointer = factory->NewJSNativePointer(icuUnicodeString, callback);
    iterator->SetUnicodeString(thread, pointer.GetTaggedValue());
}

JSHandle<JSSegmentIterator> JSSegmentIterator::CreateSegmentIterator(JSThread *thread,
    icu::BreakIterator* icuBreakIterator, const JSHandle<EcmaString> &string, GranularityOption granularity)
{
    // 1. Let internalSlotsList be « [[IteratingSegmenter]], [[IteratedString]],
    //    [[IteratedStringNextSegmentCodeUnitIndex]] ».
    // 2. Let iterator be OrdinaryObjectCreate(%SegmentIteratorPrototype%, internalSlotsList).
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> segIterCtor(env->GetSegmentIterator());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSSegmentIterator> iterator(factory->NewJSObjectByConstructor(segIterCtor));
    icuBreakIterator = icuBreakIterator->clone();
    // 5. Set iterator.[[IteratedStringNextSegmentCodeUnitIndex]] to 0.
    icuBreakIterator->first();
    icu::UnicodeString* uString = new icu::UnicodeString();
    icuBreakIterator->getText().getText(*uString);
    // 3. Set iterator.[[IteratingSegmenter]] to segmenter.
    SetIcuBreakIterator(thread, iterator, icuBreakIterator, JSSegmentIterator::FreeIcuBreakIterator);
    iterator->SetGranularity(granularity);
    // 4. Set iterator.[[IteratedString]] to string.
    iterator->SetIteratedString(thread, string);
    SetUString(thread, iterator, uString, JSSegmentIterator::FreeUString);
    // 6. Return iterator.
    return iterator;
}

JSTaggedValue JSSegmentIterator::Next(JSThread *thread, const JSHandle<JSSegmentIterator> &iterator)
{
    icu::BreakIterator* icuBreakIterator = iterator->GetIcuBreakIterator(thread);
    // 5. Let startIndex be iterator.[[IteratedStringNextSegmentCodeUnitIndex]].
    int32_t startIndex = icuBreakIterator->current();
    // 6. Let endIndex be ! FindBoundary(segmenter, string, startIndex, after).
    int32_t endIndex = icuBreakIterator->next();
    // 7. If endIndex is not finite, then
    if (endIndex == icu::BreakIterator::DONE) {
        // a. Return CreateIterResultObject(undefined, true).
        JSHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
        return JSIterator::CreateIterResultObject(thread, result, true).GetTaggedValue();
    }

    // 8. Set iterator.[[IteratedStringNextSegmentCodeUnitIndex]] to endIndex.
    // 9. Let segmentData be ! CreateSegmentDataObject(segmenter, string, startIndex, endIndex).
    icu::UnicodeString unicodeString;
    icuBreakIterator->getText().getText(unicodeString);
    JSHandle<JSObject> segmentData = JSSegments::CreateSegmentDataObject(thread, iterator->GetGranularity(),
        icuBreakIterator, JSHandle<EcmaString>(thread, iterator->GetIteratedString(thread)),
        unicodeString, startIndex, endIndex);

    // 10. Return CreateIterResultObject(segmentData, false).
    return JSIterator::CreateIterResultObject(thread, JSHandle<JSTaggedValue>::Cast(segmentData), false)
                                              .GetTaggedValue();
}
}  // namespace panda::ecmascript
