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

#include "gtest/gtest.h"

#include <fstream>
#include <sstream>

#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/pgo_profiler/ap_file/pgo_method_type_set.h"
#include "ecmascript/pgo_profiler/ap_file/pgo_profile_type_pool.h"
#include "ecmascript/pgo_profiler/ap_file/pgo_proto_transition_type_pool.h"
#include "ecmascript/pgo_profiler/ap_file/pgo_record_pool.h"
#include "ecmascript/pgo_profiler/pgo_profiler_decoder.h"
#include "ecmascript/pgo_profiler/pgo_profiler_info.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"
#include "ecmascript/pgo_profiler/types/pgo_profile_type.h"
#include "ecmascript/pgo_profiler/types/pgo_profiler_type.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::pgo;

namespace panda::test {

class TextFormatterTest : public testing::Test {
public:
    static constexpr uint32_t TEST_THRESHOLD = 0;
    static constexpr uint32_t HIGH_THRESHOLD = UINT32_MAX;

    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    static std::string GenerateProfile(const char* dir, const char* abcName, const char* entryPoint)
    {
        mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        RuntimeOption option;
        option.SetEnableProfile(true);
        option.SetLogLevel(common::LOG_LEVEL::INFO);
        option.SetProfileDir(dir);
        EcmaVM* vm = JSNApi::CreateJSVM(option);
        if (vm == nullptr) {
            return "";
        }
        std::string abcPath = std::string(TARGET_ABC_PATH) + abcName;
        JSNApi::Execute(vm, abcPath, entryPoint, false);
        JSNApi::DestroyJSVM(vm);
        return std::string(dir) + "modules.ap";
    }

