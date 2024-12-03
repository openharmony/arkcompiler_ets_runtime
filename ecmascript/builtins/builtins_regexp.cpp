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

#include "ecmascript/builtins/builtins_regexp.h"
#include "ecmascript/builtins/builtins_regexp-inl.h"
#include "ecmascript/checkpoint/thread_state_transition.h"

#include <cmath>

#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/js_regexp_iterator.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/object_fast_operator-inl.h"
#include "ecmascript/property_detector-inl.h"
#include "ecmascript/regexp/regexp_executor.h"
#include "ecmascript/regexp/regexp_parser_cache.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript::builtins {
// 21.2.3.1
JSTaggedValue BuiltinsRegExp::RegExpConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), RegExp, Constructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> newTargetTemp = GetNewTarget(argv);
    JSHandle<JSTaggedValue> pattern = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> flags = GetCallArg(argv, 1);
    // 1. Let patternIsRegExp be IsRegExp(pattern).
    bool patternIsRegExp = JSObject::IsRegExp(thread, pattern);
    // 2. ReturnIfAbrupt(patternIsRegExp).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 3. If NewTarget is not undefined, let newTarget be NewTarget.
    JSHandle<JSTaggedValue> newTarget;
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    if (!newTargetTemp->IsUndefined()) {
        newTarget = newTargetTemp;
    } else {
        auto ecmaVm = thread->GetEcmaVM();
        JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
        // 4.a Let newTarget be the active function object.
        newTarget = env->GetRegExpFunction();
        JSHandle<JSTaggedValue> constructorString = globalConst->GetHandledConstructorString();
        // 4.b If patternIsRegExp is true and flags is undefined
        if (patternIsRegExp && flags->IsUndefined()) {
            // 4.b.i Let patternConstructor be Get(pattern, "constructor").
            JSTaggedValue patternConstructor = ObjectFastOperator::FastGetPropertyByValue(
                thread, pattern.GetTaggedValue(), constructorString.GetTaggedValue());
            // 4.b.ii ReturnIfAbrupt(patternConstructor).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            // 4.b.iii If SameValue(newTarget, patternConstructor) is true, return pattern.
            if (JSTaggedValue::SameValue(newTarget.GetTaggedValue(), patternConstructor)) {
                return pattern.GetTaggedValue();
            }
        }
    }
    // 5. If Type(pattern) is Object and pattern has a [[RegExpMatcher]] internal slot
    bool isJsReg = false;
    if (pattern->IsECMAObject()) {
        JSHandle<JSObject> patternObj = JSHandle<JSObject>::Cast(pattern);
        isJsReg = patternObj->IsJSRegExp();
    }
    JSHandle<JSTaggedValue> patternTemp;
    JSHandle<JSTaggedValue> flagsTemp;
    if (isJsReg) {
        JSHandle<JSRegExp> patternReg(thread, JSRegExp::Cast(pattern->GetTaggedObject()));
        // 5.a Let P be the value of pattern’s [[OriginalSource]] internal slot.
        patternTemp = JSHandle<JSTaggedValue>(thread, patternReg->GetOriginalSource());
        if (flags->IsUndefined()) {
            // 5.b If flags is undefined, let F be the value of pattern’s [[OriginalFlags]] internal slot.
            flagsTemp = JSHandle<JSTaggedValue>(thread, patternReg->GetOriginalFlags());
        } else {
            // 5.c Else, let F be flags.
            flagsTemp = JSHandle<JSTaggedValue>(thread, *JSTaggedValue::ToString(thread, flags));
        }
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // 6. Else if patternIsRegExp is true
    } else if (patternIsRegExp) {
        JSHandle<JSTaggedValue> sourceString(globalConst->GetHandledSourceString());
        JSHandle<JSTaggedValue> flagsString(globalConst->GetHandledFlagsString());
        // 6.a Let P be Get(pattern, "source").
        patternTemp = JSObject::GetProperty(thread, pattern, sourceString).GetValue();
        // 6.b ReturnIfAbrupt(P).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // 6.c If flags is undefined
        if (flags->IsUndefined()) {
            // 6.c.i Let F be Get(pattern, "flags").
            flagsTemp = JSObject::GetProperty(thread, pattern, flagsString).GetValue();
            // 6.c.ii ReturnIfAbrupt(F).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        } else {
            // 6.d Else, let F be flags.
            flagsTemp = JSHandle<JSTaggedValue>(thread, *JSTaggedValue::ToString(thread, flags));
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
    } else {
        // 7.a Let P be pattern.
        patternTemp = pattern;
        // 7.b Let F be flags.
        if (flags->IsUndefined()) {
            flagsTemp = flags;
        } else {
            flagsTemp = JSHandle<JSTaggedValue>(thread, *JSTaggedValue::ToString(thread, flags));
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
    }
    // 8. Let O be RegExpAlloc(newTarget).
    JSHandle<JSTaggedValue> object(thread, RegExpAlloc(thread, newTarget));
    // 9. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 10. Return RegExpInitialize(O, P, F).
    JSTaggedValue result = RegExpInitialize(thread, object, patternTemp, flagsTemp);
    return JSTaggedValue(result);
}

// prototype
// 20.2.5.2
JSTaggedValue BuiltinsRegExp::Exec(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), RegExp, Exec);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let R be the this value.
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    // 4. Let S be ToString(string).
    JSHandle<JSTaggedValue> inputStr = GetCallArg(argv, 0);
    JSHandle<EcmaString> stringHandle = JSTaggedValue::ToString(thread, inputStr);
    // 5. ReturnIfAbrupt(S).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> string = JSHandle<JSTaggedValue>::Cast(stringHandle);
    // 2. If Type(R) is not Object, throw a TypeError exception.
    if (!thisObj->IsECMAObject()) {
        // throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not Object", JSTaggedValue::Exception());
    }
    // 3. If R does not have a [[RegExpMatcher]] internal slot, throw a TypeError exception.
    if (!thisObj->IsJSRegExp()) {
        // throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(thread, "this does not have [[RegExpMatcher]]", JSTaggedValue::Exception());
    }

    bool useCache = true;
    bool isFastPath = IsFastRegExp(thread, thisObj.GetTaggedValue());
    JSHandle<RegExpExecResultCache> cacheTable(thread->GetCurrentEcmaContext()->GetRegExpCache());
    if (!isFastPath || cacheTable->GetLargeStrCount() == 0 || cacheTable->GetConflictCount() == 0) {
        useCache = false;
    }

    // 6. Return RegExpBuiltinExec(R, S).
    return RegExpBuiltinExec(thread, thisObj, string, isFastPath, useCache);
}

// 20.2.5.13
JSTaggedValue BuiltinsRegExp::Test(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), RegExp, Test);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let R be the this value.
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    JSHandle<JSTaggedValue> inputStr = GetCallArg(argv, 0);
    // 2. If Type(R) is not Object, throw a TypeError exception.
    if (!thisObj->IsECMAObject()) {
        // throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not Object", JSTaggedValue::Exception());
    }
    // 3. Let string be ToString(S).
    // 4. ReturnIfAbrupt(string).
    JSHandle<EcmaString> stringHandle = JSTaggedValue::ToString(thread, inputStr);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> string = JSHandle<JSTaggedValue>::Cast(stringHandle);
    // test fast path
    if (IsFastRegExp(thread, thisObj.GetTaggedValue())) {
        return RegExpTestFast(thread, thisObj, string, true);
    }

    // 5. Let match be RegExpExec(R, string).
    JSTaggedValue matchResult = RegExpExec(thread, thisObj, string, false);
    // 6. ReturnIfAbrupt(match).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 7. If match is not null, return true; else return false.
    return GetTaggedBoolean(!matchResult.IsNull());
}

bool BuiltinsRegExp::IsFastRegExp(JSThread *thread, JSTaggedValue regexp,
                                  RegExpSymbol symbolTag)
{
    DISALLOW_GARBAGE_COLLECTION;
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHClass *hclass = JSObject::Cast(regexp)->GetJSHClass();
    JSHClass *originHClass = JSHClass::Cast(globalConst->GetJSRegExpClass().GetTaggedObject());
    // regexp instance hclass
    if (hclass != originHClass) {
        return false;
    }
    // lastIndex type is Int
    JSTaggedValue lastIndex = JSObject::Cast(regexp)->GetPropertyInlinedProps(LAST_INDEX_OFFSET);
    if (!lastIndex.IsInt() || lastIndex.GetInt() < 0) {
        return false;
    }
    // RegExp.prototype hclass
    JSTaggedValue proto = hclass->GetPrototype();
    JSHClass *regexpHclass = proto.GetTaggedObject()->GetClass();
    JSHandle<JSTaggedValue> originRegexpClassValue = env->GetRegExpPrototypeClass();
    JSHClass *originRegexpHclass = JSHClass::Cast(originRegexpClassValue.GetTaggedValue().GetTaggedObject());
    if (regexpHclass != originRegexpHclass) {
        return false;
    }
    JSObject* protoObj = JSObject::Cast(proto);
    // RegExp.prototype.exec
    JSTaggedValue execVal = protoObj->GetPropertyInlinedProps(JSRegExp::EXEC_INLINE_PROPERTY_INDEX);
    if (execVal != env->GetTaggedRegExpExecFunction()) {
        return false;
    }
    JSTaggedValue symbolFunc = JSTaggedValue::Hole();
    switch (symbolTag) {
        case RegExpSymbol::UNKNOWN:
            break;
#define V(UpperCase, Camel)                                                     \
        case RegExpSymbol::UpperCase:                                           \
            symbolFunc = protoObj->GetPropertyInlinedProps(                     \
                JSRegExp::UpperCase##_INLINE_PROPERTY_INDEX);                   \
            if (symbolFunc != env->GetTaggedRegExp##Camel##Function()) {        \
                return false;                                                   \
            }                                                                   \
            break;
            REGEXP_SYMBOL_FUNCTION_LIST(V)
#undef V
    }
    if (!PropertyDetector::IsRegExpFlagsDetectorValid(env)) {
        return false;
    }
    return true;
}

JSTaggedValue BuiltinsRegExp::RegExpTestFast(JSThread *thread, JSHandle<JSTaggedValue> regexp,
                                             const JSHandle<JSTaggedValue> inputStr, bool useCache)
{
    // 1. Assert: Type(S) is String.
    ASSERT(inputStr->IsString());
    uint32_t lastIndex = static_cast<uint32_t>(GetLastIndex(thread, regexp, true));
    // 2. Search RegExpExecResult cache
    JSHandle<RegExpExecResultCache> cacheTable(thread->GetCurrentEcmaContext()->GetRegExpCache());
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    if (useCache) {
        JSTaggedValue cacheResult = cacheTable->FindCachedResult(thread, inputStr,
                                                                 RegExpExecResultCache::TEST_TYPE, regexp,
                                                                 JSTaggedValue(lastIndex), undefined);
        if (!cacheResult.IsUndefined()) {
            return cacheResult;
        }
    }

    uint32_t length = EcmaStringAccessor(inputStr->GetTaggedObject()).GetLength();
    if (lastIndex > length) {
        SetLastIndex(thread, regexp, JSTaggedValue(0), true);
        return JSTaggedValue::False();
    }
    JSHandle<EcmaString> inputString = JSHandle<EcmaString>::Cast(inputStr);
    bool matchResult = RegExpExecInternal(thread, regexp, inputString, lastIndex);
    // 2. Check whether the regexp is global or sticky, which determines whether we update last index later on.
    bool global = GetOriginalFlag(thread, regexp, RegExpParser::FLAG_GLOBAL);
    bool sticky = GetOriginalFlag(thread, regexp, RegExpParser::FLAG_STICKY);
    bool ifUpdateLastIndex = global || sticky;
    if (!matchResult) {
        if (ifUpdateLastIndex) {
            SetLastIndex(thread, regexp, JSTaggedValue(0), true);
        }
        if (useCache) {
            RegExpExecResultCache::AddResultInCache(thread, cacheTable, regexp, inputStr,
                                                    JSHandle<JSTaggedValue>(thread, JSTaggedValue(matchResult)),
                                                    RegExpExecResultCache::TEST_TYPE,
                                                    lastIndex, 0, undefined); // 0: match fail so lastIndex is 0
        }
        return JSTaggedValue::False();
    }
    JSHandle<RegExpGlobalResult> globalTable(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    JSTaggedValue endIndex = globalTable->GetEndIndex();
    uint32_t newLastIndex = lastIndex;
    if (ifUpdateLastIndex) {
        newLastIndex = static_cast<uint32_t>(endIndex.GetInt());
        SetLastIndex(thread, regexp, endIndex, true);
    }
    if (useCache) {
        RegExpExecResultCache::AddResultInCache(thread, cacheTable, regexp, inputStr,
                                                JSHandle<JSTaggedValue>(thread, JSTaggedValue(matchResult)),
                                                RegExpExecResultCache::TEST_TYPE,
                                                lastIndex, newLastIndex, undefined);
    }
    return GetTaggedBoolean(matchResult);
}

// 20.2.5.14
JSTaggedValue BuiltinsRegExp::ToString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), RegExp, ToString);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let R be the this value.
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    auto ecmaVm = thread->GetEcmaVM();
    // 2. If Type(R) is not Object, throw a TypeError exception.
    if (!thisObj->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not Object", JSTaggedValue::Exception());
    }
    const GlobalEnvConstants *globalConstants = thread->GlobalConstants();
    JSMutableHandle<JSTaggedValue> getSource(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> getFlags(thread, JSTaggedValue::Undefined());
    JSHandle<EcmaString> sourceStrHandle;
    JSHandle<EcmaString> flagsStrHandle;
    if (IsFastRegExp(thread, thisObj.GetTaggedValue())) {
        JSHandle<JSRegExp> regexp(thread, JSRegExp::Cast(thisObj->GetTaggedObject()));
        // 3. Let pattern be ToString(Get(R, "source")).
        getSource.Update(regexp->GetOriginalSource());
        sourceStrHandle = JSTaggedValue::ToString(thread, getSource);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        uint8_t flagsBits = static_cast<uint8_t>(regexp->GetOriginalFlags().GetInt());
        getFlags.Update(FlagsBitsToString(thread, flagsBits));
        flagsStrHandle = JSTaggedValue::ToString(thread, getFlags);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    } else {
        JSHandle<JSTaggedValue> sourceString(globalConstants->GetHandledSourceString());
        JSHandle<JSTaggedValue> flagsString(globalConstants->GetHandledFlagsString());
        // 3. Let pattern be ToString(Get(R, "source")).
        getSource.Update(JSObject::GetProperty(thread, thisObj, sourceString).GetValue());
        sourceStrHandle = JSTaggedValue::ToString(thread, getSource);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        getFlags.Update(JSObject::GetProperty(thread, thisObj, flagsString).GetValue());
        flagsStrHandle = JSTaggedValue::ToString(thread, getFlags);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    JSHandle<EcmaString> slashStr = JSHandle<EcmaString>::Cast(globalConstants->GetHandledBackslashString());
    // 7. Let result be the String value formed by concatenating "/", pattern, and "/", and flags.
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<EcmaString> tempStr = factory->ConcatFromString(slashStr, sourceStrHandle);
    JSHandle<EcmaString> resultTemp = factory->ConcatFromString(tempStr, slashStr);
    return factory->ConcatFromString(resultTemp, flagsStrHandle).GetTaggedValue();
}

// 20.2.5.3
JSTaggedValue BuiltinsRegExp::GetFlags(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), RegExp, GetFlags);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let R be the this value.
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    // 2. If Type(R) is not Object, throw a TypeError exception.
    if (!thisObj->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not Object", JSTaggedValue::Exception());
    }
    // 3. Let result be the empty String.
    // 4. ~ 19.
    if (!IsFastRegExp(thread, thisObj.GetTaggedValue())) {
        return GetAllFlagsInternal(thread, thisObj);
    }
    uint8_t flagsBits = static_cast<uint8_t>(JSRegExp::Cast(thisObj->GetTaggedObject())->GetOriginalFlags().GetInt());
    return FlagsBitsToString(thread, flagsBits);
}

JSTaggedValue BuiltinsRegExp::GetAllFlagsInternal(JSThread *thread, JSHandle<JSTaggedValue> &thisObj)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConstants = thread->GlobalConstants();
    uint8_t *flagsStr = new uint8_t[RegExpParser::FLAG_NUM + 1];  // FLAG_NUM flags + '\0'
    if (flagsStr == nullptr) {
        LOG_ECMA(FATAL) << "BuiltinsRegExp::GetAllFlagsInternal:flagsStr is nullptr";
    }
    size_t flagsLen = 0;
    JSHandle<EcmaString> emptyString = factory->GetEmptyString();
    JSHandle<JSTaggedValue> hasIndicesKey(factory->NewFromASCII("hasIndices"));
    JSHandle<JSTaggedValue> hasIndicesResult = JSObject::GetProperty(thread, thisObj, hasIndicesKey).GetValue();
    RETURN_VALUE_IF_ABRUPT_COMPLETION_WITH_DATA_DELETE(thread, emptyString.GetTaggedValue(), flagsStr);
    if (hasIndicesResult->ToBoolean()) {
        flagsStr[flagsLen] = 'd';
        flagsLen++;
    }
    JSHandle<JSTaggedValue> globalKey(globalConstants->GetHandledGlobalString());
    JSHandle<JSTaggedValue> globalResult = JSObject::GetProperty(thread, thisObj, globalKey).GetValue();
    RETURN_VALUE_IF_ABRUPT_COMPLETION_WITH_DATA_DELETE(thread, emptyString.GetTaggedValue(), flagsStr);
    if (globalResult->ToBoolean()) {
        flagsStr[flagsLen] = 'g';
        flagsLen++;
    }
    JSHandle<JSTaggedValue> ignoreCaseKey(factory->NewFromASCII("ignoreCase"));
    JSHandle<JSTaggedValue> ignoreCaseResult = JSObject::GetProperty(thread, thisObj, ignoreCaseKey).GetValue();
    RETURN_VALUE_IF_ABRUPT_COMPLETION_WITH_DATA_DELETE(thread, emptyString.GetTaggedValue(), flagsStr);
    if (ignoreCaseResult->ToBoolean()) {
        flagsStr[flagsLen] = 'i';
        flagsLen++;
    }
    JSHandle<JSTaggedValue> multilineKey(factory->NewFromASCII("multiline"));
    JSHandle<JSTaggedValue> multilineResult = JSObject::GetProperty(thread, thisObj, multilineKey).GetValue();
    RETURN_VALUE_IF_ABRUPT_COMPLETION_WITH_DATA_DELETE(thread, emptyString.GetTaggedValue(), flagsStr);
    if (multilineResult->ToBoolean()) {
        flagsStr[flagsLen] = 'm';
        flagsLen++;
    }
    JSHandle<JSTaggedValue> dotAllKey(factory->NewFromASCII("dotAll"));
    JSHandle<JSTaggedValue> dotAllResult = JSObject::GetProperty(thread, thisObj, dotAllKey).GetValue();
    RETURN_VALUE_IF_ABRUPT_COMPLETION_WITH_DATA_DELETE(thread, emptyString.GetTaggedValue(), flagsStr);
    if (dotAllResult->ToBoolean()) {
        flagsStr[flagsLen] = 's';
        flagsLen++;
    }
    JSHandle<JSTaggedValue> unicodeKey(globalConstants->GetHandledUnicodeString());
    JSHandle<JSTaggedValue> unicodeResult = JSObject::GetProperty(thread, thisObj, unicodeKey).GetValue();
    RETURN_VALUE_IF_ABRUPT_COMPLETION_WITH_DATA_DELETE(thread, emptyString.GetTaggedValue(), flagsStr);
    if (unicodeResult->ToBoolean()) {
        flagsStr[flagsLen] = 'u';
        flagsLen++;
    }
    JSHandle<JSTaggedValue> stickyKey(globalConstants->GetHandledStickyString());
    JSHandle<JSTaggedValue> stickyResult = JSObject::GetProperty(thread, thisObj, stickyKey).GetValue();
    RETURN_VALUE_IF_ABRUPT_COMPLETION_WITH_DATA_DELETE(thread, emptyString.GetTaggedValue(), flagsStr);
    if (stickyResult->ToBoolean()) {
        flagsStr[flagsLen] = 'y';
        flagsLen++;
    }
    flagsStr[flagsLen] = '\0';
    JSHandle<EcmaString> flagsString = factory->NewFromUtf8(flagsStr, flagsLen);
    delete[] flagsStr;

    return flagsString.GetTaggedValue();
}

