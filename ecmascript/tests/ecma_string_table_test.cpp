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

#include "common_components/objects/string_table/hashtriemap.h"
#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/ecma_string_table_optimization-inl.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class EcmaStringTableTest : public BaseTestWithScope<false> {
public:
    template <common::TrieMapConfig::SlotBarrier barrier>
    void TestLoadOrStoreConcurrentAccess();
    template <common::TrieMapConfig::SlotBarrier barrier>
    void TestLoadOrStoreInsertNewKey();
    template <common::TrieMapConfig::SlotBarrier barrier>
    void TestLoadOrStoreStoreExistingKey();
    template<common::TrieMapConfig::SlotBarrier barrier>
    void TestExpandHashCollisionHandling();
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

// Used for thread concurrency loadOrStore verification
/**
 * @tc.name: LoadOrStore_ConcurrentAccess
 * @tc.desc: Test thread-safe insertion under concurrent access
 * @tc.type: FUNC
 * @tc.require: AR005
 */
HWTEST_F_L0(EcmaStringTableTest, LoadOrStore_ConcurrentAccess)
{
    TestLoadOrStoreConcurrentAccess<common::TrieMapConfig::NeedSlotBarrier>();
    TestLoadOrStoreConcurrentAccess<common::TrieMapConfig::NoSlotBarrier>();
}

// Check BitFiled of Entry
template<typename Mutex, typename ThreadHolder, common::TrieMapConfig::SlotBarrier SlotBarrier>
bool CheckBitFields(common::HashTrieMap<Mutex, ThreadHolder, SlotBarrier>* map)
{
    bool highBitsNotSet = true;
    std::function<bool(common::HashTrieMapNode *)> checkbit = [&highBitsNotSet](common::HashTrieMapNode *node) {
        if (node->IsEntry()) {
            uint64_t bitfield = node->AsEntry()->GetBitField();
            if ((bitfield & common::TrieMapConfig::HIGH_8_BIT_MASK) != 0) {
                highBitsNotSet = false;
                return false;
            }
        }
        return true;
    };
    map->Range(checkbit);
    return highBitsNotSet;
}

template<common::TrieMapConfig::SlotBarrier barrier>
void EcmaStringTableTest::TestLoadOrStoreConcurrentAccess()
{
    for (uint32_t i = 0; i < 20; i++) {
        auto *map = new common::HashTrieMap<EcmaStringTableMutex, JSThread, barrier>();

        EcmaVM *vm = thread->GetEcmaVM();
        [[maybe_unused]] JSHandle<EcmaString> values[] = {
            vm->GetFactory()->NewFromASCII("581610154"),
            vm->GetFactory()->NewFromASCII("648719018"),
            vm->GetFactory()->NewFromASCII("715827882"),
            vm->GetFactory()->NewFromASCII("782936746"),
            vm->GetFactory()->NewFromASCII("850045610"),
            vm->GetFactory()->NewFromASCII("917154474"),
            vm->GetFactory()->NewFromASCII("984263338"),
        };
        constexpr int string_count = 7;
        constexpr int thread_count = 8;
        JSRuntimeOptions options;
        auto thread_proc = [&]() {
            EcmaVM *ecmaVm1{nullptr};
            EcmaHandleScope *scope{nullptr};
            JSThread *thread{nullptr};
            TestHelper::CreateEcmaVMWithScope(ecmaVm1, thread, scope);
            JSHandle<EcmaString> value[] = {
                ecmaVm1->GetFactory()->NewFromASCII("581610154"),
                ecmaVm1->GetFactory()->NewFromASCII("648719018"),
                ecmaVm1->GetFactory()->NewFromASCII("715827882"),
                ecmaVm1->GetFactory()->NewFromASCII("782936746"),
                ecmaVm1->GetFactory()->NewFromASCII("850045610"),
                ecmaVm1->GetFactory()->NewFromASCII("917154474"),
                ecmaVm1->GetFactory()->NewFromASCII("984263338"),
            };
            for (int i = 0; i < 1000; ++i) {
                auto readBarrier = [ecmaVm1](const void *obj, size_t offset) -> TaggedObject * {
                    return Barriers::GetTaggedObject(ecmaVm1->GetAssociatedJSThread(), obj, offset);
                };
                map->template LoadOrStore<true>(
                    ecmaVm1->GetJSThread(), value[i % string_count]->ToBaseString()->GetHashcode(readBarrier),
                    [value, i]() {
                        return value[i % string_count];
                    },
                    [ecmaVm1, &value, i](BaseString *v) {
                        return EcmaStringAccessor::StringsAreEqual(ecmaVm1->GetJSThread(),
                                                                   EcmaString::FromBaseString(v),
                                                                   *value[i % string_count]);
                    });
            }
            TestHelper::DestroyEcmaVMWithScope(ecmaVm1, scope);
            return nullptr;
        };
        std::vector<std::thread> threads;
        for (int i = 0; i < thread_count; ++i) {
            threads.emplace_back(thread_proc);
        }
        for (auto &t: threads) {
            ecmascript::ThreadSuspensionScope suspensionScope(thread);
            t.join();
        }
        uint32_t count = 0;
        std::function<bool(common::HashTrieMapNode *)> iter = [&count](common::HashTrieMapNode *node) {
            if (node->IsEntry()) {
                count++;
            }
            return true;
        };
        map->Range(iter);
        ASSERT_PRINT(count == string_count, count);
        ASSERT_TRUE(CheckBitFields(map)) << "Bits 56-63 of Bitfield are incorrectly set!";
        delete map;
    }
}
/**
 * @tc.name: LoadOrStore_InsertNewKey
 * @tc.desc: Test inserting a new key-value pair via LoadOrStore
 * @tc.type: FUNC
 * @tc.require: AR001
 */
