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
Math.ceil = replace
printCeil(111);          //: 111
Math.ceil = backup

// Check with NaN param
printCeil(NaN);          //: NaN

// Check with infinity param
printCeil(-Infinity);    //: -Infinity
printCeil(+Infinity);    //: Infinity

// Check with zero param
printCeil(-0.0);         //: 0
printCeil(0.0);          //: 0
printCeil(+0.0);         //: 0

// Check with integer param
printCeil(-1.0);         //: -1
printCeil(+1.0);         //: 1
printCeil(-12.0);        //: -12
printCeil(+12.0);        //: 12
printCeil(-123.0);       //: -123
printCeil(+123.0);       //: 123

printCeil(1.5);                 //: 2
// Call standard builtin with non-number param
printCeil("abc");               //: NaN
printCeil("1.5");               //: 2

if (ArkTools.isAOTCompiled(printCeil)) {
    // Replace standard builtin after call to standard builtin was profiled
    Math.ceil = replace
}
printCeil(1.5);                 //aot: 1.5
                                //pgo: 2
printCeil("abc");               //aot: abc
                                //pgo: NaN

Math.ceil = backup

// Check with fractional param
printCeil(-1.25);        //: -1
printCeil(+1.25);        //: 2
printCeil(-1.50);        //: -1
printCeil(+1.50);        //: 2
printCeil(-1.75);        //: -1
printCeil(+1.75);        //: 2

// Check with non-number param
printCeil("string");     //: NaN
printCeil(null);         //: 0
printCeil(undefined);    //: NaN
printCeil(false);        //: 0
printCeil(true);         //: 1
printCeil(new Object);   //: NaN
printCeil("1.3333");
                         //: 2