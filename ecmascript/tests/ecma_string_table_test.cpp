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
    EcmaString *emptyEcmaStr = table->TryGetInternString(emptyEcmaStrHandle);
    EXPECT_STREQ(EcmaStringAccessor(emptyEcmaStr).ToCString().c_str(), "");
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
    EXPECT_STREQ(EcmaStringAccessor(ecmaStrGetPtr).ToCString().c_str(), "hello");
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
    EXPECT_STREQ(EcmaStringAccessor(ecmaStrGetPtr).ToCString().c_str(), "编码解码");
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
    EXPECT_STREQ(EcmaStringAccessor(ecmaStrGetPtr).ToCString().c_str(), "hello world");
    EXPECT_TRUE(EcmaStringAccessor(ecmaStrGetPtr).IsInternString());

#if ENABLE_NEXT_OPTIMIZATION
    EcmaString *ecmaStr = table->TryGetInternString(ecmaStrCreateHandle);
    EXPECT_STREQ(EcmaStringAccessor(ecmaStr).ToCString().c_str(), "hello world");
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
    EXPECT_TRUE(thread->GetEcmaVM()->GetEcmaStringTable()->CheckStringTableValidity());
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
    HashTrieMap *map = new HashTrieMap();

    const int thread_count = 8;
    std::atomic<int> counter {0};
    JSRuntimeOptions options;
    auto thread_proc = [&]() {
        EcmaVM *ecmaVm1 {nullptr};
        EcmaHandleScope *scope {nullptr};
        JSThread *thread {nullptr};
        TestHelper::CreateEcmaVMWithScope(ecmaVm1, thread, scope);
        JSHandle<EcmaString> value = ecmaVm1->GetFactory()->NewFromASCII("value");
        for (int i = 0; i < 1000; ++i) {
            map->LoadOrStore<true>(
                ecmaVm1, counter.fetch_add(1), [value]() { return value; }, [](EcmaString *v) { return false; });
        }
        TestHelper::DestroyEcmaVMWithScope(ecmaVm1, scope);
        return nullptr;
    };
    std::vector<std::thread> threads;
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back(thread_proc);
    }
    for (auto &t : threads) {
        ThreadSuspensionScope suspensionScope(thread);
        t.join();
    }

    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<EcmaString> value1 = vm->GetFactory()->NewFromASCII("value");
    for (int i = 0; i < 8000; ++i) {  // check 8000 store operations
        ASSERT_TRUE(map->Load<false>(i, *value1) != nullptr);
    }
    delete map;
}
/**
 * @tc.name: LoadOrStore_InsertNewKey
 * @tc.desc: Test inserting a new key-value pair via LoadOrStore
 * @tc.type: FUNC
 * @tc.require: AR001
 */
