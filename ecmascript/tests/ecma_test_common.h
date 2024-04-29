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
#include "ecmascript/js_api/js_api_arraylist.h"
#include "ecmascript/js_api/js_api_arraylist_iterator.h"
#include "ecmascript/js_api/js_api_deque.h"
#include "ecmascript/js_api/js_api_deque_iterator.h"
#include "ecmascript/js_api/js_api_hashmap.h"
#include "ecmascript/js_api/js_api_hashmap_iterator.h"
#include "ecmascript/js_api/js_api_hashset.h"
#include "ecmascript/js_api/js_api_hashset_iterator.h"
#include "ecmascript/js_api/js_api_lightweightmap.h"
#include "ecmascript/js_api/js_api_lightweightmap_iterator.h"
#include "ecmascript/js_api/js_api_lightweightset.h"
#include "ecmascript/js_api/js_api_lightweightset_iterator.h"
#include "ecmascript/js_api/js_api_linked_list.h"
#include "ecmascript/js_api/js_api_linked_list_iterator.h"
#include "ecmascript/js_api/js_api_list.h"
#include "ecmascript/js_api/js_api_list_iterator.h"
#include "ecmascript/js_api/js_api_plain_array.h"
#include "ecmascript/js_api/js_api_plain_array_iterator.h"
#include "ecmascript/js_api/js_api_queue.h"
#include "ecmascript/js_api/js_api_queue_iterator.h"
#include "ecmascript/js_api/js_api_stack.h"
#include "ecmascript/js_api/js_api_tree_map.h"
#include "ecmascript/js_date_time_format.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_tree.h"
#include "ecmascript/tests/test_common.h"

namespace panda::test {

using namespace panda;
using namespace panda::ecmascript;
using ecmascript::base::BuiltinsBase;

constexpr uint32_t g_defaultSize = 8;

class EcmaTestCommon {
public:
    using CreateStringFunc =
        std::function<EcmaString *(const EcmaVM *vm, const uint8_t *utf8Data, size_t length, bool canBeCompress)>;
    using CreateStringUtf16Func =
        std::function<EcmaString *(const EcmaVM *vm, const uint16_t *utf8Data, size_t length, bool canBeCompress)>;
    using StringAreEqualUtf16Func =
        std::function<bool(const EcmaString *str1, const uint16_t *utf16Data, uint32_t utf16Len)>;
    using StringIsEqualUint8Func =
        std::function<bool(const EcmaString *str1, const uint8_t *dataAddr, uint32_t dataLen, bool canBeCompress)>;
    using Compare =
        std::function<int32_t(const EcmaVM *vm, const JSHandle<EcmaString> &left, const JSHandle<EcmaString> &right)>;
    static void ConcatCommonCase(JSThread *thread, const EcmaVM *instance, std::vector<uint8_t> front,
                                 std::vector<uint8_t> back, CreateStringFunc createStringFunc)
    {
        EcmaString *frontEcmaStr = createStringFunc(instance, front.data(), front.size(), true);
        EcmaString *backEcmaStr = createStringFunc(instance, back.data(), back.size(), true);
        JSHandle<EcmaString> ecmaFront(thread, frontEcmaStr);
        JSHandle<EcmaString> ecmaBack(thread, backEcmaStr);
        JSHandle<EcmaString> ecmStr(thread, EcmaStringAccessor::Concat(instance, ecmaFront, ecmaBack));

        EXPECT_TRUE(EcmaStringAccessor(ecmStr).IsUtf8());
        for (uint32_t i = 0; i < front.size(); i++) {
            EXPECT_EQ(EcmaStringAccessor(ecmStr).Get(i), front[i]);
        }
        for (uint32_t i = 0; i < back.size(); i++) {
            EXPECT_EQ(EcmaStringAccessor(ecmStr).Get(i + front.size()), back[i]);
        }
        EXPECT_EQ(EcmaStringAccessor(ecmStr).GetLength(), front.size() + back.size());
    }

