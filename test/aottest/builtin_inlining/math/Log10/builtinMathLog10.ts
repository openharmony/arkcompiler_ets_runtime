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
function replace(a : number)
{
    return a;
}

let res:number = 0;

// Check without params
res = Math.log10();
//aot: [trace] aot inline builtin: Math.log10, caller function name:func_main_0@builtinMathLog10
print(res); //: NaN

// Check with single param
res = Math.log10(-0);
//aot: [trace] aot inline builtin: Math.log10, caller function name:func_main_0@builtinMathLog10
print(res); //: -Infinity

res = Math.log10(+0);
//aot: [trace] aot inline builtin: Math.log10, caller function name:func_main_0@builtinMathLog10
print(res); //: -Infinity

res = Math.log10(-123);
//aot: [trace] aot inline builtin: Math.log10, caller function name:func_main_0@builtinMathLog10
print(res); //: NaN

res = Math.log10(10);
//aot: [trace] aot inline builtin: Math.log10, caller function name:func_main_0@builtinMathLog10
print(res); //: 1

// Check with 2 params
res = Math.log10(10, 10);
//aot: [trace] aot inline builtin: Math.log10, caller function name:func_main_0@builtinMathLog10
print(res); //: 1

// Check with 3 params
res = Math.log10(10, 10, 10);
//aot: [trace] aot inline builtin: Math.log10, caller function name:func_main_0@builtinMathLog10
print(res); //: 1

// Check with 4 params
res = Math.log10(10, 10, 10, 10);
//aot: [trace] aot inline builtin: Math.log10, caller function name:func_main_0@builtinMathLog10
print(res); //: 1

// Check with 5 params
res = Math.log10(10, 10, 10, 10, 10);
//aot: [trace] aot inline builtin: Math.log10, caller function name:func_main_0@builtinMathLog10
print(res); //: 1

try {
    //aot: [trace] aot inline builtin: Math.log10, caller function name:func_main_0@builtinMathLog10
    print(Math.log10(10)); //: 1
} catch(e) {}

// Replace standart builtin
let true_log10 = Math.log10
Math.log10 = replace
res = Math.log10(111);
print(res); //: 111

// Call standart builtin with non-number param
Math.log10 = true_log10
//aot: [trace] aot inline builtin: Math.log10, caller function name:func_main_0@builtinMathLog10
//aot: [trace] Check Type: NotNumber1
res = Math.log10("-0"); // deopt
print(res); //: -Infinity
