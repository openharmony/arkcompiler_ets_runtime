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
function replace()
{
    return "replaced";
}

function doClear(): any {
    print(myMap.size);
    print(myMap.clear());
    print(myMap.size);
}

function printClear() {
    try {
        doClear();
    } finally {
    }
}

function printClear1(x: any) {
    try {
        print(x.clear());
    } finally {
    }
}

let myMap = new Map([[0, 0], [-1, 1], [2.5, -2.5]]);

// Check without params
print(myMap.size); //: 3
//aot: [trace] aot call builtin: Map.clear, caller function name:func_main_0@builtinMapClear
print(myMap.clear()); //: undefined
print(myMap.size); //: 0

// Check with single param
let myMap1 = new Map([[0, 0], [-1, 1], [2.5, -2.5]]);
print(myMap1.size); //: 3
//aot: [trace] aot call builtin: Map.clear, caller function name:func_main_0@builtinMapClear
print(myMap1.clear(125)); //: undefined
print(myMap1.size); //: 0

// Check with 2 params
let myMap2 = new Map([[0, 0], [-1, 1], [2.5, -2.5]]);
print(myMap2.size); //: 3
//aot: [trace] aot call builtin: Map.clear, caller function name:func_main_0@builtinMapClear
print(myMap2.clear(0, undefined)); //: undefined
print(myMap2.size); //: 0

//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapClear
myMap.set(0, 0);
//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapClear
myMap.set(12, 3);

// Replace standard builtin
let true_clear = myMap.clear
myMap.clear = replace
// no deopt
print(myMap.clear()); //: replaced

myMap.clear = true_clear
printClear(); //: 2
              //aot: [trace] aot call builtin: Map.clear, caller function name:#*#doClear@builtinMapClear
              //: undefined
              //: 0

// Call standard builtin with non-number param
//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapClear
myMap.set(0, 0);
//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapClear
myMap.set(12, 3);

print(myMap.size); //: 2
//aot: [trace] aot call builtin: Map.clear, caller function name:func_main_0@builtinMapClear
myMap.clear("abc");
print(myMap.size); //: 0

if (ArkTools.isAOTCompiled(printClear)) {
    // Replace standard builtin after call to standard builtin was profiled
    myMap.clear = replace
}

//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapClear
myMap.set(0, 0);
//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapClear
myMap.set(12, 3);
printClear(); //: 2
              //pgo: undefined
              //pgo: 0
              //aot: [trace] Check Type: NotCallTarget1
              //aot: replaced
              //aot: 2

myMap.clear = true_clear
//aot: [trace] aot call builtin: Map.clear, caller function name:func_main_0@builtinMapClear
myMap.clear();

// Check IR correctness inside try-block
try {
    //aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapClear
    myMap.set(0, 0);
    //aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapClear
    myMap.set(12, 3);
    printClear(); //: 2
                  //aot: [trace] aot call builtin: Map.clear, caller function name:#*#doClear@builtinMapClear
                  //: undefined
                  //: 0
} catch (e) {
}

let obj = {};
obj.valueOf = (() => { return 0; })

//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapClear
myMap.set(0, 0);
//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapClear
myMap.set(12, 3);
print(myMap.size); //: 2
//aot: [trace] aot call builtin: Map.clear, caller function name:func_main_0@builtinMapClear
print(myMap.clear(obj)); //: undefined
print(myMap.size); //: 0

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
    //aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapClear
    myMap.set(0, 0);
    //aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapClear
    myMap.set(12, 3);
    print(myMap.size); //: 2
    //aot: [trace] aot call builtin: Map.clear, caller function name:func_main_0@builtinMapClear
    print(myMap.clear(throwingObj)); //: undefined
    print(myMap.size); //: 0
} catch(e) {
    print(e);
} finally {
    //aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapClear
    myMap.set(0, 0);
    //aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapClear
    myMap.set(12, 3);
    print(myMap.size); //: 2
    //aot: [trace] aot call builtin: Map.clear, caller function name:func_main_0@builtinMapClear
    print(myMap.clear(obj)); //: undefined
    print(myMap.size); //: 0
}

let trueclear = Map.prototype.clear;
let m = new Map();
//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapClear
m.set(1, 2);
//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapClear
m.set(2, 4);
//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapClear
m.set("ab", 5);
//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapClear
m.set("cd", "e");
let obj1 = {};
//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapClear
m.set(obj1, "obj");

