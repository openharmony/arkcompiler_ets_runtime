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

function doclz32(x: any): number {
    return Math.clz32(x);
}

function printclz32(x: any) {
    try {
        print(doclz32(x));
    } finally {
    }
}

// Check without params
print(Math.clz32()); // 32

// Check with single int param
print(Math.clz32(0)); // 32
print(Math.clz32(1)); // 31
print(Math.clz32(1 << 31)) // 0
print(Math.clz32(1 << 30)) // 1
print(Math.clz32(3)); // 0b11 -> 30
print(Math.clz32(-5)); // 0
print(Math.clz32(4294967298)); // MAX_UINT32 + 3 -> 30

// Check with single float param
print(Math.clz32(1.9999999999999)); // 31
print(Math.clz32(126.55555555555)); // 25
print(Math.clz32(126.999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999)) // 25

// Negative arguments
print(Math.clz32((1 << 30) + (1 << 31))) // -1073741824 (0xC0000000) -> 0
print(Math.clz32(-5)) // 0
print(Math.clz32(-126.99999999999999999999999)) // 0


// Corner cases
print(Math.clz32(NaN)) // 32
print(Math.clz32(+0.0)) // 32
print(Math.clz32(-0.0)) // 32
print(Math.clz32(Infinity)); // 32
print(Math.clz32(-Infinity)); // 32

printclz32(4); // 29
printclz32("abc"); // 32
printclz32("abcd"); // 32

let true_clz32 = Math.clz32
if (ArkTools.isAOTCompiled(printclz32)) {
    // Replace standard builtin after call to standard builtin was profiled
    Math.clz32 = replace
}
printclz32(-8); // 0; or -8, deopt
printclz32("abc"); // 32; or abc, deopt
Math.clz32 = true_clz32

// Check IR correctness inside try-block
try {
    printclz32(16); // 27
    printclz32("abc"); // 32
} catch (e) {
}

let obj = {};
obj.valueOf = (() => { return 32; })
print(Math.clz32(obj)); // 26

function Throwing() {
    this.value = 64;
}
Throwing.prototype.valueOf = function() {
    if (this.value > 100) {
        throw new Error("so big");
    }
    return this.value;
}
let throwingObj = new Throwing();

try {
    print(Math.clz32(throwingObj)); // 25
    throwingObj.value = 128;
    print(Math.clz32(throwingObj)); // exception
} catch(e) {
    print(e);
} finally {
    print(Math.clz32(obj)); // 26
}