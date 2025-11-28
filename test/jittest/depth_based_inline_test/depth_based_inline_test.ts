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

function Depth10_Small(x: number): number {
    return x + 10;
}

function Depth9_Small(x: number): number {
    return Depth10_Small(x) + 9;
}

function Depth8_Small(x: number): number {
    return Depth9_Small(x) + 8;
}

function Depth7_Small(x: number): number {
    return Depth8_Small(x) + 7;
}

function Depth6_Small(x: number): number {
    return Depth7_Small(x) + 6;
}

function Depth5_Small(x: number): number {
    return Depth6_Small(x) + 5;
}

function Depth4_Small(x: number): number {
    return Depth5_Small(x) + 4;
}

function Depth3_Small(x: number): number {
    return Depth4_Small(x) + 3;
}

function Depth2_Small(x: number): number {
    return Depth3_Small(x) + 2;
}

function Depth1_Small(x: number): number {
    return Depth2_Small(x) + 1;
}

function Depth4_Medium(x: number): number {
    let result = x + 4;
    for (let i = 0; i < 8; i++) {
        result += i * x;
    }
    return result;
}

function Depth3_Medium(x: number): number {
    let result = x + 3;
    for (let i = 0; i < 10; i++) {
        result += i * 2;
        if (i % 3 === 0) result += x;
    }
    return Depth4_Medium(result) + 3;
}

function Depth2_Medium(x: number): number {
    let result = x + 2;
    for (let i = 0; i < 12; i++) {
        result = result * 2 - i;
        if (i % 5 === 0) result += x;
    }
    return Depth3_Medium(result) + 2;
}

function Depth1_Medium(x: number): number {
    let result = x + 1;
    for (let i = 0; i < 15; i++) {
        result = result + i - x/2;
        if (i % 4 === 0) result = Math.abs(result);
    }
    return Depth2_Medium(result) + 1;
}

function BranchLevel3(x: number): number {
    if (x % 2 === 0) {
        return x * 2 + 3;
    } else {
        return x * 3 - 2;
    }
}

function BranchLevel2(x: number): number {
    if (x < 50) {
        return BranchLevel3(x) + 1;
    } else {
        return BranchLevel3(x) * 2;
    }
}

function BranchLevel1(x: number): number {
    if (x > 0) {
        return BranchLevel2(x) + x;
    } else {
        return BranchLevel2(x) - x;
    }
}

function VariableDepthFunction(x: number, maxDepth: number): number {
    if (maxDepth <= 0 || x <= 0) {
        return x;
    }
    return VariableDepthFunction(x - 1, maxDepth - 1) + 1;
}

function DepthChain1(): number {
    return VariableDepthFunction(8, 3);
}

function DepthChain2(): number {
    return VariableDepthFunction(12, 5);
}

function DepthChain3(): number {
    return VariableDepthFunction(15, 8);
}

function MixedLevel5_Medium(x: number): number {
    let result = x + 5;
    for (let i = 0; i < 20; i++) {
        result += i * (x % 10);
    }
    return result;
}

function MixedLevel4_Small(x: number): number {
    return MixedLevel5_Medium(x) + 4;
}

function MixedLevel3_Small(x: number): number {
    return MixedLevel4_Small(x) + 3;
}

function MixedLevel2_Small(x: number): number {
    return MixedLevel3_Small(x) + 2;
}

function MixedLevel1_Small(x: number): number {
    return MixedLevel2_Small(x) + 1;
}

function ComplexLevel4_Small(x: number): number {
    return x * 2 + 4;
}

function ComplexLevel3_Small(x: number): number {
    if (x % 3 === 0) {
        return ComplexLevel4_Small(x) + 1;
    } else if (x % 3 === 1) {
        return ComplexLevel4_Small(x) + 2;
    } else {
        return ComplexLevel4_Small(x) + 3;
    }
}

