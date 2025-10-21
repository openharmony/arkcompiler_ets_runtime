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
#ifndef ECMA_TEST_COMMON_H
#define ECMA_TEST_COMMON_H
#include <functional>
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_date_time_format.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_tree.h"
#include "ecmascript/tests/ecma_container_common.h"
#include "ecmascript/tests/test_common.h"

namespace panda::test {

using namespace panda;
using namespace panda::ecmascript;
using ecmascript::base::BuiltinsBase;

constexpr uint32_t g_numberFive = 5;
constexpr uint32_t g_numberNine = 9;
constexpr uint32_t g_nintyNine = 99;
constexpr uint32_t g_hundred = 100;
constexpr uint32_t g_hundredAndOne = 101;
static uint8_t g_arrayFrontU8[] = {"abcdef"};
static uint32_t g_lenFrontU8 = sizeof(g_arrayFrontU8) - 1;

class EcmaTestCommon {
public:
    using CreateStringFunc =
        std::function<EcmaString *(const EcmaVM *vm, const uint8_t *utf8Data, size_t length, bool canBeCompress)>;
    using CreateStringUtf16Func =
        std::function<EcmaString *(const EcmaVM *vm, const uint16_t *utf8Data, size_t length, bool canBeCompress)>;
    using StringAreEqualUtf16Func =
        std::function<bool(JSThread*, const EcmaString *str1, const uint16_t *utf16Data, uint32_t utf16Len)>;
    using StringIsEqualUint8Func =
        std::function<bool(JSThread*, const EcmaString *str1, const uint8_t *dataAddr,
                           uint32_t dataLen, bool canBeCompress)>;
    using Compare =
        std::function<int32_t(const EcmaVM *vm, const JSHandle<EcmaString> &left, const JSHandle<EcmaString> &right)>;

    static void ConcatCommonCase2(JSThread *thread, const EcmaVM *instance)
    {
        uint16_t arrayBackU16NotComp[] = {88, 768, 1, 270, 345, 333};
        uint32_t lengthEcmaStrBackU16NotComp = sizeof(arrayBackU16NotComp) / sizeof(arrayBackU16NotComp[0]);

        EcmaString *ecmaFrontU8 = EcmaStringAccessor::CreateFromUtf8(instance, &g_arrayFrontU8[0], g_lenFrontU8, true);
        JSHandle<EcmaString> handleEcmaStrFrontU8(thread, ecmaFrontU8);
        JSHandle<EcmaString> handleEcmaStrBackU16NotComp(
            thread,
            EcmaStringAccessor::CreateFromUtf16(instance, &arrayBackU16NotComp[0], lengthEcmaStrBackU16NotComp, false));
        JSHandle<EcmaString> handleEcmaStrConcatU8U16NotComp(
            thread, EcmaStringAccessor::Concat(instance, handleEcmaStrFrontU8, handleEcmaStrBackU16NotComp));
        EXPECT_TRUE(EcmaStringAccessor(handleEcmaStrConcatU8U16NotComp).IsUtf16());
        for (uint32_t i = 0; i < g_lenFrontU8; i++) {
            EXPECT_EQ(EcmaStringAccessor(handleEcmaStrConcatU8U16NotComp).Get(thread, i), g_arrayFrontU8[i]);
        }
        for (uint32_t i = 0; i < lengthEcmaStrBackU16NotComp; i++) {
            EXPECT_EQ(EcmaStringAccessor(handleEcmaStrConcatU8U16NotComp).Get(thread, i + g_lenFrontU8),
                      arrayBackU16NotComp[i]);
        }
        EXPECT_EQ(EcmaStringAccessor(handleEcmaStrConcatU8U16NotComp).GetLength(),
                  g_lenFrontU8 + lengthEcmaStrBackU16NotComp);
    }

