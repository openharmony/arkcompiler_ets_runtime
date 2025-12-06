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

declare function print(arg: any): string;
declare const ArkTools: {
    jitCompileAsync(args: any): boolean;
    waitJitCompileFinish(args: any): boolean;
};

// ==================== Math Rounding ====================

function testMathFloor(): string {
    const a = Math.floor(4.7);
    const b = Math.floor(-4.7);
    const c = Math.floor(4.0);
    const d = Math.floor(0.5);
    const e = Math.floor(-0.5);
    return `${a},${b},${c},${d},${e}`;
}

function testMathCeil(): string {
    const a = Math.ceil(4.3);
    const b = Math.ceil(-4.3);
    const c = Math.ceil(4.0);
    const d = Math.ceil(0.1);
    const e = Math.ceil(-0.1);
    return `${a},${b},${c},${d},${e}`;
}

function testMathRound(): string {
    const a = Math.round(4.5);
    const b = Math.round(4.4);
    const c = Math.round(-4.5);
    const d = Math.round(-4.6);
    const e = Math.round(0.5);
    return `${a},${b},${c},${d},${e}`;
}

function testMathTrunc(): string {
    const a = Math.trunc(4.7);
    const b = Math.trunc(-4.7);
    const c = Math.trunc(0.9);
    const d = Math.trunc(-0.9);
    const e = Math.trunc(13.37);
    return `${a},${b},${c},${d},${e}`;
}

function testMathAbs(): string {
    const a = Math.abs(-5);
    const b = Math.abs(5);
    const c = Math.abs(-0);
    const d = Math.abs(-Infinity);
    const e = Math.abs(-3.14);
    return `${a},${b},${c},${d},${e.toFixed(2)}`;
}

function testMathSign(): string {
    const a = Math.sign(-5);
    const b = Math.sign(5);
    const c = Math.sign(0);
    const d = Math.sign(-0.5);
    const e = Math.sign(0.5);
    return `${a},${b},${c},${d},${e}`;
}

function testMathFloorEdgeCases(): string {
    const a = Math.floor(Number.MAX_VALUE);
    const b = Math.floor(Number.MIN_VALUE);
    const c = Math.floor(-Number.MIN_VALUE);
    const d = Math.floor(1e-10);
    return `${a > 0},${b},${c},${d}`;
}

function testMathCeilEdgeCases(): string {
    const a = Math.ceil(1e-10);
    const b = Math.ceil(-1e-10);
    const c = Math.ceil(0.999999999);
    const d = Math.ceil(-0.999999999);
    return `${a},${b},${c},${d}`;
}

// ==================== Math Power ====================

function testMathPow(): string {
    const a = Math.pow(2, 10);
    const b = Math.pow(9, 0.5);
    const c = Math.pow(2, -1);
    const d = Math.pow(10, 0);
    const e = Math.pow(-2, 3);
    return `${a},${b},${c},${d},${e}`;
}

function testMathSqrt(): string {
    const a = Math.sqrt(16);
    const b = Math.sqrt(2).toFixed(4);
    const c = Math.sqrt(0);
    const d = Math.sqrt(1);
    const e = Math.sqrt(100);
    return `${a},${b},${c},${d},${e}`;
}

function testMathCbrt(): string {
    const a = Math.round(Math.cbrt(27));
    const b = Math.round(Math.cbrt(-8));
    const c = Math.round(Math.cbrt(1));
    const d = Math.round(Math.cbrt(0));
    const e = Math.round(Math.cbrt(64));
    return `${a},${b},${c},${d},${e}`;
}

function testMathExp(): string {
    const a = Math.exp(0);
    const b = Math.exp(1).toFixed(4);
    const c = Math.exp(-1).toFixed(4);
    const d = Math.exp(2).toFixed(4);
    return `${a},${b},${c},${d}`;
}

function testMathExpm1(): string {
    const a = Math.expm1(0);
    const b = Math.expm1(1).toFixed(4);
    const c = Math.expm1(-1).toFixed(4);
    const d = Math.expm1(1e-10).toFixed(10);
    return `${a},${b},${c},${d}`;
}

function testMathLog(): string {
    const a = Math.log(1);
    const b = Math.log(Math.E).toFixed(4);
    const c = Math.log(10).toFixed(4);
    const d = Math.log(100).toFixed(4);
    return `${a},${b},${c},${d}`;
}

function testMathLog2(): string {
    const a = Math.log2(1);
    const b = Math.log2(2);
    const c = Math.log2(8);
    const d = Math.log2(1024);
    const e = Math.log2(0.5);
    return `${a},${b},${c},${d},${e}`;
}

