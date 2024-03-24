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

function printZero(x: any) {
    if (Object.is(x, -0)) {
        print("-0");
    } else {
        print(x);
    }
}

function replace(a : number)
{
    return a;
}

// Use try to prevent inlining to main
function printExpm1(x: any) {
    try {
        print(Math.expm1(x));
    } finally {
    }
}

let doubleObj = {
    valueOf: () => { return 2.7; }
}

let nanObj = {
    valueOf: () => { return "something"; }
}

let obj = {
    valueOf: () => {
        print("obj.valueOf")
        return -23;
    }
};

// Check without params
print(Math.expm1()); // NaN

// Check with single param
printZero(Math.expm1(0)); // 0
printZero(Math.expm1(-0)); // -0
print(Math.expm1(1)); // 1.718281828459045
print(Math.expm1(-100)); // -1
print(Math.expm1(100)); // 2.6881171418161356e+43
print(Math.expm1(10e-10)); // 1.0000000005000001e-9

// Check with special float params
print(Math.expm1(-Infinity)); // -1
print(Math.expm1(Infinity)); // Infinity
print(Math.expm1(NaN)); // NaN

// Check with 2 params
print(Math.expm1(1, 1)); // 1.718281828459045

// Check with 3 params
print(Math.expm1(1, 1, 1)); // 1.718281828459045

// Check with 4 params
print(Math.expm1(1, 1, 1, 1)); // 1.718281828459045

// Check with 5 params
print(Math.expm1(1, 1, 1, 1, 1)); // 1.718281828459045

try {
    print(Math.expm1(1)); // 1.718281828459045
} catch(e) {}

// Replace standart builtin
let trueExpm1 = Math.expm1
Math.expm1 = replace
print(Math.expm1(111)); // 111
Math.expm1 = trueExpm1

print(Math.expm1(1)); // 1.718281828459045

// Call standart builtin with non-number param
// Check Type: NotNumber1
printExpm1("1"); // 1.718281828459045
// Check Type: NotNumber1
printExpm1("NaN"); // NaN
// Check Type: NotNumber1
printExpm1("abc"); // NaN

if (ArkTools.isAOTCompiled(printExpm1)) {
    // Replace standard builtin after call to standard builtin was profiled
    Math.expm1 = replace
}

// Check Type: NotCallTarget1
printExpm1(1); // 1.718281828459045
// Check Type: NotCallTarget1
printExpm1(2); // 6.38905609893065
// Check Type: NotCallTarget1
printExpm1("1"); // 1.718281828459045
// Check Type: NotCallTarget1
printExpm1("2"); // 6.38905609893065

Math.expm1 = trueExpm1

// Check IR correctness inside try-block
try {
    print(Math.expm1(1)); //: 1.718281828459045
    print(Math.expm1(1, 2)); //: 1.718281828459045
    printExpm1(1, 2); //: 1.718281828459045
    // Check Type: NotNumber1
    printExpm1("abc", 3e3); //: NaN
} catch (e) {
}

// Check Type: NotNumber1
printExpm1(obj); // -0.9999999998973812
// Check Type: NotNumber1
printExpm1(doubleObj); // 13.879731724872837
// Check Type: NotNumber1
printExpm1(nanObj); // NaN
