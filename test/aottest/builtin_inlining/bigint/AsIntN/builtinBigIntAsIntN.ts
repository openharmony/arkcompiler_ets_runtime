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

declare interface ArkTools {
    isAOTCompiled(args: any): boolean;
}
declare function print(arg:any):string;
function replace(a, b)
{
    return a;
}

function doAsIntN(bits: number, b : bigint) {
    return BigInt.asIntN(bits, b);
}

function tryDoAsIntN(bits: number, b : bigint) {
    try {
        print(doAsIntN(bits, b));
    } catch(e) {
        print(e);
    }
}

function tryDoAsIntNEmpty() {
    try {
        print(BigInt.asIntN());
    } catch(e) {
        print(e);
    }
}

function tryDoAsIntNSingle(a) {
    try {
        print(BigInt.asIntN(a));
    } catch(e) {
        print(e);
    }
}

function printAsIntN(bits: number, b : bigint) {
    try {
        print(doAsIntN(bits, b));
    } finally {
    }
}

// Check standart behaviour
// 25n = 00011001
//aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:func_main_0@builtinBigIntAsIntN
print(BigInt.asIntN(3, 25n)); //: 1
//aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:func_main_0@builtinBigIntAsIntN
print(BigInt.asIntN(4, 25n)); //: -7

// Check without params
tryDoAsIntNEmpty(); //: TypeError: Cannot convert a undefine or null value to a BigInt

// Check with 1 param
tryDoAsIntNSingle(16); //: TypeError: Cannot convert a undefine or null value to a BigInt
tryDoAsIntNSingle(23n); //: TypeError: Cannot convert a BigInt value to a number

// Check with 2 params
// 100n = 01100100
//aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:func_main_0@builtinBigIntAsIntN
print(BigInt.asIntN(4, 100n)); //: 4

// Check with 3 params
//aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:func_main_0@builtinBigIntAsIntN
print(BigInt.asIntN(8, 100n, undefined)); //: 100

// Check with 4 params
//aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:func_main_0@builtinBigIntAsIntN
print(BigInt.asIntN(16, 100n, -20000, "abc")); //: 100

// Check some corner cases 
//aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:func_main_0@builtinBigIntAsIntN
print(BigInt.asIntN(0, 10000n)); //: 0
//aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:func_main_0@builtinBigIntAsIntN
print(BigInt.asIntN(32, 0n)); //: 0
//aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:#*#doAsIntN@builtinBigIntAsIntN
//aot: [trace] Check Type: RangeError
tryDoAsIntN(-2, 10000n); //: RangeError: integerIndex < 0 or integerIndex > SAFE_NUMBER
//aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:#*#doAsIntN@builtinBigIntAsIntN
//aot: [trace] Check Type: RangeError
tryDoAsIntN(2 ** 53, 100000n); //: RangeError: integerIndex < 0 or integerIndex > SAFE_NUMBER

// bits > kMaxLengthBits => return bigint
//aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:func_main_0@builtinBigIntAsIntN
print(BigInt.asIntN(2 ** 35, 2n ** 75n)); //: 37778931862957161709568
//aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:func_main_0@builtinBigIntAsIntN
print(BigInt.asIntN(2 ** 35, 65531n)); //: 65531

// Check maximum and minimum values
const max32 = 2n ** (32n - 1n) - 1n; // INT32_MAX = 2147483647
const max32_possible_value = 2n ** 32n - 1n; // 11...11b
const max64 = 2n ** (64n - 1n) - 1n; // INT64_MAX = 9223372036854775807
const max64_possible_value = 2n ** 64n - 1n; // 11...11b

//aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:func_main_0@builtinBigIntAsIntN
print(BigInt.asIntN(32, max32)); //: 2147483647
//aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:func_main_0@builtinBigIntAsIntN
print(BigInt.asIntN(32, max32 + 1n)); //: -2147483648
//aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:func_main_0@builtinBigIntAsIntN
print(BigInt.asIntN(32, max32_possible_value)); //: -1
//aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:func_main_0@builtinBigIntAsIntN
print(BigInt.asIntN(32, max32_possible_value + 1n)); //: 0
//aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:func_main_0@builtinBigIntAsIntN
print(BigInt.asIntN(64, max64)); //: 9223372036854775807
//aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:func_main_0@builtinBigIntAsIntN
print(BigInt.asIntN(64, max64 + 1n)); //: -9223372036854775808
//aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:func_main_0@builtinBigIntAsIntN
print(BigInt.asIntN(64, max64_possible_value)); //: -1
//aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:func_main_0@builtinBigIntAsIntN
print(BigInt.asIntN(64, max64_possible_value + 1n)); //: 0
//aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:func_main_0@builtinBigIntAsIntN
print(BigInt.asIntN(32, -max32 - 1n)); //: -2147483648
//aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:func_main_0@builtinBigIntAsIntN
print(BigInt.asIntN(32, -max32 - 2n)); //: 2147483647
//aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:func_main_0@builtinBigIntAsIntN
print(BigInt.asIntN(32, -max32_possible_value)); //: 1
//aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:func_main_0@builtinBigIntAsIntN
print(BigInt.asIntN(32, -max32_possible_value - 1n)); //: 0
//aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:func_main_0@builtinBigIntAsIntN
print(BigInt.asIntN(64, -max64 - 1n)); //: -9223372036854775808
//aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:func_main_0@builtinBigIntAsIntN
print(BigInt.asIntN(64, -max64 - 2n)); //: 9223372036854775807
//aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:func_main_0@builtinBigIntAsIntN
print(BigInt.asIntN(64, -max64_possible_value)); //: 1
//aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:func_main_0@builtinBigIntAsIntN
print(BigInt.asIntN(64, -max64_possible_value - 1n)); //: 0

// Replace standard builtin
let true_asintn = BigInt.asIntN
BigInt.asIntN = replace
print(BigInt.asIntN(-1.001, 26n)); //: -1.001
BigInt.asIntN = true_asintn
//aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:#*#doAsIntN@builtinBigIntAsIntN
printAsIntN(3, 25n); //: 1
//aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:func_main_0@builtinBigIntAsIntN
print(true_asintn(3, 25n)); //: 1

// Call standard builtin with non-number param
//aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:#*#doAsIntN@builtinBigIntAsIntN
//aot: [trace] Check Type: NotNumber1
tryDoAsIntN("abc", "abc"); //: SyntaxError: Cannot convert string to a BigInt,because not allow Infinity, decimal points, or exponents
//aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:#*#doAsIntN@builtinBigIntAsIntN
//aot: [trace] Check Type: NotNumber1
printAsIntN("3", 25n); //: 1

if (ArkTools.isAOTCompiled(printAsIntN)) {
    // Replace standard builtin after call to standard builtin was profiled
    BigInt.asIntN = replace
}

printAsIntN(3, 25n); //pgo: 1
//aot: [trace] Check Type: NotCallTarget1
//aot: 3

tryDoAsIntN("abc", "abc"); //pgo: SyntaxError: Cannot convert string to a BigInt,because not allow Infinity, decimal points, or exponents
//aot: [trace] Check Type: NotCallTarget1
//aot: abc

BigInt.asIntN = true_asintn

// Check IR correctness inside try-block
try {
    //aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:#*#doAsIntN@builtinBigIntAsIntN
    printAsIntN(3, 25n); //: 1
    //aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:#*#doAsIntN@builtinBigIntAsIntN
    //aot: [trace] Check Type: NotNumber1
    printAsIntN("abc", "abc");
} catch (e) {
}

let obj = {
    valueOf: () => { return 5; }
};

// 25n = 00011001
//aot: [trace] aot inline builtin: BigInt.asIntN, caller function name:func_main_0@builtinBigIntAsIntN
//aot: [trace] Check Type: NotNumber1
print(BigInt.asIntN(obj, 25n)); //: -7

function Throwing() {
    this.value = 5;
}
Throwing.prototype.valueOf = function() {
    if (this.value < 0) {
        throw new Error("negative bitness");
    }
    return this.value;
}
let throwingObj = new Throwing();

try {
    // 42n = 00101010
    print(BigInt.asIntN(throwingObj, 42n)); //: 10
    throwingObj.value = -8;
    print(BigInt.asIntN(throwingObj, 42n));
} catch(e) {
    print(e); //: Error: negative bitness
} finally {
    // 15n = 00001111
    print(BigInt.asIntN(obj, 15n)); //: 15
}