function testMathLog10(): string {
    const a = Math.log10(1);
    const b = Math.log10(10);
    const c = Math.log10(100);
    const d = Math.log10(1000);
    const e = Math.log10(0.1);
    return `${a},${b},${c},${d},${e}`;
}

function testMathLog1p(): string {
    const a = Math.log1p(0);
    const b = Math.log1p(1).toFixed(4);
    const c = Math.log1p(Math.E - 1).toFixed(4);
    const d = Math.log1p(1e-10).toFixed(10);
    return `${a},${b},${c},${d}`;
}

function testMathHypot(): string {
    const a = Math.hypot(3, 4);
    const b = Math.hypot(5, 12);
    const c = Math.hypot(1, 1).toFixed(4);
    const d = Math.hypot(3, 4, 0);
    const e = Math.hypot();
    return `${a},${b},${c},${d},${e}`;
}

function testMathHypotMultiple(): string {
    const a = Math.hypot(1, 2, 3).toFixed(4);
    const b = Math.hypot(1, 1, 1, 1);
    const c = Math.hypot(-3, -4);
    const d = Math.hypot(0, 0, 0);
    return `${a},${b},${c},${d}`;
}

function testMathPowEdgeCases(): string {
    const a = Math.pow(0, 0);
    const b = Math.pow(1, Infinity);
    const c = Math.pow(-1, 0.5);
    const d = Math.pow(2, 0.5).toFixed(4);
    return `${a},${b},${isNaN(c)},${d}`;
}

// ==================== Math Trig ====================

function testMathSin(): string {
    const a = Math.sin(0);
    const b = Math.sin(Math.PI / 6).toFixed(4);
    const c = Math.sin(Math.PI / 4).toFixed(4);
    const d = Math.sin(Math.PI / 2).toFixed(4);
    const e = Math.sin(Math.PI).toFixed(10);
    return `${a},${b},${c},${d},${e}`;
}

function testMathCos(): string {
    const a = Math.cos(0);
    const b = Math.cos(Math.PI / 3).toFixed(4);
    const c = Math.cos(Math.PI / 4).toFixed(4);
    const d = Math.cos(Math.PI / 2).toFixed(10);
    const e = Math.cos(Math.PI).toFixed(4);
    return `${a},${b},${c},${d},${e}`;
}

function testMathTan(): string {
    const a = Math.tan(0);
    const b = Math.tan(Math.PI / 4).toFixed(4);
    const c = Math.tan(Math.PI / 6).toFixed(4);
    const d = Math.tan(-Math.PI / 4).toFixed(4);
    return `${a},${b},${c},${d}`;
}

function testMathAsin(): string {
    const a = Math.asin(0);
    const b = Math.asin(1).toFixed(4);
    const c = Math.asin(-1).toFixed(4);
    const d = Math.asin(0.5).toFixed(4);
    return `${a},${b},${c},${d}`;
}

function testMathAcos(): string {
    const a = Math.acos(1);
    const b = Math.acos(0).toFixed(4);
    const c = Math.acos(-1).toFixed(4);
    const d = Math.acos(0.5).toFixed(4);
    return `${a},${b},${c},${d}`;
}

function testMathAtan(): string {
    const a = Math.atan(0);
    const b = Math.atan(1).toFixed(4);
    const c = Math.atan(-1).toFixed(4);
    const d = Math.atan(Infinity).toFixed(4);
    return `${a},${b},${c},${d}`;
}

function testMathAtan2(): string {
    const a = Math.atan2(0, 1);
    const b = Math.atan2(1, 1).toFixed(4);
    const c = Math.atan2(1, 0).toFixed(4);
    const d = Math.atan2(-1, -1).toFixed(4);
    const e = Math.atan2(0, -1).toFixed(4);
    return `${a},${b},${c},${d},${e}`;
}

function testMathSinh(): string {
    const a = Math.sinh(0);
    const b = Math.sinh(1).toFixed(4);
    const c = Math.sinh(-1).toFixed(4);
    const d = Math.sinh(2).toFixed(4);
    return `${a},${b},${c},${d}`;
}

function testMathCosh(): string {
    const a = Math.cosh(0);
    const b = Math.cosh(1).toFixed(4);
    const c = Math.cosh(-1).toFixed(4);
    const d = Math.cosh(2).toFixed(4);
    return `${a},${b},${c},${d}`;
}

function testMathTanh(): string {
    const a = Math.tanh(0);
    const b = Math.tanh(1).toFixed(4);
    const c = Math.tanh(-1).toFixed(4);
    const d = Math.tanh(Infinity);
    return `${a},${b},${c},${d}`;
}

