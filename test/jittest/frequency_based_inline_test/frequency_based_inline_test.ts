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

function HighFreqSimple1(x: number): number {
    let a = x * 2;
    return a - 2;
}

function HighFreqSimple2(x: number): number {
    let b = x + 5;
    return b * 3;
}

function HighFreqSimple3(x: number): number {
    let c = x / 2;
    return Math.floor(c + 10);
}

function HighFreqMedium1(x: number): number {
    let result = x + HighFreqSimple1(x);
    for (let i = 0; i < 10; i++) {
        result += i * x;
    }
    return result - 2;
}

function HighFreqComplex1(x: number, y: number): number {
    let sum = x + y;
    let diff = x - y;
    let prod = x * y;

    if (x > y) {
        sum += HighFreqSimple1(x);
        diff += HighFreqSimple2(y);
    } else {
        prod += HighFreqSimple3(x);
    }

    return sum + diff + prod;
}

function LowFreqSimple1(x: number): number {
    let a = x + 3;
    return a * 2;
}

function LowFreqSimple2(x: number): number {
    let b = x - 7;
    return b / 2;
}

function LowFreqMedium1(x: number): number {
    let result = x * 2;
    for (let i = 0; i < 15; i++) {
        result -= i;
    }
    return result + LowFreqSimple1(x);
}

function LowFreqComplex1(arr: number[]): number {
    let sum = 0;
    for (let i = 0; i < arr.length; i++) {
        sum += arr[i] * (i + 1);
        if (i % 5 === 0) {
            sum += LowFreqSimple1(arr[i]);
        }
    }
    return sum;
}

function MediumFreq1(x: number): number {
    let result = x + 1;
    result = result * 2;
    return result - 3;
}

function MediumFreq2(x: number, y: number): number {
    return (x + y) * (x - y) + 1;
}

function MediumFreq3(str: string): number {
    let count = 0;
    for (let i = 0; i < str.length; i++) {
        if (str[i] === 'a' || str[i] === 'e' || str[i] === 'i' || str[i] === 'o' || str[i] === 'u') {
            count++;
        }
    }
    return count;
}

function FrequencyTestBasic(input: number): number {
    let a = HighFreqSimple1(input);
    let sum = a / 2;

    if (input % 21 === 0) {
        sum += LowFreqSimple1(input);
    } else {
        sum += HighFreqSimple2(input);
    }

    return sum;
}

function FrequencyTestComplex(input: number): number {
    let result = HighFreqSimple1(input);

    result += HighFreqSimple2(result);
    result += HighFreqSimple3(result);

    if (input % 50 === 0) {
        result += LowFreqMedium1(input);
    }

    result += MediumFreq1(input);

    if (input % 100 === 0) {
        result = LowFreqComplex1([input, result]);
    }

    return result;
}

function FrequencyTestPattern(input: number): number {
    let operations = 0;

    for (let i = 0; i < 2; i++) {
        operations += HighFreqSimple1(input + i);

        if (i % 7 === 0) {
            operations += LowFreqSimple2(input + i);
        }

        operations += HighFreqSimple2(operations);
    }

    return operations;
}

function FrequencyTestMixed(input: number, flag: boolean): number {
    let result = input;

    if (flag) {
        result = HighFreqMedium1(result);
        result += HighFreqComplex1(result, result);
    } else {
        result = LowFreqMedium1(result);
        result += MediumFreq2(result, input);
    }

    return result;
}

function FrequencyTestArray(arr: number[]): number[] {
    const result: number[] = [];

    for (let i = 0; i < arr.length; i++) {
        const val = arr[i];

        if (i % 3 === 0) {
            result.push(HighFreqSimple1(val));
            result.push(HighFreqSimple2(val));
        } else if (i % 3 === 1) {
            result.push(HighFreqMedium1(val));
        } else {
            if (i % 10 === 0) {
                result.push(LowFreqSimple1(val));
            } else {
                result.push(MediumFreq1(val));
            }
        }
    }

    return result;
}

function FrequencyTestString(str: string): string {
    let result = "";

    for (let i = 0; i < str.length; i++) {
        const char = str[i];
        const code = char.charCodeAt(0);

        if (i % 2 === 0) {
            result += String.fromCharCode(HighFreqSimple1(code));
        } else {
            result += String.fromCharCode(HighFreqSimple2(code));
        }

        if (i % 15 === 0) {
            result += String.fromCharCode(LowFreqSimple1(code));
        }
    }

    return result;
}

