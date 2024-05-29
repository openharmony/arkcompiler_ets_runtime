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
function replace(x : any)
{
    return x;
}

function doAdd(x : any) {
    return mySet.add(x);
}

function printAdd(x : any) {
    try {
        print(doAdd(x));
    } finally {
    }
}

function tryAdd(x: any, y : any) {
    try {
        print(x.add(y));
    } finally {
    }
}

let mySet = new Set();

// Check without params
print(mySet.add());
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetAdd
//: [object Set]
print(mySet.size);
//: 1
print(mySet.has(undefined));
//aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetAdd
//: true

// Check with single param
mySet.add(125);
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetAdd
print(mySet.size);
//: 2
print(mySet.has(125));
//aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetAdd
//: true

// Check with 2 params
mySet.add(0, undefined);
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetAdd
print(mySet.size);
//: 3
print(mySet.has(0));
//aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetAdd
//: true

// Check with 3 params
mySet.add(0, "ab", 14);
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetAdd
print(mySet.size);
//: 3
print(mySet.has(0));
//aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetAdd
//: true

for (let key of mySet.values()) {
    print(key);
}
//: undefined
//: 125
//: 0

let true_add = mySet.add

printAdd(15);
//aot: [trace] aot inline builtin: Set.add, caller function name:#*#doAdd@builtinSetAdd
//: [object Set]
print(mySet.has(15));
//aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetAdd
//: true

// Call standard builtin with non-number param
mySet.add("abc");
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetAdd
print(mySet.has("abc"));
//aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetAdd
//: true

if (ArkTools.isAOTCompiled(printAdd)) {
    // Replace standard builtin after call to standard builtin was profiled
    mySet.add = replace
}

printAdd(42);
//pgo: [object Set]
//aot: [trace] Check Type: BuiltinInstanceHClassMismatch
//aot: 42
print(mySet.has(42));
//pgo: true
//aot: [trace] Check Type: BuiltinInstanceHClassMismatch
//aot: false

print("fuck") //: fuck
mySet.add = true_add
print(mySet.add("xyz"));
//: [object Set]
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetAdd
print(mySet.has("xyz"));
//aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetAdd
//: true

// Check IR correctness inside try-block
try {
    printAdd(2.5);
    //aot: [[trace] aot inline builtin: Set.add, caller function name:#*#doAdd@builtinSetAdd
    //: [object Set]
    printAdd("oops");
    //aot: [[trace] aot inline builtin: Set.add, caller function name:#*#doAdd@builtinSetAdd
    //: [object Set]
    print(mySet.has(2.5));
    //aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetAdd
    //: true
    print(mySet.has("oops"));
    //aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetAdd
    //: true
} catch (e) {
}


let obj = {};
obj.valueOf = (() => { return 7; })

mySet.add(obj);
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetAdd
print(mySet.has(obj));
//aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetAdd
//: true
print(mySet.has(7));
//aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetAdd
//: false
mySet.clear();

function Throwing() {
    this.value = 2;
    Throwing.prototype.valueOf = function() {
        if (this.value > 0) {
            throw new Error("positive");
        }
        return this.value;
    }
}

let throwingObj = new Throwing();
try {
    mySet.add(throwingObj);
    //aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetAdd
    print(mySet.has(throwingObj));
    //aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetAdd
    //: true
    print(mySet.has(2));
    //aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetAdd
    //: false
} catch(e) {
    print(e);
} finally {
    mySet.add(obj);
    //aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetAdd
    print(mySet.has(obj));
    //aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetAdd
    //: true
    print(mySet.has(7));
    //aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetAdd
    //: false
}

let trueadd = Set.prototype.add;
let m = new Set();

print("baseline"); //: baseline
let m2 = new Set([1]);
let m3 = new Set([1]);
let m4 = new Set([1]);

tryAdd(m, 13);
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetAdd
//: [object Set]
print(m.has(13));
//aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetAdd
//: true

tryAdd(m2, 13);
//aot: [trace] aot inline builtin: Set.add, caller function name:#*#doAdd@builtinSetAdd
//: [object Set]
print(m2.has(13));
//aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetAdd
//: true

tryAdd(m3, 13);
//aot: [trace] aot inline builtin: Set.add, caller function name:#*#doAdd@builtinSetAdd
//: [object Set]
print(m3.has(13));
//aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetAdd
//: true

tryAdd(m4, 13);
//aot: [trace] aot inline builtin: Set.add, caller function name:#*#doAdd@builtinSetAdd
//: [object Set]
print(m4.has(13));
//aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetAdd
//: true

print("case 0"); //: case 0
if (ArkTools.isAOTCompiled(tryAdd)) {
    m4.garbage = function(x: any) {
        return undefined;
    }
}

// Nothing changed
tryAdd(m, 25);
//aot: [trace] aot inline builtin: Set.add, caller function name:#*#doAdd@builtinSetAdd
//: [object Set]
print(m.has(25));
//aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetAdd
//: true

tryAdd(m2, 25);
//aot: [trace] aot inline builtin: Set.add, caller function name:#*#doAdd@builtinSetAdd
//: [object Set]
print(m2.has(25));
//aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetAdd
//: true

tryAdd(m3, 25);
//aot: [trace] aot inline builtin: Set.add, caller function name:#*#doAdd@builtinSetAdd
//: [object Set]
print(m3.has(25));
//aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetAdd
//: true

tryAdd(m4);
//aot: [trace] Check Type: BuiltinInstanceHClassMismatch
//aot: [trace] aot inline builtin: Set.add, caller function name:#*#doAdd@builtinSetAdd
//: [object Set]
print(m3.has(25));
//aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetAdd
//: true

print("case 2");
//: case 2
let mimicSet = {
    add: trueadd
}
let mm = new Set([1]);

tryAdd(mm, -200);
//aot: [trace] aot inline builtin: Set.add, caller function name:#*#doAdd@builtinSetAdd
//: [object Set]
print(mm.has(-200));
//aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetAdd
//: true

if (ArkTools.isAOTCompiled(tryAdd)) {
    Object.setPrototypeOf(mm, mimicSet)
}

tryAdd(mm, 32);
//aot: [trace] Check Type: BuiltinInstanceHClassMismatch
//pgo: [object Set]
//aot: [object Object]

print("case 3") //: case 3
if (ArkTools.isAOTCompiled(tryAdd)) {
    Set.prototype.add = function(x: any) {
        return "prototype";
    }
}

tryAdd(m, -1);
//aot: [trace] aot inline builtin: Set.add, caller function name:#*#doAdd@builtinSetAdd
//pgo: [object Set]
//aot: prototype
print(m.has(-1));
//aot: [trace] aot inline builtin: Set.has, caller function name:func_main_0@builtinSetAdd
//: true

function checkObjWithSetProto() {
    let o = {};
    Object.setPrototypeOf(o, Set.prototype);
    try {
        o.add(1);
    } catch(e) {
        print(e);
    }
}

//aot: [trace] Check Type: NotCallTarget1
//: TypeError: obj is not JSSet
checkObjWithSetProto();