HWTEST_F_L0(EcmaStringTableTest, LoadOrStore_InsertNewKey)
{
    TestLoadOrStoreInsertNewKey<common::TrieMapConfig::NeedSlotBarrier>();
    TestLoadOrStoreInsertNewKey<common::TrieMapConfig::NoSlotBarrier>();
}

template<common::TrieMapConfig::SlotBarrier barrier>
void EcmaStringTableTest::TestLoadOrStoreInsertNewKey()
{
    EcmaVM* vm = thread->GetEcmaVM();
    auto* map = new common::HashTrieMap<EcmaStringTableMutex, JSThread, barrier>();
    JSHandle<EcmaString> value(thread, *vm->GetFactory()->NewFromASCII("test_value"));
    uint32_t key = value->ToBaseString()->GetMixHashcode();
    auto readBarrier = [vm](const void *obj, size_t offset) -> TaggedObject * {
        return Barriers::GetTaggedObject(vm->GetAssociatedJSThread(), obj, offset);
    };
    BaseString* loadResult1 = map->template Load<false>(std::move(readBarrier), key, value->ToBaseString());
    EXPECT_EQ(loadResult1, nullptr);
    // Test insertion
    BaseString* result = map->template LoadOrStore<true>(
        vm->GetJSThread(), key, [value]() { return value; },
        [vm, value](BaseString *foudString) {
            return EcmaStringAccessor::StringsAreEqual(vm->GetAssociatedJSThread(), *value,
                                                       EcmaString::FromBaseString(foudString));
        });
    EXPECT_EQ(result, value->ToBaseString());
    BaseString* loadResult2 = map->template Load<false>(std::move(readBarrier), key, value->ToBaseString());
    EXPECT_STREQ(EcmaStringAccessor(EcmaString::FromBaseString(loadResult2)).ToCString(thread).c_str(), "test_value");
    EXPECT_EQ(loadResult2, value->ToBaseString());
    ASSERT_TRUE(CheckBitFields(map)) << "Bits 56-63 of Bitfield are incorrectly set!";
    delete map;
}

/**
 * @tc.name: LoadOrStore_StoreExistingKey
 * @tc.desc: Test updating existing key with value comparison
 * @tc.type: FUNC
 * @tc.require: AR002
 */
HWTEST_F_L0(EcmaStringTableTest, LoadOrStore_StoreExistingKey)
{
    TestLoadOrStoreStoreExistingKey<common::TrieMapConfig::NeedSlotBarrier>();
    TestLoadOrStoreStoreExistingKey<common::TrieMapConfig::NoSlotBarrier>();
}

template<common::TrieMapConfig::SlotBarrier barrier>
void EcmaStringTableTest::TestLoadOrStoreStoreExistingKey()
{
    EcmaVM *vm = thread->GetEcmaVM();
    auto *map = new common::HashTrieMap<EcmaStringTableMutex, JSThread, barrier>();
    JSHandle<EcmaString> original(thread, *vm->GetFactory()->NewFromASCII("Aa1"));
    JSHandle<EcmaString> origina2(thread, *vm->GetFactory()->NewFromASCII("BB1"));
    // key1 = key2 = 0x0000FFF1
    uint32_t key1 = original->ToBaseString()->GetMixHashcode();
    uint32_t key2 = origina2->ToBaseString()->GetMixHashcode();

    // Initial insertion
    map->template LoadOrStore<true>(
        vm->GetJSThread(), key1, [original]() { return original; },
        [this, original](BaseString *foudString) {
            return EcmaStringAccessor::StringsAreEqual(thread, *original, EcmaString::FromBaseString(foudString));
        });

    // store overflow
    map->template LoadOrStore<true>(
        vm->GetJSThread(), key2, [origina2]() { return origina2; },
        [vm, origina2](BaseString *foudString) {
            return EcmaStringAccessor::StringsAreEqual(vm->GetAssociatedJSThread(), *origina2,
                                                       EcmaString::FromBaseString(foudString));
        });
    auto readBarrier = [vm](const void *obj, size_t offset) -> TaggedObject * {
        return Barriers::GetTaggedObject(vm->GetAssociatedJSThread(), obj, offset);
    };
    EXPECT_EQ(map->template Load<false>(std::move(readBarrier), key1, original->ToBaseString()),
              original->ToBaseString());
    EXPECT_EQ(map->template Load<false>(std::move(readBarrier), key2, origina2->ToBaseString()),
              origina2->ToBaseString());
    ASSERT_TRUE(CheckBitFields(map)) << "Bits 56-63 of Bitfield are incorrectly set!";
    delete map;
}