function testMathAsinh(): string {
    const a = Math.asinh(0);
    const b = Math.asinh(1).toFixed(4);
    const c = Math.asinh(-1).toFixed(4);
    const d = Math.asinh(2).toFixed(4);
    return `${a},${b},${c},${d}`;
}

function testMathAcosh(): string {
    const a = Math.acosh(1);
    const b = Math.acosh(2).toFixed(4);
    const c = Math.acosh(10).toFixed(4);
    const d = Math.acosh(Math.cosh(1)).toFixed(4);
    return `${a},${b},${c},${d}`;
}

function testMathAtanh(): string {
    const a = Math.atanh(0);
    const b = Math.atanh(0.5).toFixed(4);
    const c = Math.atanh(-0.5).toFixed(4);
    const d = Math.atanh(0.9).toFixed(4);
    return `${a},${b},${c},${d}`;
}

// ==================== Math Misc ====================

function testMathMinMax(): string {
    const a = Math.min(1, 2, 3);
    const b = Math.max(1, 2, 3);
    const c = Math.min();
    const d = Math.max();
    const e = Math.min(-1, -2, -3);
    return `${a},${b},${c},${d},${e}`;
}

function testMathMinMaxArrays(): string {
    const arr = [5, 2, 8, 1, 9, 3];
    const a = Math.min(...arr);
    const b = Math.max(...arr);
    const c = Math.min(0, ...arr);
    const d = Math.max(10, ...arr);
    return `${a},${b},${c},${d}`;
}

function testMathRandom(): string {
    const samples = [];
    for (let i = 0; i < 10; i++) {
        const r = Math.random();
        samples.push(r >= 0 && r < 1);
    }
    return `${samples.every(x => x)}`;
}

function testMathClz32(): string {
    const a = Math.clz32(1);
    const b = Math.clz32(2);
    const c = Math.clz32(0);
    const d = Math.clz32(1 << 31);
    const e = Math.clz32(0xFFFFFFFF);
    return `${a},${b},${c},${d},${e}`;
}

function testMathImul(): string {
    const a = Math.imul(2, 4);
    const b = Math.imul(-1, 8);
    const c = Math.imul(0xffffffff, 5);
    const d = Math.imul(0xfffffffe, 5);
    const e = Math.imul(3, 3);
    return `${a},${b},${c},${d},${e}`;
}

function testMathFround(): string {
    const a = Math.fround(1.5);
    const b = Math.fround(1.337).toFixed(6);
    const c = Math.fround(1.5e38);
    const d = Math.fround(0);
    return `${a},${b},${c},${d}`;
}

function testMathConstants(): string {
    const a = Math.PI.toFixed(5);
    const b = Math.E.toFixed(5);
    const c = Math.LN2.toFixed(5);
    const d = Math.LN10.toFixed(5);
    const e = Math.LOG2E.toFixed(5);
    return `${a},${b},${c},${d},${e}`;
}

function testMathConstants2(): string {
    const a = Math.LOG10E.toFixed(5);
    const b = Math.SQRT2.toFixed(5);
    const c = Math.SQRT1_2.toFixed(5);
    return `${a},${b},${c}`;
}

// ==================== Number Methods ====================

function testNumberIsNaN(): string {
    const a = Number.isNaN(NaN);
    const b = Number.isNaN('NaN');
    const c = Number.isNaN(undefined);
    const d = Number.isNaN(0 / 0);
    const e = Number.isNaN(Number.NaN);
    return `${a},${b},${c},${d},${e}`;
}

function testNumberIsFinite(): string {
    const a = Number.isFinite(100);
    const b = Number.isFinite(Infinity);
    const c = Number.isFinite(NaN);
    const d = Number.isFinite(-Infinity);
    const e = Number.isFinite('100');
    return `${a},${b},${c},${d},${e}`;
}

function testNumberIsInteger(): string {
    const a = Number.isInteger(5);
    const b = Number.isInteger(5.0);
    const c = Number.isInteger(5.5);
    const d = Number.isInteger(-10);
    const e = Number.isInteger(0);
    return `${a},${b},${c},${d},${e}`;
}

function testNumberIsSafeInteger(): string {
    const a = Number.isSafeInteger(3);
    const b = Number.isSafeInteger(Math.pow(2, 53));
    const c = Number.isSafeInteger(Math.pow(2, 53) - 1);
    const d = Number.isSafeInteger(3.0);
    const e = Number.isSafeInteger(3.1);
    return `${a},${b},${c},${d},${e}`;
}