function ComplexLevel2_Medium(x: number): number {
    let result = x + 2;
    for (let i = 0; i < 15; i++) {
        result += i;
    }

    switch (x % 4) {
        case 0:
            return ComplexLevel3_Small(result) + 1;
        case 1:
            return ComplexLevel3_Small(result) + 2;
        case 2:
            return ComplexLevel3_Small(result) + 3;
        default:
            return ComplexLevel3_Small(result) + 4;
    }
}

function ComplexLevel1_Small(x: number): number {
    if (x > 0) {
        return ComplexLevel2_Medium(x) + x;
    } else {
        return ComplexLevel2_Medium(x) - x;
    }
}

function RecursiveLike8(x: number): number {
    return x + 8;
}

function RecursiveLike7(x: number): number {
    return RecursiveLike8(x) + 7;
}

function RecursiveLike6(x: number): number {
    return RecursiveLike7(x) + 6;
}

function RecursiveLike5(x: number): number {
    return RecursiveLike6(x) + 5;
}

function RecursiveLike4(x: number): number {
    return RecursiveLike5(x) + 4;
}

function RecursiveLike3(x: number): number {
    return RecursiveLike4(x) + 3;
}

function RecursiveLike2(x: number): number {
    return RecursiveLike3(x) + 2;
}

function RecursiveLike1(x: number): number {
    return RecursiveLike2(x) + 1;
}

function WideFunctionA(x: number): number { return x * 2; }
function WideFunctionB(x: number): number { return x * 3; }
function WideFunctionC(x: number): number { return x * 4; }
function WideFunctionD(x: number): number { return x * 5; }
function WideFunctionE(x: number): number { return x * 6; }

function WideCaller(x: number): number {
    let sum = 0;
    sum += WideFunctionA(x);
    sum += WideFunctionB(x);
    sum += WideFunctionC(x);
    sum += WideFunctionD(x);
    sum += WideFunctionE(x);
    return sum;
}

function TestSmallFunctionDepth(): void {
    print("=== Small Function Depth Test (up to 8 levels) ===");

    print(`Small depth chain result: ${Depth1_Small(5)}`);
}

function TestMediumFunctionDepth(): void {
    print("=== Medium Function Depth Test (up to 3 levels) ===");

    print(`Medium depth chain result: ${Depth1_Medium(5)}`);
}

function TestBranchingDepth(): void {
    print("=== Branching Depth Test ===");

    print(`BranchLevel1(10): ${BranchLevel1(10)}`);
}

function TestVariableDepth(): void {
    print("=== Variable Depth Test ===");

    print(`DepthChain1 (depth 4): ${DepthChain1()}`);
    print(`DepthChain2 (depth 6): ${DepthChain2()}`);
    print(`DepthChain3 (depth 9): ${DepthChain3()}`);
}

function TestMixedSizeDepth(): void {
    print("=== Mixed Size Depth Test ===");

    print(`MixedLevel1_Small(5): ${MixedLevel1_Small(5)}`);
}

function TestComplexBranchingDepth(): void {
    print("=== Complex Branching Depth Test ===");

    print(`ComplexLevel1_Small(15): ${ComplexLevel1_Small(15)}`);
}

function TestRecursiveLikeDepth(): void {
    print("=== Recursive-like Depth Test ===");

    print(`RecursiveLike1(5): ${RecursiveLike1(5)}`);
}

function TestWideNotDeep(): void {
    print("=== Wide But Not Deep Test ===");

    print(`WideCaller(10): ${WideCaller(10)}`);
}

function TestDepthBoundaryConditions(): void {
    print("=== Depth Boundary Conditions Test ===");

    print(`Small chain boundary test: ${Depth1_Small(1)}`);
    print(`Medium chain boundary test: ${Depth1_Medium(1)}`);
}

