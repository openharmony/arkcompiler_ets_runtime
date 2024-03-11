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
function replace(a : number)
{
    return a;
}

function doAbs(x: any): number {
    return Math.abs(x);
}

function printAbs(x: any) {
    try {
        print(doAbs(x));
    } finally {
    }
}

// Check without params
print(Math.abs()); // NaN

// Check with single int param
print(Math.abs(0)); // 0
print(Math.abs(3)); // 3
print(Math.abs(-5)); // 5

// Check with single float param
print(Math.abs(-1.5));
print(Math.abs(Math.PI));
print(Math.abs(-Math.PI));
print(Math.abs(-1.9e80));
print(Math.abs(1.9e80));
print(Math.abs(-1.9e-80));
print(Math.abs(1.9e-80));

// Check with special float params
print(Math.abs(Infinity)); // Infinity
print(Math.abs(-Infinity)); // Infinity
print(Math.abs(NaN)); // NaN

print("1/x: " + 1 / Math.abs(-0));

// Check with 2 params
print(Math.abs(3, 0)); // 3

// Check with 3 params
print(Math.abs(-3, 0, 0)); // 3

// Check with 4 params
print(Math.abs(4, 0, 0, 0)); // 4

// Check with 5 params
print(Math.abs(-4, 0, 0, 0, 0)); // 4

// Replace standard builtin
let true_abs = Math.abs
Math.abs = replace
print(Math.abs(-1.001)); // -1.001, no deopt
Math.abs = true_abs

// Check edge cases
const INT_MAX: number = 2147483647;
const INT_MIN: number = -INT_MAX - 1;
print(Math.abs(INT_MAX)); // INT_MAX
print(Math.abs(2147483648)); // INT_MAX + 1
print(Math.abs(-INT_MAX)); // INT_MAX
print(Math.abs(INT_MIN)); // -INT_MIN
print(Math.abs(INT_MIN - 1)); // -INT_MIN + 1

printAbs(-3);
printAbs("abc");
printAbs("abc");

printAbs(-12); // 12
// Call standard builtin with non-number param
printAbs("abc"); // NaN
printAbs("-12"); // 12

if (ArkTools.isAOTCompiled(printAbs)) {
    // Replace standard builtin after call to standard builtin was profiled
    Math.abs = replace
}
printAbs(-12); // 12; or -12, deopt
printAbs("abc"); // NaN; or abc, deopt
Math.abs = true_abs

// Check IR correctness inside try-block
try {
    printAbs(-12); // 12
    printAbs("abc"); // NaN
} catch (e) {
}

let obj = {};
obj.valueOf = (() => { return -23; })
print(Math.abs(obj)); // 23

function Throwing() {
    this.value = -14;
}
Throwing.prototype.valueOf = function() {
    if (this.value > 0) {
        throw new Error("already positive");
    }
    return this.value;
}
let throwingObj = new Throwing();

try {
    print(Math.abs(throwingObj)); // 14
    throwingObj.value = 10;
    print(Math.abs(throwingObj)); // exception
} catch(e) {
    print(e);
} finally {
    print(Math.abs(obj)); // 23
}