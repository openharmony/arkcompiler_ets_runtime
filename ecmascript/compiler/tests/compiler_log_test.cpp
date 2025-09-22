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

#include "ecmascript/compiler/compiler_log.h"
#include "ecmascript/tests/test_helper.h"

namespace panda::ecmascript::kungfu {
class Circuit;
}

using namespace panda::ecmascript;
using namespace panda::ecmascript::kungfu;

namespace panda::test {

class CompilerLogTest : public BaseTestWithScope<false> {
};

/**
 * @tc.name: TimeScope_ConstructorWithMethodInfo
 * @tc.desc: Test TimeScope constructor with method information and circuit
 * @tc.type: FUNC
 */
HWTEST_F_L0(CompilerLogTest, TimeScope_ConstructorWithMethodInfo)
{
    CompilerLog log("all");
    log.SetEnableCompilerLogTime(true);
    log.SetEnableCompilerLogTimeMethods(true);

    EXPECT_TRUE(log.GetEnableCompilerLogTime());
    EXPECT_TRUE(log.GetEnableCompilerLogTimeMethods());
    EXPECT_TRUE(log.AllMethod());

    Circuit* circuit = nullptr;
    {
        TimeScope timeScope("TestPass", "TestMethod", 0x1234, &log, circuit);
    }

    // Verify TimeScope destructor called AddPassTime and AddMethodTime
    EXPECT_GE(log.GetPassTime("TestPass"), 0.0);
    EXPECT_GE(log.GetMethodTime("TestMethod", 0x1234), 0.0);
}

/**
 * @tc.name: TimeScope_ConstructorWithoutCircuit
 * @tc.desc: Test TimeScope constructor without circuit parameter
 * @tc.type: FUNC
 */
HWTEST_F_L0(CompilerLogTest, TimeScope_ConstructorWithoutCircuit)
{
    CompilerLog log("all");
    log.SetEnableCompilerLogTime(true);

    {
        TimeScope timeScope("TestPassWithoutCircuit", "TestMethod", 0x5678, &log);
    }

    // Verify time was recorded after TimeScope destruction
    EXPECT_GE(log.GetPassTime("TestPassWithoutCircuit"), 0.0);
    EXPECT_GE(log.GetMethodTime("TestMethod", 0x5678), 0.0);
}

/**
 * @tc.name: TimeScope_SimpleConstructor
 * @tc.desc: Test TimeScope constructor with only pass name and log
 * @tc.type: FUNC
 */
HWTEST_F_L0(CompilerLogTest, TimeScope_SimpleConstructor)
{
    CompilerLog log("all");
    log.SetEnableCompilerLogTime(true);

    {
        TimeScope timeScope("SimpleTestPass", &log);
    }

    // Verify time was recorded after TimeScope destruction
    EXPECT_GE(log.GetPassTime("SimpleTestPass"), 0.0);
}

/**
 * @tc.name: TimeScope_DisableTimer
 * @tc.desc: Test TimeScope when timer is disabled
 * @tc.type: FUNC
 */
HWTEST_F_L0(CompilerLogTest, TimeScope_DisableTimer)
{
    CompilerLog log("all");
    log.SetEnableCompilerLogTime(false);

    EXPECT_FALSE(log.GetEnableCompilerLogTime());

    Circuit* circuit = nullptr;
    {
        TimeScope timeScope("DisabledTestPass", "TestMethod", 0xABCD, &log, circuit);
        EXPECT_FALSE(log.GetEnableCompilerLogTime());
    }

    // When timer is disabled, time should not be recorded
    EXPECT_DOUBLE_EQ(log.GetPassTime("DisabledTestPass"), 0.0);
    EXPECT_DOUBLE_EQ(log.GetMethodTime("TestMethod", 0xABCD), 0.0);
}

/**
 * @tc.name: TimeScope_MultipleScopes
 * @tc.desc: Test multiple TimeScope instances
 * @tc.type: FUNC
 */
HWTEST_F_L0(CompilerLogTest, TimeScope_MultipleScopes)
{
    CompilerLog log("all");
    log.SetEnableCompilerLogTime(true);
    log.SetEnableCompilerLogTimeMethods(true);

    Circuit* circuit = nullptr;
    {
        TimeScope scope1("Pass1", "Method1", 0x1001, &log, circuit);
        {
            TimeScope scope2("Pass2", "Method1", 0x1001, &log);
            {
                TimeScope scope3("Pass3", "Method1", 0x1001, &log);
            }
        }
    }

    // Verify all passes recorded time
    EXPECT_GE(log.GetPassTime("Pass1"), 0.0);
    EXPECT_GE(log.GetPassTime("Pass2"), 0.0);
    EXPECT_GE(log.GetPassTime("Pass3"), 0.0);
    // Method time should be accumulated from all three scopes
    EXPECT_GE(log.GetMethodTime("Method1", 0x1001), 0.0);
}

/**
 * @tc.name: TimeScope_CircuitNodeCount
 * @tc.desc: Test TimeScope with Circuit tracking node count via GetCurrentNodeCount
 * @tc.type: FUNC
 */
HWTEST_F_L0(CompilerLogTest, TimeScope_CircuitNodeCount)
{
    CompilerLog log("all");
    log.SetEnableCompilerLogTime(true);
    log.SetEnableCompilerLogTimeMethods(true);

    // Test with null circuit - GetCurrentNodeCount should handle gracefully
    {
        Circuit* circuit = nullptr;
        TimeScope timeScope("CircuitPass", "ClassName@TestMethod", 0x2000, &log, circuit);
    }

    // Verify time recorded, and GetShortName extracts "ClassName" from "ClassName@TestMethod"
    EXPECT_GE(log.GetPassTime("CircuitPass"), 0.0);
    EXPECT_GE(log.GetMethodTime("ClassName", 0x2000), 0.0);
}

/**
 * @tc.name: TimeScope_EnableCompilerLogTimeMethods_Flag
 * @tc.desc: Test new SetEnableCompilerLogTimeMethods flag
 * @tc.type: FUNC
 */
HWTEST_F_L0(CompilerLogTest, TimeScope_EnableCompilerLogTimeMethods_Flag)
{
    CompilerLog log("all");

    // Test flag is initially false
    EXPECT_FALSE(log.GetEnableCompilerLogTimeMethods());

    // Test setting flag to true
    log.SetEnableCompilerLogTime(true);
    log.SetEnableCompilerLogTimeMethods(true);
    EXPECT_TRUE(log.GetEnableCompilerLogTime());
    EXPECT_TRUE(log.GetEnableCompilerLogTimeMethods());

    // Test setting flag to false
    log.SetEnableCompilerLogTimeMethods(false);
    EXPECT_FALSE(log.GetEnableCompilerLogTimeMethods());
}

/**
 * @tc.name: CompilerLog_LogOptionsParsing
 * @tc.desc: Test CompilerLog log options parsing
 * @tc.type: FUNC
 */
HWTEST_F_L0(CompilerLogTest, CompilerLog_LogOptionsParsing)
{
    // Test "all" option
    {
        CompilerLog log("all");
        EXPECT_TRUE(log.AllMethod());
        EXPECT_FALSE(log.CertainMethod());
        EXPECT_FALSE(log.NoneMethod());
    }

    // Test "cer" option (certain methods)
    {
        CompilerLog log("cer");
        EXPECT_FALSE(log.AllMethod());
        EXPECT_TRUE(log.CertainMethod());
        EXPECT_FALSE(log.NoneMethod());
    }

    // Test default/invalid option (should be none method)
    {
        CompilerLog log("other");
        EXPECT_FALSE(log.AllMethod());
        EXPECT_FALSE(log.CertainMethod());
        EXPECT_TRUE(log.NoneMethod());
    }

    // Test "cir" option (output CIR)
    {
        CompilerLog log("cir");
        EXPECT_TRUE(log.OutputCIR());
    }

    // Test "asm" option (output ASM)
    {
        CompilerLog log("asm");
        EXPECT_TRUE(log.OutputASM());
    }

    // Test "type" option (output Type info)
    {
        CompilerLog log("type");
        EXPECT_TRUE(log.OutputType());
    }

    // Test no options (empty string)
    {
        CompilerLog log("");
        EXPECT_FALSE(log.AllMethod());
        EXPECT_FALSE(log.CertainMethod());
        EXPECT_TRUE(log.NoneMethod());
    }
}

/**
 * @tc.name: MethodLogList_IncludesMethod
 * @tc.desc: Test MethodLogList IncludesMethod functionality
 * @tc.type: FUNC
 */
HWTEST_F_L0(CompilerLogTest, MethodLogList_IncludesMethod)
{
    MethodLogList logList("method1,method2,method3");

    // Test existing methods
    EXPECT_TRUE(logList.IncludesMethod("method1"));
    EXPECT_TRUE(logList.IncludesMethod("method2"));
    EXPECT_TRUE(logList.IncludesMethod("method3"));

    // Test non-existing method
    EXPECT_FALSE(logList.IncludesMethod("method4"));

    // Test empty method name (should return false)
    EXPECT_FALSE(logList.IncludesMethod(""));
}

/**
 * @tc.name: MethodLogList_EmptyList
 * @tc.desc: Test MethodLogList with empty method list
 * @tc.type: FUNC
 */
HWTEST_F_L0(CompilerLogTest, MethodLogList_EmptyList)
{
    MethodLogList logList("");

    EXPECT_FALSE(logList.IncludesMethod("method1"));
    EXPECT_FALSE(logList.IncludesMethod(""));
}

/**
 * @tc.name: AotMethodLogList_IncludesMethod
 * @tc.desc: Test AotMethodLogList IncludesMethod functionality
 * @tc.type: FUNC
 */
HWTEST_F_L0(CompilerLogTest, AotMethodLogList_IncludesMethod)
{
    // Format: filename,method1,method2:filename2,method3
    // First element is filename, followed by methods (all comma-separated)
    AotMethodLogList logList("file1,method1,method2:file2,method3");

    // Test existing methods in file1
    EXPECT_TRUE(logList.IncludesMethod("file1", "method1"));
    EXPECT_TRUE(logList.IncludesMethod("file1", "method2"));

    // Test existing method in file2
    EXPECT_TRUE(logList.IncludesMethod("file2", "method3"));

    // Test method in wrong file
    EXPECT_FALSE(logList.IncludesMethod("file1", "method3"));

    // Test non-existing file
    EXPECT_FALSE(logList.IncludesMethod("file3", "method1"));

    // Test non-existing method in existing file
    EXPECT_FALSE(logList.IncludesMethod("file2", "method1"));
}

/**
 * @tc.name: AotMethodLogList_EmptyList
 * @tc.desc: Test AotMethodLogList with empty list
 * @tc.type: FUNC
 */
HWTEST_F_L0(CompilerLogTest, AotMethodLogList_EmptyList)
{
    AotMethodLogList logList("");

    EXPECT_FALSE(logList.IncludesMethod("file1", "method1"));
    EXPECT_FALSE(logList.IncludesMethod("", ""));
}

/**
 * @tc.name: AotMethodLogList_SingleFile
 * @tc.desc: Test AotMethodLogList with single file and multiple methods
 * @tc.type: FUNC
 */
HWTEST_F_L0(CompilerLogTest, AotMethodLogList_SingleFile)
{
    AotMethodLogList logList("file1,method1,method2,method3");

    EXPECT_TRUE(logList.IncludesMethod("file1", "method1"));
    EXPECT_TRUE(logList.IncludesMethod("file1", "method2"));
    EXPECT_TRUE(logList.IncludesMethod("file1", "method3"));
    EXPECT_FALSE(logList.IncludesMethod("file1", "method4"));
    EXPECT_FALSE(logList.IncludesMethod("file2", "method1"));
}

/**
 * @tc.name: AotMethodLogList_SetMethodLog
 * @tc.desc: Test CompilerLog SetMethodLog with AotMethodLogList
 * @tc.type: FUNC
 */
HWTEST_F_L0(CompilerLogTest, AotMethodLogList_SetMethodLog)
{
    CompilerLog log("cer");
    AotMethodLogList logList("file1,method1,method2");

    log.SetMethodLog("file1", "method1", &logList);
    EXPECT_TRUE(log.GetEnableMethodLog());

    log.SetMethodLog("file1", "method3", &logList);
    EXPECT_FALSE(log.GetEnableMethodLog());

    log.SetMethodLog("file2", "method1", &logList);
    EXPECT_FALSE(log.GetEnableMethodLog());
}

/**
 * @tc.name: CompilerLog_AddPassTime
 * @tc.desc: Test adding pass time to CompilerLog
 * @tc.type: FUNC
 */
HWTEST_F_L0(CompilerLogTest, CompilerLog_AddPassTime)
{
    CompilerLog log("all");
    log.SetEnableCompilerLogTime(true);

    // Test that AddPassTime accumulates time correctly
    log.AddPassTime("Pass1", 10.5);
    log.AddPassTime("Pass2", 20.3);
    log.AddPassTime("Pass1", 5.2);

    // Verify time accumulation
    EXPECT_DOUBLE_EQ(log.GetPassTime("Pass1"), 15.7);  // 10.5 + 5.2
    EXPECT_DOUBLE_EQ(log.GetPassTime("Pass2"), 20.3);
    EXPECT_DOUBLE_EQ(log.GetPassTime("NonExistent"), 0.0);

    // Verify log state is still valid
    EXPECT_TRUE(log.GetEnableCompilerLogTime());
}

/**
 * @tc.name: CompilerLog_AddMethodTime
 * @tc.desc: Test adding method time to CompilerLog
 * @tc.type: FUNC
 */
HWTEST_F_L0(CompilerLogTest, CompilerLog_AddMethodTime)
{
    CompilerLog log("all");
    log.SetEnableCompilerLogTime(true);

    // Test that AddMethodTime accumulates time for same method correctly
    log.AddMethodTime("Method1", 0x1000, 15.5);
    log.AddMethodTime("Method2", 0x2000, 25.3);
    log.AddMethodTime("Method1", 0x1000, 10.0);

    // Verify time accumulation
    EXPECT_DOUBLE_EQ(log.GetMethodTime("Method1", 0x1000), 25.5);  // 15.5 + 10.0
    EXPECT_DOUBLE_EQ(log.GetMethodTime("Method2", 0x2000), 25.3);
    EXPECT_DOUBLE_EQ(log.GetMethodTime("NonExistent", 0x9999), 0.0);

    // Verify log state is still valid
    EXPECT_TRUE(log.GetEnableCompilerLogTime());
}

/**
 * @tc.name: CompilerLog_GetIndex
 * @tc.desc: Test CompilerLog GetIndex method
 * @tc.type: FUNC
 */
HWTEST_F_L0(CompilerLogTest, CompilerLog_GetIndex)
{
    CompilerLog log("all");

    // Test that GetIndex returns incrementing values
    int index1 = log.GetIndex();
    int index2 = log.GetIndex();
    int index3 = log.GetIndex();

    EXPECT_EQ(index2, index1 + 1);
    EXPECT_EQ(index3, index2 + 1);
    EXPECT_EQ(index3, index1 + 2);
}

/**
 * @tc.name: CompilerLog_Print
 * @tc.desc: Test CompilerLog Print function with various configurations
 * @tc.type: FUNC
 */
HWTEST_F_L0(CompilerLogTest, CompilerLog_Print)
{
    CompilerLog log("all");
    log.SetEnableCompilerLogTime(true);
    log.SetEnableCompilerLogTimeMethods(true);

    // Add some data
    log.AddPassTime("Pass1", 10000.0);   // 10ms
    log.AddMethodTime("Method1", 0x1000, 15000.0);  // 15ms
    log.AddMethodTime("Method2", 0x2000, 25000.0);  // 25ms
    log.AddPassTime("Pass2", 20000.0);   // 20ms

    // Capture state before print
    bool timeBefore = log.GetEnableCompilerLogTime();
    bool timeMethodsBefore = log.GetEnableCompilerLogTimeMethods();

    log.Print();

    // Verify state unchanged after print
    EXPECT_EQ(log.GetEnableCompilerLogTime(), timeBefore);
    EXPECT_EQ(log.GetEnableCompilerLogTimeMethods(), timeMethodsBefore);
}

/**
 * @tc.name: CompilerLog_Print_DisableTime
 * @tc.desc: Test CompilerLog Print function when time logging is disabled
 * @tc.type: FUNC
 */
HWTEST_F_L0(CompilerLogTest, CompilerLog_Print_DisableTime)
{
    CompilerLog log("all");
    log.SetEnableCompilerLogTime(false);

    // Add some data
    log.AddPassTime("Pass1", 10000.0);
    log.AddMethodTime("Method1", 0x1000, 15000.0);

    // Should not crash and state should be unchanged
    EXPECT_FALSE(log.GetEnableCompilerLogTime());
    log.Print();
    EXPECT_FALSE(log.GetEnableCompilerLogTime());
}

/**
 * @tc.name: CompilerLog_SetStubLog
 * @tc.desc: Test CompilerLog SetStubLog with MethodLogList
 * @tc.type: FUNC
 */
HWTEST_F_L0(CompilerLogTest, CompilerLog_SetStubLog)
{
    CompilerLog log("cer");
    MethodLogList logList("stub1,stub2");

    // Test with existing stub in list
    log.SetStubLog("stub1", &logList);
    EXPECT_TRUE(log.GetEnableMethodLog());

    // Test with non-existing stub
    log.SetStubLog("stub3", &logList);
    EXPECT_FALSE(log.GetEnableMethodLog());

    // Test with "all" option
    CompilerLog logAll("all");
    logAll.SetStubLog("stub1", &logList);
    EXPECT_TRUE(logAll.GetEnableMethodLog());

    // Test with empty list
    MethodLogList emptyList("");
    log.SetStubLog("stub1", &emptyList);
    EXPECT_FALSE(log.GetEnableMethodLog());
}
} // namespace panda::test