function GatherFrequencyData(): void {
    print("Gathering frequency data...");

    for (let i = 0; i < 50; i++) {
        FrequencyTestBasic(i);
        FrequencyTestComplex(i);
        FrequencyTestPattern(i);
        FrequencyTestMixed(i, i % 3 === 0);

        if (i % 5 === 0) {
            FrequencyTestArray([i, i+1, i+2, i+3, i+4]);
            FrequencyTestString("frequency_test_" + i);
        }
    }

    print("Frequency data gathering complete");
}

function TestMediumFrequencyInlining(): void {
    print("=== Medium Frequency Inlining Test ===");

    print(`MediumFreq1(5): ${MediumFreq1(5)}`);
    print(`MediumFreq2(5, 6): ${MediumFreq2(5, 6)}`);
    print(`MediumFreq3("test"): ${MediumFreq3("test")}`);
}

function TestFrequencyBasedDecisions(): void {
    print("=== Frequency-based Decision Test ===");

    print(`FrequencyTestBasic(10): ${FrequencyTestBasic(10)}`);
    print(`FrequencyTestComplex(10): ${FrequencyTestComplex(10)}`);
    print(`FrequencyTestPattern(10): ${FrequencyTestPattern(10)}`);
}

function TestMixedFrequencyPatterns(): void {
    print("=== Mixed Frequency Patterns Test ===");

    print(`FrequencyTestMixed(5, true): ${FrequencyTestMixed(5, true)}`);
    print(`FrequencyTestMixed(6, false): ${FrequencyTestMixed(6, false)}`);

    const testArray = [1, 2, 3, 4, 5];
    const result = FrequencyTestArray(testArray);
    print(`FrequencyTestArray([${testArray.join(', ')}]): [${result.join(', ')}]`);

    const stringResult = FrequencyTestString("string_test");
    print(`FrequencyTestString("string_test"): ${stringResult}`);
}

function TestFrequencyThresholds(): void {
    print("=== Frequency Threshold Test ===");

    for (let i = 0; i < 20; i++) {
        HighFreqSimple1(i);
        MediumFreq1(i);
    }

    LowFreqSimple1(5);
    LowFreqSimple1(15);

    print("Frequency threshold test completed - check inline decisions in JIT output");
}

function DynamicHighFreqFunc1(x: number): number {
    return x + 100;
}

function DynamicHighFreqFunc2(x: number): number {
    return x * 3;
}

function DynamicHighFreqFunc3(x: number): number {
    return Math.floor(x / 2);
}

function DynamicLowFreqFunc1(x: number): number {
    return x - 50;
}

function DynamicLowFreqFunc2(x: number): number {
    return x % 7;
}

function DynamicLowFreqFunc3(x: number): number {
    return Math.sqrt(x);
}

function DynamicFrequencyOrchestrator(iterations: number, pattern: string): number {
    let result = 0;

    for (let i = 0; i < iterations; i++) {
        switch (pattern) {
            case "high-heavy":
                result += DynamicHighFreqFunc1(i);
                result += DynamicHighFreqFunc2(result);
                result += DynamicHighFreqFunc3(result);
                if (i % 100 === 0) {
                    result += DynamicLowFreqFunc1(i);
                }
                break;

            case "low-heavy":
                result += DynamicLowFreqFunc1(i);
                if (i % 10 === 0) {
                    result += DynamicHighFreqFunc1(i);
                    result += DynamicHighFreqFunc2(result);
                }
                break;

            case "balanced":
                result += DynamicHighFreqFunc1(i);
                result += DynamicLowFreqFunc1(result);
                if (i % 2 === 0) {
                    result += DynamicHighFreqFunc2(result);
                } else {
                    result += DynamicLowFreqFunc2(result);
                }
                break;

            case "waves":
                if (i % 50 < 25) {
                    result += DynamicHighFreqFunc1(i);
                    result += DynamicHighFreqFunc2(result);
                    result += DynamicHighFreqFunc3(result);
                } else {
                    result += DynamicLowFreqFunc1(i);
                    result += DynamicLowFreqFunc2(result);
                }
                break;

            default:
                result += i;
        }
    }

    return result;
}

