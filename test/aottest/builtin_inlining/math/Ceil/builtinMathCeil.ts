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
//aot: [trace] aot inline builtin: Math.ceil, caller function name:func_main_0@builtinMathCeil
print(Math.ceil());             //: NaN

// Replace standart builtin
let backup = Math.ceil
//aot: [trace] Check Type: NotCallTarget1
Math.ceil = replace
printCeil(111);          //: 111
Math.ceil = backup

// Check with NaN param
//aot: [trace] aot inline builtin: Math.ceil, caller function name:#*#doCeil@builtinMathCeil
printCeil(NaN);          //: NaN

// Check with infinity param
//aot: [trace] aot inline builtin: Math.ceil, caller function name:#*#doCeil@builtinMathCeil
printCeil(-Infinity);    //: -Infinity
//aot: [trace] aot inline builtin: Math.ceil, caller function name:#*#doCeil@builtinMathCeil
printCeil(+Infinity);    //: Infinity

// Check with zero param
//aot: [trace] aot inline builtin: Math.ceil, caller function name:#*#doCeil@builtinMathCeil
printCeil(-0.0);         //: 0
//aot: [trace] aot inline builtin: Math.ceil, caller function name:#*#doCeil@builtinMathCeil
printCeil(0.0);          //: 0
//aot: [trace] aot inline builtin: Math.ceil, caller function name:#*#doCeil@builtinMathCeil
printCeil(+0.0);         //: 0

// Check with integer param
//aot: [trace] aot inline builtin: Math.ceil, caller function name:#*#doCeil@builtinMathCeil
printCeil(-1.0);         //: -1
//aot: [trace] aot inline builtin: Math.ceil, caller function name:#*#doCeil@builtinMathCeil
printCeil(+1.0);         //: 1
//aot: [trace] aot inline builtin: Math.ceil, caller function name:#*#doCeil@builtinMathCeil
printCeil(-12.0);        //: -12
//aot: [trace] aot inline builtin: Math.ceil, caller function name:#*#doCeil@builtinMathCeil
printCeil(+12.0);        //: 12
//aot: [trace] aot inline builtin: Math.ceil, caller function name:#*#doCeil@builtinMathCeil
printCeil(-123.0);       //: -123
//aot: [trace] aot inline builtin: Math.ceil, caller function name:#*#doCeil@builtinMathCeil
printCeil(+123.0);       //: 123

//aot: [trace] aot inline builtin: Math.ceil, caller function name:#*#doCeil@builtinMathCeil
printCeil(1.5);                 //: 2
// Call standard builtin with non-number param
//aot: [trace] aot inline builtin: Math.ceil, caller function name:#*#doCeil@builtinMathCeil
printCeil("abc");               //aot: [trace] Check Type: NotNumber1
                                //: NaN
//aot: [trace] aot inline builtin: Math.ceil, caller function name:#*#doCeil@builtinMathCeil
printCeil("1.5");               //aot: [trace] Check Type: NotNumber1
                                //: 2

if (ArkTools.isAOTCompiled(printCeil)) {
    // Replace standard builtin after call to standard builtin was profiled
    Math.ceil = replace
}
printCeil(1.5);                 //aot: [trace] Check Type: NotCallTarget1
                                //aot: 1.5
                                //pgo: 2
printCeil("abc");               //aot: [trace] Check Type: NotCallTarget1
                                //aot: abc
                                //pgo: NaN

Math.ceil = backup

// Check with fractional param
//aot: [trace] aot inline builtin: Math.ceil, caller function name:#*#doCeil@builtinMathCeil
printCeil(-1.25);        //: -1
//aot: [trace] aot inline builtin: Math.ceil, caller function name:#*#doCeil@builtinMathCeil
printCeil(+1.25);        //: 2
//aot: [trace] aot inline builtin: Math.ceil, caller function name:#*#doCeil@builtinMathCeil
printCeil(-1.50);        //: -1
//aot: [trace] aot inline builtin: Math.ceil, caller function name:#*#doCeil@builtinMathCeil
printCeil(+1.50);        //: 2
//aot: [trace] aot inline builtin: Math.ceil, caller function name:#*#doCeil@builtinMathCeil
printCeil(-1.75);        //: -1
//aot: [trace] aot inline builtin: Math.ceil, caller function name:#*#doCeil@builtinMathCeil
printCeil(+1.75);        //: 2

// Check with non-number param
//aot: [trace] aot inline builtin: Math.ceil, caller function name:#*#doCeil@builtinMathCeil
printCeil("string");     //aot: [trace] Check Type: NotNumber1
                         //: NaN
//aot: [trace] aot inline builtin: Math.ceil, caller function name:#*#doCeil@builtinMathCeil
printCeil(null);         //aot: [trace] Check Type: NotNumber1
                         //: 0
//aot: [trace] aot inline builtin: Math.ceil, caller function name:#*#doCeil@builtinMathCeil
printCeil(undefined);    //aot: [trace] Check Type: NotNumber1
                         //: NaN
//aot: [trace] aot inline builtin: Math.ceil, caller function name:#*#doCeil@builtinMathCeil
printCeil(false);        //aot: [trace] Check Type: NotNumber1
                         //: 0
//aot: [trace] aot inline builtin: Math.ceil, caller function name:#*#doCeil@builtinMathCeil
printCeil(true);         //aot: [trace] Check Type: NotNumber1
                         //: 1
//aot: [trace] aot inline builtin: Math.ceil, caller function name:#*#doCeil@builtinMathCeil
printCeil(new Object);   //aot: [trace] Check Type: NotNumber1
                         //: NaN
printCeil("1.3333");
                         //: 2