/**
 * @tc.name: Expand_HashCollisionHandling
 * @tc.desc: Test node expansion during hash collision
 * @tc.type: FUNC
 * @tc.require: AR004
 */
HWTEST_F_L0(EcmaStringTableTest, Expand_HashCollisionHandling)
{
    TestExpandHashCollisionHandling<common::TrieMapConfig::NeedSlotBarrier>();
    TestExpandHashCollisionHandling<common::TrieMapConfig::NoSlotBarrier>();
}

template <common::TrieMapConfig::SlotBarrier barrier>
void EcmaStringTableTest::TestExpandHashCollisionHandling()
{
    EcmaVM* vm = thread->GetEcmaVM();
    auto* map = new common::HashTrieMap<EcmaStringTableMutex, JSThread, barrier>();

    JSHandle<EcmaString> value1(thread, *vm->GetFactory()->NewFromASCII("ADF3"));
    JSHandle<EcmaString> value2(thread, *vm->GetFactory()->NewFromASCII("A ?0"));
    JSHandle<EcmaString> value3(thread, *vm->GetFactory()->NewFromASCII("AAa1"));
    JSHandle<EcmaString> value4(thread, *vm->GetFactory()->NewFromASCII("ABB1"));
    uint32_t key1 = value1->ToBaseString()->GetMixHashcode();
    uint32_t key2 = value2->ToBaseString()->GetMixHashcode();
    uint32_t key3 = value3->ToBaseString()->GetMixHashcode();
    uint32_t key4 = value4->ToBaseString()->GetMixHashcode();
    uint32_t ROOT_ID = key1 & common::TrieMapConfig::ROOT_BIT_MASK;
    // Insert first key
    map->template LoadOrStore<true>(
        vm->GetJSThread(), key1, [value1]() { return value1; },
        [this, value1](BaseString *foudString) {
            return EcmaStringAccessor::StringsAreEqual(thread, *value1, EcmaString::FromBaseString(foudString));
        });

    // Insert second key causing collision
    map->template LoadOrStore<true>(
        vm->GetJSThread(), key1, [value1]() { return value1; },
        [this, value1](BaseString *foudString) {
            return EcmaStringAccessor::StringsAreEqual(thread, *value1, EcmaString::FromBaseString(foudString));
        });

    // Insert second key causing collision
    map->template LoadOrStore<true>(
        vm->GetJSThread(), key2, [value2]() { return value2; },
        [this, value2](BaseString *foudString) {
            return EcmaStringAccessor::StringsAreEqual(thread, *value2, EcmaString::FromBaseString(foudString));
        });
    // Insert overflow key3:[value3 value4]
    map->template LoadOrStore<true>(
        vm->GetJSThread(), key3, [value3]() { return value3; },
        [this, value3](BaseString *foudString) {
            return EcmaStringAccessor::StringsAreEqual(thread, *value3, EcmaString::FromBaseString(foudString));
        });

    map->template LoadOrStore<true>(
        vm->GetJSThread(), key4, [value4]() { return value4; },
        [this, value4](BaseString *foudString) {
            return EcmaStringAccessor::StringsAreEqual(thread, *value4, EcmaString::FromBaseString(foudString));
        });

    /*map:
    └── Indirect (----)
      Children: [1, 2]
      └── Child[1]:
      └── Indirect (----)
            Children: [0, 1]
            └── Child[0]:
            └── Entry [key2, value=0x001E0C10]
            └── Child[2]:
            └── Entry [key3, value=0x001E8C10]
                  └── Overflow ->  └── Entry [key4, value=0x001E8C10]
      └── Child[2]:
            └── Entry [key1, value=0x001E9410]
    key1 = 000_000_000_001_111_010_010_10000010000
    key2 = 000_000_000_001_111_000_001_10000010000
    key3 = 000_000_000_001_111_010_001_10000010000
    key4 = 000_000_000_001_111_010_001_10000010000

    */
    // Verify structure after expansion
    common::HashTrieMapIndirect* root = map->GetRoot(ROOT_ID).load();
    ASSERT_TRUE(root->GetChild(0x1).load() != nullptr); // Check first collision level

    common::HashTrieMapIndirect* level1 = root->GetChild(0x1).
        load()->AsIndirect();
    ASSERT_TRUE(level1->GetChild(0x0).load() != nullptr);
    ASSERT_TRUE(level1->GetChild(0x2).load() != nullptr);
    common::HashTrieMapEntry* entry = level1->GetChild(0x2).load()->AsEntry();
    // Verify overflow
    ASSERT_TRUE(entry->Overflow().load() != nullptr);
    ASSERT_TRUE(root->GetChild(0x2).load() != nullptr);
    ASSERT_TRUE(CheckBitFields(map)) << "Bits 56-63 of Bitfield are incorrectly set!";
    delete map;
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
