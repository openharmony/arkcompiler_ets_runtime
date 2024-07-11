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

declare function print(arg:any):string;
var x = [1,2.5,NaN,undefined,null,false,true]
function foo(ele){
    print(ele)
    if (ele != 1 || ele != true){
        return true
    }
    return false
}
//aot: [trace] aot inline builtin: Array.prototype.foreach, caller function name:func_main_0@builtinArrayForEach
x.forEach(ele=>{
    print(ele)
    if (ele != 1 || ele != true){
        return true
    }
})
//: 1
//: 2.5
//: NaN
//: undefined
//: null
//: false
//: true
//aot: [trace] aot inline builtin: Array.prototype.foreach, caller function name:func_main_0@builtinArrayForEach
x.forEach(foo)
//: 1
//: 2.5
//: NaN
//: undefined
//: null
//: false
//: true

// Check without args
try {
    print(x.forEach())
} catch(e) {
    print(e) //: TypeError: the callbackfun is not callable.
}

//aot: [trace] aot inline builtin: Array.prototype.foreach, caller function name:func_main_0@builtinArrayForEach
print(x.forEach(() => {})) //: undefined
//aot: [trace] aot inline builtin: Array.prototype.foreach, caller function name:func_main_0@builtinArrayForEach
print(x.forEach(x => true)) //: undefined
//aot: [trace] aot inline builtin: Array.prototype.foreach, caller function name:func_main_0@builtinArrayForEach
print(x.forEach(x => false)) //: undefined
//aot: [trace] aot inline builtin: Array.prototype.foreach, caller function name:func_main_0@builtinArrayForEach
print(x.forEach((x,y) => {})) //: undefined
//aot: [trace] aot inline builtin: Array.prototype.foreach, caller function name:func_main_0@builtinArrayForEach
print(x.forEach((x,y) => x == true)) //: undefined
//aot: [trace] aot inline builtin: Array.prototype.foreach, caller function name:func_main_0@builtinArrayForEach
print(x.forEach(x => x == true, 0)) //: undefined

// Check inside try-block
try {
    //aot: [trace] aot inline builtin: Array.prototype.foreach, caller function name:func_main_0@builtinArrayForEach
    print(x.forEach(x => true)) //: undefined
} catch(e) { 
}

// Replace standard builtin
function replace(a : any) {
    return a;
}

let newArr = [1, 2]
let true_forEach = newArr.forEach
newArr.forEach = replace

print(newArr.forEach(x => {})); //: Cannot get source code of funtion
newArr.forEach = true_forEach

//aot: [trace] Check Type: BuiltinInstanceHClassMismatch
print(newArr.forEach(x => true)); //: undefined

function testForeachEndHoleArray() {
    let y = [1,2,3,,,]
    y.forEach(x=>{
        print(x)
    })
}
//aot: [trace] Check Type: NotStableArray2
//: 1
//: 2
//: 3
testForeachEndHoleArray()

function forEachCase1() {
    print('case 1 forEach') //: case 1 forEach
    let arr1 = [1, 2]
    let arr2 = [1, 2]
    arr2.garbage = function(x: any): any {
        return undefined;
    }
    //aot: [trace] aot inline builtin: Array.prototype.foreach, caller function name:#*#forEachCase1@builtinArrayForEach
    print(arr1.forEach(x => x == 1)); //: undefined
    print(arr2.forEach(x => x == 1)); //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
                                      //: undefined
}
forEachCase1()


function forEachCase2() {
    print('case 2 forEach') //: case 2 forEach
    let arr1 = [1, 2]
    let arr2 = [1, 2]
    arr2.forEach = function(x: any) {
        return x
    }

    //aot: [trace] aot inline builtin: Object.getPrototypeOf, caller function name:#*#forEachCase2@builtinArrayForEach
    print(Object.getPrototypeOf(arr2) === Array.prototype) //: true

    //aot: [trace] aot inline builtin: Array.prototype.foreach, caller function name:#*#forEachCase2@builtinArrayForEach
    print(arr1.forEach(x => x == 1)); //: undefined
    print(arr2.forEach(x => x == 1)); //: Cannot get source code of funtion
}
forEachCase2()


function forEachCase3() {
    print('case 3 forEach') //: case 3 forEach
    let marr = [1, 2]
    let true_forEach = marr.forEach
    let mimicArray = {
        forEach: true_forEach,
    }

    //aot: [trace] aot inline builtin: Array.prototype.foreach, caller function name:#*#forEachCase3@builtinArrayForEach
    print(marr.forEach(x => x == 1)); //: undefined
    Object.setPrototypeOf(marr, mimicArray)

    print(marr.forEach(x => x == 1)); //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
                                      //: undefined
}
forEachCase3()


function forEachCase4() {
    print('case 4 forEach') //: case 4 forEach
    let arr1 = [1, 2]
    let arr2 = [1, 2]
    let notArray = {
        forEach(x: any) {
            return x(0)
        }
    }
    Object.setPrototypeOf(arr2, notArray)

    //aot: [trace] Check Type: NotStableArray1
    print(arr1.forEach(x => x == 1)); //: undefined
    //aot: [trace] aot inline function name: #*@4*#^1@builtinArrayForEach caller function name: #*@4*#forEach@builtinArrayForEach
    print(arr2.forEach(x => x == 1)); //: false
}
forEachCase4()


function forEachCase5() {
    print('case 5 forEach') //: case 5 forEach
    let arr1 = [1, 2]
    Array.prototype.foreach = function(x: any) {
        return x(1)
    }

    //aot: [trace] Check Type: BuiltinPrototypeHClassMismatch1
    print(arr1.forEach(x => x == 1)); //: undefined
}
forEachCase5()
