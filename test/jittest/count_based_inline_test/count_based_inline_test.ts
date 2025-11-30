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

function InlineFoo1(input: number): number {
    return input * 2;
}

function InlineFoo2(input: number): number {
    return input * 2;
}

function InlineFoo3(input: number): number {
    return input * 2;
}

function InlineFoo4(input: number): number {
    return input * 2;
}

function InlineFoo5(input: number): number {
    return input * 2;
}

function InlineFoo6(input: number): number {
    return input * 2;
}

function InlineFoo7(input: number): number {
    return input * 2;
}

function InlineFoo8(input: number): number {
    return input * 2;
}

function InlineFoo9(input: number): number {
    return input * 2;
}

function InlineFoo10(input: number): number {
    return input * 2;
}

function BasicInlineCountTest(): number {
    let a1 = InlineFoo1(1);
    let a2 = InlineFoo2(a1);
    let a3 = InlineFoo3(a2);
    let a4 = InlineFoo4(a3);
    let a5 = InlineFoo5(a4);
    let a6 = InlineFoo6(a5);
    let a7 = InlineFoo7(a6);
    let a8 = InlineFoo8(a7);
    let a9 = InlineFoo9(a8);
    let a10 = InlineFoo10(a9);
    return a10;
}

function MixedInlineCountTest(input: number): number {
    let result = input;

    result = InlineFoo1(result);
    result = InlineFoo2(result);
    result = InlineFoo3(result);
    result = InlineFoo4(result);
    result = InlineFoo5(result);
    result = InlineFoo6(result);

    if (result > 100) {
        result = InlineFoo7(result);
        result = InlineFoo8(result);
    }

    for (let i = 0; i < 5; i++) {
        result = InlineFoo9(result);
    }

    return result;
}

function ConditionalInlineCountTest(x: number): number {
    let result = x;

    if (x < 50) {
        result = InlineFoo1(result);
        result = InlineFoo2(result);
        result = InlineFoo3(result);
        result = InlineFoo4(result);
        result = InlineFoo5(result);
    }
    else if (x < 100) {
        result = InlineFoo1(result);
        result = InlineFoo2(result);
        result = InlineFoo3(result);
        result = InlineFoo4(result);
        result = InlineFoo5(result);
        result = InlineFoo6(result);
        result = InlineFoo7(result);
    }
    else {
        result = InlineFoo1(result);
        result = InlineFoo2(result);
        result = InlineFoo3(result);
        result = InlineFoo4(result);
        result = InlineFoo5(result);
        result = InlineFoo6(result);
        result = InlineFoo7(result);
        result = InlineFoo8(result);
        result = InlineFoo9(result);
        result = InlineFoo10(result);
    }

    return result;
}

function ComposeFunc1(x: number): number { return x + 1; }
function ComposeFunc2(x: number): number { return x * 2; }
function ComposeFunc3(x: number): number { return x - 3; }
function ComposeFunc4(x: number): number { return x / 4; }
function ComposeFunc5(x: number): number { return x % 5; }
function ComposeFunc6(x: number): number { return Math.floor(x); }
function ComposeFunc7(x: number): number { return Math.ceil(x); }
function ComposeFunc8(x: number): number { return Math.round(x); }
function ComposeFunc9(x: number): number { return Math.abs(x); }
function ComposeFunc10(x: number): number { return Math.sqrt(x); }

function CompositionInlineTest(x: number): number {
    let result = x;

    result = ComposeFunc1(result);
    result = ComposeFunc2(result);
    result = ComposeFunc3(result);
    result = ComposeFunc4(result);
    result = ComposeFunc5(result);
    result = ComposeFunc6(result);
    result = ComposeFunc7(result);
    result = ComposeFunc8(result);
    result = ComposeFunc9(result);

    if (result >= 0) {
        result = ComposeFunc10(result);
    }

    return result;
}

function NestedInlineOuter(x: number): number {
    let a = InlineFoo1(x);
    let b = InlineFoo2(x);

    if (x > 0) {
        let c = InlineFoo3(a);
        let d = InlineFoo4(b);

        if (x > 50) {
            let e = InlineFoo5(c);
            let f = InlineFoo6(d);
            let g = InlineFoo7(e);
            let h = InlineFoo8(f);

            return g + h;
        } else {
            let i = InlineFoo9(c);
            return i;
        }
    } else {
        let j = InlineFoo10(a);
        return j;
    }
}

function ProcessArrayFunc1(arr: number[]): number {
    return arr.reduce((sum, x) => sum + x, 0);
}

function ProcessArrayFunc2(arr: number[]): number {
    return arr.reduce((prod, x) => prod * x, 1);
}

function ProcessArrayFunc3(arr: number[]): number {
    return Math.max(...arr);
}

function ProcessArrayFunc4(arr: number[]): number {
    return Math.min(...arr);
}

function ProcessArrayFunc5(arr: number[]): number {
    return arr.length;
}

function ProcessArrayFunc6(arr: number[]): number {
    return arr.filter(x => x > 0).length;
}

function ProcessArrayFunc7(arr: number[]): number {
    return arr.map(x => x * 2).reduce((sum, x) => sum + x, 0);
}

function ArrayInlineCountTest(arrays: number[][]): number {
    let total = 0;

    total += ProcessArrayFunc1(arrays[0] || []);
    total += ProcessArrayFunc2(arrays[1] || []);
    total += ProcessArrayFunc3(arrays[2] || []);
    total += ProcessArrayFunc4(arrays[3] || []);
    total += ProcessArrayFunc5(arrays[4] || []);
    total += ProcessArrayFunc6(arrays[5] || []);
    total += ProcessArrayFunc7(arrays[6] || []);

    return total;
}

