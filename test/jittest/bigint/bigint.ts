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

// ==================== BigInt Creation ====================

function testBigIntLiteral(): string {
    const a = 123n;
    const b = 9007199254740993n;
    return `${a},${b}`;
}

function testBigIntConstructor(): string {
    const a = BigInt(123);
    const b = BigInt('9007199254740993');
    const c = BigInt('0xff');
    return `${a},${b},${c}`;
}

function testBigIntFromHex(): string {
    const a = BigInt('0x1a');
    const b = BigInt('0xFF');
    const c = BigInt('0x100');
    return `${a},${b},${c}`;
}

function testBigIntFromBinary(): string {
    const a = BigInt('0b1010');
    const b = BigInt('0b11111111');
    return `${a},${b}`;
}

function testBigIntFromOctal(): string {
    const a = BigInt('0o12');
    const b = BigInt('0o377');
    return `${a},${b}`;
}

// ==================== BigInt Arithmetic ====================

function testBigIntAddition(): string {
    const a = 10n;
    const b = 20n;
    const c = 9007199254740991n + 2n;
    return `${a + b},${c}`;
}

function testBigIntSubtraction(): string {
    const a = 100n - 30n;
    const b = 9007199254740993n - 1n;
    return `${a},${b}`;
}

function testBigIntMultiplication(): string {
    const a = 100n * 200n;
    const b = 9007199254740991n * 2n;
    return `${a},${b}`;
}

function testBigIntDivision(): string {
    const a = 100n / 3n;
    const b = 9007199254740993n / 2n;
    return `${a},${b}`;
}

function testBigIntRemainder(): string {
    const a = 100n % 7n;
    const b = 9007199254740993n % 10n;
    return `${a},${b}`;
}

function testBigIntExponentiation(): string {
    const a = 2n ** 10n;
    const b = 10n ** 20n;
    return `${a},${b}`;
}

function testBigIntNegation(): string {
    const a = -100n;
    const b = -(-50n);
    return `${a},${b}`;
}

// ==================== BigInt Comparison ====================

function testBigIntComparison(): string {
    const a = 10n;
    const b = 20n;
    const c = 10n;
    return `${a < b},${a > b},${a === c},${a !== b}`;
}

function testBigIntMixedComparison(): string {
    const a = 10n;
    const b = 10;
    return `${a == b},${a === (b as any)},${10n > 9},${10n < 11}`;
}

function testBigIntSort(): string {
    const arr = [3n, 1n, 4n, 1n, 5n, 9n, 2n, 6n];
    arr.sort((a, b) => (a < b ? -1 : a > b ? 1 : 0));
    return arr.slice(0, 5).join(',');
}

// ==================== BigInt Bitwise Operations ====================

function testBigIntBitwiseAnd(): string {
    const a = 0b1010n & 0b1100n;
    const b = 255n & 15n;
    return `${a},${b}`;
}

function testBigIntBitwiseOr(): string {
    const a = 0b1010n | 0b1100n;
    const b = 240n | 15n;
    return `${a},${b}`;
}

function testBigIntBitwiseXor(): string {
    const a = 0b1010n ^ 0b1100n;
    const b = 255n ^ 15n;
    return `${a},${b}`;
}

function testBigIntBitwiseNot(): string {
    const a = ~0n;
    const b = ~1n;
    const c = ~(-1n);
    return `${a},${b},${c}`;
}

function testBigIntLeftShift(): string {
    const a = 1n << 10n;
    const b = 255n << 8n;
    return `${a},${b}`;
}

function testBigIntRightShift(): string {
    const a = 1024n >> 3n;
    const b = 65535n >> 8n;
    return `${a},${b}`;
}

// ==================== BigInt Conversion ====================

function testBigIntToString(): string {
    const a = 255n;
    return `${a.toString()},${a.toString(2)},${a.toString(16)}`;
}

function testBigIntToLocaleString(): string {
    const a = 1234567890n;
    const str = a.toString();
    return `${str.length > 0}`;
}

