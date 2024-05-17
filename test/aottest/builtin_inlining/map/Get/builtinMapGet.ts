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

function doGet(x: any): any {
    return myMap.get(x);
}

function printGet(x: any) {
    try {
        print(doGet(x));
    } finally {
    }
}

function printGet2(x: any, y: any) {
    try {
        print(x.get(y));
    } finally {
    }
}

let myMap = new Map([[0, 0], [0.0, 5], [-1, 1], [2.5, -2.5], [NaN, Infinity], [2000, -0.0], [56, "oops"], ["xyz", "12345"], [-3, 1]]);

// Check without params
//aot: [trace] aot inline builtin: Map.get, caller function name:func_main_0@builtinMapGet
print(myMap.get()); //: undefined

// Check with adding element undefined
myMap.set(undefined, 42);
//aot: [trace] aot inline builtin: Map.get, caller function name:func_main_0@builtinMapGet
print(myMap.get()); //: 42

// Check with single param
//aot: [trace] aot inline builtin: Map.get, caller function name:func_main_0@builtinMapGet
print(myMap.get(0)); //: 5
//aot: [trace] aot inline builtin: Map.get, caller function name:func_main_0@builtinMapGet
print(myMap.get(3)); //: undefined
//aot: [trace] aot inline builtin: Map.get, caller function name:func_main_0@builtinMapGet
print(myMap.get(2.5)); //: -2.5
//aot: [trace] aot inline builtin: Map.get, caller function name:func_main_0@builtinMapGet
print(myMap.get(NaN)); //: Infinity
//aot: [trace] aot inline builtin: Map.get, caller function name:func_main_0@builtinMapGet
print("1/x: " + 1 / myMap.get(2000)); //: 1/x: -Infinity

// Check with 2 params
//aot: [trace] aot inline builtin: Map.get, caller function name:func_main_0@builtinMapGet
print(myMap.get(0, 0)); //: 5

// Check with 3 params
//aot: [trace] aot inline builtin: Map.get, caller function name:func_main_0@builtinMapGet
print(myMap.get(-1, 10.2, 15)); //: 1

// Check with 4 params
//aot: [trace] aot inline builtin: Map.get, caller function name:func_main_0@builtinMapGet
print(myMap.get(2.5, -800, 0.56, 0)); //: -2.5

// Check after inserting elements
myMap.set(2000, 1e-98);
myMap.set(133.33, -1);
//aot: [trace] aot inline builtin: Map.get, caller function name:func_main_0@builtinMapGet
print(myMap.get(2000)); //: 1e-98
//aot: [trace] aot inline builtin: Map.get, caller function name:func_main_0@builtinMapGet
print(myMap.get(133.33)); //: -1

// Replace standard builtin
let true_get = myMap.get
myMap.get = replace

// no deopt
print(myMap.get(2.5)); //: 2.5
myMap.get = true_get

//aot: [trace] aot inline builtin: Map.get, caller function name:#*#doGet@builtinMapGet
printGet(-1); //: 1
// Call standard builtin with non-number param
//aot: [trace] aot inline builtin: Map.get, caller function name:func_main_0@builtinMapGet
print(myMap.get("abc")); //: undefined
//aot: [trace] aot inline builtin: Map.get, caller function name:func_main_0@builtinMapGet
print(myMap.get("-1")); //: undefined
//aot: [trace] aot inline builtin: Map.get, caller function name:func_main_0@builtinMapGet
print(myMap.get(56)); //: oops
//aot: [trace] aot inline builtin: Map.get, caller function name:func_main_0@builtinMapGet
print(myMap.get("xyz")); //: 12345

if (ArkTools.isAOTCompiled(printGet)) {
    // Replace standard builtin after call to standard builtin was profiled
    myMap.get = replace
}
printGet(2.5); //pgo: -2.5
//aot: [trace] Check Type: NotCallTarget1
//aot: 2.5

printGet("abc"); //pgo: undefined
//aot: [trace] Check Type: NotCallTarget1
//aot: abc

myMap.get = true_get

// Check IR correctness inside try-block
try {
    //aot: [trace] aot inline builtin: Map.get, caller function name:#*#doGet@builtinMapGet
    printGet(2.5); //: -2.5
    //aot: [trace] aot inline builtin: Map.get, caller function name:#*#doGet@builtinMapGet
    printGet("abc"); //: undefined
} catch (e) {
}

let obj = {};
obj.valueOf = (() => { return 0; })
//aot: [trace] aot inline builtin: Map.get, caller function name:func_main_0@builtinMapGet
print(myMap.get(obj)); //: undefined

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
    //aot: [trace] aot inline builtin: Map.get, caller function name:func_main_0@builtinMapGet
    print(myMap.get(throwingObj)); //: undefined
} catch(e) {
    print(e);
} finally {
    //aot: [trace] aot inline builtin: Map.get, caller function name:func_main_0@builtinMapGet
    print(myMap.get(obj)); //: undefined
}

// Check after clearing
myMap.clear();
//aot: [trace] aot inline builtin: Map.clear, caller function name:func_main_0@builtinMapGet
print(myMap.get(2000));
//aot: [trace] aot inline builtin: Map.get, caller function name:func_main_0@builtinMapGet
//: undefined

let trueGet = Map.prototype.get
let m = new Map()
m.set(1, 2)
m.set(2, 4)
m.set("ab", 5)
m.set("cd", "e")
let obj1 = {}
m.set(obj1, "obj")

