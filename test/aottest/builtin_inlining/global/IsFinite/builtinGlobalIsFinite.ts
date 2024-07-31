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

function doIsFinite(x: any): any {
    return isFinite(x);
}

function printIsFinite(x: any) {
    try {
        print(doIsFinite(x));
    } finally {
    }
}
// Check with single int param
//aot: [trace] aot inline builtin: isFinite, caller function name:func_main_0@builtinGlobalIsFinite
print(isFinite(0)); //: true
//aot: [trace] aot inline builtin: isFinite, caller function name:func_main_0@builtinGlobalIsFinite
print(isFinite(3)); //: true
//aot: [trace] aot inline builtin: isFinite, caller function name:func_main_0@builtinGlobalIsFinite
print(isFinite(-5)); //: true

// Check with single float param
//aot: [trace] aot inline builtin: isFinite, caller function name:func_main_0@builtinGlobalIsFinite
print(isFinite(-1.5)); //: true
//aot: [trace] aot inline builtin: isFinite, caller function name:func_main_0@builtinGlobalIsFinite
print(isFinite(1.5)); //: true

// Check with special float params
//aot: [trace] aot inline builtin: isFinite, caller function name:func_main_0@builtinGlobalIsFinite
print(isFinite(Infinity)); //: false
//aot: [trace] aot inline builtin: isFinite, caller function name:func_main_0@builtinGlobalIsFinite
print(isFinite(-Infinity)); //: false
//aot: [trace] aot inline builtin: isFinite, caller function name:func_main_0@builtinGlobalIsFinite
print(isFinite(NaN)); //: false
//aot: [trace] aot inline builtin: Math.exp, caller function name:func_main_0@builtinGlobalIsFinite
//aot: [trace] aot inline builtin: isFinite, caller function name:func_main_0@builtinGlobalIsFinite
print(isFinite(Math.exp(800))); //: false

// Check with 2 params
//aot: [trace] aot inline builtin: isFinite, caller function name:func_main_0@builtinGlobalIsFinite
print(isFinite(3, Infinity)); //: true

// Check with 3 params
//aot: [trace] aot inline builtin: isFinite, caller function name:func_main_0@builtinGlobalIsFinite
print(isFinite(-3, NaN, Infinity)); //: true


// Replace standard builtin
let true_is_finite = isFinite
isFinite = replace
print(isFinite(NaN)); //: NaN
isFinite = true_is_finite


//aot: [trace] aot inline builtin: isFinite, caller function name:#*#doIsFinite@builtinGlobalIsFinite
printIsFinite(-3);    //: true
//aot: [trace] aot inline builtin: isFinite, caller function name:#*#doIsFinite@builtinGlobalIsFinite
//aot: [trace] Check Type: NotNumber1
printIsFinite("abc"); //: false
//aot: [trace] aot inline builtin: isFinite, caller function name:#*#doIsFinite@builtinGlobalIsFinite
//aot: [trace] Check Type: NotNumber1
printIsFinite("abc"); //: false

//aot: [trace] aot inline builtin: isFinite, caller function name:#*#doIsFinite@builtinGlobalIsFinite
printIsFinite(-12); //: true
// Call standard builtin with non-number param
//aot: [trace] aot inline builtin: isFinite, caller function name:#*#doIsFinite@builtinGlobalIsFinite
//aot: [trace] Check Type: NotNumber1
printIsFinite("abc"); //: false
//aot: [trace] aot inline builtin: isFinite, caller function name:#*#doIsFinite@builtinGlobalIsFinite
//aot: [trace] Check Type: NotNumber1
printIsFinite("-12"); //: true

if (ArkTools.isAOTCompiled(doIsFinite)) {
    // Replace standard builtin after call to standard builtin was profiled
    isFinite = replace
}
printIsFinite(-12);   //pgo: true
                      //aot: [trace] Check Type: NotCallTarget1
                      //aot: -12
printIsFinite("abc"); //pgo: false
                      //aot: [trace] Check Type: NotCallTarget1
                      //aot: abc

isFinite = true_is_finite

// Check IR correctness inside try-block
try {
    //aot: [trace] aot inline builtin: isFinite, caller function name:#*#doIsFinite@builtinGlobalIsFinite
    printIsFinite(-12); //: true
    //aot: [trace] aot inline builtin: isFinite, caller function name:#*#doIsFinite@builtinGlobalIsFinite
    //aot: [trace] Check Type: NotNumber1
    printIsFinite("abc"); //: false
} catch (e) {
}

let obj = {};
obj.valueOf = (() => { return -23; })
//aot: [trace] aot inline builtin: isFinite, caller function name:func_main_0@builtinGlobalIsFinite
//aot: [trace] Check Type: NotNumber1
print(isFinite(obj)); //: true

function Throwing() {
    this.value = -14;
}
Throwing.prototype.valueOf = function() {
    if (this.value > 0) {
        throw new Error("already positive");
    }
    return this.value;
}
let throwingObj = new Throwing();

try {
    print(isFinite(throwingObj)); //: true
    throwingObj.value = 10;
    print(isFinite(throwingObj)); //: Error: already positive
} catch(e) {
    print(e);
} finally {
    print(isFinite(obj)); //: true
}