    static void CleanupDir(const char* dir, const char* apFile = "modules.ap")
    {
        std::string apPath = std::string(dir) + apFile;
        unlink(apPath.c_str());
        rmdir(dir);
    }
};

// ============================================
// Test Suite 1: TextFormatter Basic Methods
// ============================================

HWTEST_F_L0(TextFormatterTest, IndentZero)
{
    TextFormatter fmt;
    fmt.Indent(0);
    EXPECT_EQ(fmt.Str(), "");
}

HWTEST_F_L0(TextFormatterTest, IndentOne)
{
    TextFormatter fmt;
    fmt.Indent(1);
    EXPECT_EQ(fmt.Str(), "  ");
}

HWTEST_F_L0(TextFormatterTest, IndentMultiple)
{
    TextFormatter fmt;
    fmt.Indent(3);
    EXPECT_EQ(fmt.Str(), "      ");
}

HWTEST_F_L0(TextFormatterTest, TextWithString)
{
    TextFormatter fmt;
    fmt.Text("hello");
    EXPECT_EQ(fmt.Str(), "hello");
}

HWTEST_F_L0(TextFormatterTest, TextWithInt)
{
    TextFormatter fmt;
    fmt.Text(42);
    EXPECT_EQ(fmt.Str(), "42");
}

HWTEST_F_L0(TextFormatterTest, TextWithDouble)
{
    TextFormatter fmt;
    fmt.Text(3.14);
    EXPECT_TRUE(fmt.Str().find("3.14") != std::string::npos);
}

HWTEST_F_L0(TextFormatterTest, NewLine)
{
    TextFormatter fmt;
    fmt.Text("a").NewLine().Text("b");
    EXPECT_EQ(fmt.Str(), "a\nb");
}

HWTEST_F_L0(TextFormatterTest, Pipe)
{
    TextFormatter fmt;
    fmt.Text("x").Pipe().Text("y");
    EXPECT_EQ(fmt.Str(), "x | y");
}

HWTEST_F_L0(TextFormatterTest, LabelValue)
{
    TextFormatter fmt;
    fmt.Label("Name").Value("Test");
    EXPECT_EQ(fmt.Str(), "Name: Test");
}

HWTEST_F_L0(TextFormatterTest, LabelValueWithAlign)
{
    TextFormatter fmt;
    fmt.SetLabelWidth(10);
    fmt.Label("ID", true).Value(100);
    std::string result = fmt.Str();
    EXPECT_TRUE(result.find("ID") != std::string::npos);
    EXPECT_TRUE(result.find("100") != std::string::npos);
}

// ============================================
// Test Suite 2: Formatting Methods
// ============================================

HWTEST_F_L0(TextFormatterTest, HexZero)
{
    TextFormatter fmt;
    fmt.Hex(0);
    EXPECT_EQ(fmt.Str(), " 0x0");
}

HWTEST_F_L0(TextFormatterTest, Hex255)
{
    TextFormatter fmt;
    fmt.Hex(255);
    EXPECT_EQ(fmt.Str(), " 0xff");
}

HWTEST_F_L0(TextFormatterTest, Hex4096)
{
    TextFormatter fmt;
    fmt.Hex(4096);
    EXPECT_EQ(fmt.Str(), " 0x1000");
}

HWTEST_F_L0(TextFormatterTest, HexStr)
{
    EXPECT_EQ(TextFormatter::HexStr(0), "0x0");
    EXPECT_EQ(TextFormatter::HexStr(255), "0xff");
    EXPECT_EQ(TextFormatter::HexStr(4096), "0x1000");
}

HWTEST_F_L0(TextFormatterTest, LeftAlign)
{
    TextFormatter fmt;
    fmt.Left("abc", 10);
    std::string result = fmt.Str();
    EXPECT_EQ(result.length(), 10U);
    EXPECT_EQ(result.substr(0, 3), "abc");
}

HWTEST_F_L0(TextFormatterTest, RightAlign)
{
    TextFormatter fmt;
    fmt.Right(123, 10);
    std::string result = fmt.Str();
    EXPECT_EQ(result.length(), 10U);
    EXPECT_EQ(result.substr(7, 3), "123");
}

HWTEST_F_L0(TextFormatterTest, SectionLine)
{
    TextFormatter fmt;
    fmt.SectionLine();
    EXPECT_EQ(fmt.Str().length(), TextFormatter::SECTION_LINE_WIDTH);
    EXPECT_EQ(fmt.Str(), std::string(80, '='));
}

HWTEST_F_L0(TextFormatterTest, CenteredTitle)
{
    TextFormatter fmt;
    fmt.CenteredTitle("Test");
    std::string result = fmt.Str();
    // Title should be centered in 80 chars
    EXPECT_TRUE(result.find("Test") != std::string::npos);
    // Should have padding before title
    EXPECT_TRUE(result.length() > 4);
}

HWTEST_F_L0(TextFormatterTest, Fixed)
{
    TextFormatter fmt;
    fmt.Fixed(3.14159, 2);
    std::string result = fmt.Str();
    EXPECT_TRUE(result.find("3.14") != std::string::npos);
}

// ============================================
// Test Suite 3: Relative Indent Support
// ============================================

HWTEST_F_L0(TextFormatterTest, PushPopIndent)
{
    TextFormatter fmt;
    EXPECT_EQ(fmt.GetIndentLevel(), 0);

    fmt.PushIndent(2);
    EXPECT_EQ(fmt.GetIndentLevel(), 2);

    fmt.PushIndent(1);
    EXPECT_EQ(fmt.GetIndentLevel(), 3);

    fmt.PopIndent(1);
    EXPECT_EQ(fmt.GetIndentLevel(), 2);

    fmt.PopIndent(5);  // Should not go below 0
    EXPECT_EQ(fmt.GetIndentLevel(), 0);
}

HWTEST_F_L0(TextFormatterTest, AutoIndent)
{
    TextFormatter fmt;
    fmt.PushIndent(2);
    fmt.AutoIndent().Text("test");
    std::string result = fmt.Str();
    // Should have 4 spaces (2 indents * 2 spaces each)
    EXPECT_EQ(result, "    test");
}

HWTEST_F_L0(TextFormatterTest, ResetIndent)
{
    TextFormatter fmt;
    fmt.PushIndent(5);
    EXPECT_EQ(fmt.GetIndentLevel(), 5);
    fmt.ResetIndent();
    EXPECT_EQ(fmt.GetIndentLevel(), 0);
}

HWTEST_F_L0(TextFormatterTest, IndentScopeRAII)
{
    TextFormatter fmt;
    EXPECT_EQ(fmt.GetIndentLevel(), 0);
    {
        IndentScope scope(fmt);
        EXPECT_EQ(fmt.GetIndentLevel(), 1);
        {
            IndentScope innerScope(fmt);
            EXPECT_EQ(fmt.GetIndentLevel(), 2);
        }
        EXPECT_EQ(fmt.GetIndentLevel(), 1);
    }
    EXPECT_EQ(fmt.GetIndentLevel(), 0);
}

// ============================================
// Test Suite 4: Clear and GetStream
// ============================================

HWTEST_F_L0(TextFormatterTest, Clear)
{
    TextFormatter fmt;
    fmt.Text("hello");
    EXPECT_EQ(fmt.Str(), "hello");
    fmt.Clear();
    EXPECT_EQ(fmt.Str(), "");
}

HWTEST_F_L0(TextFormatterTest, GetStream)
{
    TextFormatter fmt;
    fmt.GetStream() << "direct";
    EXPECT_EQ(fmt.Str(), "direct");
}

// ============================================
// Test Suite 5: Chained Calls
// ============================================

HWTEST_F_L0(TextFormatterTest, ChainedCalls)
{
    TextFormatter fmt;
    fmt.Indent(1).Label("ID").Value(100).Pipe().Label("Name").Value("test").NewLine();
    std::string result = fmt.Str();
    EXPECT_TRUE(result.find("ID:") != std::string::npos);
    EXPECT_TRUE(result.find("100") != std::string::npos);
    EXPECT_TRUE(result.find("Name:") != std::string::npos);
    EXPECT_TRUE(result.find("test") != std::string::npos);
    EXPECT_TRUE(result.find("\n") != std::string::npos);
}

HWTEST_F_L0(TextFormatterTest, ChainedCallReturnValues)
{
    TextFormatter fmt;

    TextFormatter& hexRef = fmt.Hex(0xABCD);
    EXPECT_EQ(&hexRef, &fmt);
    EXPECT_TRUE(fmt.Str().find("0xabcd") != std::string::npos);

    fmt.Clear();

    TextFormatter& sectionRef = fmt.SectionLine();
    EXPECT_EQ(&sectionRef, &fmt);
    EXPECT_EQ(fmt.Str().length(), TextFormatter::SECTION_LINE_WIDTH);

    fmt.Clear();

    TextFormatter& titleRef = fmt.CenteredTitle("Test");
    EXPECT_EQ(&titleRef, &fmt);
    EXPECT_TRUE(fmt.Str().find("Test") != std::string::npos);
}

// ============================================
// Test Suite 6: ProfileType::KindToString
// ============================================

HWTEST_F_L0(TextFormatterTest, KindToStringClassId)
{
    EXPECT_STREQ(ProfileType::KindToString(ProfileType::Kind::ClassId), "ClassId");
}

HWTEST_F_L0(TextFormatterTest, KindToStringObjectLiteralId)
{
    EXPECT_STREQ(ProfileType::KindToString(ProfileType::Kind::ObjectLiteralId), "ObjectLiteralId");
}

HWTEST_F_L0(TextFormatterTest, KindToStringArrayLiteralId)
{
    EXPECT_STREQ(ProfileType::KindToString(ProfileType::Kind::ArrayLiteralId), "ArrayLiteralId");
}

HWTEST_F_L0(TextFormatterTest, KindToStringBuiltinsId)
{
    EXPECT_STREQ(ProfileType::KindToString(ProfileType::Kind::BuiltinsId), "BuiltinsId");
}

HWTEST_F_L0(TextFormatterTest, KindToStringMethodId)
{
    EXPECT_STREQ(ProfileType::KindToString(ProfileType::Kind::MethodId), "MethodId");
}

HWTEST_F_L0(TextFormatterTest, KindToStringBuiltinFunctionId)
{
    EXPECT_STREQ(ProfileType::KindToString(ProfileType::Kind::BuiltinFunctionId), "BuiltinFunctionId");
}

HWTEST_F_L0(TextFormatterTest, KindToStringRecordClassId)
{
    EXPECT_STREQ(ProfileType::KindToString(ProfileType::Kind::RecordClassId), "RecordClassId");
}

HWTEST_F_L0(TextFormatterTest, KindToStringPrototypeId)
{
    EXPECT_STREQ(ProfileType::KindToString(ProfileType::Kind::PrototypeId), "PrototypeId");
}

HWTEST_F_L0(TextFormatterTest, KindToStringConstructorId)
{
    EXPECT_STREQ(ProfileType::KindToString(ProfileType::Kind::ConstructorId), "ConstructorId");
}

HWTEST_F_L0(TextFormatterTest, KindToStringMegaStateKinds)
{
    EXPECT_STREQ(ProfileType::KindToString(ProfileType::Kind::MegaStateKinds), "MegaStateKinds");
}

HWTEST_F_L0(TextFormatterTest, KindToStringTotalKinds)
{
    EXPECT_STREQ(ProfileType::KindToString(ProfileType::Kind::TotalKinds), "TotalKinds");
}

HWTEST_F_L0(TextFormatterTest, KindToStringUnknowId)
{
    EXPECT_STREQ(ProfileType::KindToString(ProfileType::Kind::UnknowId), "UnknowId");
}

HWTEST_F_L0(TextFormatterTest, KindToStringGlobalsId)
{
    EXPECT_STREQ(ProfileType::KindToString(ProfileType::Kind::GlobalsId), "GlobalsId");
}

HWTEST_F_L0(TextFormatterTest, KindToStringJITClassId)
{
    EXPECT_STREQ(ProfileType::KindToString(ProfileType::Kind::JITClassId), "JITClassId");
}

HWTEST_F_L0(TextFormatterTest, KindToStringTransitionClassId)
{
    EXPECT_STREQ(ProfileType::KindToString(ProfileType::Kind::TransitionClassId), "TransitionClassId");
}

HWTEST_F_L0(TextFormatterTest, KindToStringTransitionPrototypeId)
{
    EXPECT_STREQ(ProfileType::KindToString(ProfileType::Kind::TransitionPrototypeId), "TransitionPrototypeId");
}

HWTEST_F_L0(TextFormatterTest, KindToStringNapiId)
{
    EXPECT_STREQ(ProfileType::KindToString(ProfileType::Kind::NapiId), "NapiId");
}

HWTEST_F_L0(TextFormatterTest, KindToStringInvalidId)
{
    EXPECT_STREQ(ProfileType::KindToString(ProfileType::Kind::InvalidId), "InvalidId");
}

HWTEST_F_L0(TextFormatterTest, KindToStringUnknown)
{
    EXPECT_STREQ(ProfileType::KindToString(static_cast<ProfileType::Kind>(255)), "Unknown");
}

// ============================================
// Test Suite 7: ElementsKindToString
// ============================================

HWTEST_F_L0(TextFormatterTest, ElementsKindToStringNONE)
{
    EXPECT_STREQ(PGODefineOpType::ElementsKindToString(ecmascript::ElementsKind::NONE), "NONE");
}

HWTEST_F_L0(TextFormatterTest, ElementsKindToStringHOLE)
{
    EXPECT_STREQ(PGODefineOpType::ElementsKindToString(ecmascript::ElementsKind::HOLE), "HOLE");
}

HWTEST_F_L0(TextFormatterTest, ElementsKindToStringINT)
{
    EXPECT_STREQ(PGODefineOpType::ElementsKindToString(ecmascript::ElementsKind::INT), "INT");
}

HWTEST_F_L0(TextFormatterTest, ElementsKindToStringHOLE_INT)
{
    EXPECT_STREQ(PGODefineOpType::ElementsKindToString(ecmascript::ElementsKind::HOLE_INT), "HOLE_INT");
}

HWTEST_F_L0(TextFormatterTest, ElementsKindToStringNUMBER)
{
    EXPECT_STREQ(PGODefineOpType::ElementsKindToString(ecmascript::ElementsKind::NUMBER), "NUMBER");
}

HWTEST_F_L0(TextFormatterTest, ElementsKindToStringHOLE_NUMBER)
{
    EXPECT_STREQ(PGODefineOpType::ElementsKindToString(ecmascript::ElementsKind::HOLE_NUMBER), "HOLE_NUMBER");
}

HWTEST_F_L0(TextFormatterTest, ElementsKindToStringSTRING)
{
    EXPECT_STREQ(PGODefineOpType::ElementsKindToString(ecmascript::ElementsKind::STRING), "STRING");
}

HWTEST_F_L0(TextFormatterTest, ElementsKindToStringHOLE_STRING)
{
    EXPECT_STREQ(PGODefineOpType::ElementsKindToString(ecmascript::ElementsKind::HOLE_STRING), "HOLE_STRING");
}

HWTEST_F_L0(TextFormatterTest, ElementsKindToStringOBJECT)
{
    EXPECT_STREQ(PGODefineOpType::ElementsKindToString(ecmascript::ElementsKind::OBJECT), "OBJECT");
}

HWTEST_F_L0(TextFormatterTest, ElementsKindToStringHOLE_OBJECT)
{
    EXPECT_STREQ(PGODefineOpType::ElementsKindToString(ecmascript::ElementsKind::HOLE_OBJECT), "HOLE_OBJECT");
}

HWTEST_F_L0(TextFormatterTest, ElementsKindToStringTAGGED)
{
    EXPECT_STREQ(PGODefineOpType::ElementsKindToString(ecmascript::ElementsKind::TAGGED), "TAGGED");
}

HWTEST_F_L0(TextFormatterTest, ElementsKindToStringHOLE_TAGGED)
{
    EXPECT_STREQ(PGODefineOpType::ElementsKindToString(ecmascript::ElementsKind::HOLE_TAGGED), "HOLE_TAGGED");
}

HWTEST_F_L0(TextFormatterTest, ElementsKindToStringUNKNOWN)
{
    EXPECT_STREQ(PGODefineOpType::ElementsKindToString(static_cast<ecmascript::ElementsKind>(255)), "UNKNOWN");
}

HWTEST_F_L0(TextFormatterTest, PGODefineOpTypeGetTypeString)
{
    PGODefineOpType defineOp;
    TextFormatter fmt;
    defineOp.GetTypeString(fmt);
    std::string result = fmt.Str();

    // Verify label:value structure with precise assertions
    EXPECT_TRUE(result.find("local:") != std::string::npos);
    EXPECT_TRUE(result.find("ctor:") != std::string::npos);
    EXPECT_TRUE(result.find("proto:") != std::string::npos);
    EXPECT_TRUE(result.find("elementsKind:") != std::string::npos);

    // Verify elementsKind value is valid
    EXPECT_TRUE(result.find("NONE") != std::string::npos || result.find("INT") != std::string::npos
                || result.find("TAGGED") != std::string::npos || result.find("UNKNOWN") != std::string::npos);
}

HWTEST_F_L0(TextFormatterTest, PGODefineOpTypeWithElementsLengthAndSpaceFlag)
{
    PGODefineOpType defineOp;
    defineOp.SetElementsLength(100);
    defineOp.SetSpaceFlag(RegionSpaceFlag::IN_YOUNG_SPACE);
    defineOp.SetElementsKind(ElementsKind::INT);

    TextFormatter fmt;
    defineOp.GetTypeString(fmt);
    std::string result = fmt.Str();

    EXPECT_TRUE(result.find("size:") != std::string::npos);
    EXPECT_TRUE(result.find("100") != std::string::npos);
    EXPECT_TRUE(result.find("space:") != std::string::npos);
    EXPECT_TRUE(result.find("young space") != std::string::npos);
}

// ============================================
// Test Suite 8: PGOSampleType GetTypeString
// ============================================

HWTEST_F_L0(TextFormatterTest, PGOSampleTypeToString)
{
    // Test all primitive types
    EXPECT_EQ(PGOSampleType(PGOSampleType::NoneType()).GetTypeString(), "none");
    EXPECT_EQ(PGOSampleType(PGOSampleType::IntType()).GetTypeString(), "int");
    EXPECT_EQ(PGOSampleType(PGOSampleType::IntOverFlowType()).GetTypeString(), "int_overflow");
    EXPECT_EQ(PGOSampleType(PGOSampleType::DoubleType()).GetTypeString(), "double");
    EXPECT_EQ(PGOSampleType(PGOSampleType::NumberType()).GetTypeString(), "int_or_double");
    EXPECT_EQ(PGOSampleType(static_cast<PGOSampleType::Type>(7)).GetTypeString(), "int_overflow_or_double");
    EXPECT_EQ(PGOSampleType(PGOSampleType::BooleanType()).GetTypeString(), "boolean");
    EXPECT_EQ(PGOSampleType(PGOSampleType::UndefinedOrNullType()).GetTypeString(), "undefined_or_null");
    EXPECT_EQ(PGOSampleType(PGOSampleType::SpecialType()).GetTypeString(), "special");
    EXPECT_EQ(PGOSampleType(static_cast<PGOSampleType::Type>(40)).GetTypeString(), "boolean_or_special");
    EXPECT_EQ(PGOSampleType(PGOSampleType::InternStringType()).GetTypeString(), "intern_string");
    EXPECT_EQ(PGOSampleType(PGOSampleType::StringType()).GetTypeString(), "string");
    EXPECT_EQ(PGOSampleType(PGOSampleType::NumberOrStringType()).GetTypeString(), "number_or_string");
    EXPECT_EQ(PGOSampleType(PGOSampleType::BigIntType()).GetTypeString(), "big_int");
    EXPECT_EQ(PGOSampleType(PGOSampleType::HeapObjectType()).GetTypeString(), "heap_object");
    EXPECT_EQ(PGOSampleType(static_cast<PGOSampleType::Type>(272)).GetTypeString(), "heap_or_undefined_or_null");
    EXPECT_EQ(PGOSampleType(PGOSampleType::AnyType()).GetTypeString(), "any");

    // Test invalid value
    EXPECT_TRUE(PGOSampleType(static_cast<PGOSampleType::Type>(2000)).GetTypeString().find("2000")
                != std::string::npos);
}

// ============================================
// Test Suite 9: ProfileType GetTypeString
// ============================================

HWTEST_F_L0(TextFormatterTest, ProfileTypeGetTypeString)
{
    ProfileType pt(1, 100, ProfileType::Kind::ClassId, true, false);
    std::string result = pt.GetTypeString();

    // Verify format: (kind: ClassId, abcId: 1, id: 100, root: Y, everOOB: N)
    EXPECT_TRUE(result.find("kind: ClassId") != std::string::npos);
    EXPECT_TRUE(result.find("abcId: 1") != std::string::npos);
    EXPECT_TRUE(result.find("id: 100") != std::string::npos);
    EXPECT_TRUE(result.find("root: Y") != std::string::npos);
    EXPECT_TRUE(result.find("everOOB: N") != std::string::npos);
}

HWTEST_F_L0(TextFormatterTest, ProfileTypeNone)
{
    ProfileType pt = ProfileType::PROFILE_TYPE_NONE;
    EXPECT_TRUE(pt.IsNone());
}

// ============================================
// Test Suite 10: Constants
// ============================================

HWTEST_F_L0(TextFormatterTest, ConstantsValues)
{
    EXPECT_STREQ(TextFormatter::NEW_LINE, "\n");
    EXPECT_STREQ(TextFormatter::PIPE, " | ");
    EXPECT_STREQ(TextFormatter::INDENT, "  ");
    EXPECT_EQ(TextFormatter::LABEL_WIDTH_LARGE, 20U);
    EXPECT_EQ(TextFormatter::LABEL_WIDTH_MEDIUM, 16U);
    EXPECT_EQ(TextFormatter::LABEL_WIDTH_SMALL, 8U);
    EXPECT_EQ(TextFormatter::SECTION_LINE_WIDTH, 80U);
}

// ============================================
// Test Suite 11: ProcessToText Pattern
// ============================================

HWTEST_F_L0(TextFormatterTest, ProcessToTextSharedFormatter)
{
    // Test the pattern where a single TextFormatter is shared across multiple ProcessToText calls
    TextFormatter fmt;

    // Simulate first ProcessToText call
    fmt.Text("[Section 1]").NewLine();
    fmt.PushIndent();
    fmt.AutoIndent().Label("Key1").Value("Value1").NewLine();
    fmt.PopIndent();

    // Simulate second ProcessToText call appending to same formatter
    fmt.Text("[Section 2]").NewLine();
    fmt.PushIndent();
    fmt.AutoIndent().Label("Key2").Value("Value2").NewLine();
    fmt.PopIndent();

    std::string result = fmt.Str();

    // Verify both sections are in the output
    EXPECT_TRUE(result.find("[Section 1]") != std::string::npos);
    EXPECT_TRUE(result.find("[Section 2]") != std::string::npos);
    EXPECT_TRUE(result.find("Key1: Value1") != std::string::npos);
    EXPECT_TRUE(result.find("Key2: Value2") != std::string::npos);
}

HWTEST_F_L0(TextFormatterTest, ProcessToTextIndentPersistence)
{
    // Test that indent level persists correctly across "ProcessToText" calls
    TextFormatter fmt;

    // First "method" sets indent to 1
    fmt.PushIndent();
    fmt.AutoIndent().Text("Level1").NewLine();

    // Simulate nested "ProcessToText" call
    fmt.PushIndent();
    fmt.AutoIndent().Text("Level2").NewLine();
    fmt.PopIndent();

    // Back to level 1
    fmt.AutoIndent().Text("BackToLevel1").NewLine();
    fmt.PopIndent();

    std::string result = fmt.Str();

    // Level1 should have 2 spaces (1 indent)
    EXPECT_TRUE(result.find("  Level1") != std::string::npos);
    // Level2 should have 4 spaces (2 indents)
    EXPECT_TRUE(result.find("    Level2") != std::string::npos);
    // BackToLevel1 should have 2 spaces (1 indent)
    EXPECT_TRUE(result.find("  BackToLevel1") != std::string::npos);
}

HWTEST_F_L0(TextFormatterTest, ProcessToTextOutputToStream)
{
    // Test the pattern of writing TextFormatter content to stream at the end
    TextFormatter fmt;
    fmt.Text("[Header]").NewLine();
    fmt.Indent().Label("Version").Value("1.0").NewLine();
    fmt.Text("[Body]").NewLine();
    fmt.Indent().Text("Content").NewLine();

    std::ostringstream oss;
    oss << fmt.Str();

    std::string result = oss.str();
    EXPECT_TRUE(result.find("[Header]") != std::string::npos);
    EXPECT_TRUE(result.find("Version: 1.0") != std::string::npos);
    EXPECT_TRUE(result.find("[Body]") != std::string::npos);
    EXPECT_TRUE(result.find("Content") != std::string::npos);
}

// ============================================
// Test Suite 7: Boundary Conditions
// ============================================

HWTEST_F_L0(TextFormatterTest, EmptyStringText)
{
    TextFormatter fmt;
    fmt.Text("");
    EXPECT_EQ(fmt.Str(), "");
}

HWTEST_F_L0(TextFormatterTest, EmptyStringLabel)
{
    TextFormatter fmt;
    fmt.Label("").Value("");
    EXPECT_EQ(fmt.Str(), ": ");
}

HWTEST_F_L0(TextFormatterTest, VeryLongString)
{
    TextFormatter fmt;
    std::string longStr(1000, 'x');
    fmt.Text(longStr);
    EXPECT_EQ(fmt.Str().length(), 1000U);
}

HWTEST_F_L0(TextFormatterTest, LeftAlignLongString)
{
    TextFormatter fmt;
    std::string longStr(100, 'c');
    fmt.Left(longStr, 50);
    // When string is longer than width, it should not be truncated by Left
    EXPECT_EQ(fmt.Str().length(), 100U);
}

HWTEST_F_L0(TextFormatterTest, RightAlignLongString)
{
    TextFormatter fmt;
    std::string longStr(100, 'd');
    fmt.Right(longStr, 50);
    // When string is longer than width, it should not be truncated by Right
    EXPECT_EQ(fmt.Str().length(), 100U);
}

HWTEST_F_L0(TextFormatterTest, MultipleNewLines)
{
    TextFormatter fmt;
    fmt.NewLine().NewLine().NewLine();
    EXPECT_EQ(fmt.Str(), "\n\n\n");
}

HWTEST_F_L0(TextFormatterTest, MultiplePipes)
{
    TextFormatter fmt;
    fmt.Pipe().Pipe().Pipe();
    EXPECT_EQ(fmt.Str(), " |  |  | ");
}

HWTEST_F_L0(TextFormatterTest, DeepIndent)
{
    TextFormatter fmt;
    for (int i = 0; i < 100; i++) {
        fmt.PushIndent();
    }
    EXPECT_EQ(fmt.GetIndentLevel(), 100);
    fmt.AutoIndent().Text("deep");
    // 100 indents * 2 spaces each = 200 spaces + "deep"
    EXPECT_EQ(fmt.Str().length(), 204U);
}

HWTEST_F_L0(TextFormatterTest, PopIndentBeyondZero)
{
    TextFormatter fmt;
    fmt.PushIndent(2);
    fmt.PopIndent(10);
    EXPECT_EQ(fmt.GetIndentLevel(), 0);
    fmt.PopIndent(5);
    EXPECT_EQ(fmt.GetIndentLevel(), 0);
}

// ============================================
// Test Suite 8: PGOProfilerHeader Integration Test
// ============================================

HWTEST_F_L0(TextFormatterTest, ProfilerHeaderStyleOutput)
{
    // Simulate the output format used by PGOProfilerHeader::ProcessToText
    TextFormatter fmt;
    fmt.SectionLine().NewLine();
    fmt.CenteredTitle("PGO Profile File Info").NewLine();
    fmt.SectionLine().NewLine();
    fmt.NewLine();
    fmt.Text("[Header Information]").NewLine();
    fmt.SetLabelWidth(TextFormatter::LABEL_WIDTH_LARGE).LabelAlign();
    fmt.PushIndent();
    fmt.AutoIndent().Label("Profiler Version").Value("12.0.6.0").NewLine();
    fmt.AutoIndent().Label("Compatible Version").Value("12.0.6.0").NewLine();
    fmt.AutoIndent().Label("File Size").Value("1024 bytes").NewLine();
    fmt.AutoIndent().Label("Header Size").Value("64 bytes").NewLine();
    fmt.AutoIndent().Label("Checksum").Hex(0xABCD1234).NewLine();
    fmt.PopIndent();
    fmt.LabelReset();
    fmt.NewLine();

    std::string result = fmt.Str();

    // Verify section lines
    EXPECT_TRUE(result.find(std::string(80, '=')) != std::string::npos);
    // Verify centered title
    EXPECT_TRUE(result.find("PGO Profile File Info") != std::string::npos);
    // Verify header section
    EXPECT_TRUE(result.find("[Header Information]") != std::string::npos);
    // Verify label-value pairs (labels are padded when LabelAlign is enabled)
    EXPECT_TRUE(result.find("Profiler Version") != std::string::npos);
    EXPECT_TRUE(result.find("12.0.6.0") != std::string::npos);
    EXPECT_TRUE(result.find("Compatible Version") != std::string::npos);
    EXPECT_TRUE(result.find("File Size") != std::string::npos);
    EXPECT_TRUE(result.find("1024 bytes") != std::string::npos);
    EXPECT_TRUE(result.find("Header Size") != std::string::npos);
    EXPECT_TRUE(result.find("64 bytes") != std::string::npos);
    EXPECT_TRUE(result.find("Checksum") != std::string::npos);
    EXPECT_TRUE(result.find("0xabcd1234") != std::string::npos);
}

HWTEST_F_L0(TextFormatterTest, RecordPoolStyleOutput)
{
    // Simulate the output format used by record pool ProcessToText
    TextFormatter fmt;
    fmt.SectionLine().NewLine();
    fmt.CenteredTitle("Pools Info").NewLine();
    fmt.SectionLine().NewLine();
    fmt.NewLine();
    fmt.Text("[Record Pool] (").Text(std::to_string(3)).Text(" entries)").NewLine();
    fmt.Indent().Text("Type1").Text(" -> ").Text("Record1").NewLine();
    fmt.Indent().Text("Type2").Text(" -> ").Text("Record2").NewLine();
    fmt.Indent().Text("Type3").Text(" -> ").Text("Record3").NewLine();
    fmt.NewLine();

    std::string result = fmt.Str();

    EXPECT_TRUE(result.find("Pools Info") != std::string::npos);
    EXPECT_TRUE(result.find("[Record Pool] (3 entries)") != std::string::npos);
    EXPECT_TRUE(result.find("Type1 -> Record1") != std::string::npos);
    EXPECT_TRUE(result.find("Type2 -> Record2") != std::string::npos);
    EXPECT_TRUE(result.find("Type3 -> Record3") != std::string::npos);
}

HWTEST_F_L0(TextFormatterTest, MethodInfoStyleOutput)
{
    // Simulate the output format used by PGOMethodInfoMap::ProcessToText
    TextFormatter fmt;
    fmt.SetLabelWidth(TextFormatter::LABEL_WIDTH_MEDIUM);
    fmt.SectionLine().NewLine();
    fmt.Indent().Label("Record").Value("com.example.app").Pipe();
    fmt.Label("Methods").Value(5).Pipe();
    fmt.Label("Total Calls").Value(1000).NewLine();
    fmt.SectionLine().NewLine();

    fmt.PushIndent();
    fmt.NewLine();
    fmt.AutoIndent().Text("[Method]").Indent();
    fmt.Label("Name").Value("foo").Pipe();
    fmt.Label("Method ID").Value(123).Pipe();
    fmt.Label("Count").Value(500).Pipe();
    fmt.Label("Mode").Value("CALL").Pipe();
    fmt.Label("Checksum").Value("0x12345678").NewLine();
    fmt.PopIndent();

    std::string result = fmt.Str();

    EXPECT_TRUE(result.find("Record:") != std::string::npos);
    EXPECT_TRUE(result.find("com.example.app") != std::string::npos);
    EXPECT_TRUE(result.find("Methods:") != std::string::npos);
    EXPECT_TRUE(result.find("Total Calls:") != std::string::npos);
    EXPECT_TRUE(result.find("[Method]") != std::string::npos);
    EXPECT_TRUE(result.find("Name:") != std::string::npos);
    EXPECT_TRUE(result.find("foo") != std::string::npos);
    EXPECT_TRUE(result.find("Method ID:") != std::string::npos);
    EXPECT_TRUE(result.find("123") != std::string::npos);
    EXPECT_TRUE(result.find("Count:") != std::string::npos);
    EXPECT_TRUE(result.find("500") != std::string::npos);
    EXPECT_TRUE(result.find("Mode:") != std::string::npos);
    EXPECT_TRUE(result.find("CALL") != std::string::npos);
    EXPECT_TRUE(result.find("Checksum:") != std::string::npos);
}

HWTEST_F_L0(TextFormatterTest, PandaFileInfoStyleOutput)
{
    // Simulate the output format used by PGOPandaFileInfos::ProcessToText
    TextFormatter fmt;
    fmt.Text("[Panda Files]").NewLine();
    fmt.Indent()
        .Right("ABC ID", TextFormatter::COL_WIDTH_ABC_ID)
        .Pipe()
        .Right("Checksum", TextFormatter::COL_WIDTH_CHECKSUM)
        .NewLine();
    fmt.Indent()
        .Text(std::string(TextFormatter::COL_WIDTH_ABC_ID, '-'))
        .Pipe()
        .Text(std::string(TextFormatter::COL_WIDTH_CHECKSUM, '-'))
        .NewLine();
    fmt.Indent()
        .Right(1, TextFormatter::COL_WIDTH_ABC_ID)
        .Pipe()
        .Right(TextFormatter::HexStr(0x12345678), TextFormatter::COL_WIDTH_CHECKSUM)
        .NewLine();
    fmt.Indent()
        .Right(2, TextFormatter::COL_WIDTH_ABC_ID)
        .Pipe()
        .Right(TextFormatter::HexStr(0xABCDEF00), TextFormatter::COL_WIDTH_CHECKSUM)
        .NewLine();

    std::string result = fmt.Str();

    EXPECT_TRUE(result.find("[Panda Files]") != std::string::npos);
    EXPECT_TRUE(result.find("ABC ID") != std::string::npos);
    EXPECT_TRUE(result.find("Checksum") != std::string::npos);
    EXPECT_TRUE(result.find("--------") != std::string::npos);
    EXPECT_TRUE(result.find("0x12345678") != std::string::npos);
    EXPECT_TRUE(result.find("0xabcdef00") != std::string::npos);
}

// ============================================
// Test Suite 12: ProcessToText Direct Call Tests
// ============================================

HWTEST_F_L0(TextFormatterTest, PGOPandaFileInfosProcessToText)
{
    // Directly test PGOPandaFileInfos::ProcessToText
    PGOPandaFileInfos infos;
    infos.Sample(0x12345678, 1);
    infos.Sample(0xABCDEF00, 2);

    TextFormatter fmt;
    infos.ProcessToText(fmt);
    std::string result = fmt.Str();

    // Verify output format
    EXPECT_TRUE(result.find("[Panda Files]") != std::string::npos);
    EXPECT_TRUE(result.find("ABC ID") != std::string::npos);
    EXPECT_TRUE(result.find("Checksum") != std::string::npos);
    EXPECT_TRUE(result.find("0x12345678") != std::string::npos);
    EXPECT_TRUE(result.find("0xabcdef00") != std::string::npos);
}

HWTEST_F_L0(TextFormatterTest, PGOPandaFileInfosProcessToTextEmpty)
{
    // Test empty PGOPandaFileInfos
    PGOPandaFileInfos infos;

    TextFormatter fmt;
    infos.ProcessToText(fmt);
    std::string result = fmt.Str();

    // Even empty should have header
    EXPECT_TRUE(result.find("[Panda Files]") != std::string::npos);
    EXPECT_TRUE(result.find("ABC ID") != std::string::npos);
}

// ============================================
// Test Suite 13: Weight Methods Tests
// ============================================

HWTEST_F_L0(TextFormatterTest, PGOSampleTypeGetTrueWeight)
{
    // Create a PGOSampleType with weight
    // Weight is stored in upper bits: true weight in upper 11 bits, false weight in lower 11 bits
    // weightStartBit = 10, weightBits = 11, WEIGHT_MASK = 0x7FF (2047)
    constexpr uint32_t weightStartBit = 10;
    constexpr uint32_t weightBits = 11;

    uint32_t trueWeight = 100;
    uint32_t falseWeight = 50;
    uint32_t combinedWeight = (trueWeight << weightBits) | falseWeight;
    uint32_t typeWithWeight
        = (combinedWeight << weightStartBit) | static_cast<uint32_t>(PGOSampleType::Type::INT);

    PGOSampleType type(typeWithWeight);

    EXPECT_EQ(type.GetTrueWeight(), trueWeight);
}

HWTEST_F_L0(TextFormatterTest, PGOSampleTypeGetFalseWeight)
{
    constexpr uint32_t weightStartBit = 10;
    constexpr uint32_t weightBits = 11;

    uint32_t trueWeight = 100;
    uint32_t falseWeight = 50;
    uint32_t combinedWeight = (trueWeight << weightBits) | falseWeight;
    uint32_t typeWithWeight
        = (combinedWeight << weightStartBit) | static_cast<uint32_t>(PGOSampleType::Type::INT);

    PGOSampleType type(typeWithWeight);

    EXPECT_EQ(type.GetFalseWeight(), falseWeight);
}

HWTEST_F_L0(TextFormatterTest, PGOSampleTypeGetTypeStringWithWeight)
{
    constexpr uint32_t weightStartBit = 10;
    constexpr uint32_t weightBits = 11;

    uint32_t trueWeight = 100;
    uint32_t falseWeight = 50;
    uint32_t combinedWeight = (trueWeight << weightBits) | falseWeight;
    uint32_t typeWithWeight
        = (combinedWeight << weightStartBit) | static_cast<uint32_t>(PGOSampleType::Type::INT);

    PGOSampleType type(typeWithWeight);
    std::string result = type.GetTypeString();

    // Should contain weight information
    EXPECT_TRUE(result.find("weight") != std::string::npos);
    EXPECT_TRUE(result.find("true=100") != std::string::npos);
    EXPECT_TRUE(result.find("false=50") != std::string::npos);
}

// ============================================
// Test Suite 14: PGOObjectInfo Tests
// ============================================

HWTEST_F_L0(TextFormatterTest, PGOObjectInfoGetInfoString)
{
    PGOObjectInfo info;
    TextFormatter fmt;
    info.GetInfoString(fmt);
    std::string result = fmt.Str();

    // Should output receiver and hold info
    EXPECT_TRUE(result.find("receiverRoot") != std::string::npos);
    EXPECT_TRUE(result.find("receiver") != std::string::npos);
    EXPECT_TRUE(result.find("holdRoot") != std::string::npos);
    EXPECT_TRUE(result.find("hold") != std::string::npos);
}

// ============================================
// Test Suite 15: PGOProtoTransitionType Tests
// ============================================

HWTEST_F_L0(TextFormatterTest, PGOProtoTransitionTypeGetTypeString)
{
    PGOProtoTransitionType transType;
    std::string result = transType.GetTypeString();

    // Should contain ihc, base, trans info
    EXPECT_TRUE(result.find("ihc") != std::string::npos);
    EXPECT_TRUE(result.find("base") != std::string::npos);
    EXPECT_TRUE(result.find("transIhc") != std::string::npos);
    EXPECT_TRUE(result.find("transProto") != std::string::npos);
}

HWTEST_F_L0(TextFormatterTest, PGOProtoTransitionPoolProcessToText)
{
    PGOProtoTransitionPool pool;

    ProfileType ihcType(1, 100, ProfileType::Kind::ClassId);
    ProfileType transType(1, 200, ProfileType::Kind::ClassId);
    PGOProtoTransitionType protoTransType(ihcType);
    protoTransType.SetTransitionType(transType);
    EXPECT_FALSE(protoTransType.IsNone());

    pool.Add(protoTransType);

    TextFormatter fmt;
    bool result = pool.ProcessToText(fmt);
    EXPECT_TRUE(result);

    std::string output = fmt.Str();
    EXPECT_TRUE(output.find("[Proto Transition Pool]") != std::string::npos);
    EXPECT_TRUE(output.find("1 entries") != std::string::npos);
    EXPECT_TRUE(output.find("ihc") != std::string::npos);
}

HWTEST_F_L0(TextFormatterTest, PGOProtoTransitionPoolProcessToTextEmpty)
{
    PGOProtoTransitionPool pool;

    TextFormatter fmt;
    bool result = pool.ProcessToText(fmt);
    EXPECT_TRUE(result);
    EXPECT_EQ(fmt.Str(), "");
}

// ============================================
// Test Suite 16: Integration Tests with JS Execution
// ============================================

HWTEST_F_L0(TextFormatterTest, SaveAPTextFileIntegration)
{
    const char* dir = "ark-profiler-saveaptextfile/";
    std::string apPath = GenerateProfile(dir, "sample_test.abc", "sample_test");
    ASSERT_FALSE(apPath.empty());

    std::string txtPath = std::string(dir) + "modules.txt";
    PGOProfilerDecoder decoder(apPath.c_str(), TEST_THRESHOLD);
    std::shared_ptr<PGOAbcFilePool> abcFilePool = std::make_shared<PGOAbcFilePool>();
    ASSERT_TRUE(decoder.LoadFull(abcFilePool));
    EXPECT_TRUE(decoder.SaveAPTextFile(txtPath));

    std::ifstream textFile(txtPath);
    ASSERT_TRUE(textFile.is_open());
    std::stringstream buffer;
    buffer << textFile.rdbuf();
    std::string textContent = buffer.str();
    textFile.close();

    EXPECT_TRUE(textContent.find("PGO Profile File Info") != std::string::npos);
    EXPECT_TRUE(textContent.find("Profiler Version") != std::string::npos);
    EXPECT_TRUE(textContent.find("[Panda Files]") != std::string::npos);
    EXPECT_TRUE(textContent.find("Methods Summary") != std::string::npos);

    unlink(txtPath.c_str());
    CleanupDir(dir);
}

HWTEST_F_L0(TextFormatterTest, PGORecordDetailInfosProcessToText)
{
    std::string apPath = GenerateProfile("ark-profiler-record/", "sample_test.abc", "sample_test");
    ASSERT_FALSE(apPath.empty());

    PGOProfilerDecoder decoder(apPath.c_str(), TEST_THRESHOLD);
    std::shared_ptr<PGOAbcFilePool> abcFilePool = std::make_shared<PGOAbcFilePool>();
    ASSERT_TRUE(decoder.LoadFull(abcFilePool));

    TextFormatter fmt;
    decoder.GetRecordDetailInfos().ProcessToText(fmt);
    std::string textResult = fmt.Str();

    EXPECT_TRUE(textResult.find("Methods Summary") != std::string::npos);
    EXPECT_TRUE(textResult.find("Total Records") != std::string::npos);
    EXPECT_TRUE(textResult.find("Total Methods") != std::string::npos);

    CleanupDir("ark-profiler-record/");
}

HWTEST_F_L0(TextFormatterTest, CollectOverallStatsIntegration)
{
    std::string apPath = GenerateProfile("ark-profiler-stats/", "sample_test.abc", "sample_test");
    ASSERT_FALSE(apPath.empty());

    PGOProfilerDecoder decoder(apPath.c_str(), TEST_THRESHOLD);
    std::shared_ptr<PGOAbcFilePool> abcFilePool = std::make_shared<PGOAbcFilePool>();
    ASSERT_TRUE(decoder.LoadFull(abcFilePool));

    auto stats = decoder.GetRecordDetailInfos().CollectOverallStats();
    EXPECT_GT(stats.totalRecords, 0U);
    EXPECT_GT(stats.totalMethods, 0U);
    EXPECT_GE(stats.hotMethods, 0U);
    EXPECT_GT(stats.totalCalls, 0ULL);

    CleanupDir("ark-profiler-stats/");
}

// ============================================
// Test Suite 17: Empty Pool Tests
// ============================================

HWTEST_F_L0(TextFormatterTest, EmptyRecordPoolProcessToText)
{
    PGORecordPool pool;
    TextFormatter fmt;
    bool result = pool.ProcessToText(fmt);
    EXPECT_TRUE(result);
    EXPECT_EQ(fmt.Str(), "");
}

HWTEST_F_L0(TextFormatterTest, EmptyProfileTypePoolProcessToText)
{
    PGOProfileTypePool pool;
    TextFormatter fmt;
    bool result = pool.GetPool()->ProcessToText(fmt);
    EXPECT_TRUE(result);
    EXPECT_EQ(fmt.Str(), "");
}

// ============================================
// Test Suite 18: Multiple RWType Comma Separation
// ============================================

HWTEST_F_L0(TextFormatterTest, PGOMethodTypeSetMultipleRWTypes)
{
    PGOMethodTypeSet typeSet;

    ProfileType type1(ApEntityId(0), 100, ProfileType::Kind::ClassId);
    ProfileType type2(ApEntityId(0), 200, ProfileType::Kind::ClassId);
    PGOObjectInfo info1(type1);
    PGOObjectInfo info2(type2);

    uint32_t offset = 10;
    typeSet.AddObjectInfo(offset, info1);
    typeSet.AddObjectInfo(offset, info2);

    TextFormatter fmt;
    typeSet.ProcessToText(fmt);

    std::string result = fmt.Str();
    EXPECT_TRUE(result.find(", ") != std::string::npos);
    EXPECT_TRUE(result.find("RWType") != std::string::npos);
}

// ============================================
// Test Suite 19: PGOProfilerHeader ProcessToText Failure
// ============================================

HWTEST_F_L0(TextFormatterTest, ProfilerHeaderProcessToTextVerifyFails)
{
    PGOProfilerHeader header;
    // Version > LAST_VERSION makes Verify() fail
    base::FileHeaderBase::VersionType invalidVersion = {0, 0, 0, 17};
    header.SetVersion(invalidVersion);

    TextFormatter fmt;
    EXPECT_FALSE(header.ProcessToText(fmt));
    EXPECT_EQ(fmt.Str(), "");
}

// ============================================
// Test Suite 20: IsFilter else branch coverage
// ============================================

HWTEST_F_L0(TextFormatterTest, CollectStatsWithHighThreshold)
{
    std::string apPath = GenerateProfile("ark-profiler-filter/", "sample_test.abc", "sample_test");
    ASSERT_FALSE(apPath.empty());

    // High threshold filters CALL_MODE methods (HOTNESS_MODE bypasses filter)
    PGOProfilerDecoder decoder(apPath.c_str(), HIGH_THRESHOLD);
    std::shared_ptr<PGOAbcFilePool> abcFilePool = std::make_shared<PGOAbcFilePool>();
    ASSERT_TRUE(decoder.LoadFull(abcFilePool));

    auto stats = decoder.GetRecordDetailInfos().CollectOverallStats();
    EXPECT_GT(stats.totalMethods, 0U);
    EXPECT_LE(stats.hotMethods, stats.totalMethods);

    CleanupDir("ark-profiler-filter/");
}

// ============================================
// Test Suite 21: maxCalls else branch coverage
// ============================================

HWTEST_F_L0(TextFormatterTest, CollectOverallStatsMultipleRecords)
{
    std::string ap1 = GenerateProfile("ark-profiler-multi1/", "sample_test.abc", "sample_test");
    std::string ap2 = GenerateProfile("ark-profiler-multi2/", "string_equal.abc", "string_equal");
    std::string ap3 = GenerateProfile("ark-profiler-multi3/", "object_literal.abc", "object_literal");
    ASSERT_FALSE(ap1.empty() || ap2.empty() || ap3.empty());

    PGOProfilerDecoder decoder1(ap1.c_str(), TEST_THRESHOLD);
    PGOProfilerDecoder decoder2(ap2.c_str(), TEST_THRESHOLD);
    PGOProfilerDecoder decoder3(ap3.c_str(), TEST_THRESHOLD);
    std::shared_ptr<PGOAbcFilePool> pool1 = std::make_shared<PGOAbcFilePool>();
    std::shared_ptr<PGOAbcFilePool> pool2 = std::make_shared<PGOAbcFilePool>();
    std::shared_ptr<PGOAbcFilePool> pool3 = std::make_shared<PGOAbcFilePool>();
    ASSERT_TRUE(decoder1.LoadFull(pool1) && decoder2.LoadFull(pool2) && decoder3.LoadFull(pool3));

    // Merge to create multiple records, covers maxCalls else branch
    decoder1.GetRecordDetailInfos().Merge(decoder2.GetRecordDetailInfos());
    decoder1.GetRecordDetailInfos().Merge(decoder3.GetRecordDetailInfos());

    auto stats = decoder1.GetRecordDetailInfos().CollectOverallStats();
    EXPECT_GE(stats.totalRecords, 2U);
    EXPECT_GT(stats.totalMethods, 0U);
    EXPECT_GT(stats.totalCalls, 0ULL);

    CleanupDir("ark-profiler-multi1/");
    CleanupDir("ark-profiler-multi2/");
    CleanupDir("ark-profiler-multi3/");
}

// ============================================
// Test Suite 22: ProcessToText else branch coverage
// ============================================

HWTEST_F_L0(TextFormatterTest, ProcessToTextEmptyRecordDetailInfos)
{
    // Empty infos covers: totalMethods==0 and hclassTreeDescInfos empty
    PGORecordDetailInfos emptyInfos(TEST_THRESHOLD);

    TextFormatter fmt;
    emptyInfos.ProcessToText(fmt);
    std::string result = fmt.Str();

    EXPECT_TRUE(result.find("Methods Summary") != std::string::npos);
    EXPECT_TRUE(result.find("Average Calls") == std::string::npos);
    EXPECT_TRUE(result.find("HClass Tree Desc") == std::string::npos);
}

HWTEST_F_L0(TextFormatterTest, PGOMethodTypeSetProcessToTextEmpty)
{
    // Empty type set covers hasOutput==false branch
    PGOMethodTypeSet emptyTypeSet;

    TextFormatter fmt;
    emptyTypeSet.ProcessToText(fmt);
    EXPECT_EQ(fmt.Str(), "");
}

} // namespace panda::test