//aot: [trace] aot inline builtin: Map.get, caller function name:func_main_0@builtinMapGet
print(m.get(1)) //: 2
//aot: [trace] aot inline builtin: Map.get, caller function name:func_main_0@builtinMapGet
print(m.get(2)) //: 4
//aot: [trace] aot inline builtin: Map.get, caller function name:func_main_0@builtinMapGet
print(m.get(3)) //: undefined
//aot: [trace] aot inline builtin: Map.get, caller function name:func_main_0@builtinMapGet
print(m.get("ab")) //: 5
//aot: [trace] aot inline builtin: Map.get, caller function name:func_main_0@builtinMapGet
print(m.get("cd")) //: e
//aot: [trace] aot inline builtin: Map.get, caller function name:func_main_0@builtinMapGet
print(m.get("x")) //: undefined
//aot: [trace] aot inline builtin: Map.get, caller function name:func_main_0@builtinMapGet
print(m.get(obj1)) //: obj
let obj2 = {}
//aot: [trace] aot inline builtin: Map.get, caller function name:func_main_0@builtinMapGet
print(m.get(obj2)) //: undefined
//aot: [trace] aot inline builtin: Map.get, caller function name:func_main_0@builtinMapGet
print(m.get()) //: undefined
m.set(undefined, -1)
//aot: [trace] aot inline builtin: Map.get, caller function name:func_main_0@builtinMapGet
print(m.get()) //: -1

function checkObjWithMapProto() {
    let o = {};
    Object.setPrototypeOf(o, Map.prototype);
    try {
        print((o as Map<number, number>).get(1));
    } catch(e) {
        print(e);
    }
}

//aot: [trace] Check Type: NotCallTarget1
//: TypeError: obj is not JSMap
checkObjWithMapProto();

let notMap = {
    get(x: any) {
        return this[x] + x
    }
}

print("baseline") //: baseline
m[10] = 20
let m2 = new Map([[1, 2]])
let m3 = new Map([[1, 2]])
let m4 = new Map([[1, 2]])
//aot: [trace] aot inline builtin: Map.get, caller function name:#*#printGet2@builtinMapGet
printGet2(m, 10) //: undefined
//aot: [trace] aot inline builtin: Map.get, caller function name:#*#printGet2@builtinMapGet
printGet2(m2, 1) //: 2
//aot: [trace] aot inline builtin: Map.get, caller function name:#*#printGet2@builtinMapGet
printGet2(m3, 1) //: 2
//aot: [trace] aot inline builtin: Map.get, caller function name:#*#printGet2@builtinMapGet
printGet2(m4, 1) //: 2

print("case 0") //: case 0
if (ArkTools.isAOTCompiled(printGet2)) {
    m4.garbage = function(x: any) {
        return undefined
    }
}
// Nothing changed
//aot: [trace] aot inline builtin: Map.get, caller function name:#*#printGet2@builtinMapGet
printGet2(m, 10) //: undefined
//aot: [trace] aot inline builtin: Map.get, caller function name:#*#printGet2@builtinMapGet
printGet2(m2, 1) //: 2
//aot: [trace] aot inline builtin: Map.get, caller function name:#*#printGet2@builtinMapGet
printGet2(m3, 1) //: 2
printGet2(m4, 1) //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
                //: 2

print("case 1") //: case 1
if (ArkTools.isAOTCompiled(printGet2)) {
    m3.get = function(x: any) {
        return -x
    }
}

//aot: [trace] aot inline builtin: Object.getPrototypeOf, caller function name:func_main_0@builtinMapGet
print(Object.getPrototypeOf(m3) === Map.prototype) //: true

//aot: [trace] aot inline builtin: Map.get, caller function name:#*#printGet2@builtinMapGet
printGet2(m, 10) //: undefined
//aot: [trace] aot inline builtin: Map.get, caller function name:#*#printGet2@builtinMapGet
printGet2(m2, 1) //: 2
printGet2(m3, 1) //pgo: 2
                //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
                //aot: -1

print("case 2") //: case 2
let mimicMap = {
    get: trueGet
}
let mm = new Map([[1, 2]])
//aot: [trace] aot inline builtin: Map.get, caller function name:#*#printGet2@builtinMapGet
printGet2(mm, 1) //: 2
if (ArkTools.isAOTCompiled(printGet2)) {
    Object.setPrototypeOf(mm, mimicMap)
}
printGet2(mm, 1) //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
                 //: 2

print("case 3") //: case 3
if (ArkTools.isAOTCompiled(printGet2)) {
    Map.prototype.get = function(x: any) {
        return -x * 10
    }
}

printGet2(m, 10) //pgo: undefined
                //aot: [trace] Check Type: NotCallTarget1
                //aot: -100
printGet2(m2, 1) //pgo: 2
                //aot: [trace] Check Type: NotCallTarget1
                //aot: -10
printGet2(m3, 1) //pgo: 2
                //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
                //aot: -1

print("case 4") //: case 4
if (ArkTools.isAOTCompiled(printGet2)) {
    Object.setPrototypeOf(m, notMap)
}

printGet2(m, 10) //pgo: undefined
                //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
                //aot: 30
printGet2(m2, 1) //pgo: 2
                //aot: [trace] Check Type: NotCallTarget1
                //aot: -10
printGet2(m3, 1) //pgo: 2
                //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
                //aot: -1