    static void FastSubStringCommonCase(JSThread *thread, const EcmaVM *instance, std::vector<uint8_t> &data,
                                        CreateStringFunc createStringFunc)
    {
        EcmaString *frontEcmaStr = createStringFunc(instance, data.data(), data.size(), true);
        JSHandle<EcmaString> handleEcmaStrU8(thread, frontEcmaStr);
        uint32_t indexStartSubU8 = 2;  // 2: sub index
        uint32_t lengthSubU8 = 2;      // 2: length
        JSHandle<EcmaString> handleEcmaStrSubU8(
            thread, EcmaStringAccessor::FastSubString(instance, handleEcmaStrU8, indexStartSubU8, lengthSubU8));
        for (uint32_t i = 0; i < lengthSubU8; i++) {
            EXPECT_EQ(EcmaStringAccessor(handleEcmaStrSubU8).Get(thread, i),
                      EcmaStringAccessor(handleEcmaStrU8).Get(thread, i + indexStartSubU8));
        }
        EXPECT_EQ(EcmaStringAccessor(handleEcmaStrSubU8).GetLength(), lengthSubU8);
    }

    static void IndexOfCommonCase(JSThread *thread, const EcmaVM *instance, std::vector<uint16_t> &dataU16,
                                  std::vector<uint8_t> &dataU8, CreateStringFunc createStringFunc)
    {
        JSHandle<EcmaString> ecmaU16(
            thread, EcmaStringAccessor::CreateFromUtf16(instance, dataU16.data(), dataU16.size(), true));

        EcmaString *ecmaStr = createStringFunc(instance, dataU8.data(), dataU8.size(), true);
        JSHandle<EcmaString> ecmaU8(thread, ecmaStr);

        int32_t posStart = 0;
        EXPECT_EQ(EcmaStringAccessor::IndexOf(instance, ecmaU8, ecmaU16, posStart), 3);  // 3: value
        EXPECT_EQ(EcmaStringAccessor::IndexOf(instance, ecmaU16, ecmaU8, posStart), -1);
        posStart = -1;
        EXPECT_EQ(EcmaStringAccessor::IndexOf(instance, ecmaU8, ecmaU16, posStart), 3);  // 3: value
        posStart = 1;
        EXPECT_EQ(EcmaStringAccessor::IndexOf(instance, ecmaU8, ecmaU16, posStart), 3);  // 3: value
        posStart = 3;                                                                    // 3: pos start
        EXPECT_EQ(EcmaStringAccessor::IndexOf(instance, ecmaU8, ecmaU16, posStart), 3);  // 3: value
        posStart = 4;                                                                    // 4: pos start
        EXPECT_EQ(EcmaStringAccessor::IndexOf(instance, ecmaU8, ecmaU16, posStart), -1);
    }