function MainDepthTest(): void {
    print("=== Inline Depth Testing ===");

    print("Phase 1: Small function depth tests");
    TestSmallFunctionDepth();

    print("Phase 2: Medium function depth tests");
    TestMediumFunctionDepth();

    print("Phase 3: Branching depth tests");
    TestBranchingDepth();

    print("Phase 4: Variable depth tests");
    TestVariableDepth();

    print("Phase 5: Mixed size depth tests");
    TestMixedSizeDepth();

    print("Phase 6: Complex branching depth tests");
    TestComplexBranchingDepth();

    print("Phase 7: Recursive-like depth tests");
    TestRecursiveLikeDepth();

    print("Phase 8: Wide but not deep tests");
    TestWideNotDeep();

    print("Phase 9: Depth boundary conditions");
    TestDepthBoundaryConditions();

    print("Depth-based inline testing completed");
}

function UltraDeep15(x: number): number { return x + 15; }
function UltraDeep14(x: number): number { return UltraDeep15(x) + 14; }
function UltraDeep13(x: number): number { return UltraDeep14(x) + 13; }
function UltraDeep12(x: number): number { return UltraDeep13(x) + 12; }
function UltraDeep11(x: number): number { return UltraDeep12(x) + 11; }
function UltraDeep10(x: number): number { return UltraDeep11(x) + 10; }
function UltraDeep9(x: number): number { return UltraDeep10(x) + 9; }
function UltraDeep8(x: number): number { return UltraDeep9(x) + 8; }
function UltraDeep7(x: number): number { return UltraDeep8(x) + 7; }
function UltraDeep6(x: number): number { return UltraDeep7(x) + 6; }
function UltraDeep5(x: number): number { return UltraDeep6(x) + 5; }
function UltraDeep4(x: number): number { return UltraDeep5(x) + 4; }
function UltraDeep3(x: number): number { return UltraDeep4(x) + 3; }
function UltraDeep2(x: number): number { return UltraDeep3(x) + 2; }
function UltraDeep1(x: number): number { return UltraDeep2(x) + 1; }

function MediumDepth4(x: number): number {
    let result = x + 4;
    for (let i = 0; i < 12; i++) {
        result += i * x;
        if (i % 3 === 0) result *= 2;
    }
    return result;
}

function MediumDepth3(x: number): number {
    let result = x + 3;
    for (let i = 0; i < 15; i++) {
        result = result * 2 - i;
        if (i % 4 === 0) result += x;
    }
    return MediumDepth4(result) + 3;
}

function MediumDepth2(x: number): number {
    let result = x + 2;
    for (let i = 0; i < 18; i++) {
        result = result + i - x/2;
        if (i % 5 === 0) result = Math.abs(result);
    }
    return MediumDepth3(result) + 2;
}

function MediumDepth1(x: number): number {
    let result = x + 1;
    for (let i = 0; i < 20; i++) {
        result = result + i - x;
        if (i % 6 === 0) result = Math.sqrt(result + 1);
    }
    return MediumDepth2(result) + 1;
}

function VariableSizeDepthSmall1(x: number): number { return x * 2; }
function VariableSizeDepthSmall2(x: number): number { return VariableSizeDepthSmall1(x) + 3; }
function VariableSizeDepthMedium1(x: number): number {
    let result = x + 10;
    for (let i = 0; i < 10; i++) {
        result += i * 2;
    }
    return result;
}
function VariableSizeDepthMedium2(x: number): number { return VariableSizeDepthMedium1(x) + VariableSizeDepthSmall2(x); }
function VariableSizeDepthLarge1(x: number): number {
    let result = x + 20;
    for (let i = 0; i < 30; i++) {
        for (let j = 0; j < 5; j++) {
            result += i * j * x;
        }
    }
    return result;
}
function VariableSizeDepthLarge2(x: number): number { return VariableSizeDepthLarge1(x) + VariableSizeDepthMedium2(x); }

function ComplexBranchDepth4(x: number): number {
    if (x % 4 === 0) return x * 4;
    if (x % 4 === 1) return x / 2;
    if (x % 4 === 2) return x + 10;
    return x - 5;
}

function ComplexBranchDepth3(x: number): number {
    const temp = ComplexBranchDepth4(x);
    if (temp > 20) return temp * 2;
    if (temp > 10) return temp + 5;
    return temp;
}