function testParseInt(): string {
    const a = parseInt('42');
    const b = parseInt('42.5');
    const c = parseInt('  42  ');
    const d = parseInt('42abc');
    const e = parseInt('abc42');
    return `${a},${b},${c},${d},${isNaN(e)}`;
}

function testParseIntRadix(): string {
    const a = parseInt('1010', 2);
    const b = parseInt('ff', 16);
    const c = parseInt('77', 8);
    const d = parseInt('z', 36);
    const e = parseInt('10', 10);
    return `${a},${b},${c},${d},${e}`;
}

function testParseFloat(): string {
    const a = parseFloat('3.14');
    const b = parseFloat('3.14abc');
    const c = parseFloat('  3.14  ');
    const d = parseFloat('.5');
    const e = parseFloat('-.5');
    return `${a},${b},${c},${d},${e}`;
}

function testParseFloatEdgeCases(): string {
    const a = parseFloat('1e10');
    const b = parseFloat('1e-10');
    const c = parseFloat('Infinity');
    const d = parseFloat('-Infinity');
    const e = isNaN(parseFloat('abc'));
    return `${a},${b},${c},${d},${e}`;
}

function testNumberToFixed(): string {
    const n = 3.14159;
    const a = n.toFixed(2);
    const b = n.toFixed(0);
    const c = n.toFixed(4);
    const d = (123.456).toFixed(1);
    const e = (0.1 + 0.2).toFixed(1);
    return `${a},${b},${c},${d},${e}`;
}

function testNumberToPrecision(): string {
    const n = 123.456;
    const a = n.toPrecision(4);
    const b = n.toPrecision(2);
    const c = n.toPrecision(6);
    const d = (0.000123).toPrecision(2);
    const e = (1234567).toPrecision(3);
    return `${a},${b},${c},${d},${e}`;
}

function testNumberToExponential(): string {
    const a = (12345).toExponential(2);
    const b = (0.00012345).toExponential(2);
    const c = (123.456).toExponential(1);
    const d = (1).toExponential(0);
    const e = (1000000).toExponential(3);
    return `${a},${b},${c},${d},${e}`;
}

function testNumberToString(): string {
    const a = (255).toString(16);
    const b = (255).toString(2);
    const c = (255).toString(8);
    const d = (255).toString(10);
    const e = (100).toString(36);
    return `${a},${b},${c},${d},${e}`;
}

function testNumberConstants(): string {
    const a = Number.MAX_SAFE_INTEGER;
    const b = Number.MIN_SAFE_INTEGER;
    const c = Number.EPSILON > 0;
    const d = Number.MAX_VALUE > 0;
    const e = Number.MIN_VALUE > 0;
    return `${a},${b},${c},${d},${e}`;
}

function testNumberConstants2(): string {
    const a = Number.POSITIVE_INFINITY === Infinity;
    const b = Number.NEGATIVE_INFINITY === -Infinity;
    const c = Number.isNaN(Number.NaN);
    return `${a},${b},${c}`;
}

function testNumberValueOf(): string {
    const a = (42).valueOf();
    const b = new Number(42).valueOf();
    const c = Number.prototype.valueOf.call(42);
    return `${a},${b},${c}`;
}

// ==================== Math Calculations ====================

function testMathDistanceFormula(): string {
    const distance = (x1: number, y1: number, x2: number, y2: number) => {
        return Math.sqrt(Math.pow(x2 - x1, 2) + Math.pow(y2 - y1, 2));
    };
    const a = distance(0, 0, 3, 4);
    const b = distance(0, 0, 1, 1).toFixed(4);
    const c = distance(-1, -1, 2, 3);
    return `${a},${b},${c}`;
}

function testMathQuadraticFormula(): string {
    const quadratic = (a: number, b: number, c: number) => {
        const discriminant = b * b - 4 * a * c;
        if (discriminant < 0) return 'no real roots';
        const x1 = (-b + Math.sqrt(discriminant)) / (2 * a);
        const x2 = (-b - Math.sqrt(discriminant)) / (2 * a);
        return `${x1},${x2}`;
    };
    const a = quadratic(1, -5, 6);
    const b = quadratic(1, 2, 1);
    const c = quadratic(1, 0, -4);
    return `${a}|${b}|${c}`;
}

function testMathDegreesToRadians(): string {
    const toRadians = (deg: number) => deg * (Math.PI / 180);
    const a = toRadians(0);
    const b = toRadians(90).toFixed(4);
    const c = toRadians(180).toFixed(4);
    const d = toRadians(360).toFixed(4);
    return `${a},${b},${c},${d}`;
}

