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

function Print()
{
    print("function without args.");
}

function PrintWithOne(one)
{
    print("function with one arg: " + one);
}

function PrintWithTwo(one, two)
{
    print("function with two args: " + one + ", " + two);
}

function PrintWithThree(one, two, three)
{
    print("function with three args: " + one + ", " + two + ", " + three);
}

function PrintWithFour(one, two, three, four)
{
    print("function with four args: " + one + ", " + two + ", " + three + "," + four);
}

class TestClass {
    Print()
    {
        print("class function without args.");
    }

    PrintWithOne(one)
    {
        print("class function with one arg: " + one);
    }

    PrintWithTwo(one, two)
    {
        print("class function with two args: " + one + ", " + two);
    }

    PrintWithThree(one, two, three)
    {
        print("class function with three args: " + one + ", " + two + ", " + three);
    }

    PrintWithFour(one, two, three, four)
    {
        print("class function with four args: " + one + ", " + two + ", " + three + "," + four);
    }
}

function Test() {
    // CALLARG0_IMM8
    Print();
    // CALLARG1_IMM8_V8
    PrintWithOne(1);
    // CALLARGS2_IMM8_V8_V8
    PrintWithTwo(1, 2);
    // CALLARGS3_IMM8_V8_V8_V8
    PrintWithThree(1, 2, 3);
    // CALLRANGE_IMM8_IMM8_V8
    PrintWithFour(1, 2, 3, 4);
    const testClass = new TestClass();
    // CALLTHIS0_IMM8_V8
    testClass.Print();
    // CALLTHIS1_IMM8_V8_V8
    testClass.PrintWithOne(1);
    // CALLTHIS2_IMM8_V8_V8_V8
    testClass.PrintWithTwo(1, 2);
    // CALLTHIS3_IMM8_V8_V8_V8_V8
    testClass.PrintWithThree(1, 2, 3);
    // CALLTHISRANGE_IMM8_IMM8_V8
    testClass.PrintWithFour(1, 2, 3, 4);
}

Test();

ArkTools.jitCompileAsync(Test);
print(ArkTools.waitJitCompileFinish(Test));

Test();
