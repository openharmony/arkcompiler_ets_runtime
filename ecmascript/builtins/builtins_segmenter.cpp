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

#include "builtins_segmenter.h"

#include "ecmascript/intl/locale_helper.h"
#include "ecmascript/js_segments.h"

namespace panda::ecmascript::builtins {
// 18.1.1 Intl.Segmenter ( [ locales [ , options ] ] )
JSTaggedValue BuiltinsSegmenter::SegmenterConstructor(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Segmenter, Constructor);
    [[maybe_unused]] EcmaHandleScope scope(thread);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();

    // 1. If NewTarget is undefined, throw a TypeError exception.
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "newTarget is undefined", JSTaggedValue::Exception());
    }

    // 2. Let internalSlotsList be « [[InitializedSegmenter]], [[Locale]], [[SegmenterGranularity]] ».
    // 3. Let segmenter be ? OrdinaryCreateFromConstructor(NewTarget, "%Segmenter.prototype%", internalSlotsList).
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSObject> newObject = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSSegmenter> segmenter = JSHandle<JSSegmenter>::Cast(newObject);

    // 3. Perform ? InitializeSegmenter(segmenter, locales, options).
    JSHandle<JSTaggedValue> locales = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> options = GetCallArg(argv, 1);
    JSSegmenter::InitializeSegmenter(thread, segmenter, locales, options);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return segmenter.GetTaggedValue();
}

// 18.2.2 Intl.Segmenter.supportedLocalesOf ( locales [ , options ] )
JSTaggedValue BuiltinsSegmenter::SupportedLocalesOf(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Segmenter, SupportedLocalesOf);
    [[maybe_unused]] EcmaHandleScope scope(thread);

    // 1. Let availableLocales be %Segmenter%.[[AvailableLocales]].
    JSHandle<TaggedArray> availableLocales = JSSegmenter::GetAvailableLocales(thread);

    // 2. Let requestedLocales be ? CanonicalizeLocaleList(locales).
    JSHandle<JSTaggedValue> locales = GetCallArg(argv, 0);
    JSHandle<TaggedArray> requestedLocales = intl::LocaleHelper::CanonicalizeLocaleList(thread, locales);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Return ? SupportedLocales(availableLocales, requestedLocales, options).
    JSHandle<JSTaggedValue> options = GetCallArg(argv, 1);
    JSHandle<JSArray> result = JSLocale::SupportedLocales(thread, availableLocales, requestedLocales, options);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return result.GetTaggedValue();
}

// 18.3.3 Intl.Segmenter.prototype.segment ( string )
JSTaggedValue BuiltinsSegmenter::Segment(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Segmenter, Segment);
    [[maybe_unused]] EcmaHandleScope scope(thread);

    // 1. Let segmenter be the this value.
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);

    // 2. Perform ? RequireInternalSlot(segmenter, [[InitializedSegmenter]]).
    if (!thisValue->IsJSSegmenter()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not segmenter object", JSTaggedValue::Exception());
    }

    // 3. Let string be ? ToString(string).
    JSHandle<JSTaggedValue> stringValue = GetCallArg(argv, 0);
    JSHandle<EcmaString> string = JSTaggedValue::ToString(thread, stringValue);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 4. Return ! CreateSegmentsObject(segmenter, string).
    JSHandle<JSSegments> segments =
        JSSegments::CreateSegmentsObject(thread, JSHandle<JSSegmenter>::Cast(thisValue), string);
    return segments.GetTaggedValue();
}

// 18.3.4 Intl.Segmenter.prototype.resolvedOptions ( )
JSTaggedValue BuiltinsSegmenter::ResolvedOptions(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Segmenter, ResolvedOptions);
    [[maybe_unused]] EcmaHandleScope scope(thread);

    // 1. Let segmenter be the this value.
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);

    // 2. Perform ? RequireInternalSlot(segmenter, [[InitializedSegmenter]]).
    if (!thisValue->IsJSSegmenter()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not segmenter object", JSTaggedValue::Exception());
    }

    // 3. Let options be OrdinaryObjectCreate(%Object.prototype%).
    auto ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSFunction> ctor(env->GetObjectFunction());
    JSHandle<JSObject> options(factory->NewJSObjectByConstructor(ctor));

    // 4. perform resolvedOptions
    JSHandle<JSSegmenter> segmenter = JSHandle<JSSegmenter>::Cast(thisValue);
    JSSegmenter::ResolvedOptions(thread, segmenter, options);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 5. Return options.
    return options.GetTaggedValue();
}
}  // namespace panda::ecmascript::builtins