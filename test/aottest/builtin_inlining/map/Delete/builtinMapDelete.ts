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
    return myMap.delete(x);
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

let myMap = new Map([[0, 0], [0.0, 5], [-1, 1], [2.5, -2.5], [NaN, Infinity], [2000, -0.0], [56, "oops"], ["xyz", "12345"], [-3, 1]]);

// Check without params
//aot: [trace] aot inline builtin: Map.delete, caller function name:func_main_0@builtinMapDelete
print(myMap.delete()); //: false

// Check with seting element undefined
//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapDelete
myMap.set(undefined, 42);
//aot: [trace] aot inline builtin: Map.delete, caller function name:func_main_0@builtinMapDelete
print(myMap.delete()); //: true

// Check with single param
//aot: [trace] aot inline builtin: Map.delete, caller function name:func_main_0@builtinMapDelete
print(myMap.delete(0)); //: true
//aot: [trace] aot inline builtin: Map.delete, caller function name:func_main_0@builtinMapDelete
print(myMap.delete(3)); //: false
//aot: [trace] aot inline builtin: Map.delete, caller function name:func_main_0@builtinMapDelete
print(myMap.delete(NaN)); //: true

// Check with 2 params
//aot: [trace] aot inline builtin: Map.delete, caller function name:func_main_0@builtinMapDelete
print(myMap.delete(2000, 0)); //: true

// Check with 3 params
//aot: [trace] aot inline builtin: Map.delete, caller function name:func_main_0@builtinMapDelete
print(myMap.delete(-51, 10.2, 15)); //: false

// Check after inserting elements
//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapDelete
myMap.set(2000, 1e-98);
//aot: [trace] aot inline builtin: Map.delete, caller function name:func_main_0@builtinMapDelete
print(myMap.delete(2000)); //: true

// Replace standard builtin
let true_delete = myMap.delete
myMap.delete = replace
// no deopt
print(myMap.delete(2.5)); //: 2.5

myMap.delete = true_delete
//aot: [trace] aot inline builtin: Map.delete, caller function name:#*#doDelete@builtinMapDelete
printDelete(-1); //: true

// Call standard builtin with non-number param
//aot: [trace] aot inline builtin: Map.delete, caller function name:func_main_0@builtinMapDelete
print(myMap.delete("abc")); //: false
//aot: [trace] aot inline builtin: Map.delete, caller function name:func_main_0@builtinMapDelete
print(myMap.delete("2.5")); //: false
//aot: [trace] aot inline builtin: Map.delete, caller function name:func_main_0@builtinMapDelete
print(myMap.delete("xyz")); //: true

if (ArkTools.isAOTCompiled(printDelete)) {
    // Replace standard builtin after call to standard builtin was profiled
    myMap.delete = replace
}

printDelete(2.5); //pgo: true
                  //aot: [trace] Check Type: NotCallTarget1
                  //aot: 2.5

printDelete("abc"); //pgo: false
                    //aot: [trace] Check Type: NotCallTarget1
                    //aot: abc

myMap.delete = true_delete

// Check IR correctness inside try-block
try {
    print("try-block"); //: try-block
    //aot: [trace] aot inline builtin: Map.delete, caller function name:#*#doDelete@builtinMapDelete
    printDelete(0); //: false
    //aot: [trace] aot inline builtin: Map.delete, caller function name:#*#doDelete@builtinMapDelete
    printDelete("xyz"); //: false
} catch (e) {
}

let obj = {};
obj.valueOf = (() => { return 0; });

//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapDelete
myMap.set(0, 0);
//aot: [trace] aot inline builtin: Map.delete, caller function name:func_main_0@builtinMapDelete
print(myMap.delete(obj)); //: false

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
//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapDelete
myMap.set(2, 4);
try {
//aot: [trace] aot inline builtin: Map.delete, caller function name:func_main_0@builtinMapDelete
print(myMap.delete(throwingObj)); //: false
} catch(e) {
    print(e);
} finally {
//aot: [trace] aot inline builtin: Map.delete, caller function name:func_main_0@builtinMapDelete
    print(myMap.delete(obj)); //: false
}

// Check after clearing
myMap.clear();
//aot: [trace] aot call builtin: Map.clear, caller function name:func_main_0@builtinMapDelete
print(myMap.delete(2000));
//aot: [trace] aot inline builtin: Map.delete, caller function name:func_main_0@builtinMapDelete
//: false

let truedelete = Map.prototype.delete;
let m = new Map();
//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapDelete
m.set(1, 2);
//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapDelete
m.set(2, 4);
//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapDelete
m.set("ab", 5);
//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapDelete
m.set("cd", "e");
let obj1 = {};
//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapDelete
m.set(obj1, "obj");