function testMathRadiansToDegrees(): string {
    const toDegrees = (rad: number) => rad * (180 / Math.PI);
    const a = toDegrees(0);
    const b = toDegrees(Math.PI / 2).toFixed(4);
    const c = toDegrees(Math.PI).toFixed(4);
    const d = toDegrees(2 * Math.PI).toFixed(4);
    return `${a},${b},${c},${d}`;
}

function testMathClamp(): string {
    const clamp = (val: number, min: number, max: number) => Math.min(Math.max(val, min), max);
    const a = clamp(5, 0, 10);
    const b = clamp(-5, 0, 10);
    const c = clamp(15, 0, 10);
    const d = clamp(5, 5, 5);
    return `${a},${b},${c},${d}`;
}

function testMathLerp(): string {
    const lerp = (a: number, b: number, t: number) => a + (b - a) * t;
    const a = lerp(0, 10, 0);
    const b = lerp(0, 10, 1);
    const c = lerp(0, 10, 0.5);
    const d = lerp(-10, 10, 0.5);
    return `${a},${b},${c},${d}`;
}

function testMathFactorial(): string {
    const factorial = (n: number): number => n <= 1 ? 1 : n * factorial(n - 1);
    const a = factorial(0);
    const b = factorial(1);
    const c = factorial(5);
    const d = factorial(10);
    return `${a},${b},${c},${d}`;
}

function testMathGCD(): string {
    const gcd = (a: number, b: number): number => b === 0 ? a : gcd(b, a % b);
    const a = gcd(12, 8);
    const b = gcd(100, 35);
    const c = gcd(17, 13);
    const d = gcd(24, 36);
    return `${a},${b},${c},${d}`;
}

function testMathLCM(): string {
    const gcd = (a: number, b: number): number => b === 0 ? a : gcd(b, a % b);
    const lcm = (a: number, b: number) => Math.abs(a * b) / gcd(a, b);
    const a = lcm(4, 6);
    const b = lcm(3, 5);
    const c = lcm(12, 18);
    const d = lcm(7, 11);
    return `${a},${b},${c},${d}`;
}

function testMathIsPrime(): string {
    const isPrime = (n: number): boolean => {
        if (n < 2) return false;
        for (let i = 2; i <= Math.sqrt(n); i++) {
            if (n % i === 0) return false;
        }
        return true;
    };
    const a = isPrime(2);
    const b = isPrime(17);
    const c = isPrime(18);
    const d = isPrime(97);
    const e = isPrime(1);
    return `${a},${b},${c},${d},${e}`;
}

// Warmup
for (let i = 0; i < 20; i++) {
    testMathFloor(); testMathCeil(); testMathRound(); testMathTrunc();
    testMathAbs(); testMathSign(); testMathFloorEdgeCases(); testMathCeilEdgeCases();
    testMathPow(); testMathSqrt(); testMathCbrt(); testMathExp();
    testMathExpm1(); testMathLog(); testMathLog2(); testMathLog10();
    testMathLog1p(); testMathHypot(); testMathHypotMultiple(); testMathPowEdgeCases();
    testMathSin(); testMathCos(); testMathTan(); testMathAsin();
    testMathAcos(); testMathAtan(); testMathAtan2(); testMathSinh();
    testMathCosh(); testMathTanh(); testMathAsinh(); testMathAcosh(); testMathAtanh();
    testMathMinMax(); testMathMinMaxArrays(); testMathRandom();
    testMathClz32(); testMathImul(); testMathFround();
    testMathConstants(); testMathConstants2();
    testNumberIsNaN(); testNumberIsFinite(); testNumberIsInteger(); testNumberIsSafeInteger();
    testParseInt(); testParseIntRadix(); testParseFloat(); testParseFloatEdgeCases();
    testNumberToFixed(); testNumberToPrecision(); testNumberToExponential();
    testNumberToString(); testNumberConstants(); testNumberConstants2(); testNumberValueOf();
    testMathDistanceFormula(); testMathQuadraticFormula();
    testMathDegreesToRadians(); testMathRadiansToDegrees();
    testMathClamp(); testMathLerp(); testMathFactorial();
    testMathGCD(); testMathLCM(); testMathIsPrime();
}

