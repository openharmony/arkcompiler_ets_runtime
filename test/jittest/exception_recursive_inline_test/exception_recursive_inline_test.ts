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

function SimpleCalcFunc1(x: number): number {
    return x * 2;
}

function SimpleCalcFunc2(x: number): number {
    return x + 3;
}

function SimpleCalcFunc3(x: number): number {
    return x - 1;
}

function StringProcessFunc1(s: string): string {
    return s.toUpperCase();
}

function StringProcessFunc2(s: string): string {
    return s.trim();
}

function StringProcessFunc3(s: string): string {
    return s + "_processed";
}

function BasicTryCatchInlineTest(x: number): number {
    let res: number;
    try {
        res = SimpleCalcFunc1(x);
        res = SimpleCalcFunc2(res);
        res = SimpleCalcFunc3(res);
    } catch (error) {
        res = SimpleCalcFunc1(0);
        res = SimpleCalcFunc3(res);
    }
    return res;
}

function BasicTryCatchStringTest(text: string): string {
    let res: string;
    try {
        res = StringProcessFunc1(text);
        res = StringProcessFunc2(res);
        res = StringProcessFunc3(res);
    } catch (error) {
        res = StringProcessFunc1("error");
    }
    return res;
}

function ComplexNestedTryCatchTest(x: number): number {
    let result = x;

    try {
        result = SimpleCalcFunc1(result);

        try {
            result = SimpleCalcFunc2(result);

            if (result > 100) {
                throw new Error("Too large");
            }

            result = SimpleCalcFunc3(result);

        } catch (innerError) {
            result = SimpleCalcFunc1(result);
            result = SimpleCalcFunc2(result);
        }

    } catch (outerError) {
        result = SimpleCalcFunc3(result);
        result = SimpleCalcFunc1(result);
    }

    return result;
}

function ComplexNestedTryCatchStringTest(text: string): string {
    let result = text;

    try {
        result = StringProcessFunc1(result);

        try {
            result = StringProcessFunc2(result);

            if (result.length > 20) {
                throw new Error("Too long");
            }

            result = StringProcessFunc3(result);

        } catch (innerError) {
            result = StringProcessFunc1("inner_error");
            result = StringProcessFunc2(result);
        }

    } catch (outerError) {
        result = StringProcessFunc3("outer_error");
    }

    return result;
}

function ThrowIfNegative(x: number): number {
    if (x < 0) {
        throw new Error(`Negative value: ${x}`);
    }
    return x * 2;
}

function ThrowIfZero(x: number): number {
    if (x === 0) {
        throw new Error("Zero value");
    }
    return x + 1;
}

function ProcessWithExceptions(x: number): number {
    let result = x;

    try {
        result = SimpleCalcFunc1(result);
        result = ThrowIfNegative(result);
        result = SimpleCalcFunc2(result);
        result = ThrowIfZero(result);
        result = SimpleCalcFunc3(result);

    } catch (error) {
        result = SimpleCalcFunc1(1);
        result = StringProcessFunc1(result.toString().length);
    }

    return result;
}

function FinallyBlockTest(x: number): number {
    let result: number;
    let cleanup: boolean = false;

    try {
        result = SimpleCalcFunc1(x);
        result = SimpleCalcFunc2(result);

        if (x % 3 === 0) {
            throw new Error("Test exception");
        }

        result = SimpleCalcFunc3(result);

    } catch (error) {
        result = SimpleCalcFunc1(0);
    } finally {
        cleanup = true;
        result = SimpleCalcFunc2(result || x);
        result = SimpleCalcFunc3(result);
    }

    return result || x;
}

function MultipleCatchTest(x: number): string {
    let result: string;

    try {
        if (x < 0) {
            throw new RangeError("Negative input");
        } else if (x > 100) {
            throw new TypeError("Too large input");
        } else if (x === 50) {
            throw new Error("Special case");
        }

        result = StringProcessFunc1(x.toString());
        result = StringProcessFunc3(result);

    } catch (error) {
        const errorMessage = (error as Error).message;

        if (errorMessage.includes("Negative") || errorMessage.includes("range")) {
            result = StringProcessFunc2("range_error");
            result = StringProcessFunc3(result);
        } else if (errorMessage.includes("Too large") || errorMessage.includes("type")) {
            result = StringProcessFunc1("type_error");
            result = StringProcessFunc2(result);
        } else {
            result = StringProcessFunc3("general_error");
        }
    }

    return result;
}

