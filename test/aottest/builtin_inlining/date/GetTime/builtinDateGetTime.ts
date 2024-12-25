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

function doDateGetTimeOneParam(x: any): number {
    return time.getTime(x);
}

function printDateGetTimeOneParam(x: any) {
    try {
        print(doDateGetTimeOneParam(x));
    } finally {
    }
}

function doDateGetTimeWithoutParam(): number {
    return time.getTime();
}

function printDateGetTimeWithoutParam() {
    try {
        print(doDateGetTimeWithoutParam());
    } finally {
    }
}

let time = new Date('July 20, 99 20:17:40 GMT+00:00')
//aot: [trace] aot inline builtin: Date.prototype.getTime, caller function name:func_main_0@builtinDateGetTime
print(time.getTime()); //: 932501860000

time = new Date(990000000000)
//aot: [trace] aot inline builtin: Date.prototype.getTime, caller function name:func_main_0@builtinDateGetTime
print(time.getTime()); //: 990000000000
//aot: [trace] aot inline builtin: Date.prototype.getTime, caller function name:func_main_0@builtinDateGetTime
let temp = time.getTime() + 1
print(temp) //: 990000000001

time = new Date(1990, 2, 15, 16, 17, 18);
//aot: [trace] aot inline builtin: Date.prototype.getTime, caller function name:func_main_0@builtinDateGetTime
print(time.getTime()); //: 637489038000
//aot: [trace] aot inline builtin: Date.prototype.getTime, caller function name:func_main_0@builtinDateGetTime
print(time.getTime(123)); //: 637489038000
//aot: [trace] aot inline builtin: Date.prototype.getTime, caller function name:func_main_0@builtinDateGetTime
print(time.getTime("string")); //: 637489038000
//aot: [trace] aot inline builtin: Date.prototype.getTime, caller function name:func_main_0@builtinDateGetTime
print(time.getTime(1, 2)); //: 637489038000
//aot: [trace] aot inline builtin: Date.prototype.getTime, caller function name:func_main_0@builtinDateGetTime
print(time.getTime(1, 2, 3)); //: 637489038000
//aot: [trace] aot inline builtin: Date.prototype.getTime, caller function name:func_main_0@builtinDateGetTime
print(time.getTime(1, 2, 3, 4)); //: 637489038000


time = new Date('May 1, 69 8:0:0 GMT+00:00');
//aot: [trace] aot inline builtin: Date.prototype.getTime, caller function name:func_main_0@builtinDateGetTime
print(time.getTime()); //: -21139200000
//aot: [trace] aot inline builtin: Date.prototype.getTime, caller function name:func_main_0@builtinDateGetTime
print(time.getTime({})); //: -21139200000

time = new Date('Jan 0, 2000 10:11:12 GMT+00:00');
//aot: [trace] aot inline builtin: Date.prototype.getTime, caller function name:func_main_0@builtinDateGetTime
print(time.getTime()); //: NaN

time = new Date(50000000000)
let true_func = time.getTime

//aot: [trace] aot call builtin: Function.prototype.call, caller function name:func_main_0@builtinDateGetTime
print(true_func.call(time)) //: 50000000000
//aot: [trace] aot call builtin: Function.prototype.call, caller function name:func_main_0@builtinDateGetTime
print(true_func.call(time, 1)) //: 50000000000
//aot: [trace] aot call builtin: Function.prototype.call, caller function name:func_main_0@builtinDateGetTime
print(true_func.call(time, 1, 2)) //: 50000000000

if (ArkTools.isAOTCompiled(printDateGetTimeOneParam)) {
    // Replace standard builtin after call to standard builtin was profiled
    time.getTime = replace;
}

printDateGetTimeWithoutParam() //pgo: 50000000000
                               //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
                               //aot: 12345
printDateGetTimeOneParam(123); //pgo: 50000000000
                               //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
                               //aot: 12345
printDateGetTimeOneParam("abc"); //pgo: 50000000000
                                 //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
                                 //aot: 12345

time.getTime = true_func

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
    //aot: [trace] aot inline builtin: Date.prototype.getTime, caller function name:func_main_0@builtinDateGetTime
    print(time.getTime(throwingObj)); //: 50000000000
    throwingObj.value = 10;
    //aot: [trace] aot inline builtin: Date.prototype.getTime, caller function name:func_main_0@builtinDateGetTime
    print(time.getTime(throwingObj)); //: 50000000000
} catch(e) {
    print(e);
} finally {
    //aot: [trace] aot inline builtin: Date.prototype.getTime, caller function name:func_main_0@builtinDateGetTime
    print(time.getTime(obj)); //: 50000000000
}

function checkObjWithDateProto() {
    let o = {};
    //aot: [trace] aot call builtin: Object.SetPrototypeOf, caller function name:#*#checkObjWithDateProto@builtinDateGetTime
    Object.setPrototypeOf(o, Date.prototype);
    try {
        o.getTime();
    } catch(e) {
        print(e);
    }
}

//aot: [trace] Check Type: NotCallTarget1
//: TypeError: Not a Date Object
checkObjWithDateProto();
