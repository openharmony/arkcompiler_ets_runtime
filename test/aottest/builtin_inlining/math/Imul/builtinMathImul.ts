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

declare function print(arg:any):string;
declare interface ArkTools {
    isAOTCompiled(args: any): boolean;
}
function replace(a : number, b : number)
{
    return a;
}

function doImul(x: any, y: any): number {
    return Math.imul(x, y);
}

function printImul(x: any, y: any) {
    try {
        print(doImul(x, y));
    } finally {
    }
}

let len:number = 1;

len = Math.imul(2, 3)
print(len) // 6

// Check without params
len = Math.imul();
print(len); // 0

// Check with single param
len = Math.imul(0);
print(len); // 0

// Check with three param
len = Math.imul(2, 4, 6);
print(len); // 8

// If an input is NaN, return 0
len = Math.imul(2, NaN)
print(len) // 0

// Check with 0
len = Math.imul(3, +0.0);
print(len); // 0

len = Math.imul(3, -0.0);
print(len); // 0

// If an input is Infinity or -Infinity, return 0
len = Math.imul(-Infinity, 5);
print(len); // 0
len = Math.imul(Infinity, 6);
print(len); // 0
len = Math.imul(3, -Infinity);
print(len); // 0
len = Math.imul(5, Infinity);
print(len); // 0

// Check int
len = Math.imul(-2, -10);
print(len); // 20
len = Math.imul(5, 10000);
print(len); // 50000
len = Math.imul(-600, -20);
print(len); // 12000

// Check double
len = Math.imul(5.3, 2.7);
print(len); // 10
len = Math.imul(-2.4, -4.7);
print(len); // 8

len = Math.imul(-7.3, 6.2);
print(len); // -42

//big double
len = Math.imul(2, 1.9e80);
print(len); // 0

//small double
len = Math.imul(2, 1.9e-80);
print(len); // 0


// Check edge cases
const INT_MAX: number = 2147483647;
const INT_MIN: number = -INT_MAX - 1;
print(Math.imul(INT_MAX, 1)); // INT_MAX
print(Math.imul(2147483648 , 2)); // 0
print(Math.imul(-INT_MAX, 2)); //2
print(Math.imul(INT_MIN, 2)); // 0
print(Math.imul(INT_MIN - 1, 2)); // -2

len = Math.imul(2, "three");
print(len); // 0, deopt

// Replace standard builtin
let true_imul = Math.imul
Math.imul= replace
print(Math.imul(2, 40)); // 2
Math.imul = true_imul

len = Math.imul(3, 3)
print(len) // 9
printImul(-12, 2); // -24
// Call standard builtin with non-number param
printImul("abc", 2); // 0
printImul("-12", 2); // -24

if (ArkTools.isAOTCompiled(printImul)) {
    // Replace standard builtin after call to standard builtin was profiled
    Math.imul= replace
}
printImul(-12, 2); // -24; or -12, deopt
printImul("abc", 2); // 0; or abc, deopt
Math.imul = true_imul

// Check IR correctness inside try-block
try {
    printImul(-12, 2); // -24
    printImul("abc", 2); // 0
} catch (e) {
}

let obj = {};
obj.valueOf = (() => { return -23; })
printImul(obj, -2); // 46

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
    print(Math.imul(throwingObj, 2)); // -28
    throwingObj.value = 10;
    print(Math.imul(throwingObj), 2); // exception
} catch(e) {
    print(e);
} finally {
    print(Math.imul(obj, 2)); // -46
}
