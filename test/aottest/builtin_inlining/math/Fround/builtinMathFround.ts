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

function printZero(x: any) {
    if (Object.is(x, -0)) {
        print("-0");
    } else {
        print(x);
    }
}

function doFround(x: any): number {
    return Math.fround(x);
}

function printFround(x: any) {
    try {
        print(doFround(x));
    } finally {
    }
}

// Check without params
print(Math.fround()); //: NaN

// Check with single int param
print(Math.fround(0)); //: 0
print(Math.fround(3)); //: 3
print(Math.fround(-5)); //: -5

// Check with single float param
print(Math.fround(1.9e80)); //: Infinity
printZero(Math.fround(2.5)); //: 2.5
printZero(Math.fround(1.5)); //: 1.5
printZero(Math.fround(0.5)); //: 0.5
printZero(Math.fround(0.2)); //: 0.20000000298023224
printZero(Math.fround(-0)); //: -0
printZero(Math.fround(-1.9e-80)); //: -0
printZero(Math.fround(-0.1)); //: -0.10000000149011612
print(Math.fround(-0.5)); //: -0.5
print(Math.fround(-1.5)); //: -1.5
print(Math.fround(-2.1)); //: -2.0999999046325684
print(Math.fround(-2.49)); //: -2.490000009536743
print(Math.fround(-2.5)); //: -2.5
print(Math.fround(-2.7)); //: -2.700000047683716
print(Math.fround(-1.9e80)); //: -Infinity
print(Math.fround(1.9e-80)); //: 0

// Check with special float params
print(Math.fround(Infinity)); //: Infinity
print(Math.fround(-Infinity)); //: -Infinity
print(Math.fround(NaN)); //: NaN

// Check with 2 params
print(Math.fround(3, 0)); //: 3

// Check with 3 params
print(Math.fround(-3.5, 0, 0)); //: -3.5

// Check with 4 params
print(Math.fround(4.1, 0, 0, 0)); //: 4.099999904632568

// Check with 5 params
print(Math.fround(-4.1, 0, 0, 0, 0)); //: -4.099999904632568

// Replace standard builtin
let true_fround = Math.fround
Math.fround = replace

// no deopt
print(Math.fround(-1.001)); //: -1.001
Math.fround = true_fround

printFround(12.3); //: 12.300000190734863
// Call standard builtin with non-number param
//aot: [trace] Check Type: NotNumber2
printFround("abc"); //: NaN
//aot: [trace] Check Type: NotNumber2
printFround("-12.9"); //: -12.899999618530273

if (ArkTools.isAOTCompiled(printFround)) {
    // Replace standard builtin after call to standard builtin was profiled
    Math.fround = replace
}
printFround(-12.2); //pgo: -12.199999809265137
//aot: [trace] Check Type: NotCallTarget1
//aot: -12.2

printFround("abc"); //pgo: NaN
//aot: [trace] Check Type: NotCallTarget1
//aot: abc

Math.fround = true_fround

// Check IR correctness inside try-block
try {
    print(Math.fround()) //: NaN
    print(Math.fround(0.3)) //: 0.30000001192092896
    printFround(-12); //: -12
    //aot: [trace] Check Type: NotNumber2
    printFround("abc"); //: NaN
} catch (e) {
}

let obj = {
    valueOf: () => { return -22.5; }
};
//aot: [trace] Check Type: NotNumber2
print(Math.fround(obj)); //: -22.5
