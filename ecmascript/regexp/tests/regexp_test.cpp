/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/regexp/regexp_executor.h"
#include "ecmascript/regexp/regexp_parser.h"
#include "ecmascript/tests/test_helper.h"

namespace panda::test {
using namespace panda::ecmascript;
using RegExpGlobalResult = ecmascript::builtins::RegExpGlobalResult;

class RegExpTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override
    {
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
        chunk_ = thread->GetEcmaVM()->GetChunk();
        regExpCachedChunk_ = new RegExpCachedChunk(thread);
    }

    void TearDown() override
    {
        delete regExpCachedChunk_;
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    bool IsValidAlphaEscapeInAtom(char s) const
    {
        switch (s) {
            // Assertion [U] :: \b
            case 'b':
            // Assertion [U] :: \B
            case 'B':
            // ControlEscape :: one of f n r t v
            case 'f':
            case 'n':
            case 'r':
            case 't':
            case 'v':
            // CharacterClassEscape :: one of d D s S w W
            case 'd':
            case 'D':
            case 's':
            case 'S':
            case 'w':
            case 'W':
                return true;
            default:
                return false;
        }
    }

    bool IsValidAlphaEscapeInClass(char s) const
    {
        switch (s) {
            // ClassEscape[U] :: b
            case 'b':
            // ControlEscape :: one of f n r t v
            case 'f':
            case 'n':
            case 'r':
            case 't':
            case 'v':
            // CharacterClassEscape :: one of d D s S w W
            case 'd':
            case 'D':
            case 's':
            case 'S':
            case 'w':
            case 'W':
                return true;
            default:
                return false;
        }
    }

    JSHandle<EcmaString> GetSubString(JSHandle<RegExpGlobalResult>regExpGlobalResult,
                                      JSHandle<EcmaString> inputStr, uint32_t index)
    {
        uint32_t startIndex = regExpGlobalResult->GetStartOfCaptureIndex(index).GetInt();
        uint32_t len = regExpGlobalResult->GetEndOfCaptureIndex(index).GetInt() - startIndex;
        return JSHandle<EcmaString>(thread, EcmaStringAccessor::FastSubString(
            thread->GetEcmaVM(), inputStr, startIndex, len));
    }

protected:
    EcmaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
    Chunk *chunk_ {nullptr};
    RegExpCachedChunk *regExpCachedChunk_ {nullptr};
};

HWTEST_F_L0(RegExpTest, ParseError1)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("0{2,1}");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError2)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("^[z-a]$");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError3)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("\\");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError4)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("a**");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError5)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("a***");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError6)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("a**");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError7)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("a++");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError8)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("a+++");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError9)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("a???");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError10)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("a????");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError11)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("*a");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError12)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("**a");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError13)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("+a");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError14)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("++a");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError15)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("?a");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError16)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("??a");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError17)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("x{1}{1,}");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError18)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("x{1,2}{1}");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError19)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("x{1,}{1}");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError20)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("x{0,1}{1,}");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError21)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("[b-ac-e]");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError22)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("[\\10b-G]");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError23)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("[\\0b-G]");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError24)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("(");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 16);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError25)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source(")");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 16);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError26)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("{");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 16);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError27)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("}");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 16);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError28)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("[");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 16);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError29)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("]");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 16);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError30)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("\\c");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 16);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError31)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("\\c\024");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 16);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError32)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("[\\c]");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 16);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError33)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("[\\c\024]");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 16);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError34)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("[\\d-a]");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 16);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError35)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("[\\s-a]");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 16);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError36)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("[\\s-\\w]");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 16);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError37)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("[a-\\w]");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 16);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError38)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("\\{");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 16);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError39)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("\\/");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 16);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError40)
{
    for (char cu = 0x41; cu <= 0x5a; ++cu) {
        if (!IsValidAlphaEscapeInAtom(cu)) {
            CString source("\\");
            source += CString(&cu, 1);
            RegExpParser parser = RegExpParser(thread, chunk_);
            parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 16);
            parser.Parse();
            bool parseResult = parser.IsError();
            ASSERT_TRUE(parseResult);
        }
    }
    for (char cu = 0x61; cu <= 0x7a; ++cu) {
        if (!IsValidAlphaEscapeInAtom(cu)) {
            CString source("\\");
            source += CString(&cu, 1);
            RegExpParser parser = RegExpParser(thread, chunk_);
            parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 16);
            parser.Parse();
            bool parseResult = parser.IsError();
            ASSERT_TRUE(parseResult);
        }
    }
    for (char cu = 0x41; cu <= 0x5a; ++cu) {
        CString source("[\\");
        if (!IsValidAlphaEscapeInAtom(cu)) {
            source += CString(&cu, 1);
            source += CString("]");
            RegExpParser parser = RegExpParser(thread, chunk_);
            parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 16);
            parser.Parse();
            bool parseResult = parser.IsError();
            ASSERT_TRUE(parseResult);
        }
    }
    for (char cu = 0x61; cu <= 0x7a; ++cu) {
        CString source("[\\");
        if (!IsValidAlphaEscapeInAtom(cu)) {
            source += CString(&cu, 1);
            source += CString("]");
            RegExpParser parser = RegExpParser(thread, chunk_);
            parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 16);
            parser.Parse();
            bool parseResult = parser.IsError();
            ASSERT_TRUE(parseResult);
        }
    }
}