function testBigIntValueOf(): string {
    const a = 123n;
    return `${a.valueOf() === 123n}`;
}

function testBigIntAsIntN(): string {
    const a = BigInt.asIntN(8, 255n);
    const b = BigInt.asIntN(8, 256n);
    const c = BigInt.asIntN(8, -1n);
    return `${a},${b},${c}`;
}

function testBigIntAsUintN(): string {
    const a = BigInt.asUintN(8, 255n);
    const b = BigInt.asUintN(8, 256n);
    const c = BigInt.asUintN(8, -1n);
    return `${a},${b},${c}`;
}

// ==================== BigInt Type Checking ====================

function testBigIntTypeof(): string {
    const a = 123n;
    return `${typeof a}`;
}

function testBigIntInstanceCheck(): string {
    const a = 123n;
    const b = BigInt(456);
    return `${typeof a === 'bigint'},${typeof b === 'bigint'}`;
}

// ==================== BigInt Edge Cases ====================

function testBigIntZero(): string {
    const a = 0n;
    const b = -0n;
    return `${a === b},${a === 0n}`;
}

function testBigIntLargeNumbers(): string {
    const a = 12345678901234567890123456789012345678901234567890n;
    const b = a + 1n;
    return `${b > a},${(b - a) === 1n}`;
}

function testBigIntNegative(): string {
    const a = -123n;
    const b = -9007199254740993n;
    return `${a},${b}`;
}

// ==================== BigInt Operations in Functions ====================

function testBigIntFactorial(): string {
    function factorial(n: bigint): bigint {
        if (n <= 1n) return 1n;
        return n * factorial(n - 1n);
    }
    return `${factorial(20n)}`;
}

function testBigIntFibonacci(): string {
    function fib(n: number): bigint {
        if (n <= 1) return BigInt(n);
        let a = 0n;
        let b = 1n;
        for (let i = 2; i <= n; i++) {
            [a, b] = [b, a + b];
        }
        return b;
    }
    return `${fib(50)}`;
}

function testBigIntGCD(): string {
    function gcd(a: bigint, b: bigint): bigint {
        while (b !== 0n) {
            [a, b] = [b, a % b];
        }
        return a;
    }
    return `${gcd(48n, 18n)},${gcd(100n, 35n)}`;
}

function testBigIntPower(): string {
    function power(base: bigint, exp: bigint): bigint {
        let result = 1n;
        while (exp > 0n) {
            if (exp % 2n === 1n) {
                result *= base;
            }
            base *= base;
            exp /= 2n;
        }
        return result;
    }
    return `${power(2n, 20n)}`;
}

function testBigIntIsPrime(): string {
    function isPrime(n: bigint): boolean {
        if (n < 2n) return false;
        if (n === 2n) return true;
        if (n % 2n === 0n) return false;
        for (let i = 3n; i * i <= n; i += 2n) {
            if (n % i === 0n) return false;
        }
        return true;
    }
    return `${isPrime(2n)},${isPrime(17n)},${isPrime(18n)},${isPrime(104729n)}`;
}

// ==================== BigInt Array Operations ====================

function testBigIntInArray(): string {
    const arr: bigint[] = [1n, 2n, 3n, 4n, 5n];
    const sum = arr.reduce((a, b) => a + b, 0n);
    const product = arr.reduce((a, b) => a * b, 1n);
    return `${sum},${product}`;
}

function testBigIntArrayMap(): string {
    const arr = [1n, 2n, 3n, 4n, 5n];
    const doubled = arr.map(x => x * 2n);
    return doubled.join(',');
}

function testBigIntArrayFilter(): string {
    const arr = [1n, 2n, 3n, 4n, 5n, 6n, 7n, 8n, 9n, 10n];
    const evens = arr.filter(x => x % 2n === 0n);
    return evens.join(',');
}

// ==================== BigInt with JSON ====================

