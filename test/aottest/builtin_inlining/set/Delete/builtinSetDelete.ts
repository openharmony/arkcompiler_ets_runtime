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

function doDelete(x: any): any {
    return mySet.delete(x);
}

function printDelete(x: any) {
    try {
        print(doDelete(x));
    } finally {
    }
}

function printDelete2(x: any, y: any) {
    try {
        print(x.delete(y));
    } finally {
    }
}

let mySet = new Set([0, 0.0, -5, 2.5, 1e-78, NaN, "xyz", "12345"]);

// Check without params
//aot: [trace] aot inline builtin: Set.delete, caller function name:func_main_0@builtinSetDelete
print(mySet.delete()); //: false

// Check with adding element undefined
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetDelete
mySet.add(undefined, 42);
//aot: [trace] aot inline builtin: Set.delete, caller function name:func_main_0@builtinSetDelete
print(mySet.delete()); //: true

// Check with single param
//aot: [trace] aot inline builtin: Set.delete, caller function name:func_main_0@builtinSetDelete
print(mySet.delete(0)); //: true
//aot: [trace] aot inline builtin: Set.delete, caller function name:func_main_0@builtinSetDelete
print(mySet.delete(3)); //: false
//aot: [trace] aot inline builtin: Set.delete, caller function name:func_main_0@builtinSetDelete
print(mySet.delete(NaN)); //: true

// Check with 2 params
//aot: [trace] aot inline builtin: Set.delete, caller function name:func_main_0@builtinSetDelete
print(mySet.delete(-5, 0)); //: true

// Check with 3 params
//aot: [trace] aot inline builtin: Set.delete, caller function name:func_main_0@builtinSetDelete
print(mySet.delete(-51, 10.2, 15)); //: false

// Check after inserting elements
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetDelete
mySet.add(2000);
//aot: [trace] aot inline builtin: Set.delete, caller function name:func_main_0@builtinSetDelete
print(mySet.delete(2000)); //: true

// Replace standard builtin
let true_delete = mySet.delete
mySet.delete = replace
// no deopt
print(mySet.delete(2.5)); //: 2.5

mySet.delete = true_delete
//aot: [trace] aot inline builtin: Set.delete, caller function name:#*#doDelete@builtinSetDelete
printDelete(1e-78); //: true

// Call standard builtin with non-number param
//aot: [trace] aot inline builtin: Set.delete, caller function name:func_main_0@builtinSetDelete
print(mySet.delete("abc")); //: false
//aot: [trace] aot inline builtin: Set.delete, caller function name:func_main_0@builtinSetDelete
print(mySet.delete("2.5")); //: false
//aot: [trace] aot inline builtin: Set.delete, caller function name:func_main_0@builtinSetDelete
print(mySet.delete("xyz")); //: true

if (ArkTools.isAOTCompiled(printDelete)) {
    // Replace standard builtin after call to standard builtin was profiled
    mySet.delete = replace
}

printDelete(2.5); //pgo: true
                  //aot: [trace] Check Type: NotCallTarget1
                  //aot: 2.5

printDelete("abc"); //pgo: false
                    //aot: [trace] Check Type: NotCallTarget1
                    //aot: abc

mySet.delete = true_delete

// Check IR correctness inside try-block
try {
    print("try-block"); //: try-block
    //aot: [trace] aot inline builtin: Set.delete, caller function name:#*#doDelete@builtinSetDelete
    printDelete(0); //: false
    //aot: [trace] aot inline builtin: Set.delete, caller function name:#*#doDelete@builtinSetDelete
    printDelete("xyz"); //: false
} catch (e) {
}

let obj = {};
obj.valueOf = (() => { return 0; });

//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetDelete
mySet.add(0, 0);
//aot: [trace] aot inline builtin: Set.delete, caller function name:func_main_0@builtinSetDelete
print(mySet.delete(obj)); //: false

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
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetDelete
mySet.add(2);
try {
//aot: [trace] aot inline builtin: Set.delete, caller function name:func_main_0@builtinSetDelete
print(mySet.delete(throwingObj)); //: false
} catch(e) {
    print(e);
} finally {
//aot: [trace] aot inline builtin: Set.delete, caller function name:func_main_0@builtinSetDelete
print(mySet.delete(obj)); //: false
}

// Check after clearing
mySet.clear();
//aot: [trace] aot call builtin: Set.clear, caller function name:func_main_0@builtinSetDelete
print(mySet.delete(2000));
//aot: [trace] aot inline builtin: Set.delete, caller function name:func_main_0@builtinSetDelete
//: false

let truedelete = Set.prototype.delete;
let m = new Set();
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetDelete
m.add(1);
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetDelete
m.add(2);
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetDelete
m.add("ab");
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetDelete
m.add("cd");
let obj1 = {};
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetDelete
m.add(obj1);