HWTEST_F_L0(RegExpTest, ParseError44)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("\\1");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 16);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError45)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("[\\1]");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 16);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError46)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("\\00");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 16);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError47)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("[\\00]");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 16);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError48)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("(+");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 16);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError49)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("/\\p{Line_Break=Glue}/u");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 16);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseError50)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("/\\p{lb=AL}/u");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 16);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_TRUE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseNoError1)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("a{10,2147483648}"); // 2^31
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 16);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseNoError2)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("a{10,4294967306}"); // 2^32+10
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 16);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseNoError3)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("[\\⥚]");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 1);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseNoError4)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("[\\⊲|\\⇐]");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 1);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseNoError5)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("/\\p{General_Category=Letter}/u");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 1);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseNoError6)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("/\\p{gc=L}/u");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 1);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
}

HWTEST_F_L0(RegExpTest, ParseAndExec1)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("ab");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);

    RegExpExecutor executor(regExpCachedChunk_);
    CString input("abc");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.size(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);

    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("abc");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> str = factory->NewFromASCII("ab");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec2)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("(((ab)|(cd)|(de))|((ef)|(gh)|(jk)))");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);

    RegExpExecutor executor(regExpCachedChunk_);
    CString input("cabd");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.size(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);

    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 10U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("cabd");
    JSHandle<EcmaString> resultStr0 = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> resultStr1 = GetSubString(regExpGlobalResult, inputStr, 1);
    JSHandle<EcmaString> resultStr2 = GetSubString(regExpGlobalResult, inputStr, 2);
    JSHandle<EcmaString> resultStr3 = GetSubString(regExpGlobalResult, inputStr, 3);
    JSHandle<EcmaString> str = factory->NewFromASCII("ab");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr0, str) == 0);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr1, str) == 0);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr2, str) == 0);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr3, str) == 0);
    ASSERT_EQ(regExpGlobalResult->GetEndOfCaptureIndex(4).GetInt(), -1);
    ASSERT_EQ(regExpGlobalResult->GetEndOfCaptureIndex(5).GetInt(), -1);
    ASSERT_EQ(regExpGlobalResult->GetEndOfCaptureIndex(6).GetInt(), -1);
    ASSERT_EQ(regExpGlobalResult->GetEndOfCaptureIndex(7).GetInt(), -1);
    ASSERT_EQ(regExpGlobalResult->GetEndOfCaptureIndex(8).GetInt(), -1);
    ASSERT_EQ(regExpGlobalResult->GetEndOfCaptureIndex(9).GetInt(), -1);
}

