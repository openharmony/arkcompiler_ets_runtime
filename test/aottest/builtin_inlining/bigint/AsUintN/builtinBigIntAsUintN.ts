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

function doAsUintN(bits: number, b : bigint) {
    return BigInt.asUintN(bits, b);
}

function tryDoAsUintNEmpty() {
    try {
        print(BigInt.asUintN());
    } catch(e) {
        print(e);
    }
}

function tryDoAsUintNSingle(a) {
    try {
        print(BigInt.asUintN(a));
    } catch(e) {
        print(e);
    }
}

function tryDoAsUintN(bits: number, b : bigint) {
    try {
        print(doAsUintN(bits, b));
    } catch(e) {
        print(e);
    }
}

function printAsUintN(bits: number, b : bigint) {
    try {
        print(doAsUintN(bits, b));
    } finally {
    }
}

// Check standart behaviour
// 25n = 00011001
//aot: [trace] aot inline builtin: BigInt.asUintN, caller function name:func_main_0@builtinBigIntAsUintN
print(BigInt.asUintN(3, 25n)); //: 1
//aot: [trace] aot inline builtin: BigInt.asUintN, caller function name:func_main_0@builtinBigIntAsUintN
print(BigInt.asUintN(4, 25n)); //: 9

// Check without params
tryDoAsUintNEmpty(); //: TypeError: Cannot convert a undefine or null value to a BigInt

// Check with 1 param
tryDoAsUintNSingle(15); //: TypeError: Cannot convert a undefine or null value to a BigInt
tryDoAsUintNSingle(23n); //: TypeError: Cannot convert a BigInt value to a number

// Check with 2 params
// 100n = 01100100
//aot: [trace] aot inline builtin: BigInt.asUintN, caller function name:func_main_0@builtinBigIntAsUintN
print(BigInt.asUintN(4, 100n)); //: 4

// Check with 3 params
//aot: [trace] aot inline builtin: BigInt.asUintN, caller function name:func_main_0@builtinBigIntAsUintN
print(BigInt.asUintN(8, 100n, undefined)); //: 100

// Check with 4 params
//aot: [trace] aot inline builtin: BigInt.asUintN, caller function name:func_main_0@builtinBigIntAsUintN
print(BigInt.asUintN(16, 100n, -20000, "abc")); //: 100

// Check some corner cases
//aot: [trace] aot inline builtin: BigInt.asUintN, caller function name:func_main_0@builtinBigIntAsUintN
print(BigInt.asUintN(0, 10000n)); //: 0
//aot: [trace] aot inline builtin: BigInt.asUintN, caller function name:func_main_0@builtinBigIntAsUintN
print(BigInt.asUintN(32, 0n)); //: 0
//aot: [trace] aot inline builtin: BigInt.asUintN, caller function name:#*#doAsUintN@builtinBigIntAsUintN
//aot: [trace] Check Type: RangeError
tryDoAsUintN(-2, 10000n); //: RangeError: integerIndex < 0 or integerIndex > SAFE_NUMBER
//aot: [trace] aot inline builtin: BigInt.asUintN, caller function name:#*#doAsUintN@builtinBigIntAsUintN
//aot: [trace] Check Type: RangeError
tryDoAsUintN(2 ** 53, 100000n); //: RangeError: integerIndex < 0 or integerIndex > SAFE_NUMBER

// bits > kMaxLengthBits => return bigint
//aot: [trace] aot inline builtin: BigInt.asUintN, caller function name:func_main_0@builtinBigIntAsUintN
print(BigInt.asUintN(2 ** 35, 2n ** 75n)); //: 37778931862957161709568
//aot: [trace] aot inline builtin: BigInt.asUintN, caller function name:func_main_0@builtinBigIntAsUintN
print(BigInt.asUintN(2 ** 35, 65531n)); //: 65531

// Check maximum and minimum values
const max32 = 2n ** 32n - 1n; // UINT32_MAX = 4294967295
const max64 = 2n ** 64n - 1n; // UINT64_MAX = 18446744073709551615

