/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/string/hashtriemap.h"
#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/ecma_string_table_optimization-inl.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class EcmaStringTableTest : public BaseTestWithScope<false> {
};

/**
 * @tc.name: GetOrInternFlattenString_EmptyString
 * @tc.desc: Write empty string emptyStr to the Intern pool and takes the hash code as its index.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableTest, GetOrInternFlattenString_EmptyString)
{
    EcmaStringTable *table = thread->GetEcmaVM()->GetEcmaStringTable();

    JSHandle<EcmaString> emptyEcmaStrHandle(thread, EcmaStringAccessor::CreateEmptyString(thread->GetEcmaVM()));
    EXPECT_TRUE(!EcmaStringAccessor(emptyEcmaStrHandle).IsInternString());

    table->GetOrInternFlattenString(thread->GetEcmaVM(), *emptyEcmaStrHandle);
    EXPECT_TRUE(!EcmaStringAccessor(emptyEcmaStrHandle).IsInternString());
#if ENABLE_NEXT_OPTIMIZATION
    EcmaString *emptyEcmaStr = table->TryGetInternString(thread, emptyEcmaStrHandle);
    EXPECT_STREQ(EcmaStringAccessor(emptyEcmaStr).ToCString(thread).c_str(), "");
    EXPECT_TRUE(EcmaStringAccessor(emptyEcmaStr).IsInternString());
#endif
}

/**
 * @tc.name: GetOrInternString
 * @tc.desc: Obtain EcmaString string from utf8 encoded data. If the string already exists in the detention pool,
             it will be returned directly. If not, it will be added to the detention pool and then returned.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableTest, GetOrInternString_utf8Data)
{
    EcmaVM *vm = thread->GetEcmaVM();
    EcmaStringTable *table = thread->GetEcmaVM()->GetEcmaStringTable();

    uint8_t utf8Data[] = {0x68, 0x65, 0x6c, 0x6c, 0x6f}; // " hello "
    EcmaString *ecmaStrCreatePtr = EcmaStringAccessor::CreateFromUtf8(vm, utf8Data, sizeof(utf8Data), true);
    EXPECT_TRUE(!EcmaStringAccessor(ecmaStrCreatePtr).IsInternString());

    EcmaString *ecmaStrGetPtr = table->GetOrInternString(vm, utf8Data, sizeof(utf8Data), true);
    EXPECT_STREQ(EcmaStringAccessor(ecmaStrGetPtr).ToCString(thread).c_str(), "hello");
    EXPECT_TRUE(EcmaStringAccessor(ecmaStrGetPtr).IsInternString());
}

/**
 * @tc.name: GetOrInternString
 * @tc.desc: Obtain EcmaString string from utf16 encoded data. If the string already exists in the detention pool,
             it will be returned directly. If not, it will be added to the detention pool and then returned.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableTest, GetOrInternString_utf16Data)
{
    EcmaVM *vm = thread->GetEcmaVM();
    EcmaStringTable *table = thread->GetEcmaVM()->GetEcmaStringTable();

    uint16_t utf16Data[] = {0x7F16, 0x7801, 0x89E3, 0x7801}; // “ 编码解码 ”
    EcmaString *ecmaStrCreatePtr =
        EcmaStringAccessor::CreateFromUtf16(vm, utf16Data, sizeof(utf16Data) / sizeof(uint16_t), false);
    EXPECT_TRUE(!EcmaStringAccessor(ecmaStrCreatePtr).IsInternString());

    EcmaString *ecmaStrGetPtr = table->GetOrInternString(vm, utf16Data, sizeof(utf16Data) / sizeof(uint16_t), false);
    EXPECT_STREQ(EcmaStringAccessor(ecmaStrGetPtr).ToCString(thread).c_str(), "编码解码");
    EXPECT_TRUE(EcmaStringAccessor(ecmaStrGetPtr).IsInternString());
}

/**
 * @tc.name: GetOrInternString
 * @tc.desc: Obtain EcmaString string from another EcmaString. If the string already exists in the detention pool,
             it will be returned directly. If not, it will be added to the detention pool and then returned.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableTest, GetOrInternString_EcmaString)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    EcmaStringTable *table = thread->GetEcmaVM()->GetEcmaStringTable();

    JSHandle<EcmaString> ecmaStrCreateHandle = factory->NewFromASCII("hello world");
    EXPECT_TRUE(EcmaStringAccessor(ecmaStrCreateHandle).IsInternString());

    EcmaString *ecmaStrGetPtr = table->GetOrInternString(vm, *ecmaStrCreateHandle);
    EXPECT_STREQ(EcmaStringAccessor(ecmaStrGetPtr).ToCString(thread).c_str(), "hello world");
    EXPECT_TRUE(EcmaStringAccessor(ecmaStrGetPtr).IsInternString());

#if ENABLE_NEXT_OPTIMIZATION
    EcmaString *ecmaStr = table->TryGetInternString(thread, ecmaStrCreateHandle);
    EXPECT_STREQ(EcmaStringAccessor(ecmaStr).ToCString(thread).c_str(), "hello world");
    EXPECT_TRUE(EcmaStringAccessor(ecmaStr).IsInternString());
#endif
}

/**
 * @tc.name: GetOrInternString
 * @tc.desc: Check the uniqueness of string and its hashcode in stringtable to ensure that no two strings have
             same contents and same hashcode
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaStringTableTest, GetOrInternString_CheckStringTable)
{
#if ENABLE_NEXT_OPTIMIZATION
    EXPECT_TRUE(thread->GetEcmaVM()->GetEcmaStringTable()->CheckStringTableValidity(thread));
#else
    EXPECT_TRUE(thread->GetEcmaVM()->GetEcmaStringTable()->CheckStringTableValidity(thread));
#endif
}

#if ENABLE_NEXT_OPTIMIZATION

// Check BitFiled of Entry
template<typename Mutex, typename ThreadHolder, TrieMapConfig::SlotBarrier SlotBarrier>
bool CheckBitFields(HashTrieMap<Mutex>* map)
{
    HashTrieMapOperation<Mutex, ThreadHolder, SlotBarrier> hashTrieMapOperation(map);
    HashTrieMapInUseScope<Mutex> mapInUse(map);
    bool highBitsNotSet = true;
    std::function<bool(HashTrieMapNode *)> checkbit = [&highBitsNotSet](HashTrieMapNode *node) {
        if (node->IsEntry()) {
            uint64_t bitfield = node->AsEntry()->GetBitField();
            if ((bitfield & TrieMapConfig::HIGH_8_BIT_MASK) != 0) {
                highBitsNotSet = false;
                return false;
            }
        }
        return true;
    };
    hashTrieMapOperation.Range(checkbit);
    return highBitsNotSet;
}

/**
 * @tc.name: GetOrInternStringFromCompressedSubString_SubString
 * @tc.desc: Test creating interned string from a compressed substring of another string
 * @tc.type: FUNC
 * @tc.require: AR001
 */