HWTEST_F_L0(RegExpTest, ParseAndExec3)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("(aa|aabaac|ba|b|c)*");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);

    RegExpExecutor executor(regExpCachedChunk_);
    CString input("aabaac");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.size(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);

    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 2U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("aabaac");
    JSHandle<EcmaString> resultStr0 = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> resultStr1 = GetSubString(regExpGlobalResult, inputStr, 1);
    JSHandle<EcmaString> str1 = factory->NewFromASCII("aaba");
    JSHandle<EcmaString> str2 = factory->NewFromASCII("ba");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr0, str1) == 0);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr1, str2) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec4)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("a*");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);

    RegExpExecutor executor(regExpCachedChunk_);
    CString input("aabaac");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.size(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);

    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("aabaac");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str = factory->NewFromASCII("aa");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec5)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("a?");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);

    RegExpExecutor executor(regExpCachedChunk_);
    CString input("b");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.size(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);

    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("b");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str = factory->NewFromASCII("");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec6)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("(z)((a+)?(b+)?(c))*");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);

    RegExpExecutor executor(regExpCachedChunk_);
    CString input("zaacbbbcac");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.size(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);

    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 6U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("zaacbbbcac");
    JSHandle<EcmaString> resultStr0 = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> resultStr1 = GetSubString(regExpGlobalResult, inputStr, 1);
    JSHandle<EcmaString> resultStr2 = GetSubString(regExpGlobalResult, inputStr, 2);
    JSHandle<EcmaString> resultStr3 = GetSubString(regExpGlobalResult, inputStr, 3);
    JSHandle<EcmaString> resultStr5 = GetSubString(regExpGlobalResult, inputStr, 5);
    JSHandle<EcmaString> str0 = factory->NewFromASCII("zaacbbbcac");
    JSHandle<EcmaString> str1 = factory->NewFromASCII("z");
    JSHandle<EcmaString> str2 = factory->NewFromASCII("ac");
    JSHandle<EcmaString> str3 = factory->NewFromASCII("a");
    JSHandle<EcmaString> str5 = factory->NewFromASCII("c");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr0, str0) == 0);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr1, str1) == 0);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr2, str2) == 0);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr3, str3) == 0);
    ASSERT_EQ(regExpGlobalResult->GetEndOfCaptureIndex(4).GetInt(), -1);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr5, str5) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec7)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("^abc");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 4);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);

    RegExpExecutor executor(regExpCachedChunk_);
    CString input("ab\nabc");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.size(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);

    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("ab\nabc");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str = factory->NewFromASCII("abc");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec8)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("abc$");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 4);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);

    RegExpExecutor executor(regExpCachedChunk_);
    CString input("ab\nabc");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.size(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);

    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("ab\nabc");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str = factory->NewFromASCII("abc");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec9)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("er\\B");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);

    RegExpExecutor executor(regExpCachedChunk_);
    CString input("erv");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.size(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);

    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("erv");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str = factory->NewFromASCII("er");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec10)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("d\\b");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);

    RegExpExecutor executor(regExpCachedChunk_);
    CString input("bad good");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.size(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);

    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("bad good");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str = factory->NewFromASCII("d");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec11)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source(".");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);

    RegExpExecutor executor(regExpCachedChunk_);
    CString input("\na");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.size(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);

    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("\na");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str = factory->NewFromASCII("a");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec12)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source(".");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 8);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);

    RegExpExecutor executor(regExpCachedChunk_);
    CString input("\n");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.size(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);

    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("\n");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str = factory->NewFromASCII("\n");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec13)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("abc");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 4);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);

    RegExpExecutor executor(regExpCachedChunk_);
    CString input("\naabc");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.size(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);

    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("\naabc");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str = factory->NewFromASCII("abc");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec14)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("abc");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 4);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);

    RegExpExecutor executor(regExpCachedChunk_);
    CString input("\nbbabc");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.size(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);

    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("\nbbabc");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str = factory->NewFromASCII("abc");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec15)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("a(?=a)");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);

    RegExpExecutor executor(regExpCachedChunk_);
    CString input("aabc");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.size(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);

    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("aabc");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str = factory->NewFromASCII("a");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec16)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("abc");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 2);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);

    RegExpExecutor executor(regExpCachedChunk_);
    CString input("ABC");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.size(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);

    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("ABC");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str = factory->NewFromASCII("ABC");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec17)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("a\\n");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 2);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);

    RegExpExecutor executor(regExpCachedChunk_);
    CString input("a\n");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.size(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);

    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("a\n");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str = factory->NewFromASCII("a\n");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec18)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("a(?=a)");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);

    RegExpExecutor executor(regExpCachedChunk_);
    CString input("ababc");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.size(), parser.GetOriginBuffer());
    ASSERT_FALSE(ret);
}

HWTEST_F_L0(RegExpTest, ParseAndExec19)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("a(?!a)");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);

    RegExpExecutor executor(regExpCachedChunk_);
    CString input("ababc");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.size(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);

    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("ababc");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str = factory->NewFromASCII("a");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec20)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("(?=(a+))");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);

    RegExpExecutor executor(regExpCachedChunk_);
    CString input("baaabac");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.size(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);

    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 2U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("baaabac");
    JSHandle<EcmaString> resultStr0 = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> resultStr1 = GetSubString(regExpGlobalResult, inputStr, 1);
    JSHandle<EcmaString> str0 = factory->NewFromASCII("");
    JSHandle<EcmaString> str1 = factory->NewFromASCII("aaa");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr0, str0) == 0);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr1, str1) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec21)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("a(?=a(?=b))");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);

    RegExpExecutor executor(regExpCachedChunk_);
    CString input("caab");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.size(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);

    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("caab");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str = factory->NewFromASCII("a");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec22)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source(".+:");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);

    RegExpExecutor executor(regExpCachedChunk_);
    CString input("aaaa:aa");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.size(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);

    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("aaaa:aa");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str = factory->NewFromASCII("aaaa:");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec23)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("a(?<=a(?<!b))");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);

    RegExpExecutor executor(regExpCachedChunk_);
    CString input("caab");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.size(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);

    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("caab");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str = factory->NewFromASCII("a");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec24)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("a(?<=ab(?<!bc))");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);

    RegExpExecutor executor(regExpCachedChunk_);
    CString input("caab");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.size(), parser.GetOriginBuffer());
    ASSERT_FALSE(ret);
}

