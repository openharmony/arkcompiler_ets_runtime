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

function doIsFinite(x: any): any {
    return isFinite(x);
}

function printIsFinite(x: any) {
    try {
        print(doIsFinite(x));
    } finally {
    }
}
// Check with single int param
print(isFinite(0)); // true
print(isFinite(3)); // true
print(isFinite(-5)); // true

// Check with single float param
print(isFinite(-1.5)); // true
print(isFinite(1.5)); // true

// Check with special float params
print(isFinite(Infinity)); // false
print(isFinite(-Infinity)); // false
print(isFinite(NaN)); // false
print(isFinite(Math.exp(800))); // false

// Check with 2 params
print(isFinite(3, Infinity)); // true

// Check with 3 params
print(isFinite(-3, NaN, Infinity)); // true


// Replace standard builtin
let true_is_finite = isFinite
isFinite = replace
print(isFinite(NaN)); // NaN
isFinite = true_is_finite


printIsFinite(-3);    // true
printIsFinite("abc"); // false
printIsFinite("abc"); // false

printIsFinite(-12); // true
// Call standard builtin with non-number param
printIsFinite("abc"); // false
printIsFinite("-12"); // true

if (ArkTools.isAOTCompiled(doIsFinite)) {
    // Replace standard builtin after call to standard builtin was profiled
    isFinite = replace
}
printIsFinite(-12); // true; or -12, deopt
printIsFinite("abc"); // false; or abc, deopt
isFinite = true_is_finite

// Check IR correctness inside try-block
try {
    printIsFinite(-12); // true
    printIsFinite("abc"); // false
} catch (e) {
}

let obj = {};
obj.valueOf = (() => { return -23; })
print(isFinite(obj)); // true

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
    print(isFinite(throwingObj)); // true
    throwingObj.value = 10;
    print(isFinite(throwingObj)); // exception
} catch(e) {
    print(e);
} finally {
    print(isFinite(obj)); // false
}