function SmallHighFreqFunc(x: number): number { return x + 1; }
function MediumHighFreqFunc(x: number): number {
    let result = x + 10;
    for (let i = 0; i < 8; i++) {
        result += i;
    }
    return result;
}

function LargeHighFreqFunc(x: number): number {
    let result = x + 20;
    for (let i = 0; i < 40; i++) {
        for (let j = 0; j < 10; j++) {
            result += i * j;
        }
    }
    return result;
}

function SmallLowFreqFunc(x: number): number { return x - 1; }
function MediumLowFreqFunc(x: number): number {
    let result = x - 10;
    for (let i = 0; i < 12; i++) {
        result -= i;
    }
    return result;
}

function SizeFrequencyOrchestrator(x: number, sizePattern: string): number {
    let result = x;

    switch (sizePattern) {
        case "small-high":
            result = SmallHighFreqFunc(result);
            result = SmallHighFreqFunc(result);
            result = SmallHighFreqFunc(result);
            break;

        case "medium-high":
            result = MediumHighFreqFunc(result);
            result = MediumHighFreqFunc(result);
            break;

        case "large-high":
            result = LargeHighFreqFunc(result);
            break;

        case "small-low":
            result = SmallLowFreqFunc(result);
            break;

        case "medium-low":
            result = MediumLowFreqFunc(result);
            break;

        case "mixed":
            result = SmallHighFreqFunc(result);
            result = MediumHighFreqFunc(result);
            result = SmallLowFreqFunc(result);
            break;
    }

    return result;
}

function NumberHighFreqFunc(x: number): number {
    return Math.pow(x, 2) + Math.sqrt(Math.abs(x));
}

function StringHighFreqFunc(s: string): string {
    return s.toUpperCase().trim().replace(/\s+/g, '_');
}

function BooleanHighFreqFunc(flag: boolean): string {
    return flag ? "true_value" : "false_value";
}

function ArrayHighFreqFunc(arr: number[]): number {
    return arr.reduce((sum, x) => sum + x * 2, 0);
}

function ObjectHighFreqFunc(obj: { [key: string]: number }): number {
    return Object.values(obj).reduce((sum, val) => sum + val, 0);
}

function TypeBasedFrequencyOrchestrator(input: any, typePattern: string): any {
    let result = input;

    switch (typePattern) {
        case "number-heavy":
            result = NumberHighFreqFunc(typeof result === 'number' ? result : 0);
            result = NumberHighFreqFunc(result);
            result = NumberHighFreqFunc(result);
            break;

        case "string-heavy":
            const str = String(result);
            result = StringHighFreqFunc(str);
            result = StringHighFreqFunc(result);
            break;

        case "boolean-heavy":
            const flag = Boolean(result);
            result = BooleanHighFreqFunc(flag);
            break;

        case "array-heavy":
            const arr = Array.isArray(result) ? result : [result];
            result = ArrayHighFreqFunc(arr);
            result = ArrayHighFreqFunc([result]);
            break;

        case "object-heavy":
            const obj = typeof result === 'object' && result !== null ? result : { value: result };
            result = ObjectHighFreqFunc(obj);
            break;

        case "mixed-types":
            if (typeof result === 'number') {
                result = NumberHighFreqFunc(result);
                result = StringHighFreqFunc(result.toString());
            } else if (typeof result === 'string') {
                result = StringHighFreqFunc(result);
                result = NumberHighFreqFunc(parseFloat(result) || 0);
            }
            break;
    }

    return result;
}

function ConditionalHighFreqFunc(x: number): number {
    return x > 0 ? x * 2 : x * 3;
}

function ConditionalMediumFreqFunc(x: number): number {
    let result = x;
    if (x > 50) {
        result = Math.pow(x, 2);
    } else if (x > 25) {
        result = x * 10;
    } else {
        result = x + 100;
    }
    return result;
}

function ConditionalLowFreqFunc(x: number): number {
    switch (x % 10) {
        case 0: return x * 5;
        case 1: return x + 50;
        case 2: return x - 25;
        case 3: return Math.pow(x, 3);
        default: return x;
    }
}