HWTEST_F_L0(RegExpTest, ParseAndExec25)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("(?<=(ab))");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);

    RegExpExecutor executor(regExpCachedChunk_);
    CString input("cabab");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.size(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);

    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 2U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("cabab");
    JSHandle<EcmaString> resultStr0 = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> resultStr1 = GetSubString(regExpGlobalResult, inputStr, 1);
    JSHandle<EcmaString> str0 = factory->NewFromASCII("");
    JSHandle<EcmaString> str1 = factory->NewFromASCII("ab");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr0, str0) == 0);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr1, str1) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec26)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("[a-z]");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 2);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);

    RegExpExecutor executor(regExpCachedChunk_);
    CString input("A");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.size(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);

    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("A");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str = factory->NewFromASCII("A");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec27)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("[^a-b]");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 2);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);

    RegExpExecutor executor(regExpCachedChunk_);
    CString input("Z");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.size(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);

    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("Z");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str = factory->NewFromASCII("Z");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec28)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("\\s");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);

    RegExpExecutor executor(regExpCachedChunk_);
    CString input("\n");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.size(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);

    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("\n");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str = factory->NewFromASCII("\n");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec29)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("()|");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);

    RegExpExecutor executor(regExpCachedChunk_);
    CString input("");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.size(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);

    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 2U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("");
    JSHandle<EcmaString> resultStr0 = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> resultStr1 = GetSubString(regExpGlobalResult, inputStr, 1);
    JSHandle<EcmaString> str0 = factory->NewFromASCII("");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr0, str0) == 0);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr1, str0) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec30)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("|()");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);

    RegExpExecutor executor(regExpCachedChunk_);
    CString input("");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.size(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);

    executor.GetResult(thread);
    JSHandle<builtins::RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 2U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("");
    JSHandle<EcmaString> resultStr0 = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str0 = factory->NewFromASCII("");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr0, str0) == 0);
    ASSERT_EQ(regExpGlobalResult->GetEndOfCaptureIndex(1).GetInt(), -1);
}