HWTEST_F_L0(EcmaStringTableTest, LoadOrStore_InsertNewKey)
{
    EcmaVM *vm = thread->GetEcmaVM();
    HashTrieMap *map = new HashTrieMap();
    uint32_t key = 0x12345678;
    JSHandle<EcmaString> value(thread, *vm->GetFactory()->NewFromASCII("test_value"));
    EcmaString *loadResult1 = map->Load<false>(key, *value);
    EXPECT_EQ(loadResult1, nullptr);
    // Test insertion
    EcmaString *result = map->LoadOrStore<true>(
        vm, key, [value]() { return value; },
        [value](EcmaString *foudString) { return EcmaStringAccessor::StringsAreEqual(*value, foudString); });
    EXPECT_EQ(result, *value);
    EcmaString *loadResult2 = map->Load<false>(key, *value);
    EXPECT_STREQ(EcmaStringAccessor(loadResult2).ToCString().c_str(), "test_value");
    EXPECT_EQ(loadResult2, *value);
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
    EcmaVM *vm = thread->GetEcmaVM();
    HashTrieMap *map = new HashTrieMap();
    uint32_t key = 0x12345678;
    JSHandle<EcmaString> original(thread, *vm->GetFactory()->NewFromASCII("original"));
    JSHandle<EcmaString> origina2(thread, *vm->GetFactory()->NewFromASCII("origina2"));

    // Initial insertion
    map->LoadOrStore<true>(
        vm, key, [original]() { return original; },
        [original](EcmaString *foudString) { return EcmaStringAccessor::StringsAreEqual(*original, foudString); });

    // store overflow
    map->LoadOrStore<true>(
        vm, key, [origina2]() { return origina2; },
        [origina2](EcmaString *foudString) { return EcmaStringAccessor::StringsAreEqual(*origina2, foudString); });

    EXPECT_EQ(map->Load<false>(key, *original), *original);
    EXPECT_EQ(map->Load<false>(key, *origina2), *origina2);
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
    EcmaVM *vm = thread->GetEcmaVM();
    HashTrieMap *map = new HashTrieMap();
    uint32_t key1 = 0x11111111;
    uint32_t key2 = 0x11110000;
    uint32_t key3 = 0x11110010;
    JSHandle<EcmaString> value1(thread, *vm->GetFactory()->NewFromASCII("value1"));
    JSHandle<EcmaString> value2(thread, *vm->GetFactory()->NewFromASCII("value2"));
    JSHandle<EcmaString> value3(thread, *vm->GetFactory()->NewFromASCII("value3"));
    JSHandle<EcmaString> value4(thread, *vm->GetFactory()->NewFromASCII("value4"));
    // Insert first key
    map->LoadOrStore<true>(
        vm, key1, [value1]() { return value1; },
        [value1](EcmaString *foudString) { return EcmaStringAccessor::StringsAreEqual(*value1, foudString); });

    // Insert second key causing collision
    map->LoadOrStore<true>(
        vm, key2, [value2]() { return value2; },
        [value2](EcmaString *foudString) { return EcmaStringAccessor::StringsAreEqual(*value2, foudString); });
    // Insert overflow key3:[value3 value4]
    map->LoadOrStore<true>(
        vm, key3, [value3]() { return value3; },
        [value3](EcmaString *foudString) { return EcmaStringAccessor::StringsAreEqual(*value3, foudString); });

    map->LoadOrStore<true>(
        vm, key3, [value4]() { return value4; },
        [value4](EcmaString *foudString) { return EcmaStringAccessor::StringsAreEqual(*value4, foudString); });

    /*map:
    └── Indirect (----)
      Children: [0, 1]
      └── Child[0]:
      └── Indirect (----)
            Children: [0, 1]
            └── Child[0]:
            └── Entry [key=286326784, value=0x2bafc81e50]
            └── Child[1]:
            └── Entry [key=286326800, value=0x2bafc01dd0]
                  └── Overflow ->  └── Entry [key=286326800, value=0x2bafc01db8]
      └── Child[1]:
            └── Entry [key=286331153, value=0x2bafc81e38]
    */
    // Verify structure after expansion
    HashTrieMap::Indirect *root = map->GetRoot().load();
    ASSERT_TRUE(root->children_[0x0].load() != nullptr);  // Check first collision level

    HashTrieMap::Indirect *level1 = root->children_[0x0].load()->AsIndirect();
    ASSERT_TRUE(level1->children_[0x0].load() != nullptr);
    ASSERT_TRUE(level1->children_[0x1].load() != nullptr);
    HashTrieMap::Entry *entry = level1->children_[0x1].load()->AsEntry();
    // Verify overflow
    ASSERT_TRUE(entry->Overflow().load() != nullptr);
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
    EXPECT_STREQ(EcmaStringAccessor(internStr).ToCString().c_str(), "0x680x650x6c0x6c0x6f0x200x770x6f0x720x6c0x64");
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

    EXPECT_STREQ(EcmaStringAccessor(concatenated).ToCString().c_str(), "helloworld");
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

    EcmaString *retrieved = table->TryGetInternString(original);

    EXPECT_STREQ(EcmaStringAccessor(retrieved).ToCString().c_str(), "test");
    EXPECT_TRUE(EcmaStringAccessor(retrieved).IsInternString());
}
#endif
}  // namespace panda::test