function ComplexBranchDepth2(x: number): number {
    let result = ComplexBranchDepth3(x);
    if (x % 3 === 0) result += 100;
    if (x % 5 === 0) result *= 3;
    return result;
}

function ComplexBranchDepth1(x: number): number {
    const base = ComplexBranchDepth2(x);
    return x > 0 ? base + x : base - x;
}

function LoopDepth5(x: number): number {
    let result = x;
    for (let i = 0; i < 10; i++) {
        result += i;
    }
    return result;
}

function LoopDepth4(x: number): number {
    let result = x;
    for (let i = 0; i < 5; i++) {
        result += LoopDepth5(x + i);
    }
    return result;
}

function LoopDepth3(x: number): number {
    let result = LoopDepth4(x);
    for (let i = 0; i < 3; i++) {
        result *= 2;
    }
    return result;
}

function LoopDepth2(x: number): number {
    return LoopDepth3(x) + LoopDepth4(x - 1);
}

function LoopDepth1(x: number): number {
    return LoopDepth2(x) + LoopDepth3(x + 1);
}

function ArrayDepth4(arr: number[]): number {
    return arr.reduce((sum, x) => sum + x, 0);
}

function ArrayDepth3(arr: number[]): number[] {
    return arr.map(x => x * 2);
}

function ArrayDepth2(arr: number[]): number {
    const mapped = ArrayDepth3(arr);
    const summed = ArrayDepth4(mapped);
    return summed;
}

function ArrayDepth1(arr: number[]): number {
    return ArrayDepth2(arr) * 2;
}

function StringDepth4(text: string): string {
    return text.toUpperCase();
}

function StringDepth3(text: string): string {
    const upper = StringDepth4(text);
    return upper.trim();
}

function StringDepth2(text: string): string {
    const trimmed = StringDepth3(text);
    return trimmed.replace(/\s+/g, '_');
}

function StringDepth1(text: string): number {
    const processed = StringDepth2(text);
    return processed.length;
}

function MixedTypeDepth4(x: number): string {
    return x.toString();
}

function MixedTypeDepth3(x: number): number {
    const str = MixedTypeDepth4(x);
    return str.length;
}

function MixedTypeDepth2(x: number): boolean {
    const len = MixedTypeDepth3(x);
    return len > 5;
}

function MixedTypeDepth1(x: number): string {
    const isLarge = MixedTypeDepth2(x);
    return isLarge ? "large" : "small";
}

function ConditionalDepth4(x: number): number {
    return x * 2;
}

function ConditionalDepth3(x: number): number {
    return x > 0 ? ConditionalDepth4(x) : ConditionalDepth4(-x);
}

function ConditionalDepth2(x: number): number {
    return x % 2 === 0 ? ConditionalDepth3(x + 1) : ConditionalDepth3(x - 1);
}

function ConditionalDepth1(x: number): number {
    return x > 10 ? ConditionalDepth2(x / 2) : ConditionalDepth2(x * 2);
}

function ErrorDepth4(x: number): number {
    if (x < 0) throw new Error("Negative input");
    return x * 2;
}

function ErrorDepth3(x: number): number {
    try {
        return ErrorDepth4(x);
    } catch (e) {
        return ErrorDepth4(Math.abs(x));
    }
}

function ErrorDepth2(x: number): number {
    try {
        return ErrorDepth3(x);
    } catch (e) {
        return 0;
    }
}

function ErrorDepth1(x: number): number {
    try {
        return ErrorDepth2(x);
    } catch (e) {
        return x;
    }
}

interface DepthTestObject {
    value: number;
    double(): number;
    triple(): number;
    toString(): string;
}

function ObjectDepth4(value: number): DepthTestObject {
    return {
        value: value,
        double: function() { return this.value * 2; },
        triple: function() { return this.value * 3; },
        toString: function() { return this.value.toString(); }
    };
}

function ObjectDepth3(value: number): number {
    const obj = ObjectDepth4(value);
    return obj.double();
}