//aot: [trace] aot inline builtin: BigInt.asUintN, caller function name:func_main_0@builtinBigIntAsUintN
print(BigInt.asUintN(32, max32)); //: 4294967295
//aot: [trace] aot inline builtin: BigInt.asUintN, caller function name:func_main_0@builtinBigIntAsUintN
print(BigInt.asUintN(32, max32 + 1n)); //: 0
//aot: [trace] aot inline builtin: BigInt.asUintN, caller function name:func_main_0@builtinBigIntAsUintN
print(BigInt.asUintN(64, max64)); //: 18446744073709551615
//aot: [trace] aot inline builtin: BigInt.asUintN, caller function name:func_main_0@builtinBigIntAsUintN
print(BigInt.asUintN(64, max64 + 1n)); //: 0
//aot: [trace] aot inline builtin: BigInt.asUintN, caller function name:func_main_0@builtinBigIntAsUintN
print(BigInt.asUintN(32, -max32)); //: 1
//aot: [trace] aot inline builtin: BigInt.asUintN, caller function name:func_main_0@builtinBigIntAsUintN
print(BigInt.asUintN(32, -max32 - 1n)); //: 0
//aot: [trace] aot inline builtin: BigInt.asUintN, caller function name:func_main_0@builtinBigIntAsUintN
print(BigInt.asUintN(32, -max32 - 2n)); //: 4294967295
//aot: [trace] aot inline builtin: BigInt.asUintN, caller function name:func_main_0@builtinBigIntAsUintN
print(BigInt.asUintN(64, -max64)); //: 1
//aot: [trace] aot inline builtin: BigInt.asUintN, caller function name:func_main_0@builtinBigIntAsUintN
print(BigInt.asUintN(64, -max64 - 1n)); //: 0
//aot: [trace] aot inline builtin: BigInt.asUintN, caller function name:func_main_0@builtinBigIntAsUintN
print(BigInt.asUintN(64, -max64 - 2n)); //: 18446744073709551615

// Replace standard builtin
let true_asUintN = BigInt.asUintN
BigInt.asUintN = replace
print(BigInt.asUintN(-1.001, 26n)); //: -1.001
BigInt.asUintN = true_asUintN
//aot: [trace] aot inline builtin: BigInt.asUintN, caller function name:#*#doAsUintN@builtinBigIntAsUintN
printAsUintN(3, 25n); //: 1
//aot: [trace] aot inline builtin: BigInt.asUintN, caller function name:func_main_0@builtinBigIntAsUintN
print(true_asUintN(3, 25n)); //: 1

// Call standard builtin with non-number param
//aot: [trace] aot inline builtin: BigInt.asUintN, caller function name:#*#doAsUintN@builtinBigIntAsUintN
//aot: [trace] Check Type: NotNumber1
tryDoAsUintN("abc", "abc"); //: SyntaxError: Cannot convert string to a BigInt,because not allow Infinity, decimal points, or exponents
//aot: [trace] aot inline builtin: BigInt.asUintN, caller function name:#*#doAsUintN@builtinBigIntAsUintN
//aot: [trace] Check Type: NotNumber1
printAsUintN("3", 25n); //: 1

if (ArkTools.isAOTCompiled(printAsUintN)) {
    // Replace standard builtin after call to standard builtin was profiled
    BigInt.asUintN = replace
}

printAsUintN(3, 25n); //pgo: 1
//aot: [trace] Check Type: NotCallTarget1
//aot: 3

tryDoAsUintN("abc", "abc"); //pgo: SyntaxError: Cannot convert string to a BigInt,because not allow Infinity, decimal points, or exponents
//aot: [trace] Check Type: NotCallTarget1
//aot: abc

BigInt.asUintN = true_asUintN

// Check IR correctness inside try-block
try {
    //aot: [trace] aot inline builtin: BigInt.asUintN, caller function name:#*#doAsUintN@builtinBigIntAsUintN
    printAsUintN(3, 25n); //: 1
    //aot: [trace] aot inline builtin: BigInt.asUintN, caller function name:#*#doAsUintN@builtinBigIntAsUintN
    //aot: [trace] Check Type: NotNumber1
    printAsUintN("abc", "abc");
} catch (e) {
}

let obj = {
    valueOf: () => { return 5; }
};

// 25n = 00011001
//aot: [trace] aot inline builtin: BigInt.asUintN, caller function name:func_main_0@builtinBigIntAsUintN
//aot: [trace] Check Type: NotNumber1
print(BigInt.asUintN(obj, 25n)); //: 25

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
    print(BigInt.asUintN(throwingObj, 42n)); //: 10
    throwingObj.value = -8;
    print(BigInt.asUintN(throwingObj, 42n));
} catch(e) {
    print(e); //: Error: negative bitness
} finally {
    // 15n = 00001111
    print(BigInt.asUintN(obj, 15n)); //: 15
}