// JIT compile
ArkTools.jitCompileAsync(testMathFloor);
ArkTools.jitCompileAsync(testMathCeil);
ArkTools.jitCompileAsync(testMathRound);
ArkTools.jitCompileAsync(testMathTrunc);
ArkTools.jitCompileAsync(testMathAbs);
ArkTools.jitCompileAsync(testMathSign);
ArkTools.jitCompileAsync(testMathFloorEdgeCases);
ArkTools.jitCompileAsync(testMathCeilEdgeCases);
ArkTools.jitCompileAsync(testMathPow);
ArkTools.jitCompileAsync(testMathSqrt);
ArkTools.jitCompileAsync(testMathCbrt);
ArkTools.jitCompileAsync(testMathExp);
ArkTools.jitCompileAsync(testMathExpm1);
ArkTools.jitCompileAsync(testMathLog);
ArkTools.jitCompileAsync(testMathLog2);
ArkTools.jitCompileAsync(testMathLog10);
ArkTools.jitCompileAsync(testMathLog1p);
ArkTools.jitCompileAsync(testMathHypot);
ArkTools.jitCompileAsync(testMathHypotMultiple);
ArkTools.jitCompileAsync(testMathPowEdgeCases);
ArkTools.jitCompileAsync(testMathSin);
ArkTools.jitCompileAsync(testMathCos);
ArkTools.jitCompileAsync(testMathTan);
ArkTools.jitCompileAsync(testMathAsin);
ArkTools.jitCompileAsync(testMathAcos);
ArkTools.jitCompileAsync(testMathAtan);
ArkTools.jitCompileAsync(testMathAtan2);
ArkTools.jitCompileAsync(testMathSinh);
ArkTools.jitCompileAsync(testMathCosh);
ArkTools.jitCompileAsync(testMathTanh);
ArkTools.jitCompileAsync(testMathAsinh);
ArkTools.jitCompileAsync(testMathAcosh);
ArkTools.jitCompileAsync(testMathAtanh);
ArkTools.jitCompileAsync(testMathMinMax);
ArkTools.jitCompileAsync(testMathMinMaxArrays);
ArkTools.jitCompileAsync(testMathRandom);
ArkTools.jitCompileAsync(testMathClz32);
ArkTools.jitCompileAsync(testMathImul);
ArkTools.jitCompileAsync(testMathFround);
ArkTools.jitCompileAsync(testMathConstants);
ArkTools.jitCompileAsync(testMathConstants2);
ArkTools.jitCompileAsync(testNumberIsNaN);
ArkTools.jitCompileAsync(testNumberIsFinite);
ArkTools.jitCompileAsync(testNumberIsInteger);
ArkTools.jitCompileAsync(testNumberIsSafeInteger);
ArkTools.jitCompileAsync(testParseInt);
ArkTools.jitCompileAsync(testParseIntRadix);
ArkTools.jitCompileAsync(testParseFloat);
ArkTools.jitCompileAsync(testParseFloatEdgeCases);
ArkTools.jitCompileAsync(testNumberToFixed);
ArkTools.jitCompileAsync(testNumberToPrecision);
ArkTools.jitCompileAsync(testNumberToExponential);
ArkTools.jitCompileAsync(testNumberToString);
ArkTools.jitCompileAsync(testNumberConstants);
ArkTools.jitCompileAsync(testNumberConstants2);
ArkTools.jitCompileAsync(testNumberValueOf);
ArkTools.jitCompileAsync(testMathDistanceFormula);
ArkTools.jitCompileAsync(testMathQuadraticFormula);
ArkTools.jitCompileAsync(testMathDegreesToRadians);
ArkTools.jitCompileAsync(testMathRadiansToDegrees);
ArkTools.jitCompileAsync(testMathClamp);
ArkTools.jitCompileAsync(testMathLerp);
ArkTools.jitCompileAsync(testMathFactorial);
ArkTools.jitCompileAsync(testMathGCD);
ArkTools.jitCompileAsync(testMathLCM);
ArkTools.jitCompileAsync(testMathIsPrime);