print("prototype"); //: prototype
//aot: [trace] aot inline builtin: Set.delete, caller function name:func_main_0@builtinSetDelete
print(m.delete(1)); //: true
//aot: [trace] aot inline builtin: Set.delete, caller function name:func_main_0@builtinSetDelete
print(m.delete(2)); //: true
//aot: [trace] aot inline builtin: Set.delete, caller function name:func_main_0@builtinSetDelete
print(m.delete(3)); //: false
//aot: [trace] aot inline builtin: Set.delete, caller function name:func_main_0@builtinSetDelete
print(m.delete("ab")); //: true
//aot: [trace] aot inline builtin: Set.delete, caller function name:func_main_0@builtinSetDelete
print(m.delete("cd")); //: true
//aot: [trace] aot inline builtin: Set.delete, caller function name:func_main_0@builtinSetDelete
print(m.delete("x")); //: false
//aot: [trace] aot inline builtin: Set.delete, caller function name:func_main_0@builtinSetDelete
print(m.delete(obj1)); //: true

let obj2 = {};
//aot: [trace] aot inline builtin: Set.delete, caller function name:func_main_0@builtinSetDelete
print(m.delete(obj2)); //: false

//aot: [trace] aot inline builtin: Set.delete, caller function name:func_main_0@builtinSetDelete
print(m.delete()); //: false
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetDelete
m.add(undefined, -1);
//aot: [trace] aot inline builtin: Set.delete, caller function name:func_main_0@builtinSetDelete
print(m.delete()); //: true

print("baseline"); //: baseline
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetDelete
m.add(10);
let m2 = new Set([1]);
let m3 = new Set([1]);
let m4 = new Set([1]);

//aot: [trace] aot inline builtin: Set.delete, caller function name:#*#printDelete2@builtinSetDelete
printDelete2(m, 10); //: true
//aot: [trace] aot inline builtin: Set.delete, caller function name:#*#printDelete2@builtinSetDelete
printDelete2(m2, 1); //: true
//aot: [trace] aot inline builtin: Set.delete, caller function name:#*#printDelete2@builtinSetDelete
printDelete2(m3, 1); //: true
//aot: [trace] aot inline builtin: Set.delete, caller function name:#*#printDelete2@builtinSetDelete
printDelete2(m4, 1); //: true

print("case 0") //: case 0
if (ArkTools.isAOTCompiled(printDelete2)) {
    m4.garbage = function(x: any) {
        return undefined;
    }
}

// Nothing changed
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetDelete
m.add(10);
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetDelete
m2.add(10);
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetDelete
m3.add(10);
//aot: [trace] Check Type: BuiltinInstanceHClassMismatch
m4.add(10);

//aot: [trace] aot inline builtin: Set.delete, caller function name:#*#printDelete2@builtinSetDelete
printDelete2(m, 10);  //: true
//aot: [trace] aot inline builtin: Set.delete, caller function name:#*#printDelete2@builtinSetDelete
printDelete2(m2, 10); //: true
//aot: [trace] aot inline builtin: Set.delete, caller function name:#*#printDelete2@builtinSetDelete
printDelete2(m3, 10); //: true
printDelete2(m4, 10); //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
                     //: true

print("case 1") //: case 1
if (ArkTools.isAOTCompiled(printDelete2)) {
    m3.delete = function(x: any) {
        return -x;
    }
}

m.add(10);
m2.add(10);
m3.add(10);

//aot: [trace] aot inline builtin: Set.delete, caller function name:#*#printDelete2@builtinSetDelete
printDelete2(m, 10);  //: true
//aot: [trace] aot inline builtin: Set.delete, caller function name:#*#printDelete2@builtinSetDelete
printDelete2(m2, 10); //: true
printDelete2(m3, 10); //pgo: true
                      //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
                      //aot: -10

print("case 2") //: case 2
let mimicset = {
    delete: truedelete
}
let mm = new Set([1]);

//aot: [trace] aot inline builtin: Set.delete, caller function name:#*#printDelete2@builtinSetDelete
printDelete2(mm, 1) //: true
if (ArkTools.isAOTCompiled(printDelete2)) {
    Object.setPrototypeOf(mm, mimicset)
}
printDelete2(mm, 1); //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
                     //: false

print("case 3"); //: case 3

function checkObjWithSetProto() {
    let o = {};
    //aot: [trace] aot call builtin: Object.SetPrototypeOf, caller function name:#*#checkObjWithSetProto@builtinSetDelete
    Object.setPrototypeOf(o, Set.prototype);
    try {
        o.delete(1);
    } catch(e) {
        print(e);
    }
}

//aot: [trace] Check Type: NotCallTarget1
//: TypeError: obj is not JSSet
checkObjWithSetProto();


if (ArkTools.isAOTCompiled(printDelete2)) {
    Set.prototype.delete = function(x: any) {
        return -x * 10;
    }
}

m.add(10);
m2.add(10);

printDelete2(m, 10); //pgo: true
                     //aot: [trace] Check Type: NotCallTarget1
                     //aot: -100
printDelete2(m2, 10); //pgo: true
                      //aot: [trace] Check Type: NotCallTarget1
                      //aot: -100
