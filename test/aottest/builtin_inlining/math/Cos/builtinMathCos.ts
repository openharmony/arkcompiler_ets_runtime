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
len = Math.cos();
//aot: [trace] aot inline builtin: Math.cos, caller function name:func_main_0@builtinMathCos
print(len); //: NaN

// Check with single param
len = Math.cos(0);
//aot: [trace] aot inline builtin: Math.cos, caller function name:func_main_0@builtinMathCos
print(len); //: 1

// Check with single not zero param
len = Math.cos(Math.PI / 2);
//aot: [trace] aot inline builtin: Math.cos, caller function name:func_main_0@builtinMathCos
print(len); //: 6.123233995736766e-17

// Check with 2 params
len = Math.cos(0, 0);
//aot: [trace] aot inline builtin: Math.cos, caller function name:func_main_0@builtinMathCos
print(len); //: 1

// Check with 3 params
len = Math.cos(0, 0, 0);
//aot: [trace] aot inline builtin: Math.cos, caller function name:func_main_0@builtinMathCos
print(len); //: 1

// Check with 4 params
len = Math.cos(0, 0, 0, 0);
//aot: [trace] aot inline builtin: Math.cos, caller function name:func_main_0@builtinMathCos
print(len); //: 1

// Check with 5 params
len = Math.cos(0, 0, 0, 0, 0);
//aot: [trace] aot inline builtin: Math.cos, caller function name:func_main_0@builtinMathCos
print(len); //: 1

try {
    //aot: [trace] aot inline builtin: Math.cos, caller function name:func_main_0@builtinMathCos
    print(Math.cos(0)) //: 1
} catch(e) {}

// Replace standart builtin
let true_cos = Math.cos

len = true_cos();
//aot: [trace] aot inline builtin: Math.cos, caller function name:func_main_0@builtinMathCos
print(len); //: NaN

// Check with single param
len = true_cos(0);
//aot: [trace] aot inline builtin: Math.cos, caller function name:func_main_0@builtinMathCos
print(len); //: 1

// Check with single not zero param
len = true_cos(Math.PI / 2);
//aot: [trace] aot inline builtin: Math.cos, caller function name:func_main_0@builtinMathCos
print(len); //: 6.123233995736766e-17

// Check with 2 params
len = true_cos(0, 0);
//aot: [trace] aot inline builtin: Math.cos, caller function name:func_main_0@builtinMathCos
print(len); //: 1

// Check with 3 params
len = true_cos(0, 0, 0);
//aot: [trace] aot inline builtin: Math.cos, caller function name:func_main_0@builtinMathCos
print(len); //: 1

// Check with 4 params
len = true_cos(0, 0, 0, 0);
//aot: [trace] aot inline builtin: Math.cos, caller function name:func_main_0@builtinMathCos
print(len); //: 1

// Check with 5 params
len = true_cos(0, 0, 0, 0, 0);
//aot: [trace] aot inline builtin: Math.cos, caller function name:func_main_0@builtinMathCos
print(len); //: 1

try {
    //aot: [trace] aot inline builtin: Math.cos, caller function name:func_main_0@builtinMathCos
    print(true_cos(0)) //: 1
} catch(e) {}

Math.cos = replace
len = Math.cos(111);
print(len); //: 111

// Call standart builtin with non-number param
Math.cos = true_cos

//aot: [trace] aot inline builtin: Math.cos, caller function name:func_main_0@builtinMathCos
//aot: [trace] Check Type: NotNumber1
len = Math.cos("0");
print(len); //: 1

len = Math.cos("0.5");
print(len); //: 0.8775825618903728