function ConditionalFrequencyOrchestrator(base: number, conditionType: string): number {
    let result = base;

    switch (conditionType) {
        case "simple-if":
            result = ConditionalHighFreqFunc(result);
            result = ConditionalHighFreqFunc(result);
            result = ConditionalHighFreqFunc(result);
            if (result % 100 === 0) {
                result = ConditionalLowFreqFunc(result);
            }
            break;

        case "nested-if":
            if (result > 100) {
                if (result > 200) {
                    result = ConditionalHighFreqFunc(result);
                    result = ConditionalHighFreqFunc(result);
                } else {
                    result = ConditionalMediumFreqFunc(result);
                }
            } else {
                result = ConditionalLowFreqFunc(result);
            }
            break;

        case "switch-heavy":
            for (let i = 0; i < 5; i++) {
                result = ConditionalLowFreqFunc(result);
            }
            break;

        case "complex-nesting":
            if (result > 0) {
                result = ConditionalHighFreqFunc(result);
                if (result > 50) {
                    result = ConditionalMediumFreqFunc(result);
                    if (result % 2 === 0) {
                        result = ConditionalLowFreqFunc(result);
                    }
                } else {
                    result = ConditionalHighFreqFunc(result);
                }
            } else {
                result = ConditionalLowFreqFunc(result);
            }
            break;
    }

    return result;
}

function LoopHighFreqFunc(arr: number[]): number {
    let sum = 0;
    for (let i = 0; i < arr.length; i++) {
        sum += arr[i] * 2;
    }
    return sum;
}

function LoopMediumFreqFunc(limit: number): number {
    let result = 0;
    for (let i = 0; i < limit; i++) {
        result += i * (i % 3 + 1);
    }
    return result;
}

function LoopLowFreqFunc(str: string): string {
    let result = "";
    for (let i = 0; i < str.length; i++) {
        if (i % 5 === 0) {
            result += str[i].toUpperCase();
        } else {
            result += str[i];
        }
    }
    return result;
}

function LoopFrequencyOrchestrator(data: any, loopType: string): any {
    let result = data;

    switch (loopType) {
        case "array-processing":
            if (Array.isArray(result)) {
                result = LoopHighFreqFunc(result);
                result = LoopHighFreqFunc([result]);
            }
            break;

        case "numeric-iteration":
            if (typeof result === 'number') {
                result = LoopMediumFreqFunc(Math.abs(result) % 20);
                result = LoopMediumFreqFunc(Math.abs(result) % 15);
            }
            break;

        case "string-loop":
            if (typeof result === 'string') {
                result = LoopLowFreqFunc(result);
                if (result.length % 3 === 0) {
                    result = LoopLowFreqFunc(result);
                }
            }
            break;

        case "nested-loops":
            for (let i = 0; i < 3; i++) {
                result = LoopMediumFreqFunc(5);
                result = LoopHighFreqFunc([result, i]);
            }
            break;
    }

    return result;
}

function RecursiveHighFreqBase(x: number): number {
    return x <= 0 ? 0 : x + RecursiveHighFreqBase(x - 1);
}

function RecursiveLowFreqWrapper(x: number): number {
    return RecursiveHighFreqBase(x) * 2;
}

function MutualRecursionA(freq: number, depth: number): number {
    if (depth <= 0) return freq;
    return MutualRecursionB(freq + 1, depth - 1);
}

function MutualRecursionB(freq: number, depth: number): number {
    if (depth <= 0) return freq * 2;
    return MutualRecursionA(freq + 2, depth - 1);
}

function RecursiveFrequencyOrchestrator(input: number, recursionType: string): number {
    let result = input;

    switch (recursionType) {
        case "direct-recursion":
            for (let i = 0; i < 3; i++) {
                result = RecursiveHighFreqBase(result % 10);
            }
            break;

        case "wrapper-recursion":
            result = RecursiveLowFreqWrapper(result);
            break;

        case "mutual-recursion":
            result = MutualRecursionA(result, 3);
            result = MutualRecursionB(result, 2);
            break;

        case "mixed-recursion":
            result = RecursiveHighFreqBase(result % 5);
            result = NumberHighFreqFunc(result);
            result = RecursiveLowFreqWrapper(result);
            break;
    }

    return result;
}

function AllocationHighFreqFunc(size: number): number[] {
    return Array.from({length: size}, (_, i) => i * 2);
}

function AllocationMediumFreqFunc(size: number): number[] {
    const arr = Array.from({length: size}, (_, i) => i);
    return arr.map(x => x * 3 + 1);
}

function AllocationLowFreqFunc(obj: any): number {
    const newObj = { ...obj, processed: true };
    return Object.keys(newObj).length;
}