function ObjectDepth2(value: number): number {
    const doubled = ObjectDepth3(value);
    return doubled * 3;
}

function ObjectDepth1(value: number): string {
    const final = ObjectDepth2(value);
    return final.toString();
}

function MathDepth4(x: number): number {
    return Math.sin(x);
}

function MathDepth3(x: number): number {
    return Math.cos(MathDepth4(x));
}

function MathDepth2(x: number): number {
    return Math.tan(MathDepth3(x));
}

function MathDepth1(x: number): number {
    return Math.sqrt(Math.abs(MathDepth2(x)));
}

function ConversionDepth4(x: any): string {
    return String(x);
}

function ConversionDepth3(x: any): number {
    const str = ConversionDepth4(x);
    return Number(str);
}

function ConversionDepth2(x: any): boolean {
    const num = ConversionDepth3(x);
    return Boolean(num);
}

function ConversionDepth1(x: any): string {
    const bool = ConversionDepth2(x);
    return bool.toString();
}

function TestUltraDepthChains(): void {
    print("=== Ultra Deep Chain Tests ===");

    print(`UltraDeep1(5): ${UltraDeep1(5)}`);
}

function TestMediumDepthFunctions(): void {
    print("=== Medium Depth Function Tests ===");

    print(`MediumDepth1(5): ${MediumDepth1(5)}`);
}

function TestVariableSizeDepth(): void {
    print("=== Variable Size Depth Tests ===");

    print(`VariableSizeDepthSmall2(5): ${VariableSizeDepthSmall2(5)}`);
    print(`VariableSizeDepthMedium2(5): ${VariableSizeDepthMedium2(5)}`);
    print(`VariableSizeDepthLarge2(5): ${VariableSizeDepthLarge2(5)}`);
}

function TestComplexBranchDepth(): void {
    print("=== Complex Branch Depth Tests ===");

    print(`ComplexBranchDepth1(15): ${ComplexBranchDepth1(15)}`);
}

function TestLoopDepthPatterns(): void {
    print("=== Loop Depth Pattern Tests ===");

    print(`LoopDepth1(5): ${LoopDepth1(5)}`);
}

function TestArrayDepthChains(): void {
    print("=== Array Depth Chain Tests ===");

    const testArray = [1, 2, 3, 4, 5];
    print(`ArrayDepth1([${testArray.join(', ')}]): ${ArrayDepth1(testArray)}`);
}

function TestStringDepthChains(): void {
    print("=== String Depth Chain Tests ===");

    const testString = "hello";
    print(`StringDepth1("${testString}"): ${StringDepth1(testString)}`);
}

function TestMixedTypeDepthChains(): void {
    print("=== Mixed Type Depth Chain Tests ===");

    print(`MixedTypeDepth1(10): ${MixedTypeDepth1(10)}`);
}

function TestConditionalDepthChains(): void {
    print("=== Conditional Depth Chain Tests ===");

    print(`ConditionalDepth1(25): ${ConditionalDepth1(25)}`);
}

function TestErrorDepthChains(): void {
    print("=== Error Depth Chain Tests ===");

    try {
        print(`ErrorDepth1(10): ${ErrorDepth1(10)}`);
    } catch (e) {
        print("ErrorDepth1(10) caught error");
    }
}

function TestObjectDepthChains(): void {
    print("=== Object Depth Chain Tests ===");

    print(`ObjectDepth1(10): ${ObjectDepth1(10)}`);
}

function TestMathDepthChains(): void {
    print("=== Math Depth Chain Tests ===");

    print(`MathDepth1(1): ${MathDepth1(1)}`);
}

function TestConversionDepthChains(): void {
    print("=== Conversion Depth Chain Tests ===");

    print(`ConversionDepth1(42): ${ConversionDepth1(42)}`);
}

function TestDepthPerformanceBoundaries(): void {
    print("=== Depth Performance Boundary Tests ===");

    print("Performance boundaries test completed");
}