HWTEST_F_L0(RegExpTest, ParseAndExec31)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("a(a|b)\\1");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 2);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
    RegExpExecutor executor(regExpCachedChunk_);
    CString input("aabb");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.length(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);
    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 2U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("aabb");
    JSHandle<EcmaString> resultStr0 = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> resultStr1 = GetSubString(regExpGlobalResult, inputStr, 1);
    JSHandle<EcmaString> str0 = factory->NewFromASCII("abb");
    JSHandle<EcmaString> str1 = factory->NewFromASCII("b");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr0, str0) == 0);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr1, str1) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec32)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("(a(a|b))\\2");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 2);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
    RegExpExecutor executor(regExpCachedChunk_);
    CString input("aabb");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.length(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);
    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 3U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("aabb");
    JSHandle<EcmaString> resultStr0 = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> resultStr1 = GetSubString(regExpGlobalResult, inputStr, 1);
    JSHandle<EcmaString> resultStr2 = GetSubString(regExpGlobalResult, inputStr, 2);
    JSHandle<EcmaString> str0 = factory->NewFromASCII("abb");
    JSHandle<EcmaString> str1 = factory->NewFromASCII("ab");
    JSHandle<EcmaString> str2 = factory->NewFromASCII("b");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr0, str0) == 0);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr1, str1) == 0);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr2, str2) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec33)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("qya+");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 2);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
    RegExpExecutor executor(regExpCachedChunk_);
    CString input("qyqya");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.length(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);

    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("qyqya");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str = factory->NewFromASCII("qya");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec34)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("qy(?=\\s+)");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 2);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
    RegExpExecutor executor(regExpCachedChunk_);
    CString input("qyqy ");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.length(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);
    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("qyqy ");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str = factory->NewFromASCII("qy");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec35)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("(\\d{4})-(\\d{2})-(\\d{2})");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 2);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
    RegExpExecutor executor(regExpCachedChunk_);
    CString input("xx2021-01-09");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.length(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);
    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 4U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("xx2021-01-09");
    JSHandle<EcmaString> resultStr0 = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> resultStr1 = GetSubString(regExpGlobalResult, inputStr, 1);
    JSHandle<EcmaString> resultStr2 = GetSubString(regExpGlobalResult, inputStr, 2);
    JSHandle<EcmaString> resultStr3 = GetSubString(regExpGlobalResult, inputStr, 3);
    JSHandle<EcmaString> str0 = factory->NewFromASCII("2021-01-09");
    JSHandle<EcmaString> str1 = factory->NewFromASCII("2021");
    JSHandle<EcmaString> str2 = factory->NewFromASCII("01");
    JSHandle<EcmaString> str3 = factory->NewFromASCII("09");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr0, str0) == 0);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr1, str1) == 0);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr2, str2) == 0);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr3, str3) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec36)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("quick\\s(brown).+?(jumps)");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 2);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
    RegExpExecutor executor(regExpCachedChunk_);
    CString input("The Quick Brown Fox Jumps Over The Lazy Dog");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.length(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);
    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 3U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("The Quick Brown Fox Jumps Over The Lazy Dog");
    JSHandle<EcmaString> resultStr0 = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> resultStr1 = GetSubString(regExpGlobalResult, inputStr, 1);
    JSHandle<EcmaString> resultStr2 = GetSubString(regExpGlobalResult, inputStr, 2);
    JSHandle<EcmaString> str0 = factory->NewFromASCII("Quick Brown Fox Jumps");
    JSHandle<EcmaString> str1 = factory->NewFromASCII("Brown");
    JSHandle<EcmaString> str2 = factory->NewFromASCII("Jumps");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr0, str0) == 0);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr1, str1) == 0);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr2, str2) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec37)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("(ab){1,2}?c");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 2);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
    RegExpExecutor executor(regExpCachedChunk_);
    CString input("abABc");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.length(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);
    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 2U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("abABc");
    JSHandle<EcmaString> resultStr0 = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> resultStr1 = GetSubString(regExpGlobalResult, inputStr, 1);
    JSHandle<EcmaString> str0 = factory->NewFromASCII("abABc");
    JSHandle<EcmaString> str1 = factory->NewFromASCII("AB");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr0, str0) == 0);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr1, str1) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec38)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("^(([a-z]+)*[a-z]\\.)+[a-z]{2,}$");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
    RegExpExecutor executor(regExpCachedChunk_);
    CString input("www.netscape.com");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.length(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);
    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 3U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("www.netscape.com");
    JSHandle<EcmaString> resultStr0 = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> resultStr1 = GetSubString(regExpGlobalResult, inputStr, 1);
    JSHandle<EcmaString> resultStr2 = GetSubString(regExpGlobalResult, inputStr, 2);
    JSHandle<EcmaString> str0 = factory->NewFromASCII("www.netscape.com");
    JSHandle<EcmaString> str1 = factory->NewFromASCII("netscape.");
    JSHandle<EcmaString> str2 = factory->NewFromASCII("netscap");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr0, str0) == 0);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr1, str1) == 0);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr2, str2) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec39)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("(a*)b\\1+");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
    RegExpExecutor executor(regExpCachedChunk_);
    CString input("baaaac");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.length(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);
    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 2U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("baaaac");
    JSHandle<EcmaString> resultStr0 = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> resultStr1 = GetSubString(regExpGlobalResult, inputStr, 1);
    JSHandle<EcmaString> str0 = factory->NewFromASCII("b");
    JSHandle<EcmaString> str1 = factory->NewFromASCII("");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr0, str0) == 0);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr1, str1) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec40)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("a*?");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
    RegExpExecutor executor(regExpCachedChunk_);
    CString input("ab");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.length(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);
    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("ab");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str = factory->NewFromASCII("");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec41)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("(.*?)a(?!(a+)b\\2c)\\2(.*)");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
    RegExpExecutor executor(regExpCachedChunk_);
    CString input("baaabaac");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.length(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);
    executor.GetResult(thread);
    JSHandle<builtins::RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 4U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("baaabaac");
    JSHandle<EcmaString> resultStr0 = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> resultStr1 = GetSubString(regExpGlobalResult, inputStr, 1);
    JSHandle<EcmaString> resultStr3 = GetSubString(regExpGlobalResult, inputStr, 3);
    JSHandle<EcmaString> str0 = factory->NewFromASCII("baaabaac");
    JSHandle<EcmaString> str1 = factory->NewFromASCII("ba");
    JSHandle<EcmaString> str3 = factory->NewFromASCII("abaac");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr0, str0) == 0);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr1, str1) == 0);
    ASSERT_EQ(regExpGlobalResult->GetEndOfCaptureIndex(2).GetInt(), -1);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr3, str3) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec42)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("[a-c\\d]+");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
    RegExpExecutor executor(regExpCachedChunk_);
    CString input("\n\n\\abc324234");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.length(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);
    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("\n\n\\abc324234");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str = factory->NewFromASCII("abc324234");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec43)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("[\\d][\n][^\\d]");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
    RegExpExecutor executor(regExpCachedChunk_);
    CString input("line1\nline2");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.length(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);
    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("line1\nline2");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str = factory->NewFromASCII("1\nl");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec44)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source(".[\b].");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
    RegExpExecutor executor(regExpCachedChunk_);
    CString input("abc\bdef");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.length(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);
    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("abc\bdef");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str = factory->NewFromASCII("c\bd");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec45)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("[^\b]+");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
    RegExpExecutor executor(regExpCachedChunk_);
    CString input("easy\bto\u0008ride");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.length(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);
    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("easy\bto\u0008ride");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str = factory->NewFromASCII("easy");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec46)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("([\\S]+([ \t]+[\\S]+)*)[ \t]*=[ \t]*[\\S]+");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
    RegExpExecutor executor(regExpCachedChunk_);
    CString input("Course_Creator = Test");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.length(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);
    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 3U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("Course_Creator = Test");
    JSHandle<EcmaString> resultStr0 = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> resultStr1 = GetSubString(regExpGlobalResult, inputStr, 1);
    JSHandle<EcmaString> str0 = factory->NewFromASCII("Course_Creator = Test");
    JSHandle<EcmaString> str1 = factory->NewFromASCII("Course_Creator");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr0, str0) == 0);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr1, str1) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec47)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("[^o]t\\b");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 2);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
    RegExpExecutor executor(regExpCachedChunk_);
    CString input("pilOt\nsoviet robot\topenoffice");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.length(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);
    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("pilOt\nsoviet robot\topenoffice");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str = factory->NewFromASCII("et");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec49)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("(a(b)\\4(5)(5))");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 2);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
    RegExpExecutor executor(regExpCachedChunk_);
    CString input("ab55");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.length(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);
    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 5U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("ab55");
    JSHandle<EcmaString> resultStr0 = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> resultStr1 = GetSubString(regExpGlobalResult, inputStr, 1);
    JSHandle<EcmaString> resultStr2 = GetSubString(regExpGlobalResult, inputStr, 2);
    JSHandle<EcmaString> resultStr3 = GetSubString(regExpGlobalResult, inputStr, 3);
    JSHandle<EcmaString> resultStr4 = GetSubString(regExpGlobalResult, inputStr, 4);
    JSHandle<EcmaString> str0 = factory->NewFromASCII("ab55");
    JSHandle<EcmaString> str1 = factory->NewFromASCII("ab55");
    JSHandle<EcmaString> str2 = factory->NewFromASCII("b");
    JSHandle<EcmaString> str3 = factory->NewFromASCII("5");
    JSHandle<EcmaString> str4 = factory->NewFromASCII("5");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr0, str0) == 0);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr1, str1) == 0);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr2, str2) == 0);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr3, str3) == 0);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr4, str4) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec50)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("(?<year>\\d{4})-(?<date>\\d{2}-(?<day>\\d\\d))");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
    RegExpExecutor executor(regExpCachedChunk_);
    CString input("2020-12-31");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.length(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);
    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 4U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("2020-12-31");
    JSHandle<EcmaString> resultStr0 = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> resultStr1 = GetSubString(regExpGlobalResult, inputStr, 1);
    JSHandle<EcmaString> resultStr2 = GetSubString(regExpGlobalResult, inputStr, 2);
    JSHandle<EcmaString> resultStr3 = GetSubString(regExpGlobalResult, inputStr, 3);
    JSHandle<EcmaString> str0 = factory->NewFromASCII("2020-12-31");
    JSHandle<EcmaString> str1 = factory->NewFromASCII("2020");
    JSHandle<EcmaString> str2 = factory->NewFromASCII("12-31");
    JSHandle<EcmaString> str3 = factory->NewFromASCII("31");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr0, str0) == 0);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr1, str1) == 0);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr2, str2) == 0);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr3, str3) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec51)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("\\u0000");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
    RegExpExecutor executor(regExpCachedChunk_);
    std::u16string input(u"\u0000");
    bool ret = executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.length() + 1,
                                parser.GetOriginBuffer(), true);
    ASSERT_TRUE(ret);
    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
}