print(m.size); //: 5
//aot: [trace] aot call builtin: Map.clear, caller function name:func_main_0@builtinMapClear
m.clear();
print(m.size); //: 0

print("baseline"); //: baseline
//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapClear
m.set(10, 20);
let m2 = new Map([[1, 2]]);
let m3 = new Map([[1, 2]]);
let m4 = new Map([[1, 2]]);

print(m.size); //: 1
//aot: [trace] aot call builtin: Map.clear, caller function name:#*#printClear1@builtinMapClear
printClear1(m) //: undefined
print(m.size); //: 0

print(m2.size); //: 1
//aot: [trace] aot call builtin: Map.clear, caller function name:#*#printClear1@builtinMapClear
printClear1(m2) //: undefined
print(m2.size); //: 0

print(m3.size); //: 1
//aot: [trace] aot call builtin: Map.clear, caller function name:#*#printClear1@builtinMapClear
printClear1(m3) //: undefined
print(m3.size); //: 0

print(m4.size); //: 1
//aot: [trace] aot call builtin: Map.clear, caller function name:#*#printClear1@builtinMapClear
printClear1(m4) //: undefined
print(m4.size); //: 0

print("case 0"); //: case 0
if (ArkTools.isAOTCompiled(printClear1)) {
    m4.garbage = function(x: any) {
        return undefined;
    }
}

// Nothing changed
//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapClear
m.set(10, 20);
//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapClear
m2.set(10, 20);
//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapClear
m3.set(10, 20);
m4.set(10, 20); //aot: [trace] Check Type: BuiltinInstanceHClassMismatch

print(m.size); //: 1
//aot: [trace] aot call builtin: Map.clear, caller function name:#*#printClear1@builtinMapClear
printClear1(m) //: undefined
print(m.size); //: 0

print(m2.size); //: 1
//aot: [trace] aot call builtin: Map.clear, caller function name:#*#printClear1@builtinMapClear
printClear1(m2) //: undefined
print(m2.size); //: 0

print(m3.size); //: 1
//aot: [trace] aot call builtin: Map.clear, caller function name:#*#printClear1@builtinMapClear
printClear1(m3) //: undefined
print(m3.size); //: 0

print(m4.size); //: 1
printClear1(m4) //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
                //: undefined
print(m4.size); //: 0

print("case 1"); //: case 1
if (ArkTools.isAOTCompiled(printClear1)) {
    m3.clear = function() {
        return false;
    }
}

m.set(10, 20);
m2.set(10, 20);
m3.set(10, 20);

print(m.size); //: 1
//aot: [trace] aot call builtin: Map.clear, caller function name:#*#printClear1@builtinMapClear
printClear1(m) //: undefined
print(m.size); //: 0

print(m3.size); //: 1
printClear1(m3) //pgo: undefined
                //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
                //aot: false
print(m3.size); //pgo: 0
                //aot: 1

print("case 2"); //: case 2
let mimicMap = {
    clear: trueclear
}
let mm = new Map([[1, 2]]);

print(mm.size); //: 1
//aot: [trace] aot call builtin: Map.clear, caller function name:#*#printClear1@builtinMapClear
printClear1(mm) //: undefined
print(m.size);  //: 0

if (ArkTools.isAOTCompiled(printClear1)) {
    Object.setPrototypeOf(mm, mimicMap)
}

printClear1(mm) //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
                //: undefined

print("case 3") //: case 3

function checkObjWithMapProto() {
    let o = {};
    //aot: [trace] aot call builtin: Object.SetPrototypeOf, caller function name:#*#checkObjWithMapProto@builtinMapClear
    Object.setPrototypeOf(o, Map.prototype);
    try {
        print((o as Map<number, number>).clear(1));
    } catch(e) {
        print(e);
    }
}

//aot: [trace] Check Type: NotCallTarget1
//: TypeError: obj is not JSMap
checkObjWithMapProto();


if (ArkTools.isAOTCompiled(printClear1)) {
    Map.prototype.clear = function() {
        return "prototype";
    }
}

m.set(10, 20);
m2.set(10, 20);

print(m.size);  //: 1
printClear1(m); //pgo: undefined
                 //aot: [trace] Check Type: NotCallTarget1
                //aot: prototype
print(m.size);  //pgo: 0
                //aot: 1

print(m2.size);  //: 1
printClear1(m2); //pgo: undefined
                 //aot: [trace] Check Type: NotCallTarget1
                 //aot: prototype
print(m2.size);  //pgo: 0
                 //aot: 1