function ExecuteExtendedDepthTests(): void {
    print("=== Extended Depth-based Inline Testing ===");

    print("Extended Phase 1: Ultra deep chain tests");
    TestUltraDepthChains();

    print("Extended Phase 2: Medium depth function tests");
    TestMediumDepthFunctions();

    print("Extended Phase 3: Variable size depth tests");
    TestVariableSizeDepth();

    print("Extended Phase 4: Complex branch depth tests");
    TestComplexBranchDepth();

    print("Extended Phase 5: Loop depth pattern tests");
    TestLoopDepthPatterns();

    print("Extended Phase 6: Array depth chain tests");
    TestArrayDepthChains();

    print("Extended Phase 7: String depth chain tests");
    TestStringDepthChains();

    print("Extended Phase 8: Mixed type depth chain tests");
    TestMixedTypeDepthChains();

    print("Extended Phase 9: Conditional depth chain tests");
    TestConditionalDepthChains();

    print("Extended Phase 10: Error depth chain tests");
    TestErrorDepthChains();

    print("Extended Phase 11: Object depth chain tests");
    TestObjectDepthChains();

    print("Extended Phase 12: Math depth chain tests");
    TestMathDepthChains();

    print("Extended Phase 13: Conversion depth chain tests");
    TestConversionDepthChains();

    print("Extended Phase 14: Depth performance boundary tests");
    TestDepthPerformanceBoundaries();

    print("Extended depth-based inline testing completed");
}

print("=== Depth-based Inline Testing ===");

print("Small Function Depth Test:");
TestSmallFunctionDepth();
ArkTools.jitCompileAsync(TestSmallFunctionDepth);
print(ArkTools.waitJitCompileFinish(TestSmallFunctionDepth));
TestSmallFunctionDepth();

print("Medium Function Depth Test:");
TestMediumFunctionDepth();
ArkTools.jitCompileAsync(TestMediumFunctionDepth);
print(ArkTools.waitJitCompileFinish(TestMediumFunctionDepth));
TestMediumFunctionDepth();

print("Branching Depth Test:");
TestBranchingDepth();
ArkTools.jitCompileAsync(TestBranchingDepth);
print(ArkTools.waitJitCompileFinish(TestBranchingDepth));
TestBranchingDepth();

print("Variable Depth Test:");
TestVariableDepth();
ArkTools.jitCompileAsync(TestVariableDepth);
print(ArkTools.waitJitCompileFinish(TestVariableDepth));
TestVariableDepth();

print("Mixed Size Depth Test:");
TestMixedSizeDepth();
ArkTools.jitCompileAsync(TestMixedSizeDepth);
print(ArkTools.waitJitCompileFinish(TestMixedSizeDepth));
TestMixedSizeDepth();

print("Complex Branching Depth Test:");
TestComplexBranchingDepth();
ArkTools.jitCompileAsync(TestComplexBranchingDepth);
print(ArkTools.waitJitCompileFinish(TestComplexBranchingDepth));
TestComplexBranchingDepth();

print("Recursive-like Depth Test:");
TestRecursiveLikeDepth();
ArkTools.jitCompileAsync(TestRecursiveLikeDepth);
print(ArkTools.waitJitCompileFinish(TestRecursiveLikeDepth));
TestRecursiveLikeDepth();

print("Wide But Not Deep Test:");
TestWideNotDeep();
ArkTools.jitCompileAsync(TestWideNotDeep);
print(ArkTools.waitJitCompileFinish(TestWideNotDeep));
TestWideNotDeep();

print("Depth Boundary Conditions Test:");
TestDepthBoundaryConditions();
ArkTools.jitCompileAsync(TestDepthBoundaryConditions);
print(ArkTools.waitJitCompileFinish(TestDepthBoundaryConditions));
TestDepthBoundaryConditions();

print("Extended Tests:");
ExecuteExtendedDepthTests();
ArkTools.jitCompileAsync(ExecuteExtendedDepthTests);
print(ArkTools.waitJitCompileFinish(ExecuteExtendedDepthTests));
ExecuteExtendedDepthTests();

print("Depth-based inline testing completed");