function testBigIntJSON(): string {
    const obj = { value: 123n };
    try {
        JSON.stringify(obj);
        return 'no error';
    } catch (e) {
        return 'error';
    }
}

function testBigIntJSONWorkaround(): string {
    const obj = { value: 123n };
    const json = JSON.stringify(obj, (_, v) =>
        typeof v === 'bigint' ? v.toString() : v
    );
    return `${json.includes('123')}`;
}

// Warmup
for (let i = 0; i < 20; i++) {
    testBigIntLiteral();
    testBigIntConstructor();
    testBigIntFromHex();
    testBigIntFromBinary();
    testBigIntFromOctal();
    testBigIntAddition();
    testBigIntSubtraction();
    testBigIntMultiplication();
    testBigIntDivision();
    testBigIntRemainder();
    testBigIntExponentiation();
    testBigIntNegation();
    testBigIntComparison();
    testBigIntMixedComparison();
    testBigIntSort();
    testBigIntBitwiseAnd();
    testBigIntBitwiseOr();
    testBigIntBitwiseXor();
    testBigIntBitwiseNot();
    testBigIntLeftShift();
    testBigIntRightShift();
    testBigIntToString();
    testBigIntToLocaleString();
    testBigIntValueOf();
    testBigIntAsIntN();
    testBigIntAsUintN();
    testBigIntTypeof();
    testBigIntInstanceCheck();
    testBigIntZero();
    testBigIntLargeNumbers();
    testBigIntNegative();
    testBigIntFactorial();
    testBigIntFibonacci();
    testBigIntGCD();
    testBigIntPower();
    testBigIntIsPrime();
    testBigIntInArray();
    testBigIntArrayMap();
    testBigIntArrayFilter();
    testBigIntJSON();
    testBigIntJSONWorkaround();
}

// JIT compile
ArkTools.jitCompileAsync(testBigIntLiteral);
ArkTools.jitCompileAsync(testBigIntConstructor);
ArkTools.jitCompileAsync(testBigIntFromHex);
ArkTools.jitCompileAsync(testBigIntFromBinary);
ArkTools.jitCompileAsync(testBigIntFromOctal);
ArkTools.jitCompileAsync(testBigIntAddition);
ArkTools.jitCompileAsync(testBigIntSubtraction);
ArkTools.jitCompileAsync(testBigIntMultiplication);
ArkTools.jitCompileAsync(testBigIntDivision);
ArkTools.jitCompileAsync(testBigIntRemainder);
ArkTools.jitCompileAsync(testBigIntExponentiation);
ArkTools.jitCompileAsync(testBigIntNegation);
ArkTools.jitCompileAsync(testBigIntComparison);
ArkTools.jitCompileAsync(testBigIntMixedComparison);
ArkTools.jitCompileAsync(testBigIntSort);
ArkTools.jitCompileAsync(testBigIntBitwiseAnd);
ArkTools.jitCompileAsync(testBigIntBitwiseOr);
ArkTools.jitCompileAsync(testBigIntBitwiseXor);
ArkTools.jitCompileAsync(testBigIntBitwiseNot);
ArkTools.jitCompileAsync(testBigIntLeftShift);
ArkTools.jitCompileAsync(testBigIntRightShift);
ArkTools.jitCompileAsync(testBigIntToString);
ArkTools.jitCompileAsync(testBigIntToLocaleString);
ArkTools.jitCompileAsync(testBigIntValueOf);
ArkTools.jitCompileAsync(testBigIntAsIntN);
ArkTools.jitCompileAsync(testBigIntAsUintN);
ArkTools.jitCompileAsync(testBigIntTypeof);
ArkTools.jitCompileAsync(testBigIntInstanceCheck);
ArkTools.jitCompileAsync(testBigIntZero);
ArkTools.jitCompileAsync(testBigIntLargeNumbers);
ArkTools.jitCompileAsync(testBigIntNegative);
ArkTools.jitCompileAsync(testBigIntFactorial);
ArkTools.jitCompileAsync(testBigIntFibonacci);
ArkTools.jitCompileAsync(testBigIntGCD);
ArkTools.jitCompileAsync(testBigIntPower);
ArkTools.jitCompileAsync(testBigIntIsPrime);
ArkTools.jitCompileAsync(testBigIntInArray);
ArkTools.jitCompileAsync(testBigIntArrayMap);
ArkTools.jitCompileAsync(testBigIntArrayFilter);
ArkTools.jitCompileAsync(testBigIntJSON);
ArkTools.jitCompileAsync(testBigIntJSONWorkaround);