    static void ConcatCommonCase(JSThread *thread, const EcmaVM *instance, std::vector<uint8_t> front,
                                 std::vector<uint16_t> back, CreateStringFunc createStringFunc)
    {
        EcmaString *frontEcmaStr = createStringFunc(instance, front.data(), front.size(), true);
        JSHandle<EcmaString> ecmaFront(thread, frontEcmaStr);
        JSHandle<EcmaString> ecmaBackU16(
            thread, EcmaStringAccessor::CreateFromUtf16(instance, back.data(), back.size(), false));
        JSHandle<EcmaString> ecmaStrWithU16(thread, EcmaStringAccessor::Concat(instance, ecmaFront, ecmaBackU16));

        EXPECT_TRUE(EcmaStringAccessor(ecmaStrWithU16).IsUtf16());
        for (uint32_t i = 0; i < front.size(); i++) {
            EXPECT_EQ(EcmaStringAccessor(ecmaStrWithU16).Get(i), front[i]);
        }
        for (uint32_t i = 0; i < back.size(); i++) {
            EXPECT_EQ(EcmaStringAccessor(ecmaStrWithU16).Get(i + front.size()), back[i]);
        }
        EXPECT_EQ(EcmaStringAccessor(ecmaStrWithU16).GetLength(), front.size() + back.size());
    }

    static void FastSubStringCommonCase(JSThread *thread, const EcmaVM *instance, std::vector<uint8_t> data,
                                        CreateStringFunc createStringFunc)
    {
        EcmaString *frontEcmaStr = createStringFunc(instance, data.data(), data.size(), true);
        JSHandle<EcmaString> handleEcmaStrU8(thread, frontEcmaStr);
        uint32_t indexStartSubU8 = 2;
        uint32_t lengthSubU8 = 2;
        JSHandle<EcmaString> handleEcmaStrSubU8(
            thread, EcmaStringAccessor::FastSubString(instance, handleEcmaStrU8, indexStartSubU8, lengthSubU8));
        for (uint32_t i = 0; i < lengthSubU8; i++) {
            EXPECT_EQ(EcmaStringAccessor(handleEcmaStrSubU8).Get(i),
                      EcmaStringAccessor(handleEcmaStrU8).Get(i + indexStartSubU8));
        }
        EXPECT_EQ(EcmaStringAccessor(handleEcmaStrSubU8).GetLength(), lengthSubU8);
    }

    static void IndexOfCommonCase(JSThread *thread, const EcmaVM *instance, std::vector<uint16_t> dataU16,
                                  std::vector<uint8_t> dataU8, CreateStringFunc createStringFunc)
    {
        JSHandle<EcmaString> ecmaU16(
            thread, EcmaStringAccessor::CreateFromUtf16(instance, dataU16.data(), dataU16.size(), true));

        EcmaString *ecmaStr = createStringFunc(instance, dataU8.data(), dataU8.size(), true);
        JSHandle<EcmaString> ecmaU8(thread, ecmaStr);

        int32_t posStart = 0;
        EXPECT_EQ(EcmaStringAccessor::IndexOf(instance, ecmaU8, ecmaU16, posStart), 3);
        EXPECT_EQ(EcmaStringAccessor::IndexOf(instance, ecmaU16, ecmaU8, posStart), -1);
        posStart = -1;
        EXPECT_EQ(EcmaStringAccessor::IndexOf(instance, ecmaU8, ecmaU16, posStart), 3);
        posStart = 1;
        EXPECT_EQ(EcmaStringAccessor::IndexOf(instance, ecmaU8, ecmaU16, posStart), 3);
        posStart = 3;
        EXPECT_EQ(EcmaStringAccessor::IndexOf(instance, ecmaU8, ecmaU16, posStart), 3);
        posStart = 4;
        EXPECT_EQ(EcmaStringAccessor::IndexOf(instance, ecmaU8, ecmaU16, posStart), -1);
    }

