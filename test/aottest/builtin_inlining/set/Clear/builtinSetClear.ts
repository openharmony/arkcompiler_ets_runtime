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
    print(mySet.size);
    print(mySet.clear());
    print(mySet.size);
}

function printClear() {
    try {
        doClear();
    } finally {
    }
}

function printClear2(x: any) {
    try {
        print(x.clear());
    } finally {
    }
}

let mySet = new Set([0, -5, 2.5]);

// Check without params
print(mySet.size); //: 3
//aot: [trace] aot inline builtin: Set.clear, caller function name:func_main_0@builtinSetClear
print(mySet.clear()); //: undefined
print(mySet.size); //: 0

// Check with single param
let mySet1 = new Set([0, -5, 2.5]);
print(mySet1.size); //: 3
//aot: [trace] aot inline builtin: Set.clear, caller function name:func_main_0@builtinSetClear
print(mySet1.clear(125)); //: undefined
print(mySet1.size); //: 0

// Check with 2 params
let mySet2 = new Set([0, -5, 2.5]);
print(mySet2.size); //: 3
//aot: [trace] aot inline builtin: Set.clear, caller function name:func_main_0@builtinSetClear
print(mySet2.clear(0, undefined)); //: undefined
print(mySet2.size); //: 0

mySet.add(0);
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetClear
mySet.add(12);
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetClear

// Replace standard builtin
let true_clear = mySet.clear
mySet.clear = replace
// no deopt
print(mySet.clear()); //: replaced

mySet.clear = true_clear
printClear(); //: 2
              //aot: [trace] aot inline builtin: Set.clear, caller function name:#*#doClear@builtinSetClear
              //: undefined
              //: 0

// Call standard builtin with non-number param
mySet.add(0);
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetClear
mySet.add(12);
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetClear
print(mySet.size); //: 2
//aot: [trace] aot inline builtin: Set.clear, caller function name:func_main_0@builtinSetClear
mySet.clear("abc");
print(mySet.size); //: 0

if (ArkTools.isAOTCompiled(printClear)) {
    // Replace standard builtin after call to standard builtin was profiled
    mySet.clear = replace
}

mySet.add(0);
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetClear
mySet.add(12);
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetClear
printClear(); //: 2
              //aot: [trace] Check Type: NotCallTarget1
              //pgo: undefined
              //pgo: 0
              //aot: replaced
              //aot: 2

mySet.clear = true_clear
//aot: [trace] aot inline builtin: Set.clear, caller function name:func_main_0@builtinSetClear
mySet.clear();

// Check IR correctness inside try-block
try {
    mySet.add(0);
    //aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetClear
    mySet.add(12);
    //aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetClear
    printClear(); //: 2
                  //aot: [trace] aot inline builtin: Set.clear, caller function name:#*#doClear@builtinSetClear
                  //: undefined
                  //: 0
} catch (e) {
}

let obj = {};
obj.valueOf = (() => { return 0; })

mySet.add(0);
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetClear
mySet.add(12);
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetClear
print(mySet.size); //: 2
//aot: [trace] aot inline builtin: Set.clear, caller function name:func_main_0@builtinSetClear
print(mySet.clear(obj)); //: undefined
print(mySet.size); //: 0

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
    mySet.add(0);
    //aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetClear
    mySet.add(12);
    //aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetClear
    print(mySet.size); //: 2
    //aot: [trace] aot inline builtin: Set.clear, caller function name:func_main_0@builtinSetClear
    print(mySet.clear(throwingObj)); //: undefined
    print(mySet.size); //: 0
} catch(e) {
    print(e);
} finally {
    mySet.add(0);
    //aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetClear
    mySet.add(12);
    //aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetClear

    print(mySet.size); //: 2
    //aot: [trace] aot inline builtin: Set.clear, caller function name:func_main_0@builtinSetClear
    print(mySet.clear(obj)); //: undefined
    print(mySet.size); //: 0
}

let trueclear = Set.prototype.clear;
let m = new Set();
m.add(1);
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetClear
m.add(2);
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetClear
m.add("ab");
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetClear
m.add("cd");
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetClear
let obj1 = {};
m.add(obj1);
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetClear

print(m.size); //: 5
//aot: [trace] aot inline builtin: Set.clear, caller function name:func_main_0@builtinSetClear
m.clear();
print(m.size); //: 0