HWTEST_F_L0(RegExpTest, ParseAndExec52)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("(aa).+\\1");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
    RegExpExecutor executor(regExpCachedChunk_);
    CString input("aabcdaabcd");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.length(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);
    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 2U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("aabcdaabcd");
    JSHandle<EcmaString> resultStr0 = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> resultStr1 = GetSubString(regExpGlobalResult, inputStr, 1);
    JSHandle<EcmaString> str0 = factory->NewFromASCII("aabcdaa");
    JSHandle<EcmaString> str1 = factory->NewFromASCII("aa");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr0, str0) == 0);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr1, str1) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec53)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("\\x01");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
    RegExpExecutor executor(regExpCachedChunk_);
    std::u16string input(u"\u0001");
    bool ret = executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.length(),
                                parser.GetOriginBuffer(), true);
    ASSERT_TRUE(ret);
    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromUtf16(u"\u0001");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str = factory->NewFromASCII("\u0001");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec54)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("\\bot");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
    RegExpExecutor executor(regExpCachedChunk_);
    CString input("pilot\nsoviet robot\topenoffice");
    bool ret = executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.length(),
                                parser.GetOriginBuffer(), false);
    ASSERT_FALSE(ret);
}

HWTEST_F_L0(RegExpTest, ParseAndExec55)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("e\\b");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
    RegExpExecutor executor(regExpCachedChunk_);
    CString input("c\u0065");
    bool ret = executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.length(),
                                parser.GetOriginBuffer(), false);
    ASSERT_TRUE(ret);
    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("c\u0065");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str = factory->NewFromASCII("e");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec56)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("a啊");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
    RegExpExecutor executor(regExpCachedChunk_);
    std::u16string input(u"a啊");
    bool ret = executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.length(),
                                parser.GetOriginBuffer(), true);
    ASSERT_TRUE(ret);
    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromUtf16(u"a啊");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str = factory->NewFromUtf8("a啊");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec57)
{
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("\\udf06");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 16);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
    RegExpExecutor executor(regExpCachedChunk_);
    char16_t data[] = {0xd834, 0xdf06};
    bool ret = executor.Execute(reinterpret_cast<const uint8_t *>(data), 0, 2, parser.GetOriginBuffer(), true);
    ASSERT_FALSE(ret);
}