print("compile testMathFloor: " + ArkTools.waitJitCompileFinish(testMathFloor));
print("compile testMathCeil: " + ArkTools.waitJitCompileFinish(testMathCeil));
print("compile testMathRound: " + ArkTools.waitJitCompileFinish(testMathRound));
print("compile testMathTrunc: " + ArkTools.waitJitCompileFinish(testMathTrunc));
print("compile testMathAbs: " + ArkTools.waitJitCompileFinish(testMathAbs));
print("compile testMathSign: " + ArkTools.waitJitCompileFinish(testMathSign));
print("compile testMathFloorEdgeCases: " + ArkTools.waitJitCompileFinish(testMathFloorEdgeCases));
print("compile testMathCeilEdgeCases: " + ArkTools.waitJitCompileFinish(testMathCeilEdgeCases));
print("compile testMathPow: " + ArkTools.waitJitCompileFinish(testMathPow));
print("compile testMathSqrt: " + ArkTools.waitJitCompileFinish(testMathSqrt));
print("compile testMathCbrt: " + ArkTools.waitJitCompileFinish(testMathCbrt));
print("compile testMathExp: " + ArkTools.waitJitCompileFinish(testMathExp));
print("compile testMathExpm1: " + ArkTools.waitJitCompileFinish(testMathExpm1));
print("compile testMathLog: " + ArkTools.waitJitCompileFinish(testMathLog));
print("compile testMathLog2: " + ArkTools.waitJitCompileFinish(testMathLog2));
print("compile testMathLog10: " + ArkTools.waitJitCompileFinish(testMathLog10));
print("compile testMathLog1p: " + ArkTools.waitJitCompileFinish(testMathLog1p));
print("compile testMathHypot: " + ArkTools.waitJitCompileFinish(testMathHypot));
print("compile testMathHypotMultiple: " + ArkTools.waitJitCompileFinish(testMathHypotMultiple));
print("compile testMathPowEdgeCases: " + ArkTools.waitJitCompileFinish(testMathPowEdgeCases));
print("compile testMathSin: " + ArkTools.waitJitCompileFinish(testMathSin));
print("compile testMathCos: " + ArkTools.waitJitCompileFinish(testMathCos));
print("compile testMathTan: " + ArkTools.waitJitCompileFinish(testMathTan));
print("compile testMathAsin: " + ArkTools.waitJitCompileFinish(testMathAsin));
print("compile testMathAcos: " + ArkTools.waitJitCompileFinish(testMathAcos));
print("compile testMathAtan: " + ArkTools.waitJitCompileFinish(testMathAtan));
print("compile testMathAtan2: " + ArkTools.waitJitCompileFinish(testMathAtan2));
print("compile testMathSinh: " + ArkTools.waitJitCompileFinish(testMathSinh));
print("compile testMathCosh: " + ArkTools.waitJitCompileFinish(testMathCosh));
print("compile testMathTanh: " + ArkTools.waitJitCompileFinish(testMathTanh));
print("compile testMathAsinh: " + ArkTools.waitJitCompileFinish(testMathAsinh));
print("compile testMathAcosh: " + ArkTools.waitJitCompileFinish(testMathAcosh));
print("compile testMathAtanh: " + ArkTools.waitJitCompileFinish(testMathAtanh));
print("compile testMathMinMax: " + ArkTools.waitJitCompileFinish(testMathMinMax));
print("compile testMathMinMaxArrays: " + ArkTools.waitJitCompileFinish(testMathMinMaxArrays));
print("compile testMathRandom: " + ArkTools.waitJitCompileFinish(testMathRandom));
print("compile testMathClz32: " + ArkTools.waitJitCompileFinish(testMathClz32));
print("compile testMathImul: " + ArkTools.waitJitCompileFinish(testMathImul));
print("compile testMathFround: " + ArkTools.waitJitCompileFinish(testMathFround));
print("compile testMathConstants: " + ArkTools.waitJitCompileFinish(testMathConstants));
print("compile testMathConstants2: " + ArkTools.waitJitCompileFinish(testMathConstants2));
print("compile testNumberIsNaN: " + ArkTools.waitJitCompileFinish(testNumberIsNaN));
print("compile testNumberIsFinite: " + ArkTools.waitJitCompileFinish(testNumberIsFinite));
print("compile testNumberIsInteger: " + ArkTools.waitJitCompileFinish(testNumberIsInteger));
print("compile testNumberIsSafeInteger: " + ArkTools.waitJitCompileFinish(testNumberIsSafeInteger));
print("compile testParseInt: " + ArkTools.waitJitCompileFinish(testParseInt));
print("compile testParseIntRadix: " + ArkTools.waitJitCompileFinish(testParseIntRadix));
print("compile testParseFloat: " + ArkTools.waitJitCompileFinish(testParseFloat));
print("compile testParseFloatEdgeCases: " + ArkTools.waitJitCompileFinish(testParseFloatEdgeCases));
print("compile testNumberToFixed: " + ArkTools.waitJitCompileFinish(testNumberToFixed));
print("compile testNumberToPrecision: " + ArkTools.waitJitCompileFinish(testNumberToPrecision));
print("compile testNumberToExponential: " + ArkTools.waitJitCompileFinish(testNumberToExponential));
print("compile testNumberToString: " + ArkTools.waitJitCompileFinish(testNumberToString));
print("compile testNumberConstants: " + ArkTools.waitJitCompileFinish(testNumberConstants));
print("compile testNumberConstants2: " + ArkTools.waitJitCompileFinish(testNumberConstants2));
print("compile testNumberValueOf: " + ArkTools.waitJitCompileFinish(testNumberValueOf));
print("compile testMathDistanceFormula: " + ArkTools.waitJitCompileFinish(testMathDistanceFormula));
print("compile testMathQuadraticFormula: " + ArkTools.waitJitCompileFinish(testMathQuadraticFormula));
print("compile testMathDegreesToRadians: " + ArkTools.waitJitCompileFinish(testMathDegreesToRadians));
print("compile testMathRadiansToDegrees: " + ArkTools.waitJitCompileFinish(testMathRadiansToDegrees));
print("compile testMathClamp: " + ArkTools.waitJitCompileFinish(testMathClamp));
print("compile testMathLerp: " + ArkTools.waitJitCompileFinish(testMathLerp));
print("compile testMathFactorial: " + ArkTools.waitJitCompileFinish(testMathFactorial));
print("compile testMathGCD: " + ArkTools.waitJitCompileFinish(testMathGCD));
print("compile testMathLCM: " + ArkTools.waitJitCompileFinish(testMathLCM));
print("compile testMathIsPrime: " + ArkTools.waitJitCompileFinish(testMathIsPrime));