function SimpleRecursiveFunc(input: number): number {
    if (input <= 0) {
        return input;
    }
    return SimpleRecursiveFunc(input - 1) + 1;
}

function FactorialRecursive(n: number): number {
    if (n <= 1) {
        return 1;
    }
    return n * FactorialRecursive(n - 1);
}

function FibonacciRecursive(n: number): number {
    if (n <= 1) {
        return n;
    }
    return FibonacciRecursive(n - 1) + FibonacciRecursive(n - 2);
}

function TailRecursiveSum(n: number, acc: number = 0): number {
    if (n <= 0) {
        return acc;
    }
    return TailRecursiveSum(n - 1, acc + n);
}

function MutualRecursionA(n: number): number {
    if (n <= 0) return 0;
    return MutualRecursionB(n - 1) + 1;
}

function MutualRecursionB(n: number): number {
    if (n <= 0) return 0;
    return MutualRecursionA(n - 1) + 2;
}

function RecursiveWithNonRecursiveCalls(n: number): number {
    if (n <= 0) {
        return SimpleCalcFunc1(0);
    }

    let result = n;
    result = SimpleCalcFunc1(result);
    result = SimpleCalcFunc2(result);

    return RecursiveWithNonRecursiveCalls(n - 1) + result;
}

function ComplexRecursiveFunction(data: number[], index: number = 0): number {
    if (index >= data.length) {
        return 0;
    }

    let current = SimpleCalcFunc1(data[index]);
    current = SimpleCalcFunc2(current);

    if (current > 100) {
        current = StringProcessFunc1(current.toString()).length;
    }

    return current + ComplexRecursiveFunction(data, index + 1);
}

function TestBasicTryCatchInline(): void {
    print("=== Basic Try-Catch Inline Test ===");

    print(`BasicTryCatchInlineTest(10): ${BasicTryCatchInlineTest(10)}`);
    print(`BasicTryCatchInlineTest(-5): ${BasicTryCatchInlineTest(-5)}`);
    print(`BasicTryCatchStringTest("test"): ${BasicTryCatchStringTest("test")}`);
}

function TestComplexNestedTryCatch(): void {
    print("=== Complex Nested Try-Catch Test ===");

    print(`ComplexNestedTryCatchTest(50): ${ComplexNestedTryCatchTest(50)}`);
    print(`ComplexNestedTryCatchTest(200): ${ComplexNestedTryCatchTest(200)}`);
    print(`ComplexNestedTryCatchStringTest("short"): ${ComplexNestedTryCatchStringTest("short")}`);
}

function TestExceptionThrowingFunctions(): void {
    print("=== Exception Throwing Functions Test ===");

    const testValues = [-5, 0, 5, 50];
    for (const val of testValues) {
        try {
            const result = ProcessWithExceptions(val);
            print(`ProcessWithExceptions(${val}): ${result}`);
        } catch (error) {
            print(`ProcessWithExceptions(${val}) threw error`);
        }
    }
}

function TestFinallyBlockInlining(): void {
    print("=== Finally Block Inlining Test ===");

    print(`FinallyBlockTest(3): ${FinallyBlockTest(3)}`);
    print(`FinallyBlockTest(6): ${FinallyBlockTest(6)}`);
    print(`FinallyBlockTest(9): ${FinallyBlockTest(9)}`);
}

function TestMultipleCatchBlocks(): void {
    print("=== Multiple Catch Blocks Test ===");

    const testValues = [-10, 50, 150];
    for (const val of testValues) {
        try {
            const result = MultipleCatchTest(val);
            print(`MultipleCatchTest(${val}): ${result}`);
        } catch (error) {
            print(`MultipleCatchTest(${val}) threw unexpected error`);
        }
    }
}

function TestRecursiveFunctions(): void {
    print("=== Recursive Functions Test (should NOT inline recursive calls) ===");

    print(`SimpleRecursiveFunc(5): ${SimpleRecursiveFunc(5)}`);
    print(`FactorialRecursive(5): ${FactorialRecursive(5)}`);
    print(`FibonacciRecursive(6): ${FibonacciRecursive(6)}`);
    print(`TailRecursiveSum(10): ${TailRecursiveSum(10)}`);
}

function TestMutualRecursion(): void {
    print("=== Mutual Recursion Test ===");

    print(`MutualRecursionA(3): ${MutualRecursionA(3)}, MutualRecursionB(3): ${MutualRecursionB(3)}`);
    print(`MutualRecursionA(5): ${MutualRecursionA(5)}, MutualRecursionB(5): ${MutualRecursionB(5)}`);
}

