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
    EXPECT_TRUE(thread->GetEcmaVM()->GetEcmaStringTable()->CheckStringTableValidity(thread));
}
}  // namespace panda::test
