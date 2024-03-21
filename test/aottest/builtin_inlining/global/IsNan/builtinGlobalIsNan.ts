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

function doIsNaN(x: any): any {
    return isNaN(x);
}

function printIsNaN(x: any) {
    try {
        print(doIsNaN(x));
    } finally {
    }
}
// Check with single int param
print(isNaN(0)); // false
print(isNaN(3)); // false
print(isNaN(-5)); // false

// Check with single float param
print(isNaN(-1.5)); // false
print(isNaN(1.5)); // false

// Check with special float params
print(isNaN(Infinity)); // false
print(isNaN(-Infinity)); // false
print(isNaN(NaN)); // true
print(isNaN(Math.exp(800))); // false

// Check with 2 params
print(isNaN(NaN, Infinity)); // true

// Check with 3 params
print(isNaN(NaN, 3, Infinity)); // true


// Replace standard builtin
let true_is_nan = isNaN
isNaN = replace
print(isNaN(NaN)); // NaN
isNaN = true_is_nan


printIsNaN(-3);    // false
printIsNaN("abc"); // true
printIsNaN("abc"); // true

printIsNaN(-12); // false
// Call standard builtin with non-number param
printIsNaN("abc"); // true
printIsNaN("-12"); // false

if (ArkTools.isAOTCompiled(doIsNaN)) {
    // Replace standard builtin after call to standard builtin was profiled
    isNaN = replace
}
printIsNaN(-12); // false; or -12, deopt
printIsNaN("abc"); // true; or abc, deopt
isNaN = true_is_nan

// Check IR correctness inside try-block
try {
    printIsNaN(-12); // false
    printIsNaN(NaN); // true
    printIsNaN("abc"); // true
} catch (e) {
}

let obj = {};
obj.valueOf = (() => { return -23; })
print(isNaN(obj)); // false

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
    print(isNaN(throwingObj)); // false
    throwingObj.value = 10;
    print(isNaN(throwingObj)); // exception
} catch(e) {
    print(e);
} finally {
    print(isNaN(obj)); // false
}