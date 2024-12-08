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
//aot: [trace] aot inline builtin: Array.prototype.filter, caller function name:func_main_0@builtinArrayFilter
var xFilterArrow = x.filter(ele=>{
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
//: 2.5,NaN,,,false
print(xFilterArrow)
//aot: [trace] aot inline builtin: Array.prototype.filter, caller function name:func_main_0@builtinArrayFilter
var xFilterFunc = x.filter(foo)
//: 1
//: 2.5
//: NaN
//: undefined
//: null
//: false
//: true
//: 2.5,NaN,,,false
print(xFilterFunc)

// Check without args
try {
    //aot: [trace] aot call builtin: Array.prototype.filter, caller function name:func_main_0@builtinArrayFilter
    print(x.filter())
} catch(e) {
    print(e) //: TypeError: the callbackfun is not callable.
}

//aot: [trace] aot inline builtin: Array.prototype.filter, caller function name:func_main_0@builtinArrayFilter
print(x.filter(() => {}) == '') //: true
//aot: [trace] aot inline builtin: Array.prototype.filter, caller function name:func_main_0@builtinArrayFilter
print(x.filter(x => true)) //: 1,2.5,NaN,,,false,true
//aot: [trace] aot inline builtin: Array.prototype.filter, caller function name:func_main_0@builtinArrayFilter
print(x.filter(x => false) == '') //: true
//aot: [trace] aot inline builtin: Array.prototype.filter, caller function name:func_main_0@builtinArrayFilter
print(x.filter((x,y) => {}) == '') //: true
//aot: [trace] aot inline builtin: Array.prototype.filter, caller function name:func_main_0@builtinArrayFilter
print(x.filter((x,y) => x == true)) //: 1,true
//aot: [trace] aot inline builtin: Array.prototype.filter, caller function name:func_main_0@builtinArrayFilter
print(x.filter(x => x == true, 0)) //: 1,true

// Check inside try-block
try {
    //aot: [trace] aot inline builtin: Array.prototype.filter, caller function name:func_main_0@builtinArrayFilter
    print(x.filter(x => true)) //: 1,2.5,NaN,,,false,true
} catch(e) { 
}

// Replace standard builtin
function replace(a : any) {
    return a;
}

let newArr = [1, 2]
let true_filter = newArr.filter
newArr.filter = replace

print(newArr.filter(x => {})); //: Cannot get source code of funtion
newArr.filter = true_filter

//aot: [trace] Check Type: BuiltinInstanceHClassMismatch
print(newArr.filter(x => true)); //: 1,2


function filterCase1() {
    print('case 1 filter') //: case 1 filter
    let arr1 = [1, 2]
    let arr2 = [1, 2]
    arr2.garbage = function(x: any): any {
        return undefined;
    }
    //aot: [trace] aot inline builtin: Array.prototype.filter, caller function name:#*#filterCase1@builtinArrayFilter
    print(arr1.filter(x => x == 1)); //: 1
    print(arr2.filter(x => x == 1)); //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
                                     //: 1
}
filterCase1()


function filterCase2() {
    print('case 2 filter') //: case 2 filter
    let arr1 = [1, 2]
    let arr2 = [1, 2]
    arr2.filter = function(x: any) {
        return x
    }

    //aot: [trace] aot inline builtin: Object.getPrototypeOf, caller function name:#*#filterCase2@builtinArrayFilter
    print(Object.getPrototypeOf(arr2) === Array.prototype) //: true

    //aot: [trace] aot inline builtin: Array.prototype.filter, caller function name:#*#filterCase2@builtinArrayFilter
    print(arr1.filter(x => x == 1)); //: 1
    print(arr2.filter(x => x == 1)); //: Cannot get source code of funtion
}
filterCase2()


function filterCase3() {
    print('case 3 filter') //: case 3 filter
    let marr = [1, 2]
    let true_filter = marr.filter
    let mimicArray = {
        filter: true_filter,
    }

    //aot: [trace] aot inline builtin: Array.prototype.filter, caller function name:#*#filterCase3@builtinArrayFilter
    print(marr.filter(x => x == 1)); //: 1
    //aot: [trace] aot call builtin: Object.SetPrototypeOf, caller function name:#*#filterCase3@builtinArrayFilter
    Object.setPrototypeOf(marr, mimicArray)

    print(marr.filter(x => x == 1)); //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
                                     //: 1
}
filterCase3()


function filterCase4() {
    print('case 4 filter') //: case 4 filter
    let arr1 = [1, 2]
    let arr2 = [1, 2]
    let notArray = {
        filter(x: any) {
            return x(0)
        }
    }
    //aot: [trace] aot call builtin: Object.SetPrototypeOf, caller function name:#*#filterCase4@builtinArrayFilter
    Object.setPrototypeOf(arr2, notArray)

    //aot: [trace] aot inline builtin: Array.prototype.filter, caller function name:#*#filterCase4@builtinArrayFilter
    print(arr1.filter(x => x == 1)); //: 1
    //aot: [trace] aot inline function name: #*@3*#filter@builtinArrayFilter caller function name: #*#filterCase4@builtinArrayFilter
    //aot: [trace] aot inline function name: #*@3*#^1@builtinArrayFilter caller function name: #*@3*#filter@builtinArrayFilter
    print(arr2.filter(x => x == 1)); //: false
}
filterCase4()


function filterCase5() {
    print('case 5 filter') //: case 5 filter
    let arr1 = [1, 2]
    Array.prototype.filter = function(x: any) {
        return x(1)
    }

    //aot: [trace] aot inline function name: #*@4*#@builtinArrayFilter caller function name: #*#filterCase5@builtinArrayFilter
    //aot: [trace] aot inline function name: #*@4*#^1@builtinArrayFilter caller function name: #*@4*#@builtinArrayFilter
    print(arr1.filter(x => x == 1)); //: true
}
filterCase5()