HWTEST_F_L0(RegExpTest, ParseAndExec58)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("\\udf06");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);
    RegExpExecutor executor(regExpCachedChunk_);
    char16_t data[] = {0xd834, 0xdf06};
    bool ret = executor.Execute(reinterpret_cast<const uint8_t *>(data), 0, 2, parser.GetOriginBuffer(), true);
    ASSERT_TRUE(ret);
    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromUtf16(reinterpret_cast<const uint16_t *>(data), 2);
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    char16_t data1[] = {0xdf06};
    JSHandle<EcmaString> str = factory->NewFromUtf16(reinterpret_cast<const uint16_t *>(data1), 1);
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, ParseAndExec59)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    RegExpParser parser = RegExpParser(thread, chunk_);
    CString source("\\v");
    parser.Init(const_cast<char *>(reinterpret_cast<const char *>(source.c_str())), source.size(), 0);
    parser.Parse();
    bool parseResult = parser.IsError();
    ASSERT_FALSE(parseResult);

    RegExpExecutor executor(regExpCachedChunk_);
    CString input("\u000B");
    bool ret =
        executor.Execute(reinterpret_cast<const uint8_t *>(input.c_str()), 0, input.size(), parser.GetOriginBuffer());
    ASSERT_TRUE(ret);

    executor.GetResult(thread);
    JSHandle<RegExpGlobalResult> regExpGlobalResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    ASSERT_EQ(regExpGlobalResult->GetTotalCaptureCounts().GetInt(), 1U);
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("\u000B");
    JSHandle<EcmaString> resultStr = GetSubString(regExpGlobalResult, inputStr, 0);
    JSHandle<EcmaString> str = factory->NewFromASCII("\u000B");
    ASSERT_TRUE(EcmaStringAccessor::Compare(instance, resultStr, str) == 0);
}

HWTEST_F_L0(RegExpTest, RangeSet1)
{
    std::list<std::pair<uint32_t, uint32_t>> listInput = {
        std::make_pair(1, 1),
        std::make_pair(2, 2),
        std::make_pair(3, 3),
    };
    std::list<std::pair<uint32_t, uint32_t>> listExpected = {
        std::make_pair(1, 5),
    };
    RangeSet rangeResult(listInput);
    RangeSet rangeExpected(listExpected);
    rangeResult.Insert(4, 5);
    rangeResult.Compress();
    EXPECT_EQ(rangeResult, rangeExpected);
}