print("compile testBigIntLiteral: " + ArkTools.waitJitCompileFinish(testBigIntLiteral));
print("compile testBigIntConstructor: " + ArkTools.waitJitCompileFinish(testBigIntConstructor));
print("compile testBigIntFromHex: " + ArkTools.waitJitCompileFinish(testBigIntFromHex));
print("compile testBigIntFromBinary: " + ArkTools.waitJitCompileFinish(testBigIntFromBinary));
print("compile testBigIntFromOctal: " + ArkTools.waitJitCompileFinish(testBigIntFromOctal));
print("compile testBigIntAddition: " + ArkTools.waitJitCompileFinish(testBigIntAddition));
print("compile testBigIntSubtraction: " + ArkTools.waitJitCompileFinish(testBigIntSubtraction));
print("compile testBigIntMultiplication: " + ArkTools.waitJitCompileFinish(testBigIntMultiplication));
print("compile testBigIntDivision: " + ArkTools.waitJitCompileFinish(testBigIntDivision));
print("compile testBigIntRemainder: " + ArkTools.waitJitCompileFinish(testBigIntRemainder));
print("compile testBigIntExponentiation: " + ArkTools.waitJitCompileFinish(testBigIntExponentiation));
print("compile testBigIntNegation: " + ArkTools.waitJitCompileFinish(testBigIntNegation));
print("compile testBigIntComparison: " + ArkTools.waitJitCompileFinish(testBigIntComparison));
print("compile testBigIntMixedComparison: " + ArkTools.waitJitCompileFinish(testBigIntMixedComparison));
print("compile testBigIntSort: " + ArkTools.waitJitCompileFinish(testBigIntSort));
print("compile testBigIntBitwiseAnd: " + ArkTools.waitJitCompileFinish(testBigIntBitwiseAnd));
print("compile testBigIntBitwiseOr: " + ArkTools.waitJitCompileFinish(testBigIntBitwiseOr));
print("compile testBigIntBitwiseXor: " + ArkTools.waitJitCompileFinish(testBigIntBitwiseXor));
print("compile testBigIntBitwiseNot: " + ArkTools.waitJitCompileFinish(testBigIntBitwiseNot));
print("compile testBigIntLeftShift: " + ArkTools.waitJitCompileFinish(testBigIntLeftShift));
print("compile testBigIntRightShift: " + ArkTools.waitJitCompileFinish(testBigIntRightShift));
print("compile testBigIntToString: " + ArkTools.waitJitCompileFinish(testBigIntToString));
print("compile testBigIntToLocaleString: " + ArkTools.waitJitCompileFinish(testBigIntToLocaleString));
print("compile testBigIntValueOf: " + ArkTools.waitJitCompileFinish(testBigIntValueOf));
print("compile testBigIntAsIntN: " + ArkTools.waitJitCompileFinish(testBigIntAsIntN));
print("compile testBigIntAsUintN: " + ArkTools.waitJitCompileFinish(testBigIntAsUintN));
print("compile testBigIntTypeof: " + ArkTools.waitJitCompileFinish(testBigIntTypeof));
print("compile testBigIntInstanceCheck: " + ArkTools.waitJitCompileFinish(testBigIntInstanceCheck));
print("compile testBigIntZero: " + ArkTools.waitJitCompileFinish(testBigIntZero));
print("compile testBigIntLargeNumbers: " + ArkTools.waitJitCompileFinish(testBigIntLargeNumbers));
print("compile testBigIntNegative: " + ArkTools.waitJitCompileFinish(testBigIntNegative));
print("compile testBigIntFactorial: " + ArkTools.waitJitCompileFinish(testBigIntFactorial));
print("compile testBigIntFibonacci: " + ArkTools.waitJitCompileFinish(testBigIntFibonacci));
print("compile testBigIntGCD: " + ArkTools.waitJitCompileFinish(testBigIntGCD));
print("compile testBigIntPower: " + ArkTools.waitJitCompileFinish(testBigIntPower));
print("compile testBigIntIsPrime: " + ArkTools.waitJitCompileFinish(testBigIntIsPrime));
print("compile testBigIntInArray: " + ArkTools.waitJitCompileFinish(testBigIntInArray));
print("compile testBigIntArrayMap: " + ArkTools.waitJitCompileFinish(testBigIntArrayMap));
print("compile testBigIntArrayFilter: " + ArkTools.waitJitCompileFinish(testBigIntArrayFilter));
print("compile testBigIntJSON: " + ArkTools.waitJitCompileFinish(testBigIntJSON));
print("compile testBigIntJSONWorkaround: " + ArkTools.waitJitCompileFinish(testBigIntJSONWorkaround));

