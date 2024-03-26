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

function doCeil(x: any): number {
    return Math.ceil(x);
}

function printCeil(x: any) {
    try {
        print(doCeil(x));
    } finally {
    }
}

// Check without params
print(Math.ceil());             // NaN

// Check with non-number param
print(Math.ceil("string"));     // [trace] Check Type: NotNumber1
                                // NaN
print(Math.ceil(null));         // 0
print(Math.ceil(undefined));    // NaN
print(Math.ceil(false));        // 0
print(Math.ceil(true));         // 1
print(Math.ceil(new Object));   // NaN
print(Math.ceil("1.3333"));     // 2

// Replace standart builtin
let backup = Math.ceil
Math.ceil = replace
print(Math.ceil(111));          // 111
Math.ceil = backup

// Check with NaN param
print(Math.ceil(NaN));          // NaN

// Check with infinity param
print(Math.ceil(-Infinity));    // -Infinity
print(Math.ceil(+Infinity));    // Infinity

// Check with zero param
print(Math.ceil(-0.0));         // 0
print(Math.ceil(0.0));          // 0
print(Math.ceil(+0.0));         // 0

// Check with integer param
print(Math.ceil(-1.0));         // -1
print(Math.ceil(+1.0));         // 1
print(Math.ceil(-12.0));        // -12
print(Math.ceil(+12.0));        // 12
print(Math.ceil(-123.0));       // -123
print(Math.ceil(+123.0));       // 123

printCeil(1.5);                 // 2
// Call standard builtin with non-number param
printCeil("abc");               // [trace] Check Type: NotNumber1
                                // NaN
printCeil("1.5");               // [trace] Check Type: NotNumber1
                                // 2

if (ArkTools.isAOTCompiled(printCeil)) {
    // Replace standard builtin after call to standard builtin was profiled
    Math.ceil = replace
}
printCeil(1.5);                 // aot: [trace] Check Type: NotCallTarget1
                                // aot: 1.5
                                // pgo: 2
printCeil("abc");               // aot: [trace] Check Type: NotCallTarget1
                                // aot: abc
                                // pgo: NaN

Math.ceil = backup

// Check with fractional param
print(Math.ceil(-1.25));        // -1
print(Math.ceil(+1.25));        // 2
print(Math.ceil(-1.50));        // -1
print(Math.ceil(+1.50));        // 2
print(Math.ceil(-1.75));        // -1
print(Math.ceil(+1.75));        // 2