// 20.2.5.4
JSTaggedValue BuiltinsRegExp::GetGlobal(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, RegExp, GetGlobal);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    return GetFlagsInternal(thread, thisObj, constructor, RegExpParser::FLAG_GLOBAL);
}

// 22.2.6.6
JSTaggedValue BuiltinsRegExp::GetHasIndices(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, RegExp, GetHasIndices);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    return GetFlagsInternal(thread, thisObj, constructor, RegExpParser::FLAG_HASINDICES);
}

// 20.2.5.5
JSTaggedValue BuiltinsRegExp::GetIgnoreCase(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, RegExp, GetIgnoreCase);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    return GetFlagsInternal(thread, thisObj, constructor, RegExpParser::FLAG_IGNORECASE);
}

// 20.2.5.7
JSTaggedValue BuiltinsRegExp::GetMultiline(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, RegExp, GetMultiline);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    return GetFlagsInternal(thread, thisObj, constructor, RegExpParser::FLAG_MULTILINE);
}

JSTaggedValue BuiltinsRegExp::GetDotAll(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, RegExp, GetDotAll);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    return GetFlagsInternal(thread, thisObj, constructor, RegExpParser::FLAG_DOTALL);
}

// 20.2.5.10
JSTaggedValue BuiltinsRegExp::GetSource(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, RegExp, GetSource);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let R be the this value.
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    // 2. If Type(R) is not Object, throw a TypeError exception.
    // 3. If R does not have an [[OriginalSource]] internal slot, throw a TypeError exception.
    // 4. If R does not have an [[OriginalFlags]] internal slot, throw a TypeError exception.
    if (!thisObj->IsECMAObject()) {
        // throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not Object", JSTaggedValue::Exception());
    }
    if (!thisObj->IsJSRegExp()) {
        // a. If SameValue(R, %RegExp.prototype%) is true, return "(?:)".
        const GlobalEnvConstants *globalConst = thread->GlobalConstants();
        JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
        JSHandle<JSTaggedValue> objConstructor = JSTaggedValue::GetProperty(thread, thisObj, constructorKey).GetValue();
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue(false));
        JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
        if (objConstructor->IsJSFunction() && constructor->IsJSFunction()) {
            JSHandle<GlobalEnv> objRealm = JSObject::GetFunctionRealm(thread, objConstructor);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            JSHandle<GlobalEnv> ctorRealm = JSObject::GetFunctionRealm(thread, constructor);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (objRealm->GetRegExpPrototype() == thisObj && *objRealm == *ctorRealm) {
                JSHandle<EcmaString> result = thread->GetEcmaVM()->GetFactory()->NewFromASCII("(?:)");
                return result.GetTaggedValue();
            }
        }
        // b. throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(thread, "this does not have [[OriginalSource]]", JSTaggedValue::Exception());
    }
    // 5. Let src be the value of R’s [[OriginalSource]] internal slot.
    JSHandle<JSRegExp> regexpObj(thread, JSRegExp::Cast(thisObj->GetTaggedObject()));
    JSHandle<JSTaggedValue> source(thread, regexpObj->GetOriginalSource());
    // 6. Let flags be the value of R’s [[OriginalFlags]] internal slot.
    uint8_t flagsBits = static_cast<uint8_t>(regexpObj->GetOriginalFlags().GetInt());
    JSHandle<JSTaggedValue> flags(thread, FlagsBitsToString(thread, flagsBits));
    // 7. Return EscapeRegExpPattern(src, flags).
    return JSTaggedValue(EscapeRegExpPattern(thread, source, flags));
}

// 20.2.5.12
JSTaggedValue BuiltinsRegExp::GetSticky(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, RegExp, GetSticky);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    return GetFlagsInternal(thread, thisObj, constructor, RegExpParser::FLAG_STICKY);
}

// 20.2.5.15
JSTaggedValue BuiltinsRegExp::GetUnicode(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, RegExp, GetUnicode);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    return GetFlagsInternal(thread, thisObj, constructor, RegExpParser::FLAG_UTF16);
}

// 21.2.4.2
JSTaggedValue BuiltinsRegExp::GetSpecies(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), RegExp, GetSpecies);
    return GetThis(argv).GetTaggedValue();
}

// 21.2.5.6
JSTaggedValue BuiltinsRegExp::Match(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), RegExp, Match);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let rx be the this value.
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    // 3. Let S be ToString(string)
    JSHandle<JSTaggedValue> inputString = GetCallArg(argv, 0);
    JSHandle<EcmaString> stringHandle = JSTaggedValue::ToString(thread, inputString);
    // 4. ReturnIfAbrupt(string).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> string = JSHandle<JSTaggedValue>::Cast(stringHandle);
    if (!thisObj->IsECMAObject()) {
        // 2. If Type(rx) is not Object, throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not Object", JSTaggedValue::Exception());
    }
    bool isFastPath = IsFastRegExp(thread, thisObj.GetTaggedValue());
    return RegExpMatch(thread, thisObj, string, isFastPath);
}