// Verify
print("testBigIntLiteral: " + testBigIntLiteral());
print("testBigIntConstructor: " + testBigIntConstructor());
print("testBigIntFromHex: " + testBigIntFromHex());
print("testBigIntFromBinary: " + testBigIntFromBinary());
print("testBigIntFromOctal: " + testBigIntFromOctal());
print("testBigIntAddition: " + testBigIntAddition());
print("testBigIntSubtraction: " + testBigIntSubtraction());
print("testBigIntMultiplication: " + testBigIntMultiplication());
print("testBigIntDivision: " + testBigIntDivision());
print("testBigIntRemainder: " + testBigIntRemainder());
print("testBigIntExponentiation: " + testBigIntExponentiation());
print("testBigIntNegation: " + testBigIntNegation());
print("testBigIntComparison: " + testBigIntComparison());
print("testBigIntMixedComparison: " + testBigIntMixedComparison());
print("testBigIntSort: " + testBigIntSort());
print("testBigIntBitwiseAnd: " + testBigIntBitwiseAnd());
print("testBigIntBitwiseOr: " + testBigIntBitwiseOr());
print("testBigIntBitwiseXor: " + testBigIntBitwiseXor());
print("testBigIntBitwiseNot: " + testBigIntBitwiseNot());
print("testBigIntLeftShift: " + testBigIntLeftShift());
print("testBigIntRightShift: " + testBigIntRightShift());
print("testBigIntToString: " + testBigIntToString());
print("testBigIntToLocaleString: " + testBigIntToLocaleString());
print("testBigIntValueOf: " + testBigIntValueOf());
print("testBigIntAsIntN: " + testBigIntAsIntN());
print("testBigIntAsUintN: " + testBigIntAsUintN());
print("testBigIntTypeof: " + testBigIntTypeof());
print("testBigIntInstanceCheck: " + testBigIntInstanceCheck());
print("testBigIntZero: " + testBigIntZero());
print("testBigIntLargeNumbers: " + testBigIntLargeNumbers());
print("testBigIntNegative: " + testBigIntNegative());
print("testBigIntFactorial: " + testBigIntFactorial());
print("testBigIntFibonacci: " + testBigIntFibonacci());
print("testBigIntGCD: " + testBigIntGCD());
print("testBigIntPower: " + testBigIntPower());
print("testBigIntIsPrime: " + testBigIntIsPrime());
print("testBigIntInArray: " + testBigIntInArray());
print("testBigIntArrayMap: " + testBigIntArrayMap());
print("testBigIntArrayFilter: " + testBigIntArrayFilter());
print("testBigIntJSON: " + testBigIntJSON());
print("testBigIntJSONWorkaround: " + testBigIntJSONWorkaround());