    static void StringsAreEqualUtf16CommonCase(JSThread *thread, const EcmaVM *instance,
                                               StringAreEqualUtf16Func areEqualUtf16, CreateStringFunc createStringFunc)
    {
        // StringsAreEqualUtf16(). EcmaString made by CreateConstantString, Array:U16(1-127).
        uint8_t arrayU8No1[4] = {45, 92, 78};
        uint8_t arrayU8No2[5] = {45, 92, 78, 24};
        uint8_t arrayU8No3[3] = {45, 92};
        uint16_t arrayU16NotCompNo1[] = {45, 92, 78};
        uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
        uint32_t lengthEcmaStrU8No2 = sizeof(arrayU8No2) - 1;
        uint32_t lengthEcmaStrU8No3 = sizeof(arrayU8No3) - 1;
        uint32_t lengthEcmaStrU16NotCompNo1 = sizeof(arrayU16NotCompNo1) / sizeof(arrayU16NotCompNo1[0]);

        EcmaString *ecmaU8No1 = createStringFunc(instance, &arrayU8No1[0], lengthEcmaStrU8No1, true);
        EcmaString *ecmaU8No2 = createStringFunc(instance, &arrayU8No2[0], lengthEcmaStrU8No2, true);
        EcmaString *ecmaU8No3 = createStringFunc(instance, &arrayU8No3[0], lengthEcmaStrU8No3, true);

        JSHandle<EcmaString> handleEcmaStrU8No1(thread, ecmaU8No1);
        JSHandle<EcmaString> handleEcmaStrU8No2(thread, ecmaU8No2);
        JSHandle<EcmaString> handleEcmaStrU8No3(thread, ecmaU8No3);
        EXPECT_TRUE(areEqualUtf16(*handleEcmaStrU8No1, &arrayU16NotCompNo1[0], lengthEcmaStrU16NotCompNo1));
        EXPECT_FALSE(areEqualUtf16(*handleEcmaStrU8No2, &arrayU16NotCompNo1[0], lengthEcmaStrU16NotCompNo1));
        EXPECT_FALSE(areEqualUtf16(*handleEcmaStrU8No3, &arrayU16NotCompNo1[0], lengthEcmaStrU16NotCompNo1));
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
        EXPECT_FALSE(stringIsEqualFunc(*handleEcmaStrU16NotCompNo1, &arrayU8No1[0], lengthEcmaStrU8No1, false));
        EXPECT_TRUE(stringIsEqualFunc(*handleEcmaStrU16NotCompNo1, &arrayU8No1[0], lengthEcmaStrU8No1, true));
        EXPECT_FALSE(stringIsEqualFunc(*handleEcmaStrU16NotCompNo2, &arrayU8No1[0], lengthEcmaStrU8No1, false));
        EXPECT_FALSE(stringIsEqualFunc(*handleEcmaStrU16NotCompNo3, &arrayU8No1[0], lengthEcmaStrU8No1, false));
        EXPECT_FALSE(stringIsEqualFunc(*handleEcmaStrU16NotCompNo4, &arrayU8No1[0], lengthEcmaStrU8No1, false));
    }

    static void TryLowerCommonCase(JSThread *thread, const EcmaVM *instance, JSHandle<EcmaString> &lowerStr,
                                   std::vector<JSHandle<EcmaString>> caseStrings)
    {
        {
            JSHandle<EcmaString> lowerEcmaString(thread, EcmaStringAccessor::TryToLower(instance, lowerStr));
            EXPECT_TRUE(JSTaggedValue::SameValue(lowerStr.GetTaggedValue(), lowerEcmaString.GetTaggedValue()));
            EXPECT_EQ(EcmaStringAccessor(lowerStr).GetLength(), EcmaStringAccessor(lowerEcmaString).GetLength());
            EXPECT_TRUE(EcmaStringAccessor(lowerEcmaString).IsUtf8());
            EXPECT_FALSE(EcmaStringAccessor(lowerEcmaString).IsUtf16());
        }

        for (size_t i = 0; i < caseStrings.size(); i++) {
            JSHandle<EcmaString> lowerEcmaString(thread, EcmaStringAccessor::TryToLower(instance, caseStrings[i]));
            EXPECT_TRUE(JSTaggedValue::SameValue(lowerStr.GetTaggedValue(), lowerEcmaString.GetTaggedValue()));
            EXPECT_EQ(EcmaStringAccessor(lowerStr).GetLength(), EcmaStringAccessor(lowerEcmaString).GetLength());
            EXPECT_TRUE(EcmaStringAccessor(lowerEcmaString).IsUtf8());
            EXPECT_FALSE(EcmaStringAccessor(lowerEcmaString).IsUtf16());
        }
    }