// Verify
print("testMathFloor: " + testMathFloor());
print("testMathCeil: " + testMathCeil());
print("testMathRound: " + testMathRound());
print("testMathTrunc: " + testMathTrunc());
print("testMathAbs: " + testMathAbs());
print("testMathSign: " + testMathSign());
print("testMathFloorEdgeCases: " + testMathFloorEdgeCases());
print("testMathCeilEdgeCases: " + testMathCeilEdgeCases());
print("testMathPow: " + testMathPow());
print("testMathSqrt: " + testMathSqrt());
print("testMathCbrt: " + testMathCbrt());
print("testMathExp: " + testMathExp());
print("testMathExpm1: " + testMathExpm1());
print("testMathLog: " + testMathLog());
print("testMathLog2: " + testMathLog2());
print("testMathLog10: " + testMathLog10());
print("testMathLog1p: " + testMathLog1p());
print("testMathHypot: " + testMathHypot());
print("testMathHypotMultiple: " + testMathHypotMultiple());
print("testMathPowEdgeCases: " + testMathPowEdgeCases());
print("testMathSin: " + testMathSin());
print("testMathCos: " + testMathCos());
print("testMathTan: " + testMathTan());
print("testMathAsin: " + testMathAsin());
print("testMathAcos: " + testMathAcos());
print("testMathAtan: " + testMathAtan());
print("testMathAtan2: " + testMathAtan2());
print("testMathSinh: " + testMathSinh());
print("testMathCosh: " + testMathCosh());
print("testMathTanh: " + testMathTanh());
print("testMathAsinh: " + testMathAsinh());
print("testMathAcosh: " + testMathAcosh());
print("testMathAtanh: " + testMathAtanh());
print("testMathMinMax: " + testMathMinMax());
print("testMathMinMaxArrays: " + testMathMinMaxArrays());
print("testMathRandom: " + testMathRandom());
print("testMathClz32: " + testMathClz32());
print("testMathImul: " + testMathImul());
print("testMathFround: " + testMathFround());
print("testMathConstants: " + testMathConstants());
print("testMathConstants2: " + testMathConstants2());
print("testNumberIsNaN: " + testNumberIsNaN());
print("testNumberIsFinite: " + testNumberIsFinite());
print("testNumberIsInteger: " + testNumberIsInteger());
print("testNumberIsSafeInteger: " + testNumberIsSafeInteger());
print("testParseInt: " + testParseInt());
print("testParseIntRadix: " + testParseIntRadix());
print("testParseFloat: " + testParseFloat());
print("testParseFloatEdgeCases: " + testParseFloatEdgeCases());
print("testNumberToFixed: " + testNumberToFixed());
print("testNumberToPrecision: " + testNumberToPrecision());
print("testNumberToExponential: " + testNumberToExponential());
print("testNumberToString: " + testNumberToString());
print("testNumberConstants: " + testNumberConstants());
print("testNumberConstants2: " + testNumberConstants2());
print("testNumberValueOf: " + testNumberValueOf());
print("testMathDistanceFormula: " + testMathDistanceFormula());
print("testMathQuadraticFormula: " + testMathQuadraticFormula());
print("testMathDegreesToRadians: " + testMathDegreesToRadians());
print("testMathRadiansToDegrees: " + testMathRadiansToDegrees());
print("testMathClamp: " + testMathClamp());
print("testMathLerp: " + testMathLerp());
print("testMathFactorial: " + testMathFactorial());
print("testMathGCD: " + testMathGCD());
print("testMathLCM: " + testMathLCM());
print("testMathIsPrime: " + testMathIsPrime());