function StringFunc1(s: string): string { return s.toUpperCase(); }
function StringFunc2(s: string): string { return s.toLowerCase(); }
function StringFunc3(s: string): string { return s.trim(); }
function StringFunc4(s: string): string { return s.substring(0, 5); }
function StringFunc5(s: string): string { return s.padEnd(10, 'x'); }
function StringFunc6(s: string): string { return s.repeat(2); }
function StringFunc7(s: string): string { return s.split('').reverse().join(''); }
function StringFunc8(s: string): string { return s.replace(/a/g, 'b'); }
function StringFunc9(s: string): string { return s.concat('_suffix'); }
function StringFunc10(s: string): string { return s.substring(s.length - 5); }

function StringInlineCountTest(text: string): string {
    let result = text;

    result = StringFunc1(result);
    result = StringFunc2(result);
    result = StringFunc3(result);
    result = StringFunc4(result);
    result = StringFunc5(result);
    result = StringFunc6(result);
    result = StringFunc7(result);
    result = StringFunc8(result);
    result = StringFunc9(result);
    result = StringFunc10(result);

    return result;
}

function MathFunc1(x: number): number { return Math.sin(x); }
function MathFunc2(x: number): number { return Math.cos(x); }
function MathFunc3(x: number): number { return Math.tan(x); }
function MathFunc4(x: number): number { return Math.sqrt(x); }
function MathFunc5(x: number): number { return Math.pow(x, 2); }
function MathFunc6(x: number): number { return Math.log(x); }
function MathFunc7(x: number): number { return Math.exp(x); }
function MathFunc8(x: number): number { return Math.abs(x); }
function MathFunc9(x: number): number { return Math.ceil(x); }
function MathFunc10(x: number): number { return Math.floor(x); }

function MathInlineCountTest(x: number): number {
    let result = x;

    result = Math.abs(result);

    result = MathFunc1(result);
    result = MathFunc2(result);
    result = MathFunc3(result);
    result = MathFunc4(result);
    result = MathFunc5(result);
    result = MathFunc8(result);
    result = MathFunc9(result);
    result = MathFunc10(result);

    if (result > 0) {
        result = MathFunc6(result);
    }

    return result;
}

function TestBasicInlineCount(): void {
    print("=== Basic Inline Count Test (6 call limit) ===");

    print(BasicInlineCountTest());
}

function TestMixedInlineCount(): void {
    print("=== Mixed Inline Count Test ===");

    print(MixedInlineCountTest(10));
}

function TestConditionalInlineCount(): void {
    print("=== Conditional Inline Count Test ===");

    print(ConditionalInlineCountTest(25));
    print(ConditionalInlineCountTest(75));
    print(ConditionalInlineCountTest(150));
}

function TestCompositionInlineCount(): void {
    print("=== Composition Inline Count Test ===");

    print(CompositionInlineTest(5));
}

function TestNestedInlineCount(): void {
    print("=== Nested Inline Count Test ===");

    print(NestedInlineOuter(10));
    print(NestedInlineOuter(-10));
}

function TestArrayInlineCount(): void {
    print("=== Array Inline Count Test ===");

    const testArrays = [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9],
        [10, 11, 12],
        [13, 14, 15],
        [16, 17, 18],
        [19, 20, 21]
    ];

    print(ArrayInlineCountTest(testArrays));
}

function TestStringInlineCount(): void {
    print("=== String Inline Count Test ===");

    print(StringInlineCountTest("hello"));
}

function TestMathInlineCount(): void {
    print("=== Math Inline Count Test ===");

    print(MathInlineCountTest(5));
}

print("=== Inline Count Testing ===");

print("Basic Inline Count Test:");
TestBasicInlineCount();
ArkTools.jitCompileAsync(TestBasicInlineCount);
print(ArkTools.waitJitCompileFinish(TestBasicInlineCount));
TestBasicInlineCount();

print("Mixed Inline Count Test:");
TestMixedInlineCount();
ArkTools.jitCompileAsync(TestMixedInlineCount);
print(ArkTools.waitJitCompileFinish(TestMixedInlineCount));
TestMixedInlineCount();

print("Conditional Inline Count Test:");
TestConditionalInlineCount();
ArkTools.jitCompileAsync(TestConditionalInlineCount);
print(ArkTools.waitJitCompileFinish(TestConditionalInlineCount));
TestConditionalInlineCount();

print("Composition Inline Count Test:");
TestCompositionInlineCount();
ArkTools.jitCompileAsync(TestCompositionInlineCount);
print(ArkTools.waitJitCompileFinish(TestCompositionInlineCount));
TestCompositionInlineCount();

print("Nested Inline Count Test:");
TestNestedInlineCount();
ArkTools.jitCompileAsync(TestNestedInlineCount);
print(ArkTools.waitJitCompileFinish(TestNestedInlineCount));
TestNestedInlineCount();

print("Array Inline Count Test:");
TestArrayInlineCount();
ArkTools.jitCompileAsync(TestArrayInlineCount);
print(ArkTools.waitJitCompileFinish(TestArrayInlineCount));
TestArrayInlineCount();

print("String Inline Count Test:");
TestStringInlineCount();
ArkTools.jitCompileAsync(TestStringInlineCount);
print(ArkTools.waitJitCompileFinish(TestStringInlineCount));
TestStringInlineCount();

print("Math Inline Count Test:");
TestMathInlineCount();
ArkTools.jitCompileAsync(TestMathInlineCount);
print(ArkTools.waitJitCompileFinish(TestMathInlineCount));
TestMathInlineCount();

print("Inline count testing completed");