HWTEST_F_L0(RegExpTest, RangeSet2)
{
    std::list<std::pair<uint32_t, uint32_t>> listExpected = {
        std::make_pair(4, 5),
    };
    RangeSet rangeResult;
    RangeSet rangeExpected(listExpected);
    rangeResult.Insert(4, 5);
    rangeResult.Compress();
    EXPECT_EQ(rangeResult, rangeExpected);
}

HWTEST_F_L0(RegExpTest, RangeSet3)
{
    std::list<std::pair<uint32_t, uint32_t>> listInput = {
        std::make_pair(2, 2),
    };
    std::list<std::pair<uint32_t, uint32_t>> listExpected = {
        std::make_pair(1, 5),
    };
    RangeSet rangeResult(listInput);
    RangeSet rangeExpected(listExpected);
    rangeResult.Insert(1, 5);
    rangeResult.Compress();
    EXPECT_EQ(rangeResult, rangeExpected);
}

HWTEST_F_L0(RegExpTest, RangeSet4)
{
    std::list<std::pair<uint32_t, uint32_t>> listInput = {
        std::make_pair(1, 5),
    };
    std::list<std::pair<uint32_t, uint32_t>> listExpected = {
        std::make_pair(1, 5),
    };
    RangeSet rangeResult(listInput);
    RangeSet rangeExpected(listExpected);
    rangeResult.Insert(2, 4);
    rangeResult.Compress();
    EXPECT_EQ(rangeResult, rangeExpected);
}

HWTEST_F_L0(RegExpTest, RangeSet5)
{
    std::list<std::pair<uint32_t, uint32_t>> listInput = {
        std::make_pair(1, 2),
        std::make_pair(9, UINT16_MAX),
    };
    std::list<std::pair<uint32_t, uint32_t>> listExpected = {
        std::make_pair(1, 2),
        std::make_pair(4, 7),
        std::make_pair(9, UINT16_MAX),
    };
    RangeSet rangeResult(listInput);
    RangeSet rangeExpected(listExpected);
    rangeResult.Insert(4, 7);
    rangeResult.Compress();
    EXPECT_EQ(rangeResult, rangeExpected);
}

HWTEST_F_L0(RegExpTest, RangeSet6)
{
    std::list<std::pair<uint32_t, uint32_t>> listExpected = {
        std::make_pair(0, UINT16_MAX),
    };
    RangeSet rangeResult;
    RangeSet rangeExpected(listExpected);
    rangeResult.Invert(false);
    EXPECT_EQ(rangeResult, rangeExpected);
}

HWTEST_F_L0(RegExpTest, RangeSet7)
{
    std::list<std::pair<uint32_t, uint32_t>> listInput = {
        std::make_pair(1, 5),
    };
    std::list<std::pair<uint32_t, uint32_t>> listExpected = {
        std::make_pair(0, 0),
        std::make_pair(6, UINT16_MAX),
    };
    RangeSet rangeResult(listInput);
    RangeSet rangeExpected(listExpected);
    rangeResult.Invert(false);
    EXPECT_EQ(rangeResult, rangeExpected);
}

HWTEST_F_L0(RegExpTest, RangeSet8)
{
    std::list<std::pair<uint32_t, uint32_t>> listInput = {
        std::make_pair(1, 5),
        std::make_pair(0xfffe, UINT16_MAX),
    };
    std::list<std::pair<uint32_t, uint32_t>> listExpected = {
        std::make_pair(0, 0),
        std::make_pair(6, 0xfffd),
    };
    RangeSet rangeResult(listInput);
    RangeSet rangeExpected(listExpected);
    rangeResult.Invert(false);
    EXPECT_EQ(rangeResult, rangeExpected);
}

HWTEST_F_L0(RegExpTest, RangeSet9)
{
    std::list<std::pair<uint32_t, uint32_t>> listInput = {
        std::make_pair(0, 5),
        std::make_pair(0xfffe, 0xfffe),
    };
    std::list<std::pair<uint32_t, uint32_t>> listExpected = {
        std::make_pair(6, 0xfffd),
        std::make_pair(UINT16_MAX, UINT16_MAX),
    };
    RangeSet rangeResult(listInput);
    RangeSet rangeExpected(listExpected);
    rangeResult.Invert(false);
    EXPECT_EQ(rangeResult, rangeExpected);
}

HWTEST_F_L0(RegExpTest, RangeSet10)
{
    std::list<std::pair<uint32_t, uint32_t>> listInput = {
        std::make_pair(0, UINT16_MAX),
    };
    RangeSet rangeResult(listInput);
    RangeSet rangeExpected;
    rangeResult.Invert(false);
    EXPECT_EQ(rangeResult, rangeExpected);
}
}  // namespace panda::test