function AllocationFrequencyOrchestrator(input: any, allocationType: string): any {
    let result = input;

    switch (allocationType) {
        case "array-allocation":
            if (typeof result === 'number') {
                result = AllocationHighFreqFunc(result % 10);
                result = AllocationMediumFreqFunc(result.length % 8);
            }
            break;

        case "object-allocation":
            if (typeof result === 'object' && result !== null) {
                result = AllocationLowFreqFunc(result);
            } else {
                result = AllocationLowFreqFunc({ value: result });
            }
            break;

        case "mixed-allocation":
            if (typeof result === 'number') {
                const arr = AllocationHighFreqFunc(result % 5);
                result = AllocationLowFreqFunc({ array: arr });
            }
            break;

        case "intensive-allocation":
            for (let i = 0; i < 3; i++) {
                const arr = AllocationMediumFreqFunc(5);
                result = AllocationLowFreqFunc({ iteration: i, data: arr });
            }
            break;
    }

    return result;
}

function TestDynamicFrequencyPatterns(): void {
    print("=== Dynamic Frequency Pattern Tests ===");

    print(`DynamicFrequencyOrchestrator(100, "high-heavy"): ${DynamicFrequencyOrchestrator(100, "high-heavy")}`);
    print(`DynamicFrequencyOrchestrator(100, "balanced"): ${DynamicFrequencyOrchestrator(100, "balanced")}`);
}

function TestSizeFrequencyPatterns(): void {
    print("=== Size Frequency Pattern Tests ===");

    print(`SizeFrequencyOrchestrator(50, "small-high"): ${SizeFrequencyOrchestrator(50, "small-high")}`);
    print(`SizeFrequencyOrchestrator(50, "medium-high"): ${SizeFrequencyOrchestrator(50, "medium-high")}`);
    print(`SizeFrequencyOrchestrator(50, "mixed"): ${SizeFrequencyOrchestrator(50, "mixed")}`);
}

function TestTypeBasedFrequency(): void {
    print("=== Type-based Frequency Tests ===");

    print(`TypeBasedFrequencyOrchestrator(42, "number-heavy"): ${JSON.stringify(TypeBasedFrequencyOrchestrator(42, "number-heavy"))}`);
    print(`TypeBasedFrequencyOrchestrator("hello", "string-heavy"): ${JSON.stringify(TypeBasedFrequencyOrchestrator("hello", "string-heavy"))}`);
    print(`TypeBasedFrequencyOrchestrator(25, "mixed-types"): ${JSON.stringify(TypeBasedFrequencyOrchestrator(25, "mixed-types"))}`);
}

function TestConditionalFrequencyPatterns(): void {
    print("=== Conditional Frequency Pattern Tests ===");

    print(`ConditionalFrequencyOrchestrator(25, "simple-if"): ${ConditionalFrequencyOrchestrator(25, "simple-if")}`);
    print(`ConditionalFrequencyOrchestrator(100, "nested-if"): ${ConditionalFrequencyOrchestrator(100, "nested-if")}`);
}

function TestLoopFrequencyPatterns(): void {
    print("=== Loop Frequency Pattern Tests ===");

    print(`LoopFrequencyOrchestrator([1, 2, 3, 4, 5], "array-processing"): ${JSON.stringify(LoopFrequencyOrchestrator([1, 2, 3, 4, 5], "array-processing"))}`);
    print(`LoopFrequencyOrchestrator(15, "numeric-iteration"): ${JSON.stringify(LoopFrequencyOrchestrator(15, "numeric-iteration"))}`);
}

function TestRecursiveFrequencyPatterns(): void {
    print("=== Recursive Frequency Pattern Tests ===");

    print(`RecursiveFrequencyOrchestrator(5, "direct-recursion"): ${RecursiveFrequencyOrchestrator(5, "direct-recursion")}`);
    print(`RecursiveFrequencyOrchestrator(5, "wrapper-recursion"): ${RecursiveFrequencyOrchestrator(5, "wrapper-recursion")}`);
}

function TestAllocationFrequencyPatterns(): void {
    print("=== Allocation Frequency Pattern Tests ===");

    print(`AllocationFrequencyOrchestrator(8, "array-allocation"): ${JSON.stringify(AllocationFrequencyOrchestrator(8, "array-allocation"))}`);
    print(`AllocationFrequencyOrchestrator(6, "mixed-allocation"): ${JSON.stringify(AllocationFrequencyOrchestrator(6, "mixed-allocation"))}`);
}

