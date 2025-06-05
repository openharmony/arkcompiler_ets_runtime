/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#ifdef USE_CMC_GC
#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/ecma_string_table_optimization-inl.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/tests/test_helper.h"

using EcmaString = panda::ecmascript::EcmaString;
using EcmaStringAccessor = panda::ecmascript::EcmaStringAccessor;

namespace panda::test {
class BaseStringTableTest : public BaseTestWithScope<false> {
public:
    common::BaseStringTableInterface<common::BaseStringTableImpl>::HandleCreator handleCreator_ = [
        ](common::ThreadHolder* holder, common::BaseString* string) -> common::ReadOnlyHandle<common::BaseString> {
        return JSHandle<EcmaString>(holder->GetJSThread(), EcmaString::FromBaseString(string));
    };
};

/**
 * @tc.name: GetOrInternFlattenString_EmptyString
 * @tc.desc: Write empty string emptyStr to the Intern pool and takes the hash code as its index.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(BaseStringTableTest, GetOrInternFlattenString_EmptyString)
{
    auto& table = common::BaseRuntime::GetInstance()->GetStringTable();

    JSHandle<EcmaString> emptyEcmaStrHandle(thread, EcmaStringAccessor::CreateEmptyString(thread->GetEcmaVM()));
    EXPECT_TRUE(!EcmaStringAccessor(emptyEcmaStrHandle).IsInternString());

    table.GetOrInternFlattenString(thread->GetThreadHolder(), handleCreator_, emptyEcmaStrHandle->ToBaseString());
    EXPECT_TRUE(!EcmaStringAccessor(emptyEcmaStrHandle).IsInternString());
    common::BaseString* emptyEcmaStr = table.TryGetInternString(emptyEcmaStrHandle);
    EXPECT_STREQ(EcmaStringAccessor(EcmaString::FromBaseString(emptyEcmaStr)).ToCString().c_str(), "");
    EXPECT_TRUE(EcmaStringAccessor(EcmaString::FromBaseString(emptyEcmaStr)).IsInternString());
}

/**
 * @tc.name: GetOrInternString
 * @tc.desc: Obtain EcmaString string from utf8 encoded data. If the string already exists in the detention pool,
             it will be returned directly. If not, it will be added to the detention pool and then returned.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(BaseStringTableTest, GetOrInternString_utf8Data)
{
    EcmaVM* vm = thread->GetEcmaVM();
    auto& table = common::BaseRuntime::GetInstance()->GetStringTable();

    uint8_t utf8Data[] = {0x68, 0x65, 0x6c, 0x6c, 0x6f}; // " hello "
    EcmaString* ecmaStrCreatePtr = EcmaStringAccessor::CreateFromUtf8(vm, utf8Data, sizeof(utf8Data), true);
    EXPECT_TRUE(!EcmaStringAccessor(ecmaStrCreatePtr).IsInternString());

    common::BaseString* ecmaStrGetPtr = table.GetOrInternString(thread->GetThreadHolder(), handleCreator_, utf8Data,
                                                                sizeof(utf8Data), true);
    EXPECT_STREQ(EcmaStringAccessor(EcmaString::FromBaseString(ecmaStrGetPtr)).ToCString().c_str(), "hello");
    EXPECT_TRUE(EcmaStringAccessor(EcmaString::FromBaseString(ecmaStrGetPtr)).IsInternString());
}

/**
 * @tc.name: GetOrInternString
 * @tc.desc: Obtain EcmaString string from utf16 encoded data. If the string already exists in the detention pool,
             it will be returned directly. If not, it will be added to the detention pool and then returned.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(BaseStringTableTest, GetOrInternString_utf16Data)
{
    EcmaVM* vm = thread->GetEcmaVM();
    auto& table = common::BaseRuntime::GetInstance()->GetStringTable();

    uint16_t utf16Data[] = {0x7F16, 0x7801, 0x89E3, 0x7801}; // “ 编码解码 ”
    EcmaString* ecmaStrCreatePtr =
        EcmaStringAccessor::CreateFromUtf16(vm, utf16Data, sizeof(utf16Data) / sizeof(uint16_t), false);
    EXPECT_TRUE(!EcmaStringAccessor(ecmaStrCreatePtr).IsInternString());

    common::BaseString* ecmaStrGetPtr = table.GetOrInternString(thread->GetThreadHolder(), handleCreator_, utf16Data,
                                                                sizeof(utf16Data) / sizeof(uint16_t), false);
    EXPECT_STREQ(EcmaStringAccessor(EcmaString::FromBaseString(ecmaStrGetPtr)).ToCString().c_str(), "编码解码");
    EXPECT_TRUE(EcmaStringAccessor(EcmaString::FromBaseString(ecmaStrGetPtr)).IsInternString());
}

/**
 * @tc.name: GetOrInternStringFromCompressedSubString_SubString
 * @tc.desc: Test creating interned string from a compressed substring of another string
 * @tc.type: FUNC
 * @tc.require: AR001
 */
HWTEST_F_L0(BaseStringTableTest, GetOrInternStringFromCompressedSubString_SubString)
{
    EcmaVM* vm = thread->GetEcmaVM();
    ecmascript::ObjectFactory* factory = vm->GetFactory();
    auto& table = common::BaseRuntime::GetInstance()->GetStringTable();

    JSHandle<EcmaString> originalStr =
        factory->NewFromASCII("00000x680x650x6c0x6c0x6f0x200x770x6f0x720x6c0x64"); // "hello world"
    uint32_t offset = 4;
    uint32_t utf8Len = EcmaStringAccessor(*originalStr).GetLength() - offset;

    common::BaseString* internStr = table.GetOrInternStringFromCompressedSubString(
        thread->GetThreadHolder(), handleCreator_, originalStr, offset, utf8Len);
    EXPECT_STREQ(EcmaStringAccessor(EcmaString::FromBaseString(internStr)).ToCString().c_str(),
                 "0x680x650x6c0x6c0x6f0x200x770x6f0x720x6c0x64");
    EXPECT_TRUE(EcmaStringAccessor(EcmaString::FromBaseString(internStr)).IsInternString());
}
} // namespace panda::test
#endif