print("prototype"); //: prototype
//aot: [trace] aot inline builtin: Map.delete, caller function name:func_main_0@builtinMapDelete
print(m.delete(1)); //: true
//aot: [trace] aot inline builtin: Map.delete, caller function name:func_main_0@builtinMapDelete
print(m.delete(2)); //: true
//aot: [trace] aot inline builtin: Map.delete, caller function name:func_main_0@builtinMapDelete
print(m.delete(3)); //: false
//aot: [trace] aot inline builtin: Map.delete, caller function name:func_main_0@builtinMapDelete
print(m.delete("ab")); //: true
//aot: [trace] aot inline builtin: Map.delete, caller function name:func_main_0@builtinMapDelete
print(m.delete("cd")); //: true
//aot: [trace] aot inline builtin: Map.delete, caller function name:func_main_0@builtinMapDelete
print(m.delete("x")); //: false
//aot: [trace] aot inline builtin: Map.delete, caller function name:func_main_0@builtinMapDelete
print(m.delete(obj1)); //: true

let obj2 = {};
//aot: [trace] aot inline builtin: Map.delete, caller function name:func_main_0@builtinMapDelete
print(m.delete(obj2)); //: false

//aot: [trace] aot inline builtin: Map.delete, caller function name:func_main_0@builtinMapDelete
print(m.delete()); //: false
//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapDelete
m.set(undefined, -1);
//aot: [trace] aot inline builtin: Map.delete, caller function name:func_main_0@builtinMapDelete
print(m.delete()); //: true

print("baseline"); //: baseline
//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapDelete
m.set(10, 20);
let m2 = new Map([[1, 2]]);
let m3 = new Map([[1, 2]]);
let m4 = new Map([[1, 2]]);

//aot: [trace] aot inline builtin: Map.delete, caller function name:#*#printDelete2@builtinMapDelete
printDelete2(m, 10); //: true
//aot: [trace] aot inline builtin: Map.delete, caller function name:#*#printDelete2@builtinMapDelete
printDelete2(m2, 1); //: true
//aot: [trace] aot inline builtin: Map.delete, caller function name:#*#printDelete2@builtinMapDelete
printDelete2(m3, 1); //: true
//aot: [trace] aot inline builtin: Map.delete, caller function name:#*#printDelete2@builtinMapDelete
printDelete2(m4, 1); //: true

print("case 0"); //: case 0
if (ArkTools.isAOTCompiled(printDelete2)) {
    m4.garbage = function(x: any) {
        return undefined;
    }
}

// Nothing changed
//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapDelete
m.set(10, 20);
//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapDelete
m2.set(10, 20);
//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapDelete
m3.set(10, 20);
m4.set(10, 20); //aot: [trace] Check Type: BuiltinInstanceHClassMismatch

//aot: [trace] aot inline builtin: Map.delete, caller function name:#*#printDelete2@builtinMapDelete
printDelete2(m, 10);  //: true
//aot: [trace] aot inline builtin: Map.delete, caller function name:#*#printDelete2@builtinMapDelete
printDelete2(m2, 10); //: true
//aot: [trace] aot inline builtin: Map.delete, caller function name:#*#printDelete2@builtinMapDelete
printDelete2(m3, 10); //: true
printDelete2(m4, 10); //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
                      //: true

print("case 1"); //: case 1
if (ArkTools.isAOTCompiled(printDelete2)) {
    m3.delete = function(x: any) {
        return -x;
    }
}

m.set(10, 20);
m2.set(10, 20);
m3.set(10, 20);

//aot: [trace] aot inline builtin: Map.delete, caller function name:#*#printDelete2@builtinMapDelete
printDelete2(m, 10);  //: true
//aot: [trace] aot inline builtin: Map.delete, caller function name:#*#printDelete2@builtinMapDelete
printDelete2(m2, 10); //: true
printDelete2(m3, 10); //pgo: true
                      //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
                      //aot: -10

print("case 2"); //: case 2
let mimicMap = {
    delete: truedelete
}
let mm = new Map([[1, 2]]);

//aot: [trace] aot inline builtin: Map.delete, caller function name:#*#printDelete2@builtinMapDelete
printDelete2(mm, 1); //: true
if (ArkTools.isAOTCompiled(printDelete2)) {
    Object.setPrototypeOf(mm, mimicMap);
}
printDelete2(mm, 1); //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
                    //: false

print("case 3") //: case 3

function checkObjWithMapProto() {
    let o = {};
    //aot: [trace] aot call builtin: Object.SetPrototypeOf, caller function name:#*#checkObjWithMapProto@builtinMapDelete
    Object.setPrototypeOf(o, Map.prototype);
    try {
        print((o as Map<number, number>).delete(1));
    } catch(e) {
        print(e);
    }
}

//aot: [trace] Check Type: NotCallTarget1
//: TypeError: obj is not JSMap
checkObjWithMapProto();

if (ArkTools.isAOTCompiled(printDelete2)) {
    Map.prototype.delete = function(x: any) {
        return -x * 10;
    }
}

m.set(10, 20);
m2.set(10, 20);

printDelete2(m, 10); //pgo: true
                     //aot: [trace] Check Type: NotCallTarget1
                     //aot: -100
printDelete2(m2, 10); //pgo: true
                      //aot: [trace] Check Type: NotCallTarget1
                      //aot: -100