    static void StringsAreEqualUtf16CommonCase(JSThread *thread, const EcmaVM *instance,
                                               StringAreEqualUtf16Func areEqualUtf16)
    {
        // StringsAreEqualUtf16(). EcmaString made by CreateFromUtf16( , , , false), Array:U16(0-65535).
        uint16_t arrayU16NotCompNo1[] = {25645, 25692, 25678};
        uint16_t arrayU16NotCompNo2[] = {25645, 25692, 78};  // 25645 % 256 == 45...
        uint16_t arrayU16NotCompNo3[] = {25645, 25692, 25678, 65535};
        uint16_t arrayU16NotCompNo4[] = {25645, 25692};
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
            EcmaStringAccessor::CreateFromUtf16(instance, &arrayU16NotCompNo4[0], lengthEcmaStrU16NotCompNo4, true));
        EXPECT_TRUE(areEqualUtf16(*handleEcmaStrU16NotCompNo1, &arrayU16NotCompNo1[0], lengthEcmaStrU16NotCompNo1));
        EXPECT_FALSE(areEqualUtf16(*handleEcmaStrU16NotCompNo1, &arrayU16NotCompNo2[0], lengthEcmaStrU16NotCompNo2));
        EXPECT_FALSE(areEqualUtf16(*handleEcmaStrU16NotCompNo2, &arrayU16NotCompNo1[0], lengthEcmaStrU16NotCompNo1));
        EXPECT_FALSE(areEqualUtf16(*handleEcmaStrU16NotCompNo3, &arrayU16NotCompNo1[0], lengthEcmaStrU16NotCompNo1));
        EXPECT_FALSE(areEqualUtf16(*handleEcmaStrU16NotCompNo4, &arrayU16NotCompNo1[0], lengthEcmaStrU16NotCompNo1));
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
        EXPECT_EQ(compare(instance, handleEcmaStrU8No2, handleEcmaStrU16CompNo3), 49 - 45);
        EXPECT_EQ(compare(instance, handleEcmaStrU16CompNo3, handleEcmaStrU8No2), 45 - 49);
    }

    static void GcCommonCase(JSThread *thread, Heap *heap, bool nonMovable = true)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        size_t oldNativeSize = heap->GetNativeBindingSize();
        size_t newNativeSize = heap->GetNativeBindingSize();
        {
            [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
            auto newData = thread->GetEcmaVM()->GetNativeAreaAllocator()->AllocateBuffer(1 * 1024 * 1024);
            [[maybe_unused]] JSHandle<JSNativePointer> obj = factory->NewJSNativePointer(
                newData, NativeAreaAllocator::FreeBufferFunc, nullptr, nonMovable, 1 * 1024 * 1024);
            newNativeSize = heap->GetNativeBindingSize();
            EXPECT_EQ(newNativeSize - oldNativeSize, 1UL * 1024 * 1024);

            auto newData1 = thread->GetEcmaVM()->GetNativeAreaAllocator()->AllocateBuffer(1 * 1024 * 1024);
            [[maybe_unused]] JSHandle<JSNativePointer> obj2 = factory->NewJSNativePointer(
                newData1, NativeAreaAllocator::FreeBufferFunc, nullptr, false, 1 * 1024 * 1024);

            EXPECT_TRUE(newNativeSize - oldNativeSize > 0);
            EXPECT_TRUE(newNativeSize - oldNativeSize <= 2 * 1024 * 1024);
            for (int i = 0; i < 2048; i++) {
                [[maybe_unused]] ecmascript::EcmaHandleScope baseScopeForeach(thread);
                auto newData2 = thread->GetEcmaVM()->GetNativeAreaAllocator()->AllocateBuffer(1 * 1024);
                // malloc size is smaller to avoid test fail in the small devices.
                [[maybe_unused]] JSHandle<JSNativePointer> obj3 = factory->NewJSNativePointer(
                    newData2, NativeAreaAllocator::FreeBufferFunc, nullptr, true, 1 * 1024 * 1024);
            }
            newNativeSize = heap->GetNativeBindingSize();
            // Old GC should be trigger here, so the size should be reduced.
            EXPECT_TRUE(newNativeSize - oldNativeSize < 2048 * 1024 * 1024);
        }
    }