HWTEST_F_L0(EcmaStringTableTest, GetOrInternStringFromCompressedSubString_SubString)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    EcmaStringTable *table = vm->GetEcmaStringTable();

    JSHandle<EcmaString> originalStr =
        factory->NewFromASCII("00000x680x650x6c0x6c0x6f0x200x770x6f0x720x6c0x64");  // "hello world"
    uint32_t offset = 4;
    uint32_t utf8Len = EcmaStringAccessor(*originalStr).GetLength() - offset;

    EcmaString *internStr = table->GetOrInternStringFromCompressedSubString(vm, originalStr, offset, utf8Len);
    EXPECT_STREQ(EcmaStringAccessor(internStr).ToCString(thread).c_str(),
                 "0x680x650x6c0x6c0x6f0x200x770x6f0x720x6c0x64");
    EXPECT_TRUE(EcmaStringAccessor(internStr).IsInternString());
}

/**
 * @tc.name: GetOrInternString_ConcatenatedStrings
 * @tc.desc: Test interning concatenated strings from two JSHandle<EcmaString>
 * @tc.type: FUNC
 * @tc.require: AR001
 */
HWTEST_F_L0(EcmaStringTableTest, GetOrInternString_ConcatenatedStrings)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    EcmaStringTable *table = vm->GetEcmaStringTable();

    JSHandle<EcmaString> str1 = factory->NewFromASCII("hello");
    JSHandle<EcmaString> str2 = factory->NewFromASCII("world");

    EcmaString *concatenated = table->GetOrInternString(vm, str1, str2);

    EXPECT_STREQ(EcmaStringAccessor(concatenated).ToCString(thread).c_str(), "helloworld");
    EXPECT_TRUE(EcmaStringAccessor(concatenated).IsInternString());
}

/**
 * @tc.name: TryGetInternString_ExistingString
 * @tc.desc: Test retrieving existing interned string using TryGetInternString
 * @tc.type: FUNC
 * @tc.require: AR001
 */
HWTEST_F_L0(EcmaStringTableTest, TryGetInternString_ExistingString)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    EcmaStringTable *table = vm->GetEcmaStringTable();

    JSHandle<EcmaString> original = factory->NewFromASCII("test");
    table->GetOrInternString(vm, *original);

    EcmaString *retrieved = table->TryGetInternString(thread, original);

    EXPECT_STREQ(EcmaStringAccessor(retrieved).ToCString(thread).c_str(), "test");
    EXPECT_TRUE(EcmaStringAccessor(retrieved).IsInternString());
}
#endif

HWTEST_F_L0(EcmaStringTableTest, GetOrInternFlattenStringNoGC)
{
    auto vm = thread->GetEcmaVM();
    EcmaStringTable *stringTable = vm->GetEcmaStringTable();
    uint8_t utf8Data[] = {0x74, 0x65, 0x73, 0x74}; // "test"

    EcmaString *internString = EcmaStringAccessor::CreateFromUtf8(vm, utf8Data, sizeof(utf8Data), true);
    EcmaStringAccessor(internString).SetInternString();
    auto result = stringTable->GetOrInternFlattenStringNoGC(vm, internString);
    EXPECT_EQ(result, internString);

    EcmaString *nonInternString = EcmaStringAccessor::CreateFromUtf8(vm, utf8Data, sizeof(utf8Data), true);
    EXPECT_TRUE(!EcmaStringAccessor(nonInternString).IsInternString());
    internString = stringTable->GetOrInternFlattenStringNoGC(vm, nonInternString);
    EXPECT_TRUE(EcmaStringAccessor(internString).IsInternString());
    EXPECT_STREQ(EcmaStringAccessor(internString).ToCString(thread).c_str(), "test");

    EcmaString *repeatedCallString = stringTable->GetOrInternFlattenStringNoGC(vm, internString);
    EXPECT_EQ(internString, repeatedCallString);
}
}  // namespace panda::test
