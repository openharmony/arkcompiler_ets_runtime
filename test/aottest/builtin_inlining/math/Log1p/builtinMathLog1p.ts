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
res = Math.log1p();
//aot: [trace] aot inline builtin: Math.log1p, caller function name:func_main_0@builtinMathLog1p
print(res); //: NaN

// Check with single param
res = Math.log1p(-1);
//aot: [trace] aot inline builtin: Math.log1p, caller function name:func_main_0@builtinMathLog1p
print(res); //: -Infinity

res = Math.log1p(-0);
//aot: [trace] aot inline builtin: Math.log1p, caller function name:func_main_0@builtinMathLog1p
print("1/x: " + 1 / res); //: 1/x: -Infinity

res = Math.log1p(+0);
//aot: [trace] aot inline builtin: Math.log1p, caller function name:func_main_0@builtinMathLog1p
print(res); //: 0

res = Math.log1p(-123);
//aot: [trace] aot inline builtin: Math.log1p, caller function name:func_main_0@builtinMathLog1p
print(res); //: NaN

res = Math.log1p(Math.E - 1);
//aot: [trace] aot inline builtin: Math.log1p, caller function name:func_main_0@builtinMathLog1p
print(res); //: 1

// Check with 2 params
res = Math.log1p(Math.E - 1, Math.E - 1);
//aot: [trace] aot inline builtin: Math.log1p, caller function name:func_main_0@builtinMathLog1p
print(res); //: 1

// Check with 3 params
res = Math.log1p(Math.E - 1, Math.E - 1, Math.E - 1);
//aot: [trace] aot inline builtin: Math.log1p, caller function name:func_main_0@builtinMathLog1p
print(res); //: 1

// Check with 4 params
res = Math.log1p(Math.E - 1, Math.E - 1, Math.E - 1, Math.E - 1);
//aot: [trace] aot inline builtin: Math.log1p, caller function name:func_main_0@builtinMathLog1p
print(res); //: 1

// Check with 5 params
res = Math.log1p(Math.E - 1, Math.E - 1, Math.E - 1, Math.E - 1, Math.E - 1);
//aot: [trace] aot inline builtin: Math.log1p, caller function name:func_main_0@builtinMathLog1p
print(res); //: 1

try {
    //aot: [trace] aot inline builtin: Math.log1p, caller function name:func_main_0@builtinMathLog1p
    print(Math.log1p(Math.E - 1)); //: 1
} catch(e) {}

// Replace standart builtin
let true_log1p = Math.log1p
Math.log1p = replace
res = Math.log1p(111);
print(res); //: 111

// Call standart builtin with non-number param
Math.log1p = true_log1p
//aot: [trace] aot inline builtin: Math.log1p, caller function name:func_main_0@builtinMathLog1p
//aot: [trace] Check Type: NotNumber1
res = Math.log1p("-1"); // deopt
print(res); //: -Infinity