    static size_t GcCommonCase(JSThread *thread)
    {
        {
            [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
            for (int i = 0; i < 100; i++) {
                [[maybe_unused]] JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(
                    10 * 1024, JSTaggedValue::Hole(), MemSpaceType::SEMI_SPACE);
            }
        }
        return thread->GetEcmaVM()->GetHeap()->GetCommittedSize();
    }

    static JSObject *JSObjectTestCreate(JSThread *thread)
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope scope(thread);
        EcmaVM *ecmaVM = thread->GetEcmaVM();
        auto globalEnv = ecmaVM->GetGlobalEnv();
        JSHandle<JSTaggedValue> jsFunc = globalEnv->GetObjectFunction();
        JSHandle<JSObject> newObj =
            ecmaVM->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>(jsFunc), jsFunc);
        return *newObj;
    }

    static JSAPIArrayList *CreateArrayList(JSThread *thread)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto result = TestCommon::CreateContainerTaggedValue(thread, containers::ContainerTag::ArrayList);
        JSHandle<JSTaggedValue> constructor(thread, result);
        JSHandle<JSAPIArrayList> arrayList(
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        JSHandle<TaggedArray> taggedArray = factory->NewTaggedArray(JSAPIArrayList::DEFAULT_CAPACITY_LENGTH);
        arrayList->SetElements(thread, taggedArray);
        return *arrayList;
    }

    static JSAPIPlainArray *CreatePlainArray(JSThread *thread)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto result = TestCommon::CreateContainerTaggedValue(thread, containers::ContainerTag::PlainArray);
        JSHandle<JSTaggedValue> constructor(thread, result);
        JSHandle<JSAPIPlainArray> plainArray(
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        JSHandle<JSTaggedValue> keyArray = JSHandle<JSTaggedValue>(factory->NewTaggedArray(g_defaultSize));
        JSHandle<JSTaggedValue> valueArray = JSHandle<JSTaggedValue>(factory->NewTaggedArray(g_defaultSize));
        plainArray->SetKeys(thread, keyArray);
        plainArray->SetValues(thread, valueArray);
        return *plainArray;
    }

    static JSAPIDeque *CreateJSApiDeque(JSThread *thread)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto result = TestCommon::CreateContainerTaggedValue(thread, containers::ContainerTag::Deque);
        JSHandle<JSTaggedValue> constructor(thread, result);
        JSHandle<JSAPIDeque> deque(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        JSHandle<TaggedArray> newElements = factory->NewTaggedArray(JSAPIDeque::DEFAULT_CAPACITY_LENGTH);
        deque->SetElements(thread, newElements);
        return *deque;
    }

    static JSAPIHashMap *CreateHashMap(JSThread *thread)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto result = TestCommon::CreateContainerTaggedValue(thread, containers::ContainerTag::HashMap);
        JSHandle<JSTaggedValue> constructor(thread, result);
        JSHandle<JSAPIHashMap> map(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        JSTaggedValue hashMapArray = TaggedHashArray::Create(thread);
        map->SetTable(thread, hashMapArray);
        map->SetSize(0);
        return *map;
    }

    static JSAPIHashSet *CreateHashSet(JSThread *thread)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto result = TestCommon::CreateContainerTaggedValue(thread, containers::ContainerTag::HashSet);
        JSHandle<JSTaggedValue> constructor(thread, result);
        JSHandle<JSAPIHashSet> set(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        JSTaggedValue hashSetArray = TaggedHashArray::Create(thread);
        set->SetTable(thread, hashSetArray);
        set->SetSize(0);
        return *set;
    }

    static JSAPILightWeightMap *CreateLightWeightMap(JSThread *thread)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto result = TestCommon::CreateContainerTaggedValue(thread, containers::ContainerTag::LightWeightMap);
        JSHandle<JSTaggedValue> constructor(thread, result);
        JSHandle<JSAPILightWeightMap> lightWeightMap = JSHandle<JSAPILightWeightMap>::Cast(
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        JSHandle<JSTaggedValue> hashArray = JSHandle<JSTaggedValue>(factory->NewTaggedArray(g_defaultSize));
        JSHandle<JSTaggedValue> keyArray = JSHandle<JSTaggedValue>(factory->NewTaggedArray(g_defaultSize));
        JSHandle<JSTaggedValue> valueArray = JSHandle<JSTaggedValue>(factory->NewTaggedArray(g_defaultSize));
        lightWeightMap->SetHashes(thread, hashArray);
        lightWeightMap->SetKeys(thread, keyArray);
        lightWeightMap->SetValues(thread, valueArray);
        lightWeightMap->SetLength(0);
        return *lightWeightMap;
    }

    static JSAPILightWeightSet *CreateLightWeightSet(JSThread *thread)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto result = TestCommon::CreateContainerTaggedValue(thread, containers::ContainerTag::LightWeightSet);
        JSHandle<JSTaggedValue> constructor(thread, result);
        JSHandle<JSAPILightWeightSet> lightweightSet = JSHandle<JSAPILightWeightSet>::Cast(
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        JSHandle<JSTaggedValue> hashArray = JSHandle<JSTaggedValue>(factory->NewTaggedArray(g_defaultSize));
        JSHandle<JSTaggedValue> valueArray = JSHandle<JSTaggedValue>(factory->NewTaggedArray(g_defaultSize));
        lightweightSet->SetHashes(thread, hashArray);
        lightweightSet->SetValues(thread, valueArray);
        lightweightSet->SetLength(0);  // 0 means the value
        return *lightweightSet;
    }

    static JSAPILinkedList *CreateLinkedList(JSThread *thread)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto result = TestCommon::CreateContainerTaggedValue(thread, containers::ContainerTag::LinkedList);

        JSHandle<JSTaggedValue> contianer(thread, result);
        JSHandle<JSAPILinkedList> linkedList = JSHandle<JSAPILinkedList>::Cast(
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(contianer), contianer));
        JSTaggedValue doubleList = TaggedDoubleList::Create(thread);
        linkedList->SetDoubleList(thread, doubleList);
        return *linkedList;
    }

    static JSAPIList *CreateList(JSThread *thread)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto result = TestCommon::CreateContainerTaggedValue(thread, containers::ContainerTag::List);
        JSHandle<JSTaggedValue> constructor(thread, result);
        JSHandle<JSAPIList> list(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        JSTaggedValue singleList = TaggedSingleList::Create(thread);
        list->SetSingleList(thread, singleList);
        return *list;
    }

    static JSHandle<JSAPIQueue> CreateQueue(JSThread *thread, int capacaty)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto result = TestCommon::CreateContainerTaggedValue(thread, containers::ContainerTag::Queue);
        JSHandle<JSTaggedValue> constructor(thread, result);
        JSHandle<JSAPIQueue> jsQueue(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        JSHandle<TaggedArray> newElements = factory->NewTaggedArray(capacaty);
        jsQueue->SetElements(thread, newElements);
        jsQueue->SetLength(thread, JSTaggedValue(0));
        jsQueue->SetFront(0);
        jsQueue->SetTail(0);
        return jsQueue;
    }

    static JSHandle<JSAPIStack> CreateJSApiStack(JSThread *thread)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto result = TestCommon::CreateContainerTaggedValue(thread, containers::ContainerTag::Stack);

        JSHandle<JSTaggedValue> constructor(thread, result);
        JSHandle<JSAPIStack> jsStack(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        jsStack->SetTop(-1);
        return jsStack;
    }

    static JSHandle<JSAPITreeMap> CreateTreeMap(JSThread *thread)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto result = TestCommon::CreateContainerTaggedValue(thread, containers::ContainerTag::TreeMap);

        JSHandle<JSTaggedValue> constructor(thread, result);
        JSHandle<JSAPITreeMap> jsTreeMap(
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        JSTaggedValue internal = TaggedTreeMap::Create(thread);
        jsTreeMap->SetTreeMap(thread, internal);
        return jsTreeMap;
    }

    template <class T>
    static void ListAddHasCommon(JSThread *thread, JSHandle<T> &toor, JSMutableHandle<JSTaggedValue> &value,
                                 std::string myValue, int32_t numbers)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        for (int32_t i = 0; i < numbers; i++) {
            std::string ivalue = myValue + std::to_string(i);
            value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

            JSTaggedValue gValue = toor->Get(i);
            EXPECT_EQ(gValue, value.GetTaggedValue());
        }
        JSTaggedValue gValue = toor->Get(10);
        EXPECT_EQ(gValue, JSTaggedValue::Undefined());

        std::string ivalue = myValue + std::to_string(1);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        EXPECT_TRUE(toor->Has(value.GetTaggedValue()));
    }

    template <class T>
    static JSMutableHandle<JSTaggedValue> ListGetLastCommon(JSThread *thread, JSHandle<T> &toor)
    {
        constexpr uint32_t NODE_NUMBERS = 9;
        JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
        EXPECT_EQ(toor->GetLast(), JSTaggedValue::Undefined());
        EXPECT_EQ(toor->GetFirst(), JSTaggedValue::Undefined());
        for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
            value.Update(JSTaggedValue(i + 1));
            T::Add(thread, toor, value);
        }
        EXPECT_EQ(toor->GetLast().GetInt(), 9);
        EXPECT_EQ(toor->GetFirst().GetInt(), 1);

        value.Update(JSTaggedValue(99));
        int len = toor->Length();
        toor->Insert(thread, toor, value, len);
        return value;
    }

    template <class T>
    static void GetIndexOfAndGetLastIndexOfCommon(JSThread *thread, JSHandle<T> &toor)
    {
        auto value = ListGetLastCommon<T>(thread, toor);
        EXPECT_EQ(toor->GetIndexOf(value.GetTaggedValue()).GetInt(), 9);
        EXPECT_EQ(toor->GetLastIndexOf(value.GetTaggedValue()).GetInt(), 9);
        EXPECT_EQ(toor->Length(), 10);

        value.Update(JSTaggedValue(100));
        toor->Insert(thread, toor, value, 0);
        EXPECT_EQ(toor->GetIndexOf(value.GetTaggedValue()).GetInt(), 0);
        EXPECT_EQ(toor->GetLastIndexOf(value.GetTaggedValue()).GetInt(), 0);
        EXPECT_EQ(toor->Length(), 11);

        value.Update(JSTaggedValue(101));
        toor->Insert(thread, toor, value, 5);
        EXPECT_EQ(toor->GetIndexOf(value.GetTaggedValue()).GetInt(), 5);
        EXPECT_EQ(toor->GetLastIndexOf(value.GetTaggedValue()).GetInt(), 5);
        EXPECT_EQ(toor->Length(), 12);

        toor->Dump();
    }

    template <class T>
    static void InsertAndGetLastCommon(JSThread *thread, JSHandle<T> &toor)
    {
        auto value = ListGetLastCommon<T>(thread, toor);
        EXPECT_EQ(toor->GetLast().GetInt(), 99);
        EXPECT_EQ(toor->Length(), 10);

        value.Update(JSTaggedValue(100));
        toor->Insert(thread, toor, value, 0);
        EXPECT_EQ(toor->GetFirst().GetInt(), 100);
        EXPECT_EQ(toor->Length(), 11);

        toor->Dump();

        value.Update(JSTaggedValue(101));
        toor->Insert(thread, toor, value, 5);
        EXPECT_EQ(toor->Length(), 12);
        toor->Dump();
        EXPECT_EQ(toor->Get(5).GetInt(), 101);
    }

    template <class T>
    static void ListRemoveCommon(JSThread *thread, JSHandle<T> &toor, JSMutableHandle<JSTaggedValue> &value)
    {
        constexpr uint32_t NODE_NUMBERS = 20;
        EXPECT_EQ(toor->GetLast(), JSTaggedValue::Undefined());
        EXPECT_EQ(toor->GetFirst(), JSTaggedValue::Undefined());
        for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
            value.Update(JSTaggedValue(i));
            T::Add(thread, toor, value);
        }
        EXPECT_EQ(toor->Length(), NODE_NUMBERS);
        for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
            value.Update(JSTaggedValue(i));
            JSTaggedValue gValue = toor->Get(i);
            EXPECT_EQ(gValue, value.GetTaggedValue());
        }
    }

    static void SetDateOptionsTest(JSThread *thread, JSHandle<JSObject> &optionsObj,
                                   std::map<std::string, std::string> &dateOptions)
    {
        auto vm = thread->GetEcmaVM();
        auto factory = vm->GetFactory();
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
        auto vm = thread->GetEcmaVM();
        auto factory = vm->GetFactory();
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
        auto vm = thread->GetEcmaVM();
        auto factory = vm->GetFactory();
        auto env = vm->GetGlobalEnv();

        JSHandle<JSTaggedValue> localeCtor = env->GetLocaleFunction();
        JSHandle<JSTaggedValue> dtfCtor = env->GetDateTimeFormatFunction();
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
        auto vm = thread->GetEcmaVM();
        auto factory = vm->GetFactory();
        auto env = vm->GetGlobalEnv();
        auto globalConst = thread->GlobalConstants();

        JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
        JSHandle<JSObject> options = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
        JSHandle<JSTaggedValue> hourCycleKey = globalConst->GetHandledHourCycleString();
        JSHandle<JSTaggedValue> hourCycleValue(factory->NewFromASCII(cycle));
        JSHandle<JSTaggedValue> timeZoneKey = globalConst->GetHandledTimeZoneString();
        JSHandle<JSTaggedValue> timeZoneValue(factory->NewFromASCII(zone));
        JSObject::SetProperty(thread, options, timeZoneKey, timeZoneValue);
        JSObject::SetProperty(thread, options, hourCycleKey, hourCycleValue);
        return options;
    }

    static void ArrayUndefinedGcCommon(JSThread *thread, TriggerGCType type)
    {
        EcmaVM *ecmaVM = thread->GetEcmaVM();
        JSHandle<TaggedArray> array = ecmaVM->GetFactory()->NewTaggedArray(2); // 2: len
        EXPECT_TRUE(*array != nullptr);
        JSHandle<JSObject> newObj1(thread, JSObjectTestCreate(thread));
        array->Set(thread, 0, newObj1.GetTaggedValue());

        JSObject *newObj2 = JSObjectTestCreate(thread);
        JSTaggedValue value(newObj2);
        value.CreateWeakRef();
        array->Set(thread, 1, value);
        EXPECT_EQ(newObj1.GetTaggedValue(), array->Get(0));
        EXPECT_EQ(value, array->Get(1));
        ecmaVM->CollectGarbage(type);
        EXPECT_EQ(newObj1.GetTaggedValue(), array->Get(0));
        EXPECT_EQ(JSTaggedValue::Undefined(), array->Get(1));   // 1 : index
    }

    static void ArrayKeepGcCommon(JSThread *thread, TriggerGCType type)
    {
        EcmaVM *ecmaVM = thread->GetEcmaVM();
        JSHandle<TaggedArray> array = ecmaVM->GetFactory()->NewTaggedArray(2);
        EXPECT_TRUE(*array != nullptr);
        JSHandle<JSObject> newObj1(thread, EcmaTestCommon::JSObjectTestCreate(thread));
        array->Set(thread, 0, newObj1.GetTaggedValue());

        JSHandle<JSObject> newObj2(thread, EcmaTestCommon::JSObjectTestCreate(thread));
        JSTaggedValue value(newObj2.GetTaggedValue());
        value.CreateWeakRef();
        array->Set(thread, 1, value);
        EXPECT_EQ(newObj1.GetTaggedValue(), array->Get(0));
        EXPECT_EQ(value, array->Get(1));
        ecmaVM->CollectGarbage(type);
        EXPECT_EQ(newObj1.GetTaggedValue(), array->Get(0));
        EXPECT_EQ(true, array->Get(1).IsWeak());
        value = newObj2.GetTaggedValue();
        value.CreateWeakRef();
        EXPECT_EQ(value, array->Get(1));
    }
};
};  // namespace panda::test

#endif