JSTaggedValue BuiltinsRegExp::RegExpMatch(JSThread *thread, const JSHandle<JSTaggedValue> regexp,
                                          const JSHandle<JSTaggedValue> string, bool isFastPath)
{
    bool useCache = true;
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    JSHandle<RegExpExecResultCache> cacheTable(thread->GetCurrentEcmaContext()->GetRegExpCache());
    if (!isFastPath || cacheTable->GetLargeStrCount() == 0 || cacheTable->GetConflictCount() == 0) {
        useCache = false;
    }
    bool isGlobal = GetFlag(thread, regexp, RegExpParser::FLAG_GLOBAL, isFastPath);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 7. If global is false, then
    if (!isGlobal) {
        // a. Return RegExpExec(rx, S).
        if (isFastPath) {
            return RegExpBuiltinExec(thread, regexp, string, isFastPath, useCache);
        } else {
            return RegExpExec(thread, regexp, string, useCache);
        }
    }

    if (useCache) {
        uint32_t lastIndex = static_cast<uint32_t>(GetLastIndex(thread, regexp, isFastPath));
        JSTaggedValue cacheResult = cacheTable->FindCachedResult(thread, string,
                                                                 RegExpExecResultCache::MATCH_TYPE, regexp,
                                                                 JSTaggedValue(lastIndex), undefined);
        if (!cacheResult.IsUndefined()) {
            return cacheResult;
        }
    }
    bool fullUnicode = GetFlag(thread, regexp, RegExpParser::FLAG_UTF16, isFastPath);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // b. Let setStatus be Set(rx, "lastIndex", 0, true).
    SetLastIndex(thread, regexp, JSTaggedValue(0), isFastPath);
    // c. ReturnIfAbrupt(setStatus).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // d. Let A be ArrayCreate(0).
    JSHandle<JSArray> array(JSArray::ArrayCreate(thread, JSTaggedNumber(0), ArrayMode::LITERAL));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    TaggedArray *srcElements = TaggedArray::Cast(array->GetElements().GetTaggedObject());
    JSMutableHandle<TaggedArray> elements(thread, srcElements);
    // e. Let n be 0.
    uint32_t resultNum = 0;
    uint32_t arrLen = 1;
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    JSMutableHandle<EcmaString> matchString(thread, JSTaggedValue::Undefined());
    // f. Repeat,
    while (true) {
        if (isFastPath) {
            uint32_t lastIndex = static_cast<uint32_t>(GetLastIndex(thread, regexp, isFastPath));
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            result.Update(RegExpBuiltinExecWithoutResult(thread, regexp, string, isFastPath, lastIndex, false));
            JSHandle<RegExpGlobalResult> globalTable(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
            uint32_t endIndex = static_cast<uint32_t>(globalTable->GetEndOfCaptureIndex(0).GetInt());
            if (result->IsNull()) {
                // 1. If n=0, return null.
                lastIndex = static_cast<uint32_t>(GetLastIndex(thread, regexp, isFastPath));
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                if (resultNum == 0) {
                    RegExpExecResultCache::AddResultInCache(thread, cacheTable, regexp, string,
                                                            JSHandle<JSTaggedValue>(thread, JSTaggedValue::Null()),
                                                            RegExpExecResultCache::MATCH_TYPE,
                                                            0, 0, undefined);
                    return JSTaggedValue::Null();
                }
                RegExpExecResultCache::AddResultInCache(thread, cacheTable, regexp, string,
                                                        JSHandle<JSTaggedValue>::Cast(array),
                                                        RegExpExecResultCache::MATCH_TYPE,
                                                        0, 0, undefined);
                // 2. Else, return A.
                return array.GetTaggedValue();
            }
            uint32_t startIndex = static_cast<uint32_t>(globalTable->GetStartOfCaptureIndex(0).GetInt());
            uint32_t len = endIndex - startIndex;
            matchString.Update(JSTaggedValue(EcmaStringAccessor::FastSubString(
                thread->GetEcmaVM(), JSHandle<EcmaString>::Cast(string), startIndex, len)));
        } else {
            // i. Let result be RegExpExec(rx, S).
            result.Update(RegExpExec(thread, regexp, string, useCache));
            // ii. ReturnIfAbrupt(result).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            // iii. If result is null, then
            if (result->IsNull()) {
                // 1. If n=0, return null.
                if (resultNum == 0) {
                    return JSTaggedValue::Null();
                }
                // 2. Else, return A.
                return array.GetTaggedValue();
            }
            // iv. Else result is not null,
            // 1. Let matchStr be ToString(Get(result, "0")).
            JSHandle<JSTaggedValue> zeroString = thread->GlobalConstants()->GetHandledZeroString();
            JSTaggedValue matchVal = ObjectFastOperator::FastGetPropertyByValue(
                thread, result.GetTaggedValue(), zeroString.GetTaggedValue());
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            JSHandle<JSTaggedValue> matchStr(thread, matchVal);
            matchString.Update(JSTaggedValue::ToString(thread, matchStr));
            // 2. ReturnIfAbrupt(matchStr).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        JSHandle<JSTaggedValue> matchValue = JSHandle<JSTaggedValue>::Cast(matchString);
        // 3. Let status be CreateDataProperty(A, ToString(n), matchStr).
        if (arrLen > elements->GetLength()) {
                elements.Update(JSObject::GrowElementsCapacity(thread,
                    JSHandle<JSObject>::Cast(array), elements->GetLength(), true));
        }
        elements->Set(thread, arrLen - 1, matchValue);
        array->SetArrayLength(thread, arrLen);
        arrLen++;
        // 5. If matchStr is the empty String, then
        if (EcmaStringAccessor(matchString).GetLength() == 0) {
            int64_t lastIndex = GetLastIndex(thread, regexp, isFastPath);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            // c. Let nextIndex be AdvanceStringIndex(S, thisIndex, fullUnicode).
            // d. Let setStatus be Set(rx, "lastIndex", nextIndex, true).
            JSTaggedValue nextIndex = JSTaggedValue(AdvanceStringIndex(string, lastIndex, fullUnicode));
            SetLastIndex(thread, regexp, nextIndex, isFastPath);
            // e. ReturnIfAbrupt(setStatus).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        // 6. Increase n.
        resultNum++;
    }
}

JSTaggedValue BuiltinsRegExp::MatchAll(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, RegExp, MatchAll);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let R be the this value.
    // 2. If Type(R) is not Object, throw a TypeError exception.
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    if (!thisObj->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not Object", JSTaggedValue::Exception());
    }

    // 3. Let S be ? ToString(string).
    JSHandle<JSTaggedValue> inputString = GetCallArg(argv, 0);
    JSHandle<EcmaString> stringHandle = JSTaggedValue::ToString(thread, inputString);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    bool isFastPath = IsFastRegExp(thread, thisObj.GetTaggedValue());
    return RegExpMatchAll(thread, thisObj, stringHandle, isFastPath);
}

JSTaggedValue BuiltinsRegExp::RegExpMatchAll(JSThread *thread, const JSHandle<JSTaggedValue> regexp,
                                             const JSHandle<EcmaString> string, bool isFastPath)
{
    JSMutableHandle<JSTaggedValue> matcher(thread, JSTaggedValue::Undefined());
    bool global = false;
    bool fullUnicode = false;
    if (isFastPath) {
        JSHandle<JSRegExp> jsRegExp = JSHandle<JSRegExp>::Cast(regexp);
        JSHandle<JSTaggedValue> pattern(thread, jsRegExp->GetOriginalSource());
        JSHandle<JSTaggedValue> flags(thread, jsRegExp->GetOriginalFlags());
        matcher.Update(BuiltinsRegExp::RegExpCreate(thread, pattern, flags));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        SetLastIndex(thread, matcher,
            JSHandle<JSObject>::Cast(jsRegExp)->GetPropertyInlinedProps(LAST_INDEX_OFFSET), isFastPath);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        global = GetOriginalFlag(thread, matcher, RegExpParser::FLAG_GLOBAL);
        fullUnicode = GetOriginalFlag(thread, matcher, RegExpParser::FLAG_UTF16);
    } else {
        auto ecmaVm = thread->GetEcmaVM();
        // 4. Let C be ? SpeciesConstructor(R, %RegExp%).
        JSHandle<JSTaggedValue> defaultConstructor = ecmaVm->GetGlobalEnv()->GetRegExpFunction();
        JSHandle<JSObject> objHandle(regexp);
        JSHandle<JSTaggedValue> constructor = JSObject::SpeciesConstructor(thread, objHandle, defaultConstructor);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

        const GlobalEnvConstants *globalConstants = thread->GlobalConstants();
        // 5. Let flags be ? ToString(? Get(R, "flags")).
        JSHandle<JSTaggedValue> flagsString(globalConstants->GetHandledFlagsString());
        JSHandle<JSTaggedValue> getFlags(JSObject::GetProperty(thread, regexp, flagsString).GetValue());
        JSHandle<EcmaString> flagsStrHandle = JSTaggedValue::ToString(thread, getFlags);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

        // 6. Let matcher be ? Construct(C, « R, flags »).
        JSHandle<JSTaggedValue> undefined = globalConstants->GetHandledUndefined();
        EcmaRuntimeCallInfo *runtimeInfo =
            EcmaInterpreter::NewRuntimeCallInfo(thread, constructor, undefined, undefined, 2); // 2: two args
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        runtimeInfo->SetCallArg(regexp.GetTaggedValue(), flagsStrHandle.GetTaggedValue());
        JSTaggedValue taggedMatcher = JSFunction::Construct(runtimeInfo);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        matcher.Update(taggedMatcher);

        // 7. Let lastIndex be ? ToLength(? Get(R, "lastIndex")).
        JSHandle<JSTaggedValue> lastIndexString(globalConstants->GetHandledLastIndexString());
        JSHandle<JSTaggedValue> getLastIndex(JSObject::GetProperty(thread, regexp, lastIndexString).GetValue());
        JSTaggedNumber thisLastIndex = JSTaggedValue::ToLength(thread, getLastIndex);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

        // 8. Perform ? Set(matcher, "lastIndex", lastIndex, true).
        ObjectFastOperator::FastSetPropertyByValue(thread, matcher.GetTaggedValue(), lastIndexString.GetTaggedValue(),
                                                   thisLastIndex);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

        // 9. If flags contains "g", let global be true.
        // 10. Else, let global be false.
        JSHandle<EcmaString> gString(globalConstants->GetHandledGString());
        if (EcmaStringAccessor::IndexOf(ecmaVm, flagsStrHandle, gString) != -1) {
            global = true;
        }

        // 11. If flags contains "u", let fullUnicode be true.
        // 12. Else, let fullUnicode be false.
        JSHandle<EcmaString> uString(globalConstants->GetHandledUString());
        if (EcmaStringAccessor::IndexOf(ecmaVm, flagsStrHandle, uString) != -1) {
            fullUnicode = true;
        }
    }
    // 13. Return ! CreateRegExpStringIterator(matcher, S, global, fullUnicode).
    return JSRegExpIterator::CreateRegExpStringIterator(thread, matcher,
                                                        string, global, fullUnicode).GetTaggedValue();
}

JSTaggedValue BuiltinsRegExp::RegExpReplaceFast(JSThread *thread, JSHandle<JSTaggedValue> regexp,
                                                JSHandle<EcmaString> inputString, uint32_t inputLength)
{
    ASSERT(regexp->IsJSRegExp());
    BUILTINS_API_TRACE(thread, RegExp, RegExpReplaceFast);
    JSHandle<JSRegExp> regexpHandle(regexp);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // get bytecode
    JSTaggedValue bufferData = JSRegExp::Cast(regexp->GetTaggedObject())->GetByteCodeBuffer();
    void *dynBuf = JSNativePointer::Cast(bufferData.GetTaggedObject())->GetExternalPointer();
    // get flags
    auto bytecodeBuffer = reinterpret_cast<uint8_t *>(dynBuf);
    uint32_t flags = *reinterpret_cast<uint32_t *>(bytecodeBuffer + RegExpParser::FLAGS_OFFSET);
    JSHandle<RegExpExecResultCache> cacheTable(thread->GetCurrentEcmaContext()->GetRegExpCache());
    JSHandle<JSTaggedValue> tagInputString = JSHandle<JSTaggedValue>::Cast(inputString);
    bool useCache = false;
    uint32_t lastIndex = 0;
    GetLastIndex(thread, regexp, lastIndex);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> pattern(thread, regexpHandle->GetOriginalSource());
    JSHandle<JSTaggedValue> flagsBits(thread, regexpHandle->GetOriginalFlags());
    useCache = ShouldUseCache(thread, inputString);
    uint32_t lastIndexInput = lastIndex;
    JSHandle<JSTaggedValue> emptyString(thread, globalConst->GetEmptyString());
    if (useCache) {
        JSTaggedValue cacheResult = cacheTable->FindCachedResult(thread, tagInputString,
                                                                 RegExpExecResultCache::REPLACE_TYPE, regexp,
                                                                 JSTaggedValue(lastIndexInput),
                                                                 emptyString);
        if (!cacheResult.IsUndefined()) {
            return cacheResult;
        }
    }

    std::string resultString;
    MatchAndReplace(thread, regexp, inputString, flags,
                    lastIndex, inputLength, resultString);
    auto resultValue = factory->NewFromStdString(resultString);
    if (useCache) {
        RegExpExecResultCache::AddResultInCache(thread, cacheTable, regexp, tagInputString,
                                                JSHandle<JSTaggedValue>(resultValue),
                                                RegExpExecResultCache::REPLACE_TYPE, lastIndexInput, lastIndex,
                                                emptyString);
    }
    return resultValue.GetTaggedValue();
}

JSTaggedValue BuiltinsRegExp::GetLastIndex(JSThread *thread, JSHandle<JSTaggedValue> regexp,
                                           uint32_t &lastIndex)
{
    JSTaggedValue bufferData = JSRegExp::Cast(regexp->GetTaggedObject())->GetByteCodeBuffer();
    void *dynBuf = JSNativePointer::Cast(bufferData.GetTaggedObject())->GetExternalPointer();
    auto bytecodeBuffer = reinterpret_cast<uint8_t *>(dynBuf);
    uint32_t flags = *reinterpret_cast<uint32_t *>(bytecodeBuffer + RegExpParser::FLAGS_OFFSET);
    JSHandle<JSTaggedValue> lastIndexHandle(thread->GlobalConstants()->GetHandledLastIndexString());
    if ((flags & (RegExpParser::FLAG_STICKY | RegExpParser::FLAG_GLOBAL)) == 0) {
        lastIndex = 0;
    } else {
        JSTaggedValue thisIndex =
            ObjectFastOperator::FastGetPropertyByValue(thread, regexp.GetTaggedValue(),
                                                       lastIndexHandle.GetTaggedValue());
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (thisIndex.IsInt()) {
            lastIndex = static_cast<uint32_t>(thisIndex.GetInt());
        } else {
            JSHandle<JSTaggedValue> thisIndexHandle(thread, thisIndex);
            auto lengthValue = JSTaggedValue::ToLength(thread, thisIndexHandle);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            lastIndex = lengthValue.GetNumber();
        }
    }
    return JSTaggedValue::Undefined();
}

bool BuiltinsRegExp::ShouldUseCache(JSThread *thread, JSHandle<EcmaString> inputString)
{
    JSHandle<RegExpExecResultCache> cacheTable(thread->GetCurrentEcmaContext()->GetRegExpCache());
    uint32_t length = EcmaStringAccessor(inputString).GetLength();
    uint32_t largeStrCount = cacheTable->GetLargeStrCount();
    if (largeStrCount != 0) {
        if (length > MIN_REPLACE_STRING_LENGTH) {
            cacheTable->SetLargeStrCount(thread, --largeStrCount);
        }
    } else {
        cacheTable->SetStrLenThreshold(thread, MIN_REPLACE_STRING_LENGTH);
    }
    return length > cacheTable->GetStrLenThreshold();
}

JSTaggedValue BuiltinsRegExp::MatchAndReplace(JSThread *thread, JSHandle<JSTaggedValue> regexp,
                                              JSHandle<EcmaString> inputString, uint32_t &flags,
                                              uint32_t lastIndex, uint32_t inputLength,
                                              std::string &resultString)
{
    uint32_t nextPosition = 0;
    JSHandle<RegExpGlobalResult> globalTable(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    JSHandle<JSTaggedValue> lastIndexHandle(thread->GlobalConstants()->GetHandledLastIndexString());
    JSHandle<JSTaggedValue> tagInputString = JSHandle<JSTaggedValue>::Cast(inputString);

    // 12. Let done be false.
    // 13. Repeat, while done is false
    for (;;) {
        if (lastIndex > inputLength) {
            break;
        }
        bool matchResult = RegExpExecInternal(thread, regexp, inputString, lastIndex);
        if (!matchResult) {
            if (flags & (RegExpParser::FLAG_STICKY | RegExpParser::FLAG_GLOBAL)) {
                lastIndex = 0;
                ObjectFastOperator::FastSetPropertyByValue(thread, regexp.GetTaggedValue(),
                                                           lastIndexHandle.GetTaggedValue(), JSTaggedValue(0));
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            }
            break;
        }
        uint32_t startIndex = static_cast<uint32_t>(globalTable->GetStartOfCaptureIndex(0).GetInt());
        uint32_t endIndex = static_cast<uint32_t>(globalTable->GetEndIndex().GetInt());
        lastIndex = endIndex;
        if (nextPosition < startIndex) {
            auto substr = EcmaStringAccessor::FastSubString(
                thread->GetEcmaVM(), inputString, nextPosition, startIndex - nextPosition);
            resultString += EcmaStringAccessor(substr).ToStdString(StringConvertedUsage::LOGICOPERATION);
        }
        nextPosition = endIndex;
        if (!(flags & RegExpParser::FLAG_GLOBAL)) {
            // a. Let setStatus be Set(R, "lastIndex", e, true).
            ObjectFastOperator::FastSetPropertyByValue(thread, regexp.GetTaggedValue(),
                                                       lastIndexHandle.GetTaggedValue(),
                                                       JSTaggedValue(lastIndex));
            // b. ReturnIfAbrupt(setStatus).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            break;
        }
        if (endIndex == startIndex) {
            bool unicode = EcmaStringAccessor(inputString).IsUtf16() && (flags & RegExpParser::FLAG_UTF16);
            endIndex = static_cast<uint32_t>(AdvanceStringIndex(tagInputString, endIndex, unicode));
        }
        lastIndex = endIndex;
    }
    auto substr = EcmaStringAccessor::FastSubString(
        thread->GetEcmaVM(), inputString, nextPosition, inputLength - nextPosition);
    resultString += EcmaStringAccessor(substr).ToStdString(StringConvertedUsage::LOGICOPERATION);
    return JSTaggedValue::Undefined();
}

// 21.2.5.8
// NOLINTNEXTLINE(readability-function-size)
JSTaggedValue BuiltinsRegExp::Replace(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), RegExp, Replace);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let rx be the this value.
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    if (!thisObj->IsECMAObject()) {
        // 2. If Type(rx) is not Object, throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not Object", JSTaggedValue::Exception());
    }
    // 3. Let S be ToString(string).
    JSHandle<JSTaggedValue> string = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> inputReplaceValue = GetCallArg(argv, 1);
    return ReplaceInternal(thread, thisObj, string, inputReplaceValue);
}

JSTaggedValue BuiltinsRegExp::ReplaceInternal(JSThread *thread,
                                              JSHandle<JSTaggedValue> thisObj,
                                              JSHandle<JSTaggedValue> string,
                                              JSHandle<JSTaggedValue> inputReplaceValue)
{
    JSHandle<EcmaString> srcString = JSTaggedValue::ToString(thread, string);
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    // 4. ReturnIfAbrupt(S).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> inputStr = JSHandle<JSTaggedValue>::Cast(srcString);
    // 5. Let lengthS be the number of code unit elements in S.
    uint32_t length = EcmaStringAccessor(srcString).GetLength();
    // 6. Let functionalReplace be IsCallable(replaceValue).
    bool functionalReplace = inputReplaceValue->IsCallable();
    JSHandle<EcmaString> replaceValueHandle;
    // Add cache for regexp replace
    bool useCache = true;
    if (!functionalReplace) {
        replaceValueHandle = JSTaggedValue::ToString(thread, inputReplaceValue);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    } else {
        useCache = false;
    }
    // 8. Let global be ToBoolean(Get(rx, "global")).
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    bool isGlobal = false;
    bool fullUnicode = false;
    bool isFastPath = IsFastRegExp(thread, thisObj.GetTaggedValue());
    if (isFastPath) {
        isGlobal = GetOriginalFlag(thread, thisObj, RegExpParser::FLAG_GLOBAL);
        fullUnicode = GetOriginalFlag(thread, thisObj, RegExpParser::FLAG_UTF16);
        if (isGlobal) {
            SetLastIndex(thread, thisObj, JSTaggedValue(0), isFastPath);
        }
    } else {
        // 9. ReturnIfAbrupt(global).
        useCache = false;
        isGlobal = GetFlag(thread, thisObj, RegExpParser::FLAG_GLOBAL, isFastPath);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // 10. If global is true, then
        if (isGlobal) {
            // a. Let fullUnicode be ToBoolean(Get(rx, "unicode")).
            fullUnicode = GetFlag(thread, thisObj, RegExpParser::FLAG_UTF16, isFastPath);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            // b. Let setStatus be Set(rx, "lastIndex", 0, true).
            SetLastIndex(thread, thisObj, JSTaggedValue(0), isFastPath);
            // c. ReturnIfAbrupt(setStatus).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
    }

    // Add cache for the intermediate result of replace
    bool useIntermediateCache = false;
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    JSHandle<RegExpExecResultCache> cacheTable(thread->GetCurrentEcmaContext()->GetRegExpCache());
    if (isFastPath) {
        if (isGlobal && !functionalReplace && EcmaStringAccessor(replaceValueHandle).GetLength() == 0) {
            return RegExpReplaceFast(thread, thisObj, srcString, length);
        }
        JSHandle<JSRegExp> regexpHandle(thisObj);
        useIntermediateCache = true;
        if (!functionalReplace) {
            uint32_t strLength = EcmaStringAccessor(replaceValueHandle).GetLength();
            uint32_t largeStrCount = cacheTable->GetLargeStrCount();
            if (largeStrCount != 0) {
                if (strLength > MIN_REPLACE_STRING_LENGTH) {
                    cacheTable->SetLargeStrCount(thread, --largeStrCount);
                }
            } else {
                cacheTable->SetStrLenThreshold(thread, MIN_REPLACE_STRING_LENGTH);
            }
            if (strLength > cacheTable->GetStrLenThreshold()) {
                uint32_t lastIndexInput = static_cast<uint32_t>(GetLastIndex(thread, thisObj, isFastPath));
                JSTaggedValue cacheResult = cacheTable->FindCachedResult(thread, string,
                    RegExpExecResultCache::REPLACE_TYPE, thisObj, JSTaggedValue(lastIndexInput),
                    inputReplaceValue);
                if (!cacheResult.IsUndefined()) {
                    return cacheResult;
                }
            }
        }
    }

    // 11. Let results be a new empty List.
    JSMutableHandle<JSObject> resultsList(thread, JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int resultsIndex = 0;
    JSMutableHandle<JSTaggedValue> nextIndexHandle(thread, JSTaggedValue(0));
    JSMutableHandle<JSTaggedValue> execResult(thread, JSTaggedValue(0));
    // Add cache for the intermediate result of replace
    JSTaggedValue cachedResultsList(JSTaggedValue::VALUE_UNDEFINED);
    if (useIntermediateCache) {
        uint32_t lastIndexInput = static_cast<uint32_t>(GetLastIndex(thread, thisObj, isFastPath));
        cachedResultsList = cacheTable->FindCachedResult(thread, string,
            RegExpExecResultCache::INTERMEDIATE_REPLACE_TYPE, thisObj,
            JSTaggedValue(lastIndexInput), undefined, true);
    }
    if (!cachedResultsList.IsUndefined()) {
        resultsList.Update(cachedResultsList);
        resultsIndex = static_cast<int>(JSArray::Cast(resultsList.GetTaggedValue())->GetArrayLength());
    } else {
        // 12. Let done be false.
        // 13. Repeat, while done is false
        for (;;) {
            // a. Let result be RegExpExec(rx, S).
            execResult.Update(RegExpExec(thread, thisObj, inputStr, useCache, true));
            // b. ReturnIfAbrupt(result).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            // c. If result is null, set done to true.
            if (execResult->IsNull()) {
                break;
            }
            // d. Else result is not null, i. Append result to the end of results.
            TaggedArray *srcElements = TaggedArray::Cast(resultsList->GetElements().GetTaggedObject());
            JSMutableHandle<TaggedArray> elements(thread, srcElements);
            if (resultsIndex >= static_cast<int>(elements->GetLength())) {
                elements.Update(JSObject::GrowElementsCapacity(thread, resultsList, elements->GetLength(), true));
            }
            elements->Set(thread, resultsIndex, execResult);
            JSArray::Cast(*resultsList)->SetArrayLength(thread, resultsIndex + 1);
            resultsIndex++;
            // ii. If global is false, set done to true.
            if (!isGlobal) {
                break;
            }
            // iii. Else, 1. Let matchStr be ToString(Get(result, "0")).
            JSHandle<JSTaggedValue> matchedStr = globalConst->GetHandledZeroString();
            JSTaggedValue getMatchVal = ObjectFastOperator::FastGetPropertyByValue(
                thread, execResult.GetTaggedValue(), matchedStr.GetTaggedValue());
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            JSHandle<JSTaggedValue> getMatch(thread, getMatchVal);
            JSHandle<EcmaString> matchString = JSTaggedValue::ToString(thread, getMatch);
            // 2. ReturnIfAbrupt(matchStr).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            // 3. If matchStr is the empty String, then
            if (EcmaStringAccessor(matchString).GetLength() == 0) {
                // a. Let thisIndex be ToLength(Get(rx, "lastIndex")).
                uint32_t thisIndex = static_cast<uint32_t>(GetLastIndex(thread, thisObj, isFastPath));
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                // c. Let nextIndex be AdvanceStringIndex(S, thisIndex, fullUnicode).
                uint32_t nextIndex = static_cast<uint32_t>(AdvanceStringIndex(inputStr, thisIndex, fullUnicode));
                nextIndexHandle.Update(JSTaggedValue(nextIndex));
                // d. Let setStatus be Set(rx, "lastIndex", nextIndex, true).
                SetLastIndex(thread, thisObj, nextIndexHandle.GetTaggedValue(), isFastPath);
                // e. ReturnIfAbrupt(setStatus).
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            }
        }
        if (useIntermediateCache) {
            RegExpExecResultCache::AddResultInCache(thread, cacheTable, thisObj, string,
                                                    JSHandle<JSTaggedValue>(resultsList),
                                                    RegExpExecResultCache::INTERMEDIATE_REPLACE_TYPE, 0, 0,
                                                    undefined, true);
        }
    }
    // 14. Let accumulatedResult be the empty String value.
    bool isUtf8 = true;
    uint32_t resultStrLength = 0;
    uint32_t resultArrayLength = (static_cast<uint32_t>(resultsIndex) + 1) * 2;
    CVector<JSHandle<JSTaggedValue>> resultArray(resultArrayLength, globalConst->GetHandledHole());
    std::vector<uint64_t> resultLengthArray(resultArrayLength);
    CVector<JSHandle<JSTaggedValue>> capturesList;
    // 15. Let nextSourcePosition be 0.
    uint32_t nextSourcePosition = 0;
    JSMutableHandle<JSTaggedValue> getMatchString(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> resultValues(thread, JSTaggedValue(0));
    JSMutableHandle<JSTaggedValue> ncapturesHandle(thread, JSTaggedValue(0));
    JSMutableHandle<JSTaggedValue> capN(thread, JSTaggedValue(0));
    // 16. Repeat, for each result in results,
    for (int i = 0; i < resultsIndex; i++) {
        resultValues.Update(ElementAccessor::Get(resultsList, i));
        // a. Let nCaptures be ToLength(Get(result, "length")).
        uint32_t ncaptures;
        if (isFastPath) {
            ncaptures = static_cast<uint32_t>(JSArray::Cast(resultValues.GetTaggedValue())->GetArrayLength());
        } else {
            JSHandle<JSTaggedValue> lengthHandle = globalConst->GetHandledLengthString();
            ncapturesHandle.Update(ObjectFastOperator::FastGetPropertyByValue(
                thread, resultValues.GetTaggedValue(), lengthHandle.GetTaggedValue()));
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            ncaptures = JSTaggedValue::ToUint32(thread, ncapturesHandle);
        }
        // b. ReturnIfAbrupt(nCaptures).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // c. Let nCaptures be max(nCaptures − 1, 0).
        ncaptures = (ncaptures == 0) ? 0 : ncaptures - 1;
        // d. Let matched be ToString(Get(result, "0")).
        JSTaggedValue value = ObjectFastOperator::GetPropertyByIndex(thread, resultValues.GetTaggedValue(), 0);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        getMatchString.Update(value);
        JSHandle<EcmaString> matchString = JSTaggedValue::ToString(thread, getMatchString);
        // e. ReturnIfAbrupt(matched).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // f. Let matchLength be the number of code units in matched.
        uint32_t matchLength = EcmaStringAccessor(matchString).GetLength();
        // g. Let position be ToInteger(Get(result, "index")).
        JSTaggedValue positionTag = GetExecResultIndex(thread, resultValues, isFastPath);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSHandle<JSTaggedValue> positionHandle(thread, positionTag);
        uint32_t position = 0;
        if (positionHandle->IsInt()) {
            position = static_cast<uint32_t>(positionHandle->GetInt());
        } else {
            position = JSTaggedValue::ToUint32(thread, positionHandle);
            // h. ReturnIfAbrupt(position).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        // i. Let position be max(min(position, lengthS), 0).
        position = std::max<uint32_t>(std::min<uint32_t>(position, length), 0);
        // j. Let captures be an empty List.
        capturesList.resize(ncaptures);
        // l. Repeat while n < nCaptures
        for (uint32_t index = 0; index < ncaptures; index++) {
            // i. Let capN be Get(result, ToString(n)).
            capN.Update(ObjectFastOperator::FastGetPropertyByIndex(thread, resultValues.GetTaggedValue(), index + 1));
            // ii. ReturnIfAbrupt(capN).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            // iii. If capN is not undefined, then
            if (!capN->IsUndefined()) {
                // 1. Let capN be ToString(capN).
                JSHandle<EcmaString> capNStr = JSTaggedValue::ToString(thread, capN);
                // 2. ReturnIfAbrupt(capN).
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                capturesList[index] = JSHandle<JSTaggedValue>(thread, capNStr.GetTaggedValue());
            } else {
                // iv. Append capN as the last element of captures.
                capturesList[index] = JSHandle<JSTaggedValue>(thread, capN.GetTaggedValue());
            }
        }
        // j. Let namedCaptures be ? Get(result, "groups").
        JSTaggedValue named = GetExecResultGroups(thread, resultValues, isFastPath);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSHandle<JSTaggedValue> namedCaptures(thread, named);
        // m. If functionalReplace is true, then
        JSMutableHandle<EcmaString> replacementString(thread, factory->GetEmptyString());
        int emptyArrLength = 0;
        if (namedCaptures->IsUndefined()) {
            emptyArrLength = 3; // 3: «matched, pos, and string»
        } else {
            emptyArrLength = 4; // 4: «matched, pos, string, and groups»
        }
        if (functionalReplace) {
            // Let replValue be Call(replaceValue, undefined, replacerArgs).
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread,
                    inputReplaceValue, undefined, undefined, emptyArrLength + ncaptures);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            
            // i. Let replacerArgs be «matched».
            info->SetCallArg(0, getMatchString.GetTaggedValue());
            // ii. Append in list order the elements of captures to the end of the List replacerArgs.
            for (uint32_t index = 0; index < ncaptures; index++) {
                info->SetCallArg(index + 1, capturesList[index].GetTaggedValue());
            }
            // iii. Append position and S as the last two elements of replacerArgs.
            info->SetCallArg(ncaptures + EXEC_RESULT_INDEX_OFFSET, JSTaggedValue(position));
            info->SetCallArg(ncaptures + EXEC_RESULT_INPUT_OFFSET, inputStr.GetTaggedValue());
            if (!namedCaptures->IsUndefined()) {
                // iv. position of groups
                info->SetCallArg(ncaptures + EXEC_RESULT_GROUPS_OFFSET, namedCaptures.GetTaggedValue());
            }
            JSTaggedValue replaceResult = JSFunction::Call(info);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            JSHandle<JSTaggedValue> replValue(thread, replaceResult);
            // v. Let replacement be ToString(replValue).
            replacementString.Update(JSTaggedValue::ToString(thread, replValue));
            // o. ReturnIfAbrupt(replacement).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        } else {
            // n. Else,
            JSHandle<TaggedArray> capturesArray = factory->NewTaggedArray(ncaptures);
            if (!namedCaptures->IsUndefined()) {
                JSHandle<JSObject> namedCapturesObj = JSTaggedValue::ToObject(thread, namedCaptures);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                namedCaptures = JSHandle<JSTaggedValue>::Cast(namedCapturesObj);
            }
            for (uint32_t index = 0; index < ncaptures; index++) {
                capturesArray->Set(thread, index, capturesList[index]);
            }
            replacementString.Update(BuiltinsString::GetSubstitution(thread, matchString, srcString,
                position, capturesArray, namedCaptures, replaceValueHandle));
        }
        // p. If position ≥ nextSourcePosition, then
        if (position >= nextSourcePosition) {
            ASSERT(REPLACE_RESULT_VAL * i + 1 < resultArray.size());
            // ii. Let accumulatedResult be the String formed by concatenating the code units of the current value
            // of accumulatedResult with the substring of S consisting of the code units from nextSourcePosition
            // (inclusive) up to position (exclusive) and with the code units of replacement.
            // store undefined in resultArray
            resultArray[REPLACE_RESULT_VAL * i] = globalConst->GetHandledUndefined();
            uint64_t bits = 0;
            bits |= ReplaceLengthField::Encode(position - nextSourcePosition);
            bits |= ReplacePositionField::Encode(nextSourcePosition);
            // store position and length bits in resultLengthArray
            resultLengthArray[REPLACE_RESULT_VAL * i] = bits;
            resultStrLength += (position - nextSourcePosition);
            isUtf8 &= EcmaStringAccessor::SubStringIsUtf8(
                thread->GetEcmaVM(), srcString, nextSourcePosition, position - nextSourcePosition);
            // store replacement string in resultArray
            resultArray[REPLACE_RESULT_VAL * i + 1] =
                JSHandle<JSTaggedValue>(thread, replacementString.GetTaggedValue());
            uint32_t replacementLength = EcmaStringAccessor(replacementString).GetLength();
            // store length of replacement string in resultLengthArray
            resultLengthArray[REPLACE_RESULT_VAL * i + 1] = static_cast<uint64_t>(replacementLength);
            resultStrLength += replacementLength;
            isUtf8 &= EcmaStringAccessor(replacementString).IsUtf8();
            // iii. Let nextSourcePosition be position + matchLength.
            nextSourcePosition = position + matchLength;
        }
    }

    // 17. If nextSourcePosition ≥ lengthS, return accumulatedResult.
    if (nextSourcePosition < length) {
        // store undefined in resultArray
        resultArray[REPLACE_RESULT_VAL * resultsIndex] = globalConst->GetHandledUndefined();
        uint64_t bits = 0;
        bits |= ReplaceLengthField::Encode(length - nextSourcePosition);
        bits |= ReplacePositionField::Encode(nextSourcePosition);
        isUtf8 &= EcmaStringAccessor::SubStringIsUtf8(
            thread->GetEcmaVM(), srcString, nextSourcePosition, length - nextSourcePosition);
        // store position and length bits in resultLengthArray
        resultLengthArray[REPLACE_RESULT_VAL * resultsIndex] = bits;
        resultStrLength += (length - nextSourcePosition);
    }

    JSHandle<EcmaString> result =
        CreateStringFromResultArray(thread, resultArray, resultLengthArray, srcString, resultStrLength, isUtf8);
    // 18. Return the String formed by concatenating the code units of accumulatedResult with the substring of S
    // consisting of the code units from nextSourcePosition (inclusive) up through the final code unit of S(inclusive).
    if (useCache) {
        RegExpExecResultCache::AddResultInCache(thread, cacheTable, thisObj, string,
                                                JSHandle<JSTaggedValue>(result),
                                                RegExpExecResultCache::REPLACE_TYPE, 0, nextIndexHandle->GetInt(),
                                                inputReplaceValue);
    }
    return result.GetTaggedValue();
}

// 21.2.5.9
JSTaggedValue BuiltinsRegExp::Search(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), RegExp, Search);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let rx be the this value.
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    // 3. Let S be ToString(string).
    JSHandle<JSTaggedValue> inputStr = GetCallArg(argv, 0);
    JSHandle<EcmaString> stringHandle = JSTaggedValue::ToString(thread, inputStr);

    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> string = JSHandle<JSTaggedValue>::Cast(stringHandle);
    if (!thisObj->IsECMAObject()) {
        // 2. If Type(rx) is not Object, throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not Object", JSTaggedValue::Exception());
    }
    return RegExpSearch(thread, thisObj, string);
}

JSTaggedValue BuiltinsRegExp::RegExpSearch(JSThread *thread,
                                           const JSHandle<JSTaggedValue> regexp,
                                           const JSHandle<JSTaggedValue> string)
{
    bool isFastPath = IsFastRegExp(thread, regexp.GetTaggedValue());
    if (isFastPath) {
        return RegExpSearchFast(thread, regexp, string);
    }
    // 4. Let previousLastIndex be ? Get(rx, "lastIndex").
    JSHandle<JSTaggedValue> lastIndexString(thread->GlobalConstants()->GetHandledLastIndexString());
    JSHandle<JSTaggedValue> previousLastIndex = JSObject::GetProperty(thread, regexp, lastIndexString).GetValue();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 5. If SameValue(previousLastIndex, 0) is false, then
    // Perform ? Set(rx, "lastIndex", 0, true).
    if (!JSTaggedValue::SameValue(previousLastIndex.GetTaggedValue(), JSTaggedValue(0))) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(0));
        JSObject::SetProperty(thread, regexp, lastIndexString, value, true);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    // 6. Let result be ? RegExpExec(rx, S).
    JSHandle<JSTaggedValue> result(thread, RegExpExec(thread, regexp, string, false));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 7. Let currentLastIndex be ? Get(rx, "lastIndex").
    JSHandle<JSTaggedValue> currentLastIndex = JSObject::GetProperty(thread, regexp, lastIndexString).GetValue();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 8. If SameValue(currentLastIndex, previousLastIndex) is false, then
    // Perform ? Set(rx, "lastIndex", previousLastIndex, true).
    if (!JSTaggedValue::SameValue(previousLastIndex.GetTaggedValue(), currentLastIndex.GetTaggedValue())) {
        JSObject::SetProperty(thread, regexp, lastIndexString, previousLastIndex, true);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    // 9. If result is null, return -1.
    if (result->IsNull()) {
        return JSTaggedValue(-1);
    }
    // 10. Return ? Get(result, "index").
    JSHandle<JSTaggedValue> index(thread->GlobalConstants()->GetHandledIndexString());
    return JSObject::GetProperty(thread, result, index).GetValue().GetTaggedValue();
}

JSTaggedValue BuiltinsRegExp::RegExpSearchFast(JSThread *thread,
                                               const JSHandle<JSTaggedValue> regexp,
                                               const JSHandle<JSTaggedValue> string)
{
    JSHandle<RegExpExecResultCache> cacheTable(thread->GetCurrentEcmaContext()->GetRegExpCache());
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();

    JSTaggedValue cacheResult = cacheTable->FindCachedResult(thread, string,
                                                             RegExpExecResultCache::SEARCH_TYPE, regexp,
                                                             JSTaggedValue(0), undefined);
    if (!cacheResult.IsUndefined()) {
        return cacheResult;
    }
    JSHandle<EcmaString> stringHandle = JSHandle<EcmaString>::Cast(string);
    bool matchResult = RegExpExecInternal(thread, regexp, stringHandle, 0);
    if (!matchResult) {
        RegExpExecResultCache::AddResultInCache(thread, cacheTable, regexp, string,
                                                JSHandle<JSTaggedValue>(thread, JSTaggedValue(-1)),
                                                RegExpExecResultCache::SEARCH_TYPE,
                                                0, 0, undefined);
        return JSTaggedValue(-1);
    }
    JSHandle<RegExpGlobalResult> globalTable(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    JSTaggedValue result = globalTable->GetStartOfCaptureIndex(0);
    RegExpExecResultCache::AddResultInCache(thread, cacheTable, regexp, string,
                                            JSHandle<JSTaggedValue>(thread, JSTaggedValue(result)),
                                            RegExpExecResultCache::SEARCH_TYPE,
                                            0, 0, undefined);
    return result;
}

JSTaggedValue BuiltinsRegExp::RegExpSplit(JSThread *thread, const JSHandle<JSTaggedValue> regexp,
                                          JSHandle<JSTaggedValue> jsString, JSHandle<JSTaggedValue> limit,
                                          bool isFastPath)
{
    bool useCache = false;
    if (isFastPath) {
        if (limit->IsUndefined()) {
            useCache = true;
            return RegExpSplitFast(thread, regexp, jsString, MAX_SPLIT_LIMIT, useCache);
        } else if (limit->IsInt()) {
            int64_t lim = limit->GetInt();
            if (lim >= 0) {
                return RegExpSplitFast(thread, regexp, jsString, static_cast<uint32_t>(lim), useCache);
            }
        }
    }
    auto ecmaVm = thread->GetEcmaVM();
    // 5. Let C be SpeciesConstructor(rx, %RegExp%).
    JSHandle<JSTaggedValue> defaultConstructor = ecmaVm->GetGlobalEnv()->GetRegExpFunction();
    JSHandle<JSObject> objHandle(regexp);
    JSHandle<JSTaggedValue> constructor = JSObject::SpeciesConstructor(thread, objHandle, defaultConstructor);
    // 6. ReturnIfAbrupt(C).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 7. Let flags be ToString(Get(rx, "flags")).
    ObjectFactory *factory = ecmaVm->GetFactory();
    const GlobalEnvConstants *globalConstants = thread->GlobalConstants();
    JSHandle<JSTaggedValue> flagsString(globalConstants->GetHandledFlagsString());
    JSHandle<JSTaggedValue> taggedFlags = JSObject::GetProperty(thread, regexp, flagsString).GetValue();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<EcmaString> flags;

    if (taggedFlags->IsUndefined()) {
        flags = factory->GetEmptyString();
    } else {
        flags = JSTaggedValue::ToString(thread, taggedFlags);
    }
    //  8. ReturnIfAbrupt(flags).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 9. If flags contains "u", let unicodeMatching be true.
    // 10. Else, let unicodeMatching be false.
    JSHandle<EcmaString> uStringHandle(globalConstants->GetHandledUString());
    bool unicodeMatching = (EcmaStringAccessor::IndexOf(ecmaVm, flags, uStringHandle) != -1);
    // 11. If flags contains "y", let newFlags be flags.
    JSHandle<EcmaString> newFlagsHandle;
    JSHandle<EcmaString> yStringHandle = JSHandle<EcmaString>::Cast(globalConstants->GetHandledYString());
    if (EcmaStringAccessor::IndexOf(ecmaVm, flags, yStringHandle) != -1) {
        newFlagsHandle = flags;
    } else {
        // 12. Else, let newFlags be the string that is the concatenation of flags and "y".
        JSHandle<EcmaString> yStr = JSHandle<EcmaString>::Cast(globalConstants->GetHandledYString());
        newFlagsHandle = factory->ConcatFromString(flags, yStr);
    }

    // 13. Let splitter be Construct(C, «rx, newFlags»).
    JSHandle<JSObject> globalObject(thread, thread->GetEcmaVM()->GetGlobalEnv()->GetGlobalObject());
    JSHandle<JSTaggedValue> undefined = globalConstants->GetHandledUndefined();
    EcmaRuntimeCallInfo *runtimeInfo =
        EcmaInterpreter::NewRuntimeCallInfo(thread, constructor, undefined, undefined, 2); // 2: two args
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    runtimeInfo->SetCallArg(regexp.GetTaggedValue(), newFlagsHandle.GetTaggedValue());
    JSTaggedValue taggedSplitter = JSFunction::Construct(runtimeInfo);
    // 14. ReturnIfAbrupt(splitter).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSHandle<JSTaggedValue> splitter(thread, taggedSplitter);
    // 15. Let A be ArrayCreate(0).
    JSHandle<JSObject> array(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 16. Let lengthA be 0.
    uint32_t aLength = 0;

    // 17. If limit is undefined, let lim be 2^32–1; else let lim be ToUint32(limit).
    uint32_t lim;
    if (limit->IsUndefined()) {
        lim = MAX_SPLIT_LIMIT;
    } else {
        lim = JSTaggedValue::ToUint32(thread, limit);
        // 18. ReturnIfAbrupt(lim).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }

    // 19. Let size be the number of elements in S.
    uint32_t size = EcmaStringAccessor(jsString->GetTaggedObject()).GetLength();
    // 20. Let p be 0.
    uint32_t startIndex = 0;
    // 21. If lim = 0, return A.
    if (lim == 0) {
        return JSTaggedValue(static_cast<JSArray *>(array.GetTaggedValue().GetTaggedObject()));
    }
    // 22. If size = 0, then
    if (size == 0) {
        // a. Let z be RegExpExec(splitter, S).
        JSHandle<JSTaggedValue> execResult(thread, RegExpExec(thread, splitter, jsString, useCache));
        // b. ReturnIfAbrupt(z).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // c. If z is not null, return A.
        if (!execResult->IsNull()) {
            return JSTaggedValue(static_cast<JSArray *>(array.GetTaggedValue().GetTaggedObject()));
        }
        // d. Assert: The following call will never result in an abrupt completion.
        // e. Perform CreateDataProperty(A, "0", S).
        JSObject::CreateDataProperty(thread, array, 0, jsString);
        // f. Return A.
        return JSTaggedValue(static_cast<JSArray *>(array.GetTaggedValue().GetTaggedObject()));
    }
    // 23. Let q be p.
    uint32_t endIndex = startIndex;
    JSMutableHandle<JSTaggedValue> lastIndexvalue(thread, JSTaggedValue(endIndex));
    // 24. Repeat, while q < size
    JSHandle<JSTaggedValue> lastIndexString = globalConstants->GetHandledLastIndexString();
    while (endIndex < size) {
        // a. Let setStatus be Set(splitter, "lastIndex", q, true).
        lastIndexvalue.Update(JSTaggedValue(endIndex));
        JSObject::SetProperty(thread, splitter, lastIndexString, lastIndexvalue, true);
        // b. ReturnIfAbrupt(setStatus).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSHandle<JSTaggedValue> execResult(thread, RegExpExec(thread, splitter, jsString, useCache));
        // d. ReturnIfAbrupt(z).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // e. If z is null, let q be AdvanceStringIndex(S, q, unicodeMatching).
        if (execResult->IsNull()) {
            endIndex = static_cast<uint32_t>(AdvanceStringIndex(jsString, endIndex, unicodeMatching));
        } else {
            // f. Else z is not null,
            // i. Let e be ToLength(Get(splitter, "lastIndex")).
            JSHandle<JSTaggedValue> lastIndexHandle =
                JSObject::GetProperty(thread, splitter, lastIndexString).GetValue();
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            JSTaggedNumber lastIndexNumber = JSTaggedValue::ToLength(thread, lastIndexHandle);
            // ii. ReturnIfAbrupt(e).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            uint32_t lastIndex = lastIndexNumber.GetNumber();
            // iii. If e = p, let q be AdvanceStringIndex(S, q, unicodeMatching).
            if (lastIndex == startIndex) {
                endIndex = static_cast<uint32_t>(AdvanceStringIndex(jsString, endIndex, unicodeMatching));
            } else {
                // iv. Else e != p,
                // 1. Let T be a String value equal to the substring of S consisting of the elements at indices p
                // (inclusive) through q (exclusive).
                auto substr = EcmaStringAccessor::FastSubString(thread->GetEcmaVM(),
                    JSHandle<EcmaString>::Cast(jsString), startIndex, endIndex - startIndex);
                std::string stdStrT = EcmaStringAccessor(substr).ToStdString(StringConvertedUsage::LOGICOPERATION);
                // 2. Assert: The following call will never result in an abrupt completion.
                // 3. Perform CreateDataProperty(A, ToString(lengthA), T).
                JSHandle<JSTaggedValue> tValue(factory->NewFromStdString(stdStrT));
                JSObject::CreateDataProperty(thread, array, aLength, tValue);
                // 4. Let lengthA be lengthA +1.
                ++aLength;
                // 5. If lengthA = lim, return A.
                if (aLength == lim) {
                    return array.GetTaggedValue();
                }
                // 6. Let p be e.
                startIndex = lastIndex;
                // 7. Let numberOfCaptures be ToLength(Get(z, "length")).
                JSHandle<JSTaggedValue> lengthString(thread->GlobalConstants()->GetHandledLengthString());
                JSHandle<JSTaggedValue> capturesHandle =
                    JSObject::GetProperty(thread, execResult, lengthString).GetValue();
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                JSTaggedNumber numberOfCapturesNumber = JSTaggedValue::ToLength(thread, capturesHandle);
                // 8. ReturnIfAbrupt(numberOfCaptures).
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                uint32_t numberOfCaptures = numberOfCapturesNumber.GetNumber();
                // 9. Let numberOfCaptures be max(numberOfCaptures-1, 0).
                numberOfCaptures = (numberOfCaptures == 0) ? 0 : numberOfCaptures - 1;
                // 10. Let i be 1.
                uint32_t i = 1;
                // 11. Repeat, while i ≤ numberOfCaptures.
                while (i <= numberOfCaptures) {
                    // a. Let nextCapture be Get(z, ToString(i)).
                    JSHandle<JSTaggedValue> nextCapture = JSObject::GetProperty(thread, execResult, i).GetValue();
                    // b. ReturnIfAbrupt(nextCapture).
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                    // c. Perform CreateDataProperty(A, ToString(lengthA), nextCapture).
                    JSObject::CreateDataProperty(thread, array, aLength, nextCapture);
                    // d. Let i be i + 1.
                    ++i;
                    // e. Let lengthA be lengthA +1.
                    ++aLength;
                    // f. If lengthA = lim, return A.
                    if (aLength == lim) {
                        return array.GetTaggedValue();
                    }
                }
                // 12. Let q be p.
                endIndex = startIndex;
            }
        }
    }
    // 25. Let T be a String value equal to the substring of S consisting of the elements at indices p (inclusive)
    // through size (exclusive).
    auto substr = EcmaStringAccessor::FastSubString(thread->GetEcmaVM(),
        JSHandle<EcmaString>::Cast(jsString), startIndex, size - startIndex);
    std::string stdStrT = EcmaStringAccessor(substr).ToStdString(StringConvertedUsage::LOGICOPERATION);
    // 26. Assert: The following call will never result in an abrupt completion.
    // 27. Perform CreateDataProperty(A, ToString(lengthA), t).
    JSHandle<JSTaggedValue> tValue(factory->NewFromStdString(stdStrT));
    JSObject::CreateDataProperty(thread, array, aLength, tValue);
    // 28. Return A.
    return array.GetTaggedValue();
}
// 21.2.5.11
// NOLINTNEXTLINE(readability-function-size)
JSTaggedValue BuiltinsRegExp::Split(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), RegExp, Split);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let rx be the this value.
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    // 3. Let S be ToString(string).
    JSHandle<JSTaggedValue> inputString = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> limit = GetCallArg(argv, 1);
    JSHandle<EcmaString> stringHandle = JSTaggedValue::ToString(thread, inputString);

    // 4. ReturnIfAbrupt(string).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> jsString = JSHandle<JSTaggedValue>::Cast(stringHandle);
    if (!thisObj->IsECMAObject()) {
        // 2. If Type(rx) is not Object, throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not Object", JSTaggedValue::Exception());
    }
    bool isFastPath = IsFastRegExp(thread, thisObj.GetTaggedValue());
    return RegExpSplit(thread, thisObj, jsString, limit, isFastPath);
}

JSTaggedValue BuiltinsRegExp::RegExpSplitFast(JSThread *thread, const JSHandle<JSTaggedValue> regexp,
                                              JSHandle<JSTaggedValue> jsString, uint32_t limit, bool useCache)
{
    if (limit == 0) {
        return JSArray::ArrayCreate(thread, JSTaggedNumber(0), ArrayMode::LITERAL).GetTaggedValue();
    }
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    JSHandle<RegExpExecResultCache> cacheTable(thread->GetCurrentEcmaContext()->GetRegExpCache());
    if (useCache) {
        JSTaggedValue cacheResult = cacheTable->FindCachedResult(thread, jsString,
                                                                 RegExpExecResultCache::SPLIT_TYPE, regexp,
                                                                 JSTaggedValue(0), undefined);
        if (!cacheResult.IsUndefined()) {
            return cacheResult;
        }
    }
    uint32_t size = EcmaStringAccessor(jsString->GetTaggedObject()).GetLength();
    JSHandle<EcmaString> string = JSHandle<EcmaString>::Cast(jsString);

    if (size == 0) {
        bool matchResult = RegExpExecInternal(thread, regexp, string, 0); // 0: lastIndex
        if (matchResult) {
            JSHandle<JSTaggedValue> res = JSArray::ArrayCreate(thread, JSTaggedNumber(0), ArrayMode::LITERAL);
            if (useCache) {
                RegExpExecResultCache::AddResultInCache(thread, cacheTable, regexp, jsString,
                                                        res, RegExpExecResultCache::SPLIT_TYPE, 0, 0, undefined);
            }
            return res.GetTaggedValue();
        }
        JSHandle<TaggedArray> element = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(1); // 1: length
        element->Set(thread, 0, jsString);
        JSHandle<JSTaggedValue> res = JSHandle<JSTaggedValue>::Cast(JSArray::CreateArrayFromList(thread, element));
        if (useCache) {
            RegExpExecResultCache::AddResultInCache(thread, cacheTable, regexp, jsString,
                                                    res, RegExpExecResultCache::SPLIT_TYPE, 0, 0, undefined);
        }
        return res.GetTaggedValue();
    }

    bool isUnicode = GetOriginalFlag(thread, regexp, RegExpParser::FLAG_UTF16);
    bool isSticky = GetOriginalFlag(thread, regexp, RegExpParser::FLAG_STICKY);

    uint32_t nextMatchFrom = 0;
    uint32_t lastMatchEnd = 0;
    uint32_t arrLen = 1; // at least one result string
    JSHandle<JSArray> splitArray(JSArray::ArrayCreate(thread, JSTaggedNumber(1), ArrayMode::LITERAL));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    TaggedArray *srcElements = TaggedArray::Cast(splitArray->GetElements().GetTaggedObject());
    JSMutableHandle<TaggedArray> elements(thread, srcElements);
    JSMutableHandle<JSTaggedValue> matchValue(thread, JSTaggedValue::Undefined());
    while (nextMatchFrom < size) {
        bool matchResult = RegExpExecInternal(thread, regexp, string, nextMatchFrom);
        if (!matchResult) {
            if (!isSticky) {
                // done match
                break;
            }
            nextMatchFrom = static_cast<uint32_t>(AdvanceStringIndex(jsString, nextMatchFrom, isUnicode));
            continue;
        }
        // find match result
        JSHandle<RegExpGlobalResult> matchResultInfo(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
        uint32_t matchStartIndex = static_cast<uint32_t>(matchResultInfo->GetStartOfCaptureIndex(0).GetInt());
        uint32_t matchEndIndex = static_cast<uint32_t>(matchResultInfo->GetEndOfCaptureIndex(0).GetInt());
        if (matchEndIndex == lastMatchEnd && matchEndIndex == nextMatchFrom) {
            // advance index and continue if match result is empty.
            nextMatchFrom = static_cast<uint32_t>(AdvanceStringIndex(jsString, nextMatchFrom, isUnicode));
        } else {
            matchValue.Update(JSTaggedValue(EcmaStringAccessor::FastSubString(thread->GetEcmaVM(),
                string, lastMatchEnd, matchStartIndex - lastMatchEnd)));
            if (arrLen > elements->GetLength()) {
                elements.Update(JSObject::GrowElementsCapacity(thread,
                    JSHandle<JSObject>::Cast(splitArray), elements->GetLength(), true));
            }
            elements->Set(thread, arrLen - 1, matchValue);
            splitArray->SetArrayLength(thread, arrLen);
            if (arrLen == limit) {
                if (useCache) {
                    RegExpExecResultCache::AddResultInCache(thread, cacheTable, regexp, jsString,
                                                            JSHandle<JSTaggedValue>(splitArray),
                                                            RegExpExecResultCache::SPLIT_TYPE, 0, 0, undefined);
                }
                return JSTaggedValue(splitArray.GetTaggedValue().GetTaggedObject());
            }
            arrLen++;
            uint32_t capturesSize = static_cast<uint32_t>(matchResultInfo->GetTotalCaptureCounts().GetInt());
            uint32_t captureIndex = 1;
            while (captureIndex < capturesSize) {
                uint32_t captureStartIndex = static_cast<uint32_t>(
                    matchResultInfo->GetStartOfCaptureIndex(captureIndex).GetInt());
                uint32_t captureEndIndex = static_cast<uint32_t>(
                    matchResultInfo->GetEndOfCaptureIndex(captureIndex).GetInt());
                int32_t subStrLen = static_cast<int32_t>(captureEndIndex - captureStartIndex);
                if (subStrLen < 0) {
                    matchValue.Update(JSTaggedValue::Undefined());
                } else {
                    matchValue.Update(JSTaggedValue(EcmaStringAccessor::FastSubString(thread->GetEcmaVM(),
                        string, captureStartIndex, subStrLen)));
                }
                if (arrLen > elements->GetLength()) {
                    elements.Update(JSObject::GrowElementsCapacity(thread,
                        JSHandle<JSObject>::Cast(splitArray), arrLen, true));
                }
                elements->Set(thread, arrLen - 1, matchValue);
                splitArray->SetArrayLength(thread, arrLen);
                if (arrLen == limit) {
                    if (useCache) {
                        RegExpExecResultCache::AddResultInCache(thread, cacheTable, regexp, jsString,
                                                                JSHandle<JSTaggedValue>(splitArray),
                                                                RegExpExecResultCache::SPLIT_TYPE, 0, 0, undefined);
                    }
                    return JSTaggedValue(splitArray.GetTaggedValue().GetTaggedObject());
                }
                arrLen++;
                captureIndex++;
            }
            lastMatchEnd = matchEndIndex;
            nextMatchFrom = matchEndIndex;
        }
    }
    matchValue.Update(JSTaggedValue(EcmaStringAccessor::FastSubString(thread->GetEcmaVM(),
        JSHandle<EcmaString>::Cast(jsString), lastMatchEnd, size - lastMatchEnd)));
    if (arrLen > elements->GetLength()) {
        elements.Update(JSObject::GrowElementsCapacity(thread, JSHandle<JSObject>::Cast(splitArray), arrLen, true));
    }
    elements->Set(thread, arrLen - 1, matchValue);
    splitArray->SetArrayLength(thread, arrLen);
    if (limit == MAX_SPLIT_LIMIT) {
        RegExpExecResultCache::AddResultInCache(thread, cacheTable, regexp, jsString,
                                                JSHandle<JSTaggedValue>(splitArray), RegExpExecResultCache::SPLIT_TYPE,
                                                0, 0, undefined);
    }
    return JSTaggedValue(splitArray.GetTaggedValue().GetTaggedObject());
}

bool BuiltinsRegExp::RegExpExecInternal(JSThread *thread, const JSHandle<JSTaggedValue> regexp,
                                        JSHandle<EcmaString> inputString, int32_t lastIndex)
{
    size_t stringLength = EcmaStringAccessor(inputString).GetLength();
    bool isUtf16 = EcmaStringAccessor(inputString).IsUtf16();
    FlatStringInfo flatStrInfo = EcmaStringAccessor::FlattenAllString(thread->GetEcmaVM(), inputString);
    if (EcmaStringAccessor(inputString).IsTreeString()) { // use flattenedString as srcString
        inputString = JSHandle<EcmaString>(thread, flatStrInfo.GetString());
    }
    const uint8_t *strBuffer;
    if (isUtf16) {
        strBuffer = reinterpret_cast<const uint8_t *>(flatStrInfo.GetDataUtf16());
    } else {
        strBuffer = flatStrInfo.GetDataUtf8();
    }
    bool isSuccess = false;
    JSTaggedValue regexpSource = JSRegExp::Cast(regexp->GetTaggedObject())->GetOriginalSource();
    uint32_t regexpLength = EcmaStringAccessor(regexpSource).GetLength();
    if (UNLIKELY(regexpLength > MIN_REGEXP_PATTERN_LENGTH_EXECUTE_WITH_OFFHEAP_STRING && stringLength > 0)) {
        size_t utf8Len = LineEcmaString::DataSize(flatStrInfo.GetString());
        ASSERT(utf8Len > 0);
        uint8_t *offHeapString = new uint8_t[utf8Len];
        if (memcpy_s(offHeapString, utf8Len, strBuffer, utf8Len) != EOK) {
            LOG_FULL(FATAL) << "memcpy_s failed";
            UNREACHABLE();
        }
        isSuccess = Matcher(thread, regexp, offHeapString, stringLength, lastIndex, isUtf16,
            StringSource::OFFHEAP_STRING);
        delete[] offHeapString;
    } else {
        isSuccess = Matcher(thread, regexp, strBuffer, stringLength, lastIndex, isUtf16, StringSource::ONHEAP_STRING);
    }
    if (isSuccess) {
        JSHandle<RegExpGlobalResult> globalTable(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
        globalTable->ResetDollar(thread);
        globalTable->SetInputString(thread, inputString.GetTaggedValue());
    }
    return isSuccess;
}

// NOLINTNEXTLINE(readability-non-const-parameter)
bool BuiltinsRegExp::Matcher(JSThread *thread, const JSHandle<JSTaggedValue> regexp,
                             const uint8_t *buffer, size_t length, int32_t lastIndex,
                             bool isUtf16, StringSource source)
{
    BUILTINS_API_TRACE(thread, RegExp, Matcher);
    // get bytecode
    JSTaggedValue bufferData = JSRegExp::Cast(regexp->GetTaggedObject())->GetByteCodeBuffer();
    void *dynBuf = JSNativePointer::Cast(bufferData.GetTaggedObject())->GetExternalPointer();
    auto bytecodeBuffer = reinterpret_cast<uint8_t *>(dynBuf);
    // execute
    RegExpCachedChunk chunk(thread);
    RegExpExecutor executor(&chunk);
    if (lastIndex < 0) {
        lastIndex = 0;
    }
    bool ret = false;
    if (UNLIKELY(source == StringSource::OFFHEAP_STRING)) {
#ifndef NDEBUG
        SharedHeap::GetInstance()->PostGCTaskForTest<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
#endif
        ThreadNativeScope scope(thread);
        ret = executor.Execute(buffer, lastIndex, static_cast<uint32_t>(length), bytecodeBuffer, isUtf16);
    } else {
        ret = executor.Execute(buffer, lastIndex, static_cast<uint32_t>(length), bytecodeBuffer, isUtf16);
    }
    if (ret) {
        executor.GetResult(thread);
    }
    return ret;
}

int64_t BuiltinsRegExp::AdvanceStringIndex(const JSHandle<JSTaggedValue> &inputStr, int64_t index,
                                           bool unicode)
{
    // 1. Assert: Type(S) is String.
    ASSERT(inputStr->IsString());
    // 2. Assert: index is an integer such that 0≤index≤2^53 - 1
    ASSERT(0 <= index && index <= pow(2, 53) - 1);
    // 3. Assert: Type(unicode) is Boolean.
    // 4. If unicode is false, return index+1.
    if (!unicode) {
        return index + 1;
    }
    // 5. Let length be the number of code units in S.
    uint32_t length = EcmaStringAccessor(inputStr->GetTaggedObject()).GetLength();
    // 6. If index+1 ≥ length, return index+1.
    if (index + 1 >= length) {
        return index + 1;
    }
    // 7. Let first be the code unit value at index index in S.
    uint16_t first = EcmaStringAccessor(inputStr->GetTaggedObject()).Get(index);
    // 8. If first < 0xD800 or first > 0xDFFF, return index+1.
    if (first < 0xD800 || first > 0xDFFF) {  // NOLINT(readability-magic-numbers)
        return index + 1;
    }
    // 9. Let second be the code unit value at index index+1 in S.
    uint16_t second = EcmaStringAccessor(inputStr->GetTaggedObject()).Get(index + 1);
    // 10. If second < 0xDC00 or second > 0xDFFF, return index+1.
    if (second < 0xDC00 || second > 0xDFFF) {  // NOLINT(readability-magic-numbers)
        return index + 1;
    }
    // 11. Return index + 2.
    return index + 2;
}

JSTaggedValue BuiltinsRegExp::GetFlagsInternal(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                               const JSHandle<JSTaggedValue> &constructor, const uint8_t mask)
{
    BUILTINS_API_TRACE(thread, RegExp, GetFlagsInternal);
    // 1. Let R be the this value.
    // 2. If Type(R) is not Object, throw a TypeError exception.
    if (!obj->IsECMAObject()) {
        // throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not Object", JSTaggedValue(false));
    }
    // 3. If R does not have an [[OriginalFlags]] internal slot, throw a TypeError exception.
    JSHandle<JSObject> patternObj = JSHandle<JSObject>::Cast(obj);
    if (!patternObj->IsJSRegExp()) {
        // a. If SameValue(R, %RegExp.prototype%) is true, return undefined.
        const GlobalEnvConstants *globalConst = thread->GlobalConstants();
        JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
        JSHandle<JSTaggedValue> objConstructor = JSTaggedValue::GetProperty(thread, obj, constructorKey).GetValue();
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue(false));
        if (objConstructor->IsJSFunction() && constructor->IsJSFunction()) {
            JSHandle<GlobalEnv> objRealm = JSObject::GetFunctionRealm(thread, objConstructor);
            JSHandle<GlobalEnv> ctorRealm = JSObject::GetFunctionRealm(thread, constructor);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (objRealm->GetRegExpPrototype() == obj && *objRealm == *ctorRealm) {
                return JSTaggedValue::Undefined();
            }
        }
        // b. throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(thread, "this does not have [[OriginalFlags]]", JSTaggedValue(false));
    }
    // 4. Let flags be the value of R’s [[OriginalFlags]] internal slot.
    JSHandle<JSRegExp> regexpObj(thread, JSRegExp::Cast(obj->GetTaggedObject()));
    // 5. If flags contains the code unit "[flag]", return true.
    // 6. Return false.
    uint8_t flags = static_cast<uint8_t>(regexpObj->GetOriginalFlags().GetInt());
    return GetTaggedBoolean(flags & mask);
}

// 22.2.7.8
JSHandle<JSTaggedValue> BuiltinsRegExp::MakeMatchIndicesIndexPairArray(JSThread *thread,
    const std::vector<std::pair<JSTaggedValue, JSTaggedValue>>& indices,
    const std::vector<JSHandle<JSTaggedValue>>& groupNames, bool hasGroups)
{
    // 1. Let n be the number of elements in indices.
    uint32_t n = indices.size();
    // Assert: groupNames has n - 1 elements.
    ASSERT(groupNames.size() == n - 1);
    // 5. Let A be ! ArrayCreate(n).
    JSHandle<JSObject> results(JSArray::ArrayCreate(thread, JSTaggedNumber(n)));
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 6. If hasGroups is true, then
    //    a. Let groups be OrdinaryObjectCreate(null).
    // 7. Else,
    //    a. Let groups be undefined.
    JSMutableHandle<JSTaggedValue> groups(thread, JSTaggedValue::Undefined());
    if (hasGroups) {
        JSHandle<JSTaggedValue> nullHandle(thread, JSTaggedValue::Null());
        JSHandle<JSObject> nullObj = factory->OrdinaryNewJSObjectCreate(nullHandle);
        groups.Update(nullObj.GetTaggedValue());
    }
    // 8. Perform ! CreateDataPropertyOrThrow(A, "groups", groups).
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> groupsKey = globalConst->GetHandledGroupsString();
    JSObject::CreateDataProperty(thread, results, groupsKey, groups);
    // 9. For each integer i such that 0 ≤ i < n, in ascending order, do
    //    a. Let matchIndices be indices[i].
    //    b. If matchIndices is not undefined, then
    //        i. Let matchIndexPair be GetMatchIndexPair(S, matchIndices).
    //    c. Else,
    //        i. Let matchIndexPair be undefined.
    //    d. Perform ! CreateDataPropertyOrThrow(A, ! ToString(𝔽(i)), matchIndexPair).
    //    e. If i > 0 and groupNames[i - 1] is not undefined, then
    //        i. Assert: groups is not undefined.
    //        ii. Perform ! CreateDataPropertyOrThrow(groups, groupNames[i - 1], matchIndexPair).
    JSMutableHandle<JSTaggedValue> matchIndexPair(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < n; i++) {
        std::pair<JSTaggedValue, JSTaggedValue> matchIndices = indices[i];
        if (!matchIndices.first.IsUndefined()) {
            JSHandle<TaggedArray> match = factory->NewTaggedArray(2); // 2 means the length of array
            match->Set(thread, 0, matchIndices.first);
            match->Set(thread, 1, matchIndices.second);
            JSHandle<JSTaggedValue> pair(JSArray::CreateArrayFromList(thread, JSHandle<TaggedArray>::Cast(match)));
            matchIndexPair.Update(pair.GetTaggedValue());
        } else {
            matchIndexPair.Update(JSTaggedValue::Undefined());
        }
        JSObject::CreateDataProperty(thread, results, i, matchIndexPair);
        if (i > 0) {
            JSHandle<JSTaggedValue> groupName = groupNames[i - 1];
            if (!groupName->IsUndefined()) {
                JSHandle<JSObject> groupObject = JSHandle<JSObject>::Cast(groups);
                JSObject::CreateDataProperty(thread, groupObject, groupName, matchIndexPair);
            }
        }
    }
    // 10. Return A.
    return JSHandle<JSTaggedValue>::Cast(results);
}

// 21.2.5.2.2
JSTaggedValue BuiltinsRegExp::RegExpBuiltinExec(JSThread *thread, const JSHandle<JSTaggedValue> regexp,
                                                const JSHandle<JSTaggedValue> inputStr,
                                                bool isFastPath, bool useCache, bool isIntermediateResult)
{
    ASSERT(regexp->IsJSRegExp());
    ASSERT(inputStr->IsString());
    BUILTINS_API_TRACE(thread, RegExp, RegExpBuiltinExec);
    uint32_t lastIndex = static_cast<uint32_t>(GetLastIndex(thread, regexp, isFastPath));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    JSHandle<RegExpExecResultCache> cacheTable(thread->GetCurrentEcmaContext()->GetRegExpCache());
    if (useCache) {
        JSTaggedValue cacheResult = cacheTable->FindCachedResult(thread, inputStr,
                                                                 RegExpExecResultCache::EXEC_TYPE, regexp,
                                                                 JSTaggedValue(lastIndex), undefined);
        if (!cacheResult.IsUndefined()) {
            return cacheResult;
        }
    }
    JSTaggedValue result = RegExpBuiltinExecWithoutResult(thread, regexp, inputStr, isFastPath, lastIndex, useCache);
    if (result.IsNull()) {
        return result;
    }
    JSHandle<EcmaString> inputString = JSHandle<EcmaString>::Cast(inputStr);
    JSHandle<RegExpGlobalResult> globalTable(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    uint32_t capturesSize = static_cast<uint32_t>(globalTable->GetTotalCaptureCounts().GetInt());
    JSHandle<JSObject> results(JSArray::ArrayCreate(thread, JSTaggedNumber(capturesSize), ArrayMode::LITERAL));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> indexValue(thread, globalTable->GetStartOfCaptureIndex(0));
    if (isIntermediateResult) {
        // inlined intermediate result
        results->SetPropertyInlinedPropsWithRep(thread, EXEC_RESULT_INDEX_OFFSET, indexValue.GetTaggedValue());
        results->SetPropertyInlinedPropsWithRep(thread, EXEC_RESULT_INPUT_OFFSET, inputStr.GetTaggedValue());
    } else {
        // 24. Perform CreateDataProperty(A, "index", matchIndex).
        JSHandle<JSTaggedValue> indexKey = globalConst->GetHandledIndexString();
        JSObject::CreateDataProperty(thread, results, indexKey, indexValue);
        // 25. Perform CreateDataProperty(A, "input", S).
        JSHandle<JSTaggedValue> inputKey = globalConst->GetHandledInputString();
        JSObject::CreateDataProperty(thread, results, inputKey, inputStr);
    }

    // 27. Perform CreateDataProperty(A, "0", matched_substr).
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> resultElements = factory->NewTaggedArray(capturesSize);
    uint32_t startIndex = static_cast<uint32_t>(globalTable->GetStartOfCaptureIndex(0).GetInt());
    uint32_t len = static_cast<uint32_t>(globalTable->GetEndOfCaptureIndex(0).GetInt()) - startIndex;
    JSHandle<JSTaggedValue> zeroValue(thread, JSTaggedValue(EcmaStringAccessor::FastSubString(
        thread->GetEcmaVM(), inputString, startIndex, len)));
    resultElements->Set(thread, 0, zeroValue);
    // If R contains any GroupName, then
    //   a. Let groups be OrdinaryObjectCreate(null).
    //   b. Let hasGroups be true.
    // Else,
    //   a. Let groups be undefined.
    //   b. Let hasGroups be false.
    JSHandle<JSTaggedValue> groupName(thread, JSHandle<JSRegExp>::Cast(regexp)->GetGroupName());
    JSMutableHandle<JSTaggedValue> groups(thread, JSTaggedValue::Undefined());
    bool hasGroups = false;
    if (!groupName->IsUndefined()) {
        groups.Update(factory->CreateNullJSObject().GetTaggedValue());
        hasGroups = true;
    }
    if (isIntermediateResult) {
        // inlined intermediate result
        results->SetPropertyInlinedPropsWithRep(thread, EXEC_RESULT_GROUPS_OFFSET, groups.GetTaggedValue());
    } else {
        // Perform ! CreateDataPropertyOrThrow(A, "groups", groups).
        JSHandle<JSTaggedValue> groupsKey = globalConst->GetHandledGroupsString();
        JSObject::CreateDataProperty(thread, results, groupsKey, groups);
    }
    // Append match to indices
    uint32_t endIndex = globalTable->GetEndIndex().GetInt();
    std::vector<std::pair<JSTaggedValue, JSTaggedValue>> indices;
    indices.reserve(capturesSize);
    indices.emplace_back(globalTable->GetStartOfCaptureIndex(0), JSTaggedValue(endIndex));
    std::vector<JSHandle<JSTaggedValue>> groupNames;
    groupNames.reserve(capturesSize);
    
    // Create a new RegExp on global
    uint32_t captureIndex = 1;
    JSMutableHandle<JSTaggedValue> iValue(thread, JSTaggedValue::Undefined());
    // 28. For each integer i such that i > 0 and i <= n
    for (; captureIndex < capturesSize; captureIndex++) {
        // a. Let capture_i be ith element of r's captures List
        int32_t captureStartIndex = globalTable->GetStartOfCaptureIndex(captureIndex).GetInt();
        int32_t captureEndIndex = globalTable->GetEndOfCaptureIndex(captureIndex).GetInt();
        int32_t subStrLen = captureEndIndex - captureStartIndex;
        if (subStrLen < 0) {
            iValue.Update(JSTaggedValue::Undefined());
            indices.emplace_back(JSTaggedValue::Undefined(), JSTaggedValue::Undefined());
        } else {
            iValue.Update(JSTaggedValue(EcmaStringAccessor::FastSubString(
                thread->GetEcmaVM(), inputString, captureStartIndex, subStrLen)));
            indices.emplace_back(JSTaggedValue(captureStartIndex), JSTaggedValue(captureEndIndex));
        }
        // add to RegExp.$i and i must <= 9
        if (captureIndex <= REGEXP_GLOBAL_ARRAY_SIZE) {
            globalTable->SetCapture(thread, captureIndex, iValue.GetTaggedValue());
        }

        resultElements->Set(thread, captureIndex, iValue);
        if (!groupName->IsUndefined()) {
            JSHandle<JSObject> groupObject = JSHandle<JSObject>::Cast(groups);
            TaggedArray *groupArray = TaggedArray::Cast(groupName->GetTaggedObject());
            if (groupArray->GetLength() > captureIndex - 1) {
                JSHandle<JSTaggedValue> skey(thread, groupArray->Get(captureIndex - 1));
                JSObject::CreateDataProperty(thread, groupObject, skey, iValue);
                groupNames.emplace_back(skey);
            } else {
                groupNames.emplace_back(undefined);
            }
        } else {
            groupNames.emplace_back(undefined);
        }
    }
    results->SetElements(thread, resultElements);
    // If hasIndices is true, then
    //   a. Let indicesArray be MakeMatchIndicesIndexPairArray(S, indices, groupNames, hasGroups).
    //   b. Perform ! CreateDataPropertyOrThrow(A, "indices", indicesArray).
    bool hasIndices = GetOriginalFlag(thread, regexp, RegExpParser::FLAG_HASINDICES);
    if (hasIndices) {
        auto indicesArray = MakeMatchIndicesIndexPairArray(thread, indices, groupNames, hasGroups);
        JSHandle<JSTaggedValue> indicesKey = globalConst->GetHandledIndicesString();
        JSObject::CreateDataProperty(thread, results, indicesKey, indicesArray);
    }
    JSHandle<JSTaggedValue> emptyString = thread->GlobalConstants()->GetHandledEmptyString();
    while (captureIndex <= REGEXP_GLOBAL_ARRAY_SIZE) {
        globalTable->SetCapture(thread, captureIndex, emptyString.GetTaggedValue());
        ++captureIndex;
    }
    if (useCache) {
        uint32_t newLastIndex = lastIndex;
        bool global = GetOriginalFlag(thread, regexp, RegExpParser::FLAG_GLOBAL);
        bool sticky = GetOriginalFlag(thread, regexp, RegExpParser::FLAG_STICKY);
        if (global || sticky) {
            newLastIndex = static_cast<uint32_t>(globalTable->GetEndIndex().GetInt());
        }
        RegExpExecResultCache::AddResultInCache(thread, cacheTable, regexp, inputStr,
                                                JSHandle<JSTaggedValue>(results), RegExpExecResultCache::EXEC_TYPE,
                                                lastIndex, newLastIndex, undefined);
    }
    // 29. Return A.
    return results.GetTaggedValue();
}

JSTaggedValue BuiltinsRegExp::RegExpBuiltinExecWithoutResult(JSThread *thread, const JSHandle<JSTaggedValue> regexp,
                                                             const JSHandle<JSTaggedValue> inputStr,
                                                             bool isFastPath, uint32_t lastIndex, bool useCache)
{
    // check global and sticky flag to determine whether need to update lastIndex
    bool global = GetOriginalFlag(thread, regexp, RegExpParser::FLAG_GLOBAL);
    bool sticky = GetOriginalFlag(thread, regexp, RegExpParser::FLAG_STICKY);
    bool ifUpdateLastIndex = global || sticky;
    if (ifUpdateLastIndex) {
        uint32_t length = EcmaStringAccessor(inputStr->GetTaggedObject()).GetLength();
        if (lastIndex > length) {
            SetLastIndex(thread, regexp, JSTaggedValue(0), isFastPath);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            return JSTaggedValue::Null();
        }
    } else {
        lastIndex = 0;
    }
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    JSHandle<EcmaString> inputString = JSHandle<EcmaString>::Cast(inputStr);
    bool matchResult = RegExpExecInternal(thread, regexp, inputString, lastIndex);
    if (!matchResult) {
        uint32_t endIndex = lastIndex;
        if (ifUpdateLastIndex) {
            endIndex = 0;
            SetLastIndex(thread, regexp, JSTaggedValue(0), isFastPath);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        if (useCache) {
            JSHandle<RegExpExecResultCache> cacheTable(thread->GetCurrentEcmaContext()->GetRegExpCache());
            RegExpExecResultCache::AddResultInCache(thread, cacheTable, regexp, inputStr,
                                                    JSHandle<JSTaggedValue>(thread, JSTaggedValue::Null()),
                                                    RegExpExecResultCache::EXEC_TYPE,
                                                    lastIndex, endIndex, undefined);
        }
        return JSTaggedValue::Null();
    }
    JSHandle<RegExpGlobalResult> globalTable(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    JSTaggedValue endIndex = globalTable->GetEndIndex();
    if (ifUpdateLastIndex) {
        SetLastIndex(thread, regexp, endIndex, isFastPath);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    return JSTaggedValue::True();
}
// 21.2.5.2.1
JSTaggedValue BuiltinsRegExp::RegExpExec(JSThread *thread, const JSHandle<JSTaggedValue> &regexp,
                                         const JSHandle<JSTaggedValue> &inputString, bool useCache,
                                         bool isIntermediateResult)
{
    BUILTINS_API_TRACE(thread, RegExp, RegExpExec);
    // 1. Assert: Type(R) is Object.
    ASSERT(regexp->IsECMAObject());
    // 2. Assert: Type(S) is String.
    ASSERT(inputString->IsString());
    // 3. Let exec be Get(R, "exec").
    JSHandle<EcmaString> inputStr = JSTaggedValue::ToString(thread, inputString);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> execHandle = globalConst->GetHandledExecString();
    JSTaggedValue execVal = ObjectFastOperator::FastGetPropertyByValue(thread, regexp.GetTaggedValue(),
                                                                       execHandle.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSHandle<JSTaggedValue> exec(thread, execVal);
    // 4. ReturnIfAbrupt(exec).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 5. If IsCallable(exec) is true, then
    if (exec->IsCallable()) {
        JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
        EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, exec, regexp, undefined, 1);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        info->SetCallArg(inputStr.GetTaggedValue());
        JSTaggedValue result = JSFunction::Call(info);
        // b. ReturnIfAbrupt(result).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (!result.IsECMAObject() && !result.IsNull()) {
            // throw a TypeError exception.
            THROW_TYPE_ERROR_AND_RETURN(thread, "exec result is null or is not Object", JSTaggedValue::Exception());
        }
        return result;
    }
    // 6. If R does not have a [[RegExpMatcher]] internal slot, throw a TypeError exception.
    if (!regexp->IsJSRegExp()) {
        // throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(thread, "this does not have a [[RegExpMatcher]]", JSTaggedValue::Exception());
    }
    // 7. Return RegExpBuiltinExec(R, S).
    return RegExpBuiltinExec(thread, regexp, inputString, false, useCache, isIntermediateResult);
}

// 21.2.3.2.1
JSTaggedValue BuiltinsRegExp::RegExpAlloc(JSThread *thread, const JSHandle<JSTaggedValue> &newTarget)
{
    BUILTINS_API_TRACE(thread, RegExp, RegExpAlloc);
    /**
     * 1. Let obj be OrdinaryCreateFromConstructor(newTarget, "%RegExpPrototype%",
     * «[[RegExpMatcher]],[[OriginalSource]], [[OriginalFlags]]»).
     * */
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> func = env->GetRegExpFunction();
    JSHandle<JSTaggedValue> obj(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(func), newTarget));
    // 2. ReturnIfAbrupt(obj).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 5. Return obj.
    return obj.GetTaggedValue();
}

uint32_t BuiltinsRegExp::UpdateExpressionFlags(JSThread *thread, const CString &checkStr)
{
    uint32_t flagsBits = 0;
    uint32_t flagsBitsTemp = 0;
    for (char i : checkStr) {
        switch (i) {
            case 'g':
                flagsBitsTemp = RegExpParser::FLAG_GLOBAL;
                break;
            case 'i':
                flagsBitsTemp = RegExpParser::FLAG_IGNORECASE;
                break;
            case 'm':
                flagsBitsTemp = RegExpParser::FLAG_MULTILINE;
                break;
            case 's':
                flagsBitsTemp = RegExpParser::FLAG_DOTALL;
                break;
            case 'u':
                flagsBitsTemp = RegExpParser::FLAG_UTF16;
                break;
            case 'y':
                flagsBitsTemp = RegExpParser::FLAG_STICKY;
                break;
            case 'd':
                flagsBitsTemp = RegExpParser::FLAG_HASINDICES;
                break;
            default: {
                ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
                JSHandle<JSObject> syntaxError = factory->GetJSError(base::ErrorType::SYNTAX_ERROR,
                    "invalid regular expression flags", StackCheck::NO);
                THROW_NEW_ERROR_AND_RETURN_VALUE(thread, syntaxError.GetTaggedValue(), 0);
            }
        }
        if ((flagsBits & flagsBitsTemp) != 0) {
            ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
            JSHandle<JSObject> syntaxError =
                factory->GetJSError(base::ErrorType::SYNTAX_ERROR, "invalid regular expression flags", StackCheck::NO);
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, syntaxError.GetTaggedValue(), 0);
        }
        flagsBits |= flagsBitsTemp;
    }
    return flagsBits;
}

JSTaggedValue BuiltinsRegExp::FlagsBitsToString(JSThread *thread, uint8_t flags)
{
    ASSERT((flags & 0x80) == 0);  // 0x80: first bit of flags must be 0
    BUILTINS_API_TRACE(thread, RegExp, FlagsBitsToString);
    uint8_t *flagsStr = new uint8_t[RegExpParser::FLAG_NUM + 1];  // FLAG_NUM flags + '\0'
    size_t flagsLen = 0;
    if (flags & RegExpParser::FLAG_HASINDICES) {
        flagsStr[flagsLen] = 'd';
        flagsLen++;
    }
    if (flags & RegExpParser::FLAG_GLOBAL) {
        flagsStr[flagsLen] = 'g';
        flagsLen++;
    }
    if (flags & RegExpParser::FLAG_IGNORECASE) {
        flagsStr[flagsLen] = 'i';
        flagsLen++;
    }
    if (flags & RegExpParser::FLAG_MULTILINE) {
        flagsStr[flagsLen] = 'm';
        flagsLen++;
    }
    if (flags & RegExpParser::FLAG_DOTALL) {
        flagsStr[flagsLen] = 's';
        flagsLen++;
    }
    if (flags & RegExpParser::FLAG_UTF16) {
        flagsStr[flagsLen] = 'u';
        flagsLen++;
    }
    if (flags & RegExpParser::FLAG_STICKY) {
        flagsStr[flagsLen] = 'y';
        flagsLen++;
    }
    flagsStr[flagsLen] = '\0';
    JSHandle<EcmaString> flagsString = thread->GetEcmaVM()->GetFactory()->NewFromUtf8(flagsStr, flagsLen);
    delete[] flagsStr;

    return flagsString.GetTaggedValue();
}

// 21.2.3.2.2
JSTaggedValue BuiltinsRegExp::RegExpInitialize(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                               const JSHandle<JSTaggedValue> &pattern,
                                               const JSHandle<JSTaggedValue> &flags)
{
    BUILTINS_API_TRACE(thread, RegExp, RegExpInitialize);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> patternStrHandle;
    uint8_t flagsBits = 0;
    // 1. If pattern is undefined, let P be the empty String.
    if (pattern->IsUndefined()) {
        patternStrHandle = factory->GetEmptyString();
    } else {
        // 2. Else, let P be ToString(pattern).
        patternStrHandle = JSTaggedValue::ToString(thread, pattern);
        // 3. ReturnIfAbrupt(P).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    // 4. If flags is undefined, let F be the empty String.
    if (flags->IsUndefined()) {
        flagsBits = 0;
    } else if (flags->IsInt()) {
        flagsBits = static_cast<uint8_t>(flags->GetInt());
    } else {
        // 5. Else, let F be ToString(flags).
        JSHandle<EcmaString> flagsStrHandle = JSTaggedValue::ToString(thread, flags);
        // 6. ReturnIfAbrupt(F).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        /**
         * 7. If F contains any code unit other than "d", "g", "i", "m", "u", or "y" or if it contains the same code
         * unit more than once, throw a SyntaxError exception.
         **/
        CString checkStr = ConvertToString(*flagsStrHandle, StringConvertedUsage::LOGICOPERATION);
        flagsBits = static_cast<uint8_t>(UpdateExpressionFlags(thread, checkStr));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    // 9. 10.
    Chunk chunk(thread->GetNativeAreaAllocator());
    RegExpParser parser = RegExpParser(thread, &chunk);
    RegExpParserCache *regExpParserCache = thread->GetCurrentEcmaContext()->GetRegExpParserCache();
    CVector<CString> groupName;
    auto getCache = regExpParserCache->GetCache(*patternStrHandle, flagsBits, groupName);
    if (getCache.first.IsHole()) {
        // String -> CString
        bool cesu8 = !(RegExpParser::FLAG_UTF16 & flagsBits);
        CString patternStdStr = ConvertToString(*patternStrHandle, StringConvertedUsage::LOGICOPERATION, cesu8);
        parser.Init(const_cast<char *>(reinterpret_cast<const char *>(patternStdStr.c_str())), patternStdStr.size(),
                    flagsBits);
        parser.Parse();
        if (parser.IsError()) {
            JSHandle<JSObject> syntaxError =
                factory->GetJSError(base::ErrorType::SYNTAX_ERROR, parser.GetErrorMsg().c_str(), StackCheck::NO);
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, syntaxError.GetTaggedValue(), JSTaggedValue::Exception());
        }
        groupName = parser.GetGroupNames();
    }
    JSHandle<JSRegExp> regexp(thread, JSRegExp::Cast(obj->GetTaggedObject()));
    // 11. Set the value of obj’s [[OriginalSource]] internal slot to P.
    regexp->SetOriginalSource(thread, patternStrHandle.GetTaggedValue());
    // 12. Set the value of obj’s [[OriginalFlags]] internal slot to F.
    regexp->SetOriginalFlags(thread, JSTaggedValue(flagsBits));
    if (!groupName.empty()) {
        JSHandle<TaggedArray> taggedArray = factory->NewTaggedArray(groupName.size());
        for (size_t i = 0; i < groupName.size(); ++i) {
            JSHandle<JSTaggedValue> flagsKey(factory->NewFromStdString(groupName[i].c_str()));
            taggedArray->Set(thread, i, flagsKey);
        }
        regexp->SetGroupName(thread, taggedArray);
    }
    // 13. Set obj’s [[RegExpMatcher]] internal slot.
    if (getCache.first.IsHole()) {
        auto bufferSize = parser.GetOriginBufferSize();
        auto buffer = parser.GetOriginBuffer();
        factory->NewJSRegExpByteCodeData(regexp, buffer, bufferSize);
        regExpParserCache->SetCache(*patternStrHandle, flagsBits, regexp->GetByteCodeBuffer(), bufferSize, groupName);
    } else {
        regexp->SetByteCodeBuffer(thread, getCache.first);
        regexp->SetLength(static_cast<uint32_t>(getCache.second));
    }
    // 14. Let setStatus be Set(obj, "lastIndex", 0, true).
    SetLastIndex(thread, obj, JSTaggedValue(0), true);
    // 16. Return obj.
    return obj.GetTaggedValue();
}

JSTaggedValue BuiltinsRegExp::RegExpCreate(JSThread *thread, const JSHandle<JSTaggedValue> &pattern,
                                           const JSHandle<JSTaggedValue> &flags)
{
    BUILTINS_API_TRACE(thread, RegExp, Create);
    auto ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    JSHandle<JSTaggedValue> newTarget = env->GetRegExpFunction();
    // 1. Let obj be RegExpAlloc(%RegExp%).
    JSHandle<JSTaggedValue> object(thread, RegExpAlloc(thread, newTarget));
    // 2. ReturnIfAbrupt(obj).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 3. Return RegExpInitialize(obj, P, F).
    return RegExpInitialize(thread, object, pattern, flags);
}

// 21.2.3.2.4
EcmaString *BuiltinsRegExp::EscapeRegExpPattern(JSThread *thread, const JSHandle<JSTaggedValue> &src,
                                                const JSHandle<JSTaggedValue> &flags)
{
    BUILTINS_API_TRACE(thread, RegExp, EscapeRegExpPattern);
    // String -> CString
    JSHandle<EcmaString> srcStr(thread, static_cast<EcmaString *>(src->GetTaggedObject()));
    JSHandle<EcmaString> flagsStr(thread, static_cast<EcmaString *>(flags->GetTaggedObject()));
    CString srcStdStr = ConvertToString(*srcStr, StringConvertedUsage::LOGICOPERATION);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // "" -> (?:)
    if (srcStdStr.empty()) {
        srcStdStr = "(?:)";
    }
    bool escapeChar = false;
    for (size_t i = 0; i < srcStdStr.size(); i++) {
        if (srcStdStr[i] == '\\') {
            escapeChar=!escapeChar;
        } else if (!escapeChar && srcStdStr[i]=='/') {
            srcStdStr.insert(i, "\\");
            i++;
        } else {
            escapeChar = false;
        }
    }
    return *factory->NewFromUtf8(srcStdStr);
}

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SET_GET_CAPTURE_IMPL(index)                                                                                   \
    JSTaggedValue BuiltinsRegExp::GetCapture##index(JSThread *thread, [[maybe_unused]] const JSHandle<JSObject> &obj) \
    {                                                                                                                 \
        return RegExpGlobalResult::GetCapture<index>(thread);                                                         \
    }                                                                                                                 \
    bool BuiltinsRegExp::SetCapture##index([[maybe_unused]] JSThread *thread,                                         \
                                           [[maybe_unused]] const JSHandle<JSObject> &obj,                            \
                                           [[maybe_unused]] const JSHandle<JSTaggedValue> &value,                     \
                                           [[maybe_unused]] bool mayThrow)                                            \
    {                                                                                                                 \
        return true;                                                                                                  \
    }

    SET_GET_CAPTURE_IMPL(1)
    SET_GET_CAPTURE_IMPL(2)
    SET_GET_CAPTURE_IMPL(3)
    SET_GET_CAPTURE_IMPL(4)
    SET_GET_CAPTURE_IMPL(5)
    SET_GET_CAPTURE_IMPL(6)
    SET_GET_CAPTURE_IMPL(7)
    SET_GET_CAPTURE_IMPL(8)
    SET_GET_CAPTURE_IMPL(9)
#undef SET_GET_CAPTURE_IMPL

JSTaggedValue RegExpExecResultCache::CreateCacheTable(JSThread *thread)
{
    int length = CACHE_TABLE_HEADER_SIZE + INITIAL_CACHE_NUMBER * ENTRY_SIZE;

    auto table = static_cast<RegExpExecResultCache *>(
        *thread->GetEcmaVM()->GetFactory()->NewTaggedArray(length, JSTaggedValue::Undefined()));
    table->SetLargeStrCount(thread, DEFAULT_LARGE_STRING_COUNT);
    table->SetConflictCount(thread, DEFAULT_CONFLICT_COUNT);
    table->SetStrLenThreshold(thread, 0);
    table->SetHitCount(thread, 0);
    table->SetCacheCount(thread, 0);
    table->SetCacheLength(thread, INITIAL_CACHE_NUMBER);
    return JSTaggedValue(table);
}

JSTaggedValue RegExpExecResultCache::FindCachedResult(JSThread *thread,
                                                      const JSHandle<JSTaggedValue> input, CacheType type,
                                                      const JSHandle<JSTaggedValue> regexp,
                                                      JSTaggedValue lastIndexInput, JSHandle<JSTaggedValue> extend,
                                                      bool isIntermediateResult)
{
    JSHandle<JSRegExp> regexpObj(regexp);
    JSTaggedValue pattern = regexpObj->GetOriginalSource();
    JSTaggedValue flags = regexpObj->GetOriginalFlags();
    JSTaggedValue inputValue = input.GetTaggedValue();
    JSTaggedValue extendValue = extend.GetTaggedValue();
    if (!pattern.IsString() || !flags.IsInt() || !input->IsString() || !lastIndexInput.IsInt()) {
        return JSTaggedValue::Undefined();
    }
    uint32_t hash = pattern.GetKeyHashCode() + static_cast<uint32_t>(flags.GetInt()) +
                    input->GetKeyHashCode() + static_cast<uint32_t>(lastIndexInput.GetInt());
    uint32_t entry = hash & static_cast<uint32_t>(GetCacheLength() - 1);
    if (!Match(entry, pattern, flags, inputValue, lastIndexInput, extendValue, type)) {
        uint32_t entry2 = (entry + 1) & static_cast<uint32_t>(GetCacheLength() - 1);
        if (!Match(entry2, pattern, flags, inputValue, lastIndexInput, extendValue, type)) {
            return JSTaggedValue::Undefined();
        }
        entry = entry2;
    }
    ASSERT((static_cast<size_t>(CACHE_TABLE_HEADER_SIZE) +
        static_cast<size_t>(entry) * static_cast<size_t>(ENTRY_SIZE)) <= static_cast<size_t>(UINT32_MAX));
    uint32_t index = CACHE_TABLE_HEADER_SIZE + entry * ENTRY_SIZE;
    // update cached value if input value is changed
    JSTaggedValue cachedStr = Get(index + INPUT_STRING_INDEX);
    if (!cachedStr.IsUndefined() && cachedStr != inputValue) {
        Set(thread, index + INPUT_STRING_INDEX, inputValue);
    }
    JSTaggedValue result;
    switch (type) {
        case REPLACE_TYPE:
            result = Get(index + RESULT_REPLACE_INDEX);
            break;
        case SPLIT_TYPE:
            result = Get(index + RESULT_SPLIT_INDEX);
            break;
        case MATCH_TYPE:
            result = Get(index + RESULT_MATCH_INDEX);
            break;
        case EXEC_TYPE:
            result = Get(index + RESULT_EXEC_INDEX);
            break;
        case INTERMEDIATE_REPLACE_TYPE:
            result = Get(index + RESULT_INTERMEDIATE_REPLACE_INDEX);
            break;
        case TEST_TYPE:
            result = Get(index + RESULT_TEST_INDEX);
            break;
        case SEARCH_TYPE:
            result = Get(index + RESULT_SEARCH_INDEX);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
            break;
    }
    JSHandle<RegExpGlobalResult> globalTable(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    JSTaggedValue cachedTable = Get(index + CAPTURE_SIZE);
    thread->GetCurrentEcmaContext()->SetRegExpGlobalResult(cachedTable);
    SetHitCount(thread, GetHitCount() + 1);
    if (type != SEARCH_TYPE && type != SPLIT_TYPE) {
        BuiltinsRegExp::SetLastIndex(thread, regexp, Get(index + LAST_INDEX_INDEX), true);
    }
    if (!isIntermediateResult && result.IsJSArray()) {
        JSHandle<JSArray> resultHandle(thread, JSArray::Cast(result));
        JSHandle<JSArray> copyArray = thread->GetEcmaVM()->GetFactory()->CloneArrayLiteral(resultHandle);
        return copyArray.GetTaggedValue();
    }
    return result;
}

void RegExpExecResultCache::AddResultInCache(JSThread *thread, JSHandle<RegExpExecResultCache> cache,
                                             const JSHandle<JSTaggedValue> regexp,
                                             const JSHandle<JSTaggedValue> input,
                                             const JSHandle<JSTaggedValue> resultArray, CacheType type,
                                             uint32_t lastIndexInput, uint32_t lastIndex,
                                             JSHandle<JSTaggedValue> extend, bool isIntermediateResult)
{
    JSHandle<JSRegExp> regexpObj(regexp);
    JSHandle<JSTaggedValue> pattern(thread, regexpObj->GetOriginalSource());
    JSHandle<JSTaggedValue> flags(thread, regexpObj->GetOriginalFlags());
    if (!pattern->IsString() || !flags->IsInt() || !input->IsString()) {
        return;
    }

    JSHandle<JSTaggedValue> resultArrayCopy;
    if (!isIntermediateResult && resultArray->IsJSArray()) {
        JSHandle<JSArray> copyArray = thread->GetEcmaVM()->GetFactory()
                                            ->CloneArrayLiteral(JSHandle<JSArray>(resultArray));
        resultArrayCopy = JSHandle<JSTaggedValue>(copyArray);
    } else {
        resultArrayCopy = JSHandle<JSTaggedValue>(resultArray);
    }
    JSHandle<RegExpGlobalResult> globalTable(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    JSHandle<TaggedArray> taggedArray = JSHandle<TaggedArray>::Cast(globalTable);
    auto factory = thread->GetEcmaVM()->GetFactory();
    uint32_t arrayLength = globalTable->GetLength();
    JSHandle<TaggedArray> resTableArray = factory->NewAndCopyTaggedArray(taggedArray, arrayLength, arrayLength);
    JSTaggedValue patternValue = pattern.GetTaggedValue();
    JSTaggedValue flagsValue = flags.GetTaggedValue();
    JSTaggedValue inputValue = input.GetTaggedValue();
    JSTaggedValue extendValue = extend.GetTaggedValue();
    JSTaggedValue lastIndexInputValue(lastIndexInput);
    JSTaggedValue lastIndexValue(lastIndex);
    JSTaggedValue resTableArrayValue = resTableArray.GetTaggedValue();

    uint32_t hash = patternValue.GetKeyHashCode() + static_cast<uint32_t>(flagsValue.GetInt()) +
                    inputValue.GetKeyHashCode() + lastIndexInput;
    uint32_t entry = hash & static_cast<uint32_t>(cache->GetCacheLength() - 1);
    ASSERT((static_cast<size_t>(CACHE_TABLE_HEADER_SIZE) +
        static_cast<size_t>(entry) * static_cast<size_t>(ENTRY_SIZE)) <= static_cast<size_t>(UINT32_MAX));
    uint32_t index = CACHE_TABLE_HEADER_SIZE + entry * ENTRY_SIZE;
    if (cache->Get(index).IsUndefined()) {
        cache->SetCacheCount(thread, cache->GetCacheCount() + 1);
        cache->SetEntry(thread, entry, patternValue, flagsValue, inputValue,
                        lastIndexInputValue, lastIndexValue, extendValue, resTableArrayValue);
        cache->UpdateResultArray(thread, entry, resultArrayCopy.GetTaggedValue(), type);
    } else if (cache->Match(entry, patternValue, flagsValue, inputValue, lastIndexInputValue, extendValue, type)) {
        cache->UpdateResultArray(thread, entry, resultArrayCopy.GetTaggedValue(), type);
    } else {
        uint32_t entry2 = (entry + 1) & static_cast<uint32_t>(cache->GetCacheLength() - 1);
        ASSERT((static_cast<size_t>(CACHE_TABLE_HEADER_SIZE) +
            static_cast<size_t>(entry2) * static_cast<size_t>(ENTRY_SIZE)) <= static_cast<size_t>(UINT32_MAX));
        uint32_t index2 = CACHE_TABLE_HEADER_SIZE + entry2 * ENTRY_SIZE;
        if (cache->GetCacheLength() < DEFAULT_CACHE_NUMBER) {
            GrowRegexpCache(thread, cache);
            // update value after gc.
            patternValue = pattern.GetTaggedValue();
            flagsValue = flags.GetTaggedValue();
            inputValue = input.GetTaggedValue();

            cache->SetCacheLength(thread, DEFAULT_CACHE_NUMBER);
            entry2 = hash & static_cast<uint32_t>(cache->GetCacheLength() - 1);
            index2 = CACHE_TABLE_HEADER_SIZE + entry2 * ENTRY_SIZE;
        }
        extendValue = extend.GetTaggedValue();
        resTableArrayValue = resTableArray.GetTaggedValue();
        if (cache->Get(index2).IsUndefined()) {
            cache->SetCacheCount(thread, cache->GetCacheCount() + 1);
            cache->SetEntry(thread, entry2, patternValue, flagsValue, inputValue,
                            lastIndexInputValue, lastIndexValue, extendValue, resTableArrayValue);
            cache->UpdateResultArray(thread, entry2, resultArrayCopy.GetTaggedValue(), type);
        } else if (cache->Match(entry2, patternValue, flagsValue, inputValue, lastIndexInputValue, extendValue, type)) {
            cache->UpdateResultArray(thread, entry2, resultArrayCopy.GetTaggedValue(), type);
        } else {
            cache->SetConflictCount(thread, cache->GetConflictCount() > 1 ? (cache->GetConflictCount() - 1) : 0);
            cache->SetCacheCount(thread, cache->GetCacheCount() - 1);
            cache->ClearEntry(thread, entry2);
            cache->SetEntry(thread, entry2, patternValue, flagsValue, inputValue,
                            lastIndexInputValue, lastIndexValue, extendValue, resTableArrayValue);
            cache->UpdateResultArray(thread, entry2, resultArrayCopy.GetTaggedValue(), type);
        }
    }
}

void RegExpExecResultCache::GrowRegexpCache(JSThread *thread, JSHandle<RegExpExecResultCache> cache)
{
    int length = CACHE_TABLE_HEADER_SIZE + DEFAULT_CACHE_NUMBER * ENTRY_SIZE;
    auto factory = thread->GetEcmaVM()->GetFactory();
    auto newCache = factory->ExtendArray(JSHandle<TaggedArray>(cache), length, JSTaggedValue::Undefined());
    thread->GetCurrentEcmaContext()->SetRegExpCache(newCache.GetTaggedValue());
}

void RegExpExecResultCache::SetEntry(JSThread *thread, int entry, JSTaggedValue &pattern, JSTaggedValue &flags,
                                     JSTaggedValue &input, JSTaggedValue &lastIndexInputValue,
                                     JSTaggedValue &lastIndexValue, JSTaggedValue &extendValue,
                                     JSTaggedValue &resTableArray)
{
    ASSERT((static_cast<size_t>(CACHE_TABLE_HEADER_SIZE) +
            static_cast<size_t>(entry) * static_cast<size_t>(ENTRY_SIZE)) <= static_cast<size_t>(INT_MAX));
    int index = CACHE_TABLE_HEADER_SIZE + entry * ENTRY_SIZE;
    Set(thread, index + PATTERN_INDEX, pattern);
    Set(thread, index + FLAG_INDEX, flags);
    Set(thread, index + INPUT_STRING_INDEX, input);
    Set(thread, index + LAST_INDEX_INPUT_INDEX, lastIndexInputValue);
    Set(thread, index + LAST_INDEX_INDEX, lastIndexValue);
    Set(thread, index + EXTEND_INDEX, extendValue);
    Set(thread, index + CAPTURE_SIZE, resTableArray);
}

void RegExpExecResultCache::UpdateResultArray(JSThread *thread, int entry, JSTaggedValue resultArray, CacheType type)
{
    ASSERT((static_cast<size_t>(CACHE_TABLE_HEADER_SIZE) +
            static_cast<size_t>(entry) * static_cast<size_t>(ENTRY_SIZE)) <= static_cast<size_t>(INT_MAX));
    int index = CACHE_TABLE_HEADER_SIZE + entry * ENTRY_SIZE;
    switch (type) {
        case REPLACE_TYPE:
            Set(thread, index + RESULT_REPLACE_INDEX, resultArray);
            break;
        case SPLIT_TYPE:
            Set(thread, index + RESULT_SPLIT_INDEX, resultArray);
            break;
        case MATCH_TYPE:
            Set(thread, index + RESULT_MATCH_INDEX, resultArray);
            break;
        case EXEC_TYPE:
            Set(thread, index + RESULT_EXEC_INDEX, resultArray);
            break;
        case INTERMEDIATE_REPLACE_TYPE:
            Set(thread, index + RESULT_INTERMEDIATE_REPLACE_INDEX, resultArray);
            break;
        case TEST_TYPE:
            Set(thread, index + RESULT_TEST_INDEX, resultArray);
            break;
        case SEARCH_TYPE:
            Set(thread, index + RESULT_SEARCH_INDEX, resultArray);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
            break;
    }
}

void RegExpExecResultCache::ClearEntry(JSThread *thread, int entry)
{
    ASSERT((static_cast<size_t>(CACHE_TABLE_HEADER_SIZE) +
            static_cast<size_t>(entry) * static_cast<size_t>(ENTRY_SIZE)) <= static_cast<size_t>(INT_MAX));
    int index = CACHE_TABLE_HEADER_SIZE + entry * ENTRY_SIZE;
    JSTaggedValue undefined = JSTaggedValue::Undefined();
    for (int i = 0; i < ENTRY_SIZE; i++) {
        Set(thread, index + i, undefined);
    }
}

bool RegExpExecResultCache::Match(int entry, JSTaggedValue &pattern, JSTaggedValue &flags, JSTaggedValue &input,
                                  JSTaggedValue &lastIndexInputValue, JSTaggedValue &extend, CacheType type)
{
    ASSERT((static_cast<size_t>(CACHE_TABLE_HEADER_SIZE) +
            static_cast<size_t>(entry) * static_cast<size_t>(ENTRY_SIZE)) <= static_cast<size_t>(INT_MAX));
    int index = CACHE_TABLE_HEADER_SIZE + entry * ENTRY_SIZE;

    JSTaggedValue typeKey = Get(index + RESULT_REPLACE_INDEX + type);
    if (typeKey.IsUndefined()) {
        return false;
    }

    JSTaggedValue keyPattern = Get(index + PATTERN_INDEX);
    if (keyPattern.IsUndefined()) {
        return false;
    }

    uint8_t flagsBits = static_cast<uint8_t>(flags.GetInt());
    JSTaggedValue keyFlags = Get(index + FLAG_INDEX);
    uint8_t keyFlagsBits = static_cast<uint8_t>(keyFlags.GetInt());
    if (flagsBits != keyFlagsBits) {
        return false;
    }

    uint32_t lastIndexInputInt = static_cast<uint32_t>(lastIndexInputValue.GetInt());
    JSTaggedValue keyLastIndexInput = Get(index + LAST_INDEX_INPUT_INDEX);
    uint32_t keyLastIndexInputInt = static_cast<uint32_t>(keyLastIndexInput.GetInt());
    if (lastIndexInputInt != keyLastIndexInputInt) {
        return false;
    }

    JSTaggedValue keyInput = Get(index + INPUT_STRING_INDEX);
    JSTaggedValue keyExtend = Get(index + EXTEND_INDEX);
    EcmaString *patternStr = EcmaString::Cast(pattern.GetTaggedObject());
    EcmaString *inputStr = EcmaString::Cast(input.GetTaggedObject());
    EcmaString *keyPatternStr = EcmaString::Cast(keyPattern.GetTaggedObject());
    EcmaString *keyInputStr = EcmaString::Cast(keyInput.GetTaggedObject());
    bool extendEqual = false;
    if (extend.IsString() && keyExtend.IsString()) {
        EcmaString *extendStr = EcmaString::Cast(extend.GetTaggedObject());
        EcmaString *keyExtendStr = EcmaString::Cast(keyExtend.GetTaggedObject());
        extendEqual = EcmaStringAccessor::StringsAreEqual(extendStr, keyExtendStr);
    } else if (extend.IsUndefined() && keyExtend.IsUndefined()) {
        extendEqual = true;
    } else {
        return false;
    }
    return extendEqual &&
           EcmaStringAccessor::StringsAreEqual(patternStr, keyPatternStr) &&
           EcmaStringAccessor::StringsAreEqual(inputStr, keyInputStr);
}

JSTaggedValue RegExpGlobalResult::CreateGlobalResultTable(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t initialLength = GLOBAL_TABLE_SIZE + INITIAL_CAPTURE_INDICES;
    auto table = static_cast<RegExpGlobalResult *>(
        *factory->NewTaggedArray(initialLength, JSTaggedValue::Undefined()));
    // initialize dollars with empty string
    JSTaggedValue emptyString = factory->GetEmptyString().GetTaggedValue();
    for (uint32_t i = 1; i <= DOLLAR_NUMBER; i++) {
        table->SetCapture(thread, CAPTURE_START_INDEX + i, emptyString);
    }
    // initialize match info
    table->SetTotalCaptureCounts(thread, JSTaggedValue(0));
    table->SetInputString(thread, emptyString);
    for (uint32_t i = 0; i < INITIAL_CAPTURE_INDICES / 2; i++) { // 2: capture pair
        table->SetStartOfCaptureIndex(thread, i, JSTaggedValue(0));
        table->SetEndOfCaptureIndex(thread, i, JSTaggedValue(0));
    }
    return JSTaggedValue(table);
}

JSHandle<RegExpGlobalResult> RegExpGlobalResult::GrowCapturesCapacity(JSThread *thread,
    JSHandle<RegExpGlobalResult>result, uint32_t length)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> newResult = factory->ExtendArray(
        JSHandle<TaggedArray>(result), length, JSTaggedValue(0));
    thread->GetCurrentEcmaContext()->SetRegExpGlobalResult(newResult.GetTaggedValue());
    return JSHandle<RegExpGlobalResult>(newResult);
}

bool BuiltinsRegExp::GetFlag(JSThread *thread, const JSHandle<JSTaggedValue> regexp, uint32_t flag, bool isFastPath)
{
    if (isFastPath) {
        uint8_t flagsBits = static_cast<uint8_t>(JSHandle<JSRegExp>::Cast(regexp)->GetOriginalFlags().GetInt());
        return (flagsBits & flag) != 0;
    } else {
        JSMutableHandle<JSTaggedValue> flagStr(thread, JSTaggedValue::Undefined());
        switch (flag) {
            case RegExpParser::FLAG_GLOBAL:
                flagStr.Update(thread->GlobalConstants()->GetHandledGlobalString());
                break;
            case RegExpParser::FLAG_UTF16:
                flagStr.Update(thread->GlobalConstants()->GetHandledUnicodeString());
                break;
            case RegExpParser::FLAG_STICKY:
                flagStr.Update(thread->GlobalConstants()->GetHandledStickyString());
                break;
            case RegExpParser::FLAG_MULTILINE:
                flagStr.Update(thread->GlobalConstants()->GetHandledMultilineString());
                break;
            case RegExpParser::FLAG_IGNORECASE:
                flagStr.Update(thread->GlobalConstants()->GetHandledIgnoreCaseString());
                break;
            case RegExpParser::FLAG_HASINDICES:
                flagStr.Update(thread->GlobalConstants()->GetHandledHasIndicesString());
                break;
            case RegExpParser::FLAG_DOTALL:
                UNREACHABLE();
            default:
                break;
        }
        JSTaggedValue globalValue =
            ObjectFastOperator::FastGetPropertyByValue(thread, regexp.GetTaggedValue(), flagStr.GetTaggedValue());
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
        return globalValue.ToBoolean();
    }
}

bool BuiltinsRegExp::GetOriginalFlag(JSThread *thread, const JSHandle<JSTaggedValue> regexp, uint32_t flag)
{
    return GetFlag(thread, regexp, flag, true);
}

void BuiltinsRegExp::SetLastIndex(JSThread *thread, const JSHandle<JSTaggedValue> regexp,
                                  JSTaggedValue lastIndex, bool isFastPath)
{
    if (isFastPath) {
        JSHandle<JSObject>::Cast(regexp)->SetPropertyInlinedPropsWithRep(thread, LAST_INDEX_OFFSET, lastIndex);
        return;
    }
    ObjectFastOperator::FastSetPropertyByValue(thread, regexp.GetTaggedValue(),
        thread->GlobalConstants()->GetHandledLastIndexString().GetTaggedValue(), lastIndex);
}

int64_t BuiltinsRegExp::GetLastIndex(JSThread *thread, const JSHandle<JSTaggedValue> regexp, bool isFastPath)
{
    if (isFastPath) {
        return JSHandle<JSObject>::Cast(regexp)->GetPropertyInlinedProps(LAST_INDEX_OFFSET).GetInt();
    }
    JSHandle<JSTaggedValue> lastIndexHandle(thread, ObjectFastOperator::FastGetPropertyByValue(
        thread, regexp.GetTaggedValue(), thread->GlobalConstants()->GetHandledLastIndexString().GetTaggedValue()));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    JSTaggedNumber thisIndex = JSTaggedValue::ToLength(thread, lastIndexHandle);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    return thisIndex.GetNumber();
}

JSTaggedValue BuiltinsRegExp::GetExecResultIndex(JSThread *thread, const JSHandle<JSTaggedValue> &execResults,
                                                 bool isFastPath)
{
    if (isFastPath) {
        return JSHandle<JSObject>::Cast(execResults)->GetPropertyInlinedProps(EXEC_RESULT_INDEX_OFFSET);
    }
    JSHandle<JSTaggedValue> resultIndex = thread->GlobalConstants()->GetHandledIndexString();
    JSTaggedValue index = ObjectFastOperator::FastGetPropertyByValue(
        thread, execResults.GetTaggedValue(), resultIndex.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return index;
}

JSTaggedValue BuiltinsRegExp::GetExecResultGroups(JSThread *thread, const JSHandle<JSTaggedValue> &execResults,
                                                  bool isFastPath)
{
    if (isFastPath) {
        return JSHandle<JSObject>::Cast(execResults)->GetPropertyInlinedProps(EXEC_RESULT_GROUPS_OFFSET);
    }
    JSHandle<JSTaggedValue> groupKey = thread->GlobalConstants()->GetHandledGroupsString();
    JSTaggedValue groups = ObjectFastOperator::FastGetPropertyByValue(
        thread, execResults.GetTaggedValue(), groupKey.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return groups;
}

JSHandle<EcmaString> BuiltinsRegExp::CreateStringFromResultArray(JSThread *thread,
    const CVector<JSHandle<JSTaggedValue>> &resultArray,
    const std::vector<uint64_t> &resultLengthArray,
    JSHandle<EcmaString> srcString, uint32_t resultStrLength, bool isUtf8)
{
    JSHandle<EcmaString> result = JSHandle<EcmaString>(thread,
        EcmaStringAccessor::CreateLineString(thread->GetEcmaVM(), resultStrLength, isUtf8));
    FlatStringInfo flatStrInfo = EcmaStringAccessor::FlattenAllString(thread->GetEcmaVM(), srcString);
    if (EcmaStringAccessor(srcString).IsTreeString()) { // use flattenedString as srcString
        srcString = JSHandle<EcmaString>(thread, flatStrInfo.GetString());
    }
    FlatStringInfo resultInfo = FlatStringInfo(*result, 0, resultStrLength);
    uint32_t nextPos = 0;
    uint32_t resultArrayLength = resultArray.size();
    for (uint32_t i = 0; i < resultArrayLength; i++) {
        JSTaggedValue substrValue = resultArray[i].GetTaggedValue();
        if (substrValue.IsHole()) {
            continue;
        }
        resultInfo.SetStartIndex(nextPos);
        if (substrValue.IsUndefined()) {
            uint64_t bits = resultLengthArray[i];
            uint32_t subLength = ReplaceLengthField::Decode(bits);
            uint32_t subPosition = ReplacePositionField::Decode(bits);
            if (isUtf8) {
                EcmaStringAccessor::WriteToFlatWithPos<uint8_t>(*srcString, resultInfo.GetDataUtf8Writable(),
                                                                subLength, subPosition);
            } else {
                EcmaStringAccessor::WriteToFlatWithPos<uint16_t>(*srcString, resultInfo.GetDataUtf16Writable(),
                                                                 subLength, subPosition);
            }
            nextPos += subLength;
        } else {
            EcmaString *replacementStr = EcmaString::Cast(substrValue.GetTaggedObject());
            uint32_t replaceLength = static_cast<uint32_t>(resultLengthArray[i]);
            if (isUtf8) {
                EcmaStringAccessor::WriteToFlat(replacementStr, resultInfo.GetDataUtf8Writable(), replaceLength);
            } else {
                EcmaStringAccessor::WriteToFlat(replacementStr, resultInfo.GetDataUtf16Writable(), replaceLength);
            }
            nextPos += replaceLength;
        }
    }
    return result;
}
}  // namespace panda::ecmascript::builtins
