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

#include "builtins_segments.h"

#include "ecmascript/js_segment_iterator.h"
#include "ecmascript/js_segments.h"

namespace panda::ecmascript::builtins {
// %SegmentsPrototype%.containing ( index )
JSTaggedValue BuiltinsSegments::Containing(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Segments, Containing);
    [[maybe_unused]] EcmaHandleScope scope(thread);

    // 1. Let segments be the this value.
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);

    // 2. Perform ? RequireInternalSlot(segments, [[SegmentsSegmenter]]).
    if (!thisValue->IsJSSegments()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not Segments object", JSTaggedValue::Exception());
    }

    // 6. Let n be ? ToIntegerOrInfinity(index).
    JSHandle<JSTaggedValue> indexTag = GetCallArg(argv, 0);
    JSTaggedNumber indexVal = JSTaggedValue::ToInteger(thread, indexTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSSegments> segments = JSHandle<JSSegments>::Cast(thisValue);
    return JSSegments::Containing(thread, segments, indexVal.GetNumber());
}

// %SegmentsPrototype% [ @@iterator ] ( )
JSTaggedValue BuiltinsSegments::GetSegmentIterator(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Segments, GetSegmentIterator);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let segments be the this value.
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);

    // 2. Perform ? RequireInternalSlot(segments, [[SegmentsSegmenter]]).
    if (!thisValue->IsJSSegments()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not Segments object", JSTaggedValue::Exception());
    }
    JSHandle<JSSegments> segments = JSHandle<JSSegments>::Cast(thisValue);
    // 3. Let segmenter be segments.[[SegmentsSegmenter]].
    // 4. Let string be segments.[[SegmentsString]].
    JSHandle<EcmaString> string(thread, segments->GetSegmentsString());
    // 5. Return ! CreateSegmentIterator(segmenter, string).
    return JSSegmentIterator::CreateSegmentIterator(thread, segments->GetIcuBreakIterator(), string,
                                                    segments->GetGranularity()).GetTaggedValue();
}
}  // namespace panda::ecmascript::builtins