    static void StringIsEqualCommonCase(JSThread *thread, const EcmaVM *instance,
                                        StringIsEqualUint8Func stringIsEqualFunc)
    {
        uint8_t arrayU8No1[4] = {45, 92, 78};
        uint16_t arrayU16NotCompNo1[] = {45, 92, 78};
        uint16_t arrayU16NotCompNo2[] = {45, 92, 78, 24};
        uint16_t arrayU16NotCompNo3[] = {45, 92};
        uint16_t arrayU16NotCompNo4[] = {25645, 25692, 25678};
        uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
        uint32_t lengthEcmaStrU16NotCompNo1 = sizeof(arrayU16NotCompNo1) / sizeof(arrayU16NotCompNo1[0]);
        uint32_t lengthEcmaStrU16NotCompNo2 = sizeof(arrayU16NotCompNo2) / sizeof(arrayU16NotCompNo2[0]);
        uint32_t lengthEcmaStrU16NotCompNo3 = sizeof(arrayU16NotCompNo3) / sizeof(arrayU16NotCompNo3[0]);
        uint32_t lengthEcmaStrU16NotCompNo4 = sizeof(arrayU16NotCompNo4) / sizeof(arrayU16NotCompNo4[0]);
        JSHandle<EcmaString> handleEcmaStrU16NotCompNo1(
            thread,
            EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16NotCompNo1[0], lengthEcmaStrU16NotCompNo1, true));
        JSHandle<EcmaString> handleEcmaStrU16NotCompNo2(
            thread,
            EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16NotCompNo2[0], lengthEcmaStrU16NotCompNo2, true));
        JSHandle<EcmaString> handleEcmaStrU16NotCompNo3(
            thread,
            EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16NotCompNo3[0], lengthEcmaStrU16NotCompNo3, true));
        JSHandle<EcmaString> handleEcmaStrU16NotCompNo4(
            thread,
            EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16NotCompNo4[0], lengthEcmaStrU16NotCompNo4, false));
        EXPECT_TRUE(stringIsEqualFunc(thread, *handleEcmaStrU16NotCompNo1, &arrayU8No1[0], lengthEcmaStrU8No1, false));
        EXPECT_TRUE(stringIsEqualFunc(thread, *handleEcmaStrU16NotCompNo1, &arrayU8No1[0], lengthEcmaStrU8No1, true));
        EXPECT_FALSE(stringIsEqualFunc(thread, *handleEcmaStrU16NotCompNo2, &arrayU8No1[0], lengthEcmaStrU8No1, false));
        EXPECT_FALSE(stringIsEqualFunc(thread, *handleEcmaStrU16NotCompNo3, &arrayU8No1[0], lengthEcmaStrU8No1, false));
        EXPECT_FALSE(stringIsEqualFunc(thread, *handleEcmaStrU16NotCompNo4, &arrayU8No1[0], lengthEcmaStrU8No1, false));
    }

    static void TryLowerCommonCase(JSThread *thread, const EcmaVM *instance, JSHandle<EcmaString> &lowerStr,
                                   std::vector<JSHandle<EcmaString>> caseStrings)
    {
        {
            JSHandle<EcmaString> lowerEcmaString(thread, EcmaStringAccessor::TryToLower(instance, lowerStr));
            EXPECT_TRUE(JSTaggedValue::SameValue(thread, lowerStr.GetTaggedValue(), lowerEcmaString.GetTaggedValue()));
            EXPECT_EQ(EcmaStringAccessor(lowerStr).GetLength(), EcmaStringAccessor(lowerEcmaString).GetLength());
            EXPECT_TRUE(EcmaStringAccessor(lowerEcmaString).IsUtf8());
            EXPECT_FALSE(EcmaStringAccessor(lowerEcmaString).IsUtf16());
        }

        for (size_t i = 0; i < caseStrings.size(); i++) {
            JSHandle<EcmaString> lowerEcmaString(thread, EcmaStringAccessor::TryToLower(instance, caseStrings[i]));
            EXPECT_TRUE(JSTaggedValue::SameValue(thread, lowerStr.GetTaggedValue(), lowerEcmaString.GetTaggedValue()));
            EXPECT_EQ(EcmaStringAccessor(lowerStr).GetLength(), EcmaStringAccessor(lowerEcmaString).GetLength());
            EXPECT_TRUE(EcmaStringAccessor(lowerEcmaString).IsUtf8());
            EXPECT_FALSE(EcmaStringAccessor(lowerEcmaString).IsUtf16());
        }
    }

    static void CompareCommonCase(JSThread *thread, const EcmaVM *instance, CreateStringFunc createUtf8,
                                  CreateStringUtf16Func createUtf16, Compare compare)
    {
        // Compare(). EcmaString made by CreateFromUtf8() and EcmaString made by CreateFromUtf16( , , , true).
        uint8_t arrayU8No1[3] = {1, 23};
        uint8_t arrayU8No2[4] = {1, 23, 49};
        uint16_t arrayU16CompNo1[] = {1, 23};
        uint16_t arrayU16CompNo2[] = {1, 23, 49};
        uint16_t arrayU16CompNo3[] = {1, 23, 45, 97, 127};
        uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
        uint32_t lengthEcmaStrU8No2 = sizeof(arrayU8No2) - 1;
        uint32_t lengthEcmaStrU16CompNo1 = sizeof(arrayU16CompNo1) / sizeof(arrayU16CompNo1[0]);
        uint32_t lengthEcmaStrU16CompNo2 = sizeof(arrayU16CompNo2) / sizeof(arrayU16CompNo2[0]);
        uint32_t lengthEcmaStrU16CompNo3 = sizeof(arrayU16CompNo3) / sizeof(arrayU16CompNo3[0]);
        JSHandle<EcmaString> handleEcmaStrU8No1(thread, createUtf8(instance, &arrayU8No1[0], lengthEcmaStrU8No1, true));
        JSHandle<EcmaString> handleEcmaStrU8No2(thread, createUtf8(instance, &arrayU8No2[0], lengthEcmaStrU8No2, true));
        JSHandle<EcmaString> handleEcmaStrU16CompNo1(
            thread, createUtf16(instance, &arrayU16CompNo1[0], lengthEcmaStrU16CompNo1, true));
        JSHandle<EcmaString> handleEcmaStrU16CompNo2(
            thread, createUtf16(instance, &arrayU16CompNo2[0], lengthEcmaStrU16CompNo2, true));
        JSHandle<EcmaString> handleEcmaStrU16CompNo3(
            thread, createUtf16(instance, &arrayU16CompNo3[0], lengthEcmaStrU16CompNo3, true));
        EXPECT_EQ(compare(instance, handleEcmaStrU8No1, handleEcmaStrU16CompNo1), 0);
        EXPECT_EQ(compare(instance, handleEcmaStrU16CompNo1, handleEcmaStrU8No1), 0);
        EXPECT_EQ(compare(instance, handleEcmaStrU8No1, handleEcmaStrU16CompNo2), -1);
        EXPECT_EQ(compare(instance, handleEcmaStrU16CompNo2, handleEcmaStrU8No1), 1);
        EXPECT_EQ(compare(instance, handleEcmaStrU8No2, handleEcmaStrU16CompNo3), 49 - 45);  // 49: value, 45: value
        EXPECT_EQ(compare(instance, handleEcmaStrU16CompNo3, handleEcmaStrU8No2), 45 - 49);  // 49: value, 45: value
    }

    static void GcCommonCase(JSThread *thread, Heap *heap, bool nonMovable = true)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        size_t oldNativeSize = heap->GetNativeBindingSize();
        size_t newNativeSize = heap->GetNativeBindingSize();
        constexpr int32_t nums = 1024;
        {
            [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
            auto newData = thread->GetEcmaVM()->GetNativeAreaAllocator()->AllocateBuffer(1 * nums * nums);
            [[maybe_unused]] JSHandle<JSNativePointer> obj = factory->NewJSNativePointer(
                newData, NativeAreaAllocator::FreeBufferFunc, nullptr, nonMovable, 1 * nums * nums);
            newNativeSize = heap->GetNativeBindingSize();
            EXPECT_EQ(newNativeSize - oldNativeSize, 1UL * nums * nums);
            auto newData1 = thread->GetEcmaVM()->GetNativeAreaAllocator()->AllocateBuffer(1 * nums * nums);
            [[maybe_unused]] JSHandle<JSNativePointer> obj2 = factory->NewJSNativePointer(
                newData1, NativeAreaAllocator::FreeBufferFunc, nullptr, false, 1 * nums * nums);

            EXPECT_TRUE(newNativeSize - oldNativeSize > 0);
            EXPECT_TRUE(newNativeSize - oldNativeSize <= 2 * nums * nums);  // 2: value
            for (int i = 0; i < 2048; i++) {                                // 2048: loop count
                [[maybe_unused]] ecmascript::EcmaHandleScope baseScopeForeach(thread);
                auto newData2 = thread->GetEcmaVM()->GetNativeAreaAllocator()->AllocateBuffer(1 * nums);
                // malloc size is smaller to avoid test fail in the small devices.
                [[maybe_unused]] JSHandle<JSNativePointer> obj3 = factory->NewJSNativePointer(
                    newData2, NativeAreaAllocator::FreeBufferFunc, nullptr, true, 1 * nums * nums);
            }
            newNativeSize = heap->GetNativeBindingSize();
            // Old GC should be trigger here, so the size should be reduced.
            EXPECT_TRUE(newNativeSize - oldNativeSize < 2048 * nums * nums);  // 2048: value
        }
    }

    static size_t GcCommonCase(JSThread *thread)
    {
        {
            [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
            for (int i = 0; i < g_hundred; i++) {  // g_hundred: run count
                [[maybe_unused]] JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(
                    10 * 1024, JSTaggedValue::Hole(), MemSpaceType::SEMI_SPACE);  // 10: value, 1024: value
            }
        }
        return thread->GetEcmaVM()->GetHeap()->GetCommittedSize();
    }

    template <class T>
    static void ListAddHasCommon(JSThread *thread, JSHandle<T> &toor, JSMutableHandle<JSTaggedValue> &value,
                                 std::string myValue, int32_t numbers)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        for (int32_t i = 0; i < numbers; i++) {
            std::string ivalue = myValue + std::to_string(i);
            value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

            JSTaggedValue gValue = toor->Get(thread, i);
            EXPECT_EQ(gValue, value.GetTaggedValue());
        }
        JSTaggedValue gValue = toor->Get(thread, 10);  // 10: index
        EXPECT_EQ(gValue, JSTaggedValue::Undefined());

        std::string ivalue = myValue + std::to_string(1);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        EXPECT_TRUE(toor->Has(thread, value.GetTaggedValue()));
    }

    template <class T>
    static JSMutableHandle<JSTaggedValue> ListGetLastCommon(JSThread *thread, JSHandle<T> &toor)
    {
        constexpr uint32_t NODE_NUMBERS = 9;
        JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
        EXPECT_EQ(toor->GetLast(thread), JSTaggedValue::Undefined());
        EXPECT_EQ(toor->GetFirst(thread), JSTaggedValue::Undefined());
        for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
            value.Update(JSTaggedValue(i + 1));
            T::Add(thread, toor, value);
        }
        EXPECT_EQ(toor->GetLast(thread).GetInt(), g_numberNine);
        EXPECT_EQ(toor->GetFirst(thread).GetInt(), 1);

        value.Update(JSTaggedValue(g_nintyNine));
        int len = toor->Length(thread);
        toor->Insert(thread, toor, value, len);
        return value;
    }

    template <class T>
    static void GetIndexOfAndGetLastIndexOfCommon(JSThread *thread, JSHandle<T> &toor)
    {
        auto value = ListGetLastCommon<T>(thread, toor);
        EXPECT_EQ(toor->GetIndexOf(thread, value.GetTaggedValue()).GetInt(), g_numberNine);
        EXPECT_EQ(toor->GetLastIndexOf(thread, value.GetTaggedValue()).GetInt(), g_numberNine);
        EXPECT_EQ(toor->Length(thread), 10);  // 10: len

        value.Update(JSTaggedValue(g_hundred));
        toor->Insert(thread, toor, value, 0);
        EXPECT_EQ(toor->GetIndexOf(thread, value.GetTaggedValue()).GetInt(), 0);
        EXPECT_EQ(toor->GetLastIndexOf(thread, value.GetTaggedValue()).GetInt(), 0);
        EXPECT_EQ(toor->Length(thread), 11);  // 11: len

        value.Update(JSTaggedValue(g_hundredAndOne));
        toor->Insert(thread, toor, value, g_numberFive);
        EXPECT_EQ(toor->GetIndexOf(thread, value.GetTaggedValue()).GetInt(), g_numberFive);
        EXPECT_EQ(toor->GetLastIndexOf(thread, value.GetTaggedValue()).GetInt(), g_numberFive);
        EXPECT_EQ(toor->Length(thread), 12);  // 12: len

        toor->Dump(thread);
    }

    template <class T>
    static void InsertAndGetLastCommon(JSThread *thread, JSHandle<T> &toor)
    {
        auto value = ListGetLastCommon<T>(thread, toor);
        EXPECT_EQ(toor->GetLast(thread).GetInt(), g_nintyNine);
        EXPECT_EQ(toor->Length(thread), 10);  // 10: len

        value.Update(JSTaggedValue(g_hundred));
        toor->Insert(thread, toor, value, 0);
        EXPECT_EQ(toor->GetFirst(thread).GetInt(), g_hundred);
        EXPECT_EQ(toor->Length(thread), 11);  // 11: len

        toor->Dump(thread);

        value.Update(JSTaggedValue(g_hundredAndOne));
        toor->Insert(thread, toor, value, g_numberFive);
        EXPECT_EQ(toor->Length(thread), 12);  // 12: len
        toor->Dump(thread);
        EXPECT_EQ(toor->Get(thread, g_numberFive).GetInt(), g_hundredAndOne);
    }

    template <class T>
    static void ListRemoveCommon(JSThread *thread, JSHandle<T> &toor, JSMutableHandle<JSTaggedValue> &value)
    {
        constexpr uint32_t NODE_NUMBERS = 20;
        EXPECT_EQ(toor->GetLast(thread), JSTaggedValue::Undefined());
        EXPECT_EQ(toor->GetFirst(thread), JSTaggedValue::Undefined());
        for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
            value.Update(JSTaggedValue(i));
            T::Add(thread, toor, value);
        }
        EXPECT_EQ(toor->Length(thread), NODE_NUMBERS);
        for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
            value.Update(JSTaggedValue(i));
            JSTaggedValue gValue = toor->Get(thread, i);
            EXPECT_EQ(gValue, value.GetTaggedValue());
        }
    }

    static void SetDateOptionsTest(JSThread *thread, JSHandle<JSObject> &optionsObj,
                                   std::map<std::string, std::string> &dateOptions)
    {
        auto factory = thread->GetEcmaVM()->GetFactory();
        auto globalConst = thread->GlobalConstants();
        // Date options keys.
        JSHandle<JSTaggedValue> weekdayKey = globalConst->GetHandledWeekdayString();
        JSHandle<JSTaggedValue> yearKey = globalConst->GetHandledYearString();
        JSHandle<JSTaggedValue> monthKey = globalConst->GetHandledMonthString();
        JSHandle<JSTaggedValue> dayKey = globalConst->GetHandledDayString();
        // Date options values.
        JSHandle<JSTaggedValue> weekdayValue(factory->NewFromASCII(dateOptions["weekday"].c_str()));
        JSHandle<JSTaggedValue> yearValue(factory->NewFromASCII(dateOptions["year"].c_str()));
        JSHandle<JSTaggedValue> monthValue(factory->NewFromASCII(dateOptions["month"].c_str()));
        JSHandle<JSTaggedValue> dayValue(factory->NewFromASCII(dateOptions["day"].c_str()));
        // Set date options.
        JSObject::SetProperty(thread, optionsObj, weekdayKey, weekdayValue);
        JSObject::SetProperty(thread, optionsObj, yearKey, yearValue);
        JSObject::SetProperty(thread, optionsObj, monthKey, monthValue);
        JSObject::SetProperty(thread, optionsObj, dayKey, dayValue);
    }

    static void SetTimeOptionsTest(JSThread *thread, JSHandle<JSObject> &optionsObj,
                                   std::map<std::string, std::string> &timeOptionsMap)
    {
        auto factory = thread->GetEcmaVM()->GetFactory();
        auto globalConst = thread->GlobalConstants();
        // Time options keys.
        JSHandle<JSTaggedValue> dayPeriodKey = globalConst->GetHandledDayPeriodString();
        JSHandle<JSTaggedValue> hourKey = globalConst->GetHandledHourString();
        JSHandle<JSTaggedValue> minuteKey = globalConst->GetHandledMinuteString();
        JSHandle<JSTaggedValue> secondKey = globalConst->GetHandledSecondString();
        JSHandle<JSTaggedValue> fractionalSecondDigitsKey = globalConst->GetHandledFractionalSecondDigitsString();
        // Time options values.
        JSHandle<JSTaggedValue> dayPeriodValue(factory->NewFromASCII(timeOptionsMap["dayPeriod"].c_str()));
        JSHandle<JSTaggedValue> hourValue(factory->NewFromASCII(timeOptionsMap["hour"].c_str()));
        JSHandle<JSTaggedValue> minuteValue(factory->NewFromASCII(timeOptionsMap["minute"].c_str()));
        JSHandle<JSTaggedValue> secondValue(factory->NewFromASCII(timeOptionsMap["second"].c_str()));
        JSHandle<JSTaggedValue> fractionalSecondDigitsValue(
            factory->NewFromASCII(timeOptionsMap["fractionalSecond"].c_str()));
        // Set time options.
        JSObject::SetProperty(thread, optionsObj, dayPeriodKey, dayPeriodValue);
        JSObject::SetProperty(thread, optionsObj, hourKey, hourValue);
        JSObject::SetProperty(thread, optionsObj, minuteKey, minuteValue);
        JSObject::SetProperty(thread, optionsObj, secondKey, secondValue);
        JSObject::SetProperty(thread, optionsObj, fractionalSecondDigitsKey, fractionalSecondDigitsValue);
    }

    static JSHandle<JSDateTimeFormat> CreateDateTimeFormatTest(JSThread *thread, icu::Locale icuLocale,
                                                               JSHandle<JSObject> options)
    {
        auto factory = thread->GetEcmaVM()->GetFactory();

        JSHandle<JSTaggedValue> localeCtor = thread->GetEcmaVM()->GetGlobalEnv()->GetLocaleFunction();
        JSHandle<JSTaggedValue> dtfCtor = thread->GetEcmaVM()->GetGlobalEnv()->GetDateTimeFormatFunction();
        JSHandle<JSLocale> locales =
            JSHandle<JSLocale>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(localeCtor), localeCtor));
        JSHandle<JSDateTimeFormat> dtf =
            JSHandle<JSDateTimeFormat>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dtfCtor), dtfCtor));

        JSHandle<JSTaggedValue> optionsVal = JSHandle<JSTaggedValue>::Cast(options);
        factory->NewJSIntlIcuData(locales, icuLocale, JSLocale::FreeIcuLocale);
        dtf =
            JSDateTimeFormat::InitializeDateTimeFormat(thread, dtf, JSHandle<JSTaggedValue>::Cast(locales), optionsVal);
        return dtf;
    }

    static JSHandle<JSObject> SetHourCycleKeyValue(JSThread *thread, std::string &cycle, std::string &zone)
    {
        auto factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<JSTaggedValue> objFun = thread->GetEcmaVM()->GetGlobalEnv()->GetObjectFunction();
        JSHandle<JSObject> options = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
        JSHandle<JSTaggedValue> hourCycleKey = thread->GlobalConstants()->GetHandledHourCycleString();
        JSHandle<JSTaggedValue> hourCycleValue(factory->NewFromASCII(cycle));
        JSHandle<JSTaggedValue> timeZoneKey = thread->GlobalConstants()->GetHandledTimeZoneString();
        JSHandle<JSTaggedValue> timeZoneValue(factory->NewFromASCII(zone));
        JSObject::SetProperty(thread, options, timeZoneKey, timeZoneValue);
        JSObject::SetProperty(thread, options, hourCycleKey, hourCycleValue);
        return options;
    }

    static void ArrayUndefinedGcCommon(JSThread *thread, TriggerGCType type)
    {
        EcmaVM *ecmaVM = thread->GetEcmaVM();
        JSHandle<TaggedArray> array = ecmaVM->GetFactory()->NewTaggedArray(2);  // 2: len
        EXPECT_TRUE(*array != nullptr);
        JSHandle<JSObject> newObj1(thread, EcmaContainerCommon::JSObjectTestCreate(thread));
        array->Set(thread, 0, newObj1.GetTaggedValue());

        JSObject *newObj2 = EcmaContainerCommon::JSObjectTestCreate(thread);
        JSTaggedValue value(newObj2);
        value.CreateWeakRef();
        array->Set(thread, 1, value);
        EXPECT_EQ(newObj1.GetTaggedValue(), array->Get(thread, 0));
        EXPECT_EQ(value, array->Get(thread, 1));
        ecmaVM->CollectGarbage(type);
        EXPECT_EQ(newObj1.GetTaggedValue(), array->Get(thread, 0));
        EXPECT_EQ(JSTaggedValue::Undefined(), array->Get(thread, 1));  // 1 : index
    }

    static void ArrayKeepGcCommon(JSThread *thread, TriggerGCType type)
    {
        EcmaVM *ecmaVM = thread->GetEcmaVM();
        JSHandle<TaggedArray> array = ecmaVM->GetFactory()->NewTaggedArray(2);  // 2: len
        EXPECT_TRUE(*array != nullptr);
        JSHandle<JSObject> newObj1(thread, EcmaContainerCommon::JSObjectTestCreate(thread));
        array->Set(thread, 0, newObj1.GetTaggedValue());

        JSHandle<JSObject> newObj2(thread, EcmaContainerCommon::JSObjectTestCreate(thread));
        JSTaggedValue value(newObj2.GetTaggedValue());
        value.CreateWeakRef();
        array->Set(thread, 1, value);
        EXPECT_EQ(newObj1.GetTaggedValue(), array->Get(thread, 0));
        EXPECT_EQ(value, array->Get(thread, 1));
        ecmaVM->CollectGarbage(type);
        EXPECT_EQ(newObj1.GetTaggedValue(), array->Get(thread, 0));
        EXPECT_EQ(true, array->Get(thread, 1).IsWeak());
        value = newObj2.GetTaggedValue();
        value.CreateWeakRef();
        EXPECT_EQ(value, array->Get(thread, 1));
    }
};
};  // namespace panda::test

#endif