print("baseline"); //: baseline
m.add(20);
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetClear
let m2 = new Set([1]);
let m3 = new Set([1]);
let m4 = new Set([1]);

print(m.size); //: 1
//aot: [trace] aot inline builtin: Set.clear, caller function name:#*#printClear2@builtinSetClear
printClear2(m) //: undefined
print(m.size); //: 0

print(m2.size); //: 1
//aot: [trace] aot inline builtin: Set.clear, caller function name:#*#printClear2@builtinSetClear
printClear2(m2) //: undefined
print(m2.size); //: 0

print(m3.size); //: 1
//aot: [trace] aot inline builtin: Set.clear, caller function name:#*#printClear2@builtinSetClear
printClear2(m3) //: undefined
print(m3.size); //: 0

print(m4.size); //: 1
//aot: [trace] aot inline builtin: Set.clear, caller function name:#*#printClear2@builtinSetClear
printClear2(m4) //: undefined
print(m4.size); //: 0

print("case 0"); //: case 0
if (ArkTools.isAOTCompiled(printClear2)) {
    m4.garbage = function(x: any) {
        return undefined;
    }
}

// Nothing changed
m.add(20);
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetClear
m2.add(20);
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetClear
m3.add(20);
//aot: [trace] aot inline builtin: Set.add, caller function name:func_main_0@builtinSetClear
m4.add(20); //aot: [trace] Check Type: BuiltinInstanceHClassMismatch

print(m.size); //: 1
//aot: [trace] aot inline builtin: Set.clear, caller function name:#*#printClear2@builtinSetClear
printClear2(m) //: undefined
print(m.size); //: 0

print(m2.size); //: 1
//aot: [trace] aot inline builtin: Set.clear, caller function name:#*#printClear2@builtinSetClear
printClear2(m2) //: undefined
print(m2.size); //: 0

print(m3.size); //: 1
//aot: [trace] aot inline builtin: Set.clear, caller function name:#*#printClear2@builtinSetClear
printClear2(m3) //: undefined
print(m3.size); //: 0

print(m4.size); //: 1
printClear2(m4) //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
                //: undefined
print(m4.size); //: 0

print("case 1"); //: case 1
if (ArkTools.isAOTCompiled(printClear2)) {
    m3.clear = function() {
        return false;
    }
}

m.add(20);
m2.add(20);
m3.add(20);

print(m.size); //: 1
//aot: [trace] aot inline builtin: Set.clear, caller function name:#*#printClear2@builtinSetClear
printClear2(m) //: undefined
print(m.size); //: 0

print(m3.size); //: 1
printClear2(m3) //pgo: undefined
                //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
                //aot: false
print(m3.size); //pgo: 0
                //aot: 1

print("case 2"); //: case 2
let mimicSet = {
    clear: trueclear
}
let mm = new Set([1]);

print(mm.size); //: 1
//aot: [trace] aot inline builtin: Set.clear, caller function name:#*#printClear2@builtinSetClear
printClear2(mm) //: undefined
print(m.size);  //: 0

function checkObjWithSetProto() {
    let o = {};
    Object.setPrototypeOf(o, Set.prototype);
    try {
        o.clear(1);
    } catch(e) {
        print(e);
    }
}

//aot: [trace] Check Type: NotCallTarget1
//: TypeError: obj is not JSSet
checkObjWithSetProto();

if (ArkTools.isAOTCompiled(printClear2)) {
    Object.setPrototypeOf(mm, mimicSet)
}

printClear2(mm) //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
                //: undefined

print("case 3") //: case 3
if (ArkTools.isAOTCompiled(printClear2)) {
    Set.prototype.clear = function(x: any) {
        return "prototype";
    }
}

m.add(20);
m2.add(20);
m3.add(20);

print(m.size);  //: 1
printClear2(m); //pgo: undefined
                //aot: [trace] Check Type: NotCallTarget1
                //aot: prototype
print(m.size);  //pgo: 0
                //aot: 1

print(m2.size);  //: 1
printClear2(m2); //pgo: undefined
                 //aot: [trace] Check Type: NotCallTarget1
                 //aot: prototype
print(m2.size);  //pgo: 0
                 //aot: 1