function TestRecursiveWithNonRecursiveCalls(): void {
    print("=== Recursive With Non-Recursive Calls Test ===");

    print(`RecursiveWithNonRecursiveCalls(3): ${RecursiveWithNonRecursiveCalls(3)}`);
    print(`RecursiveWithNonRecursiveCalls(5): ${RecursiveWithNonRecursiveCalls(5)}`);
}

function TestComplexRecursivePatterns(): void {
    print("=== Complex Recursive Patterns Test ===");

    const testArray = [1, 2, 3, 4, 5];
    print(`ComplexRecursiveFunction([${testArray.join(', ')}]): ${ComplexRecursiveFunction(testArray)}`);
}

function TestMixedExceptionAndRecursion(): void {
    print("=== Mixed Exception and Recursion Test ===");

    function MixedFunc(x: number): number {
        try {
            let result = SimpleCalcFunc1(x);
            result = SimpleCalcFunc2(result);

            if (x > 5) {
                result = SimpleRecursiveFunc(x - 5);
            }

            result = SimpleCalcFunc3(result);
            return result;

        } catch (error) {
            return SimpleCalcFunc1(0);
        }
    }

    print(`MixedFunc(3): ${MixedFunc(3)}`);
    print(`MixedFunc(8): ${MixedFunc(8)}`);
}

print("=== Exception & Recursive Inline Testing ===");

print("Phase 1: Basic try-catch inlining tests");
TestBasicTryCatchInline();
ArkTools.jitCompileAsync(TestBasicTryCatchInline);
print(ArkTools.waitJitCompileFinish(TestBasicTryCatchInline));
TestBasicTryCatchInline();

print("Phase 2: Complex nested try-catch tests");
TestComplexNestedTryCatch();
ArkTools.jitCompileAsync(TestComplexNestedTryCatch);
print(ArkTools.waitJitCompileFinish(TestComplexNestedTryCatch));
TestComplexNestedTryCatch();

print("Phase 3: Exception throwing functions tests");
TestExceptionThrowingFunctions();
ArkTools.jitCompileAsync(TestExceptionThrowingFunctions);
print(ArkTools.waitJitCompileFinish(TestExceptionThrowingFunctions));
TestExceptionThrowingFunctions();

print("Phase 4: Finally block inlining tests");
TestFinallyBlockInlining();
ArkTools.jitCompileAsync(TestFinallyBlockInlining);
print(ArkTools.waitJitCompileFinish(TestFinallyBlockInlining));
TestFinallyBlockInlining();

print("Phase 5: Multiple catch blocks tests");
TestMultipleCatchBlocks();
ArkTools.jitCompileAsync(TestMultipleCatchBlocks);
print(ArkTools.waitJitCompileFinish(TestMultipleCatchBlocks));
TestMultipleCatchBlocks();

print("Phase 6: Recursive functions tests");
TestRecursiveFunctions();
ArkTools.jitCompileAsync(TestRecursiveFunctions);
print(ArkTools.waitJitCompileFinish(TestRecursiveFunctions));
TestRecursiveFunctions();

print("Phase 7: Mutual recursion tests");
TestMutualRecursion();
ArkTools.jitCompileAsync(TestMutualRecursion);
print(ArkTools.waitJitCompileFinish(TestMutualRecursion));
TestMutualRecursion();

print("Phase 8: Recursive with non-recursive calls tests");
TestRecursiveWithNonRecursiveCalls();
ArkTools.jitCompileAsync(TestRecursiveWithNonRecursiveCalls);
print(ArkTools.waitJitCompileFinish(TestRecursiveWithNonRecursiveCalls));
TestRecursiveWithNonRecursiveCalls();

print("Phase 9: Complex recursive patterns tests");
TestComplexRecursivePatterns();
ArkTools.jitCompileAsync(TestComplexRecursivePatterns);
print(ArkTools.waitJitCompileFinish(TestComplexRecursivePatterns));
TestComplexRecursivePatterns();

print("Phase 10: Mixed exception and recursion tests");
TestMixedExceptionAndRecursion();
ArkTools.jitCompileAsync(TestMixedExceptionAndRecursion);
print(ArkTools.waitJitCompileFinish(TestMixedExceptionAndRecursion));
TestMixedExceptionAndRecursion();

print("Exception & recursive inline testing completed");