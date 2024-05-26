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
declare function print(arg:any): string;

function replace()
{
    return 12345;
}

function doDateNowOneParam(x: any): number {
    return Date.now(x);
}

function doDateWithoutParam(): number {
    return Date.now();
}

function printDateNowOneParam(x: any) {
    try {
        print(doDateNowOneParam(x));
    } finally {
    }
}

function printDateNowWithoutParam() {
    try {
        print(doDateWithoutParam());
    } finally {
    }
}

// Сhecking that the value changes over time
//aot: [trace] aot inline builtin: Date.now, caller function name:func_main_0@builtinDateNow
print(Date.now()); //: __INT__
let delay = 20000000
let result1 = 0
let result2 = 0
while (result1 < delay) {
    result1++
    result2 += result1 % 2
}
//aot: [trace] aot inline builtin: Date.now, caller function name:func_main_0@builtinDateNow
print(Date.now()); //: __INT_MORE_PREV__
// Need for disable optimization of loop
print(result2); //: 10000000

// Check with parameters more 0
// NOTE: We don't check results between that launches, because they are very close
//aot: [trace] aot inline builtin: Date.now, caller function name:func_main_0@builtinDateNow
print(Date.now(0)); //: __INT__
//aot: [trace] aot inline builtin: Date.now, caller function name:func_main_0@builtinDateNow
print(Date.now(1, 2)); //: __INT__
//aot: [trace] aot inline builtin: Date.now, caller function name:func_main_0@builtinDateNow
print(Date.now(3, 4, 5, 6)); //: __INT__
//aot: [trace] aot inline builtin: Date.now, caller function name:func_main_0@builtinDateNow
print(Date.now({a:10, b:20})); //: __INT__
//aot: [trace] aot inline builtin: Date.now, caller function name:func_main_0@builtinDateNow
print(Date.now("abc")); //: __INT__

//aot: [trace] aot inline builtin: Date.now, caller function name:#*#doDateWithoutParam@builtinDateNow
printDateNowWithoutParam(); //: __INT__
//aot: [trace] aot inline builtin: Date.now, caller function name:#*#doDateNowOneParam@builtinDateNow
printDateNowOneParam(2); //: __INT__

let true_now = Date.now;

// Check, that copy method without "this" also is inlined
//aot: [trace] aot inline builtin: Date.now, caller function name:func_main_0@builtinDateNow
print(true_now()); //: __INT__

if (ArkTools.isAOTCompiled(printDateNowOneParam)) {
    // Replace standard builtin after call to standard builtin was profiled
    Date.now = replace;
}

printDateNowWithoutParam(); //pgo: __INT__
                            //aot: [trace] Check Type: NotCallTarget1
                            //aot: 12345

printDateNowOneParam(123); //pgo: __INT__
                           //aot: [trace] Check Type: NotCallTarget1
                           //aot: 12345

printDateNowOneParam("abc"); //pgo: __INT__
                             //aot: [trace] Check Type: NotCallTarget1
                             //aot: 12345

Date.now = true_now

// Check on replace object Date
let true_date = Date

if (ArkTools.isAOTCompiled(printDateNowWithoutParam)) {
    // Replace standard builtin after call to standard builtin was profiled
    Date = {
        now : function () { return "Now is now"}
    };
}

printDateNowWithoutParam(); //pgo: __INT__
                            //aot: [trace] Check Type: NotCallTarget1
                            //aot: Now is now

printDateNowOneParam(123); //pgo: __INT__
                           //aot: [trace] Check Type: NotCallTarget1
                           //aot: Now is now

Date = true_date

let obj = {
    valueOf: () => { return -23; }
};
function Throwing() {
    this.value = -14;
};
Throwing.prototype.valueOf = function() {
    if (this.value > 0) {
        throw new Error("already positive");
    }
    return this.value;
}
let throwingObj = new Throwing();

try {
    //aot: [trace] aot inline builtin: Date.now, caller function name:func_main_0@builtinDateNow
    print(Date.now(throwingObj)); //: __INT__
    throwingObj.value = 10;
    // Value isn't use
    //aot: [trace] aot inline builtin: Date.now, caller function name:func_main_0@builtinDateNow
    print(Date.now(throwingObj)); //: __INT__
} catch(e) {
    print(e);
} finally {
    //aot: [trace] aot inline builtin: Date.now, caller function name:func_main_0@builtinDateNow
    print(Date.now(obj)); //: __INT__
}
