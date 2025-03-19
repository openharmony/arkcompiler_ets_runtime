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

function doHas(x: any) {
    return mySet.has(x);
}

function printHas(x: any) {
    try {
        print(doHas(x));
    } finally {
    }
}

let mySet = new Set([0, 0.0, -5, 2.5, 1e-78, NaN, "xyz", "12345"]);

// Check without params
//aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetHas
print(mySet.has()); //: false

// Check with adding element undefined
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetHas
mySet.add(undefined);
//aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetHas
print(mySet.has()); //: true

// Check with single param
//aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetHas
print(mySet.has(0)); //: true
//aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetHas
print(mySet.has(3)); //: false
//aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetHas
print(mySet.has(2.5)); //: true
//aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetHas
print(mySet.has(NaN)); //: true

// Check with 2 params
//aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetHas
print(mySet.has(0, 0)); //: true

// Check with 3 params
//aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetHas
print(mySet.has(-21, 10.2, 15)); //: false

// Check with 4 params
//aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetHas
print(mySet.has(2.5, -800, 0.56, 0)); //: true

// Check after inserting elements
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetHas
mySet.add(-5);
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetHas
mySet.add(133.33);
//aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetHas
print(mySet.has(-5)); //: true
//aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetHas
print(mySet.has(133.33)); //: true

// Replace standard builtin
let true_has = mySet.has
mySet.has = replace

// no deopt
print(mySet.has(2.5)); //: 2.5
mySet.has = true_has

function checkObjWithSetProto() {
    let o = {};
    //aot: [trace] aot call builtin: Object.SetPrototypeOf, caller function name:#*#checkObjWithSetProto@builtinSetHas
    Object.setPrototypeOf(o, Set.prototype);
    try {
        print((o as Set<number>).has(1));
    } catch(e) {
        print(e);
    }
}

//aot: [trace] Check Type: NotCallTarget1
//: TypeError: obj is not JSSet
checkObjWithSetProto();

//aot: [trace] aot inline builtin: Set.has, caller function name:#*#doHas@builtinSetHas
printHas(-5); //: true
// Call standard builtin with non-number param
//aot: [trace] aot inline builtin: Set.has, caller function name:#*#doHas@builtinSetHas
printHas("abc"); //: false
//aot: [trace] aot inline builtin: Set.has, caller function name:#*#doHas@builtinSetHas
printHas("-5"); //: false
//aot: [trace] aot inline builtin: Set.has, caller function name:#*#doHas@builtinSetHas
printHas("xyz"); //: true

if (ArkTools.isAOTCompiled(printHas)) {
    // Replace standard builtin after call to standard builtin was profiled
    mySet.has = replace
}
printHas(2.5); //pgo: true
//aot: [trace] Check Type: NotCallTarget1
//aot: 2.5

printHas("abc"); //pgo: false
//aot: [trace] Check Type: NotCallTarget1
//aot: abc

mySet.has = true_has

// Check IR correctness inside try-block
try {
    //aot: [trace] aot inline builtin: Set.has, caller function name:#*#doHas@builtinSetHas
    printHas(NaN); //: true
    //aot: [trace] aot inline builtin: Set.has, caller function name:#*#doHas@builtinSetHas
    printHas("abc"); //: false
} catch (e) {
}

let obj = {};
obj.valueOf = (() => { return 0; })
//aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetHas
print(mySet.has(obj)); //: false

function Throwing() {
    this.value = 2.5;
    this.valueOf = function() {
        if (this.value > 0) {
            throw new Error("already positive");
        }
        return this.value;
    }
}

let throwingObj = new Throwing();
try {
    //aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetHas
    print(mySet.has(throwingObj)); //: false
} catch(e) {
    print(e);
} finally {
    //aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetHas
    print(mySet.has(obj)); //: false
}

// Check after clearing
mySet.clear();
//aot: [trace] aot call builtin: Set.clear, caller function name:func_main_0@builtinSetHas
print(mySet.has(0));
//aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetHas
//: false