print("=== Frequency-based Inline Testing ===");

print("Phase 1: Gathering frequency data");
GatherFrequencyData();

ArkTools.jitCompileAsync(FrequencyTestBasic);
print(ArkTools.waitJitCompileFinish(FrequencyTestBasic));
ArkTools.jitCompileAsync(FrequencyTestComplex);
print(ArkTools.waitJitCompileFinish(FrequencyTestComplex));
ArkTools.jitCompileAsync(FrequencyTestPattern);
print(ArkTools.waitJitCompileFinish(FrequencyTestPattern));
ArkTools.jitCompileAsync(FrequencyTestMixed);
print(ArkTools.waitJitCompileFinish(FrequencyTestMixed));
ArkTools.jitCompileAsync(FrequencyTestArray);
print(ArkTools.waitJitCompileFinish(FrequencyTestArray));
ArkTools.jitCompileAsync(FrequencyTestString);
print(ArkTools.waitJitCompileFinish(FrequencyTestString));

print("Phase 2: Testing high frequency inlining");
FrequencyTestBasic(1);
FrequencyTestComplex(1);
FrequencyTestPattern(1);
FrequencyTestMixed(1, true);
FrequencyTestArray([0, 1, 2, 3, 4]);
FrequencyTestString("frequency_test_" + 1);

print("Phase 3: Testing low frequency inlining");
FrequencyTestBasic(0);
FrequencyTestComplex(0);
FrequencyTestPattern(0);
FrequencyTestMixed(0, false);
FrequencyTestArray([1, 2, 3]);
FrequencyTestString("frequency_test_" + 0);

print("Phase 4: Testing medium frequency inlining");
TestMediumFrequencyInlining();

print("Phase 5: Testing frequency-based decisions");
TestFrequencyBasedDecisions();

print("Phase 6: Testing mixed frequency patterns");
TestMixedFrequencyPatterns();

print("Phase 7: Testing frequency thresholds");
TestFrequencyThresholds();
ArkTools.jitCompileAsync(TestFrequencyThresholds);
print(ArkTools.waitJitCompileFinish(TestFrequencyThresholds));
TestFrequencyThresholds();

print("Phase 8: Dynamic frequency pattern tests");
TestDynamicFrequencyPatterns();
ArkTools.jitCompileAsync(TestDynamicFrequencyPatterns);
print(ArkTools.waitJitCompileFinish(TestDynamicFrequencyPatterns));
TestDynamicFrequencyPatterns();

print("Phase 9: Size frequency pattern tests");
TestSizeFrequencyPatterns();
ArkTools.jitCompileAsync(TestSizeFrequencyPatterns);
print(ArkTools.waitJitCompileFinish(TestSizeFrequencyPatterns));
TestSizeFrequencyPatterns();

print("Phase 10: Type-based frequency tests");
TestTypeBasedFrequency();
ArkTools.jitCompileAsync(TestTypeBasedFrequency);
print(ArkTools.waitJitCompileFinish(TestTypeBasedFrequency));
TestTypeBasedFrequency();

print("Phase 11: Conditional frequency pattern tests");
TestConditionalFrequencyPatterns();
ArkTools.jitCompileAsync(TestConditionalFrequencyPatterns);
print(ArkTools.waitJitCompileFinish(TestConditionalFrequencyPatterns));
TestConditionalFrequencyPatterns();

print("Phase 12: Loop frequency pattern tests");
TestLoopFrequencyPatterns();
ArkTools.jitCompileAsync(TestLoopFrequencyPatterns);
print(ArkTools.waitJitCompileFinish(TestLoopFrequencyPatterns));
TestLoopFrequencyPatterns();

print("Phase 13: Recursive frequency pattern tests");
TestRecursiveFrequencyPatterns();
ArkTools.jitCompileAsync(TestRecursiveFrequencyPatterns);
print(ArkTools.waitJitCompileFinish(TestRecursiveFrequencyPatterns));
TestRecursiveFrequencyPatterns();

print("Phase 14: Allocation frequency pattern tests");
TestAllocationFrequencyPatterns();
ArkTools.jitCompileAsync(TestAllocationFrequencyPatterns);
print(ArkTools.waitJitCompileFinish(TestAllocationFrequencyPatterns));
TestAllocationFrequencyPatterns();

print("Frequency-based inline testing completed");