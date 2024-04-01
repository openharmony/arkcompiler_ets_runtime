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

let len:number = 1;

// Check without params
len = Math.asin();
//aot: [trace] aot inline builtin: Math.asin, caller function name:func_main_0@builtinMathAsin
print(len); //: NaN

len = Math.asin(NaN);
//aot: [trace] aot inline builtin: Math.asin, caller function name:func_main_0@builtinMathAsin
print(len); //: NaN

// Check with single param, in |x| <= 1
len = Math.asin(0);
//aot: [trace] aot inline builtin: Math.asin, caller function name:func_main_0@builtinMathAsin
print(len); //: 0

// Check with single param, in |x| <= 1
len = Math.asin(-1); // -PI/2
//aot: [trace] aot inline builtin: Math.asin, caller function name:func_main_0@builtinMathAsin
print(len); //: -1.5707963267948966

// Check with single param, in |x| <= 1
len = Math.asin(1); // PI/2
//aot: [trace] aot inline builtin: Math.asin, caller function name:func_main_0@builtinMathAsin
print(len); //: 1.5707963267948966

// Check with single param, in |x| > 1
len = Math.asin(10);
//aot: [trace] aot inline builtin: Math.asin, caller function name:func_main_0@builtinMathAsin
print(len); //: NaN

// Replace standart builtin
let true_asin = Math.asin
Math.asin = replace
len = Math.asin(111);
print(len); //: 111

// Call standart builtin with non-number param
Math.asin = true_asin
len = Math.asin("NaN"); // deopt
//aot: [trace] aot inline builtin: Math.asin, caller function name:func_main_0@builtinMathAsin
//aot: [trace] Check Type: NotNumber1
print(len); //: NaN
