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

function doFloor(x: any): number {
    return Math.floor(x);
}

function printFloor(x: any) {
    try {
        print(doFloor(x));
    } finally {
    }
}

// Check without params
//aot: [trace] aot inline builtin: Math.floor, caller function name:func_main_0@builtinMathFloor
print(Math.floor());            //: NaN

// Replace standart builtin
let backup = Math.floor
//aot: [trace] Check Type: NotCallTarget1
Math.floor = replace
printFloor(111);         //: 111
Math.floor = backup

// Check with NaN param
//aot: [trace] aot inline builtin: Math.floor, caller function name:#*#doFloor@builtinMathFloor
printFloor(NaN);         //: NaN

// Check with infinity param
//aot: [trace] aot inline builtin: Math.floor, caller function name:#*#doFloor@builtinMathFloor
printFloor(-Infinity);   //: -Infinity
//aot: [trace] aot inline builtin: Math.floor, caller function name:#*#doFloor@builtinMathFloor
printFloor(+Infinity);   //: Infinity

// Check with zero param
//aot: [trace] aot inline builtin: Math.floor, caller function name:#*#doFloor@builtinMathFloor
printFloor(-0.0);        //: 0
//aot: [trace] aot inline builtin: Math.floor, caller function name:#*#doFloor@builtinMathFloor
printFloor(0.0);         //: 0
//aot: [trace] aot inline builtin: Math.floor, caller function name:#*#doFloor@builtinMathFloor
printFloor(+0.0);        //: 0

// Check with integer param
//aot: [trace] aot inline builtin: Math.floor, caller function name:#*#doFloor@builtinMathFloor
printFloor(-1.0);        //: -1
//aot: [trace] aot inline builtin: Math.floor, caller function name:#*#doFloor@builtinMathFloor
printFloor(+1.0);        //: 1
//aot: [trace] aot inline builtin: Math.floor, caller function name:#*#doFloor@builtinMathFloor
printFloor(-12.0);       //: -12
//aot: [trace] aot inline builtin: Math.floor, caller function name:#*#doFloor@builtinMathFloor
printFloor(+12.0);       //: 12
//aot: [trace] aot inline builtin: Math.floor, caller function name:#*#doFloor@builtinMathFloor
printFloor(-123.0);      //: -123
//aot: [trace] aot inline builtin: Math.floor, caller function name:#*#doFloor@builtinMathFloor
printFloor(+123.0);      //: 123

//aot: [trace] aot inline builtin: Math.floor, caller function name:#*#doFloor@builtinMathFloor
printFloor(1.5);                //: 1
// Call standard builtin with non-number param
//aot: [trace] aot inline builtin: Math.floor, caller function name:#*#doFloor@builtinMathFloor
printFloor("abc");              //aot: [trace] Check Type: NotNumber1
                                //: NaN
//aot: [trace] aot inline builtin: Math.floor, caller function name:#*#doFloor@builtinMathFloor
printFloor("1.5");              //aot: [trace] Check Type: NotNumber1
                                //: 1

if (ArkTools.isAOTCompiled(printFloor)) {
    // Replace standard builtin after call to standard builtin was profiled
    Math.floor = replace
}
printFloor(1.5);                //aot: [trace] Check Type: NotCallTarget1
                                //aot: 1.5
                                //pgo: 1
printFloor("abc");              //aot: [trace] Check Type: NotCallTarget1
                                //aot: abc
                                //pgo: NaN

Math.floor = backup

// Check with fractional param
//aot: [trace] aot inline builtin: Math.floor, caller function name:#*#doFloor@builtinMathFloor
printFloor(-1.25);       //: -2
//aot: [trace] aot inline builtin: Math.floor, caller function name:#*#doFloor@builtinMathFloor
printFloor(+1.25);       //: 1
//aot: [trace] aot inline builtin: Math.floor, caller function name:#*#doFloor@builtinMathFloor
printFloor(-1.50);       //: -2
//aot: [trace] aot inline builtin: Math.floor, caller function name:#*#doFloor@builtinMathFloor
printFloor(+1.50);       //: 1
//aot: [trace] aot inline builtin: Math.floor, caller function name:#*#doFloor@builtinMathFloor
printFloor(-1.75);       //: -2
//aot: [trace] aot inline builtin: Math.floor, caller function name:#*#doFloor@builtinMathFloor
printFloor(+1.75);       //: 1

// Check with non-number param
//aot: [trace] aot inline builtin: Math.floor, caller function name:#*#doFloor@builtinMathFloor
printFloor("string");   //aot: [trace] Check Type: NotNumber1
                        //: NaN
//aot: [trace] aot inline builtin: Math.floor, caller function name:#*#doFloor@builtinMathFloor
printFloor(null);       //aot: [trace] Check Type: NotNumber1
                        //: 0
//aot: [trace] aot inline builtin: Math.floor, caller function name:#*#doFloor@builtinMathFloor
printFloor(undefined);  //aot: [trace] Check Type: NotNumber1
                        //: NaN
//aot: [trace] aot inline builtin: Math.floor, caller function name:#*#doFloor@builtinMathFloor
printFloor(false);      //aot: [trace] Check Type: NotNumber1
                        //: 0
//aot: [trace] aot inline builtin: Math.floor, caller function name:#*#doFloor@builtinMathFloor
printFloor(true);       //aot: [trace] Check Type: NotNumber1
                        //: 1
//aot: [trace] aot inline builtin: Math.floor, caller function name:#*#doFloor@builtinMathFloor
printFloor(new Object); //aot: [trace] Check Type: NotNumber1
                        //: NaN
printFloor("1.3333");
                        //: 1
