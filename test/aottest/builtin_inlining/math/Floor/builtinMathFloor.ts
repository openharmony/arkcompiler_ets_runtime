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
print(Math.floor());            //: NaN

// Check with non-number param
print(Math.floor("string"));    //aot: [trace] Check Type: NotNumber1
                                //: NaN
print(Math.floor(null));        //: 0
print(Math.floor(undefined));   //: NaN
print(Math.floor(false));       //: 0
print(Math.floor(true));        //: 1
print(Math.floor(new Object));  //: NaN
print(Math.floor("1.3333"));    //: 1

// Replace standart builtin
let backup = Math.floor
Math.floor = replace
print(Math.floor(111));         //: 111
Math.floor = backup

// Check with NaN param
print(Math.floor(NaN));         //: NaN

// Check with infinity param
print(Math.floor(-Infinity));   //: -Infinity
print(Math.floor(+Infinity));   //: Infinity

// Check with zero param
print(Math.floor(-0.0));        //: 0
print(Math.floor(0.0));         //: 0
print(Math.floor(+0.0));        //: 0

// Check with integer param
print(Math.floor(-1.0));        //: -1
print(Math.floor(+1.0));        //: 1
print(Math.floor(-12.0));       //: -12
print(Math.floor(+12.0));       //: 12
print(Math.floor(-123.0));      //: -123
print(Math.floor(+123.0));      //: 123

printFloor(1.5);                //: 1
// Call standard builtin with non-number param
printFloor("abc");              //aot: [trace] Check Type: NotNumber1
                                //: NaN
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
print(Math.floor(-1.25));       //: -2
print(Math.floor(+1.25));       //: 1
print(Math.floor(-1.50));       //: -2
print(Math.floor(+1.50));       //: 1
print(Math.floor(-1.75));       //: -2
print(Math.floor(+1.75));       //: 1

