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
var x = [1,2.5,NaN,undefined,null,false,true,"ark"]
//aot: [trace] aot inline builtin: Array.prototype.find, caller function name:func_main_0@builtinArrayFindFindIndex
var inlineFind = x.find(x=>{
  return x === "ark"
})
//aot: [trace] aot inline builtin: Array.prototype.find, caller function name:func_main_0@builtinArrayFindFindIndex
var inlineNotFind = x.find(x=>{
  return x === "a_rk"
})
//aot: [trace] aot inline builtin: Array.prototype.findIndex, caller function name:func_main_0@builtinArrayFindFindIndex
var inlineFindIndex = x.findIndex(x=>{
  return x === "ark"
})
//aot: [trace] aot inline builtin: Array.prototype.findIndex, caller function name:func_main_0@builtinArrayFindFindIndex
var inlineNotFindIndex = x.findIndex(x=>{
  return x === "a_rk"
})

print(inlineFind) //: ark
print(inlineNotFind) //: undefined
print(inlineFindIndex) //: 7
print(inlineNotFindIndex) //: -1

//aot: [trace] aot inline builtin: Array.prototype.find, caller function name:func_main_0@builtinArrayFindFindIndex
var inlineFindNumber = x.find(x=>{
  return x == "1"
})
//aot: [trace] aot inline builtin: Array.prototype.find, caller function name:func_main_0@builtinArrayFindFindIndex
var inlineNotFindNumber = x.find(x=>{
  return x === "1"
})
//aot: [trace] aot inline builtin: Array.prototype.findIndex, caller function name:func_main_0@builtinArrayFindFindIndex
var inlineFindNumberIndex = x.findIndex(x=>{
  return x > 1
})
//aot: [trace] aot inline builtin: Array.prototype.findIndex, caller function name:func_main_0@builtinArrayFindFindIndex
var inlineNotFindNumberIndex = x.findIndex(x=>{
  return x < false
})

print(inlineFindNumber) //: 1
print(inlineNotFindNumber) //: undefined
print(inlineFindNumberIndex) //: 1
print(inlineNotFindNumberIndex) //: -1

// Check inside try-block
try {
  //aot: [trace] aot inline builtin: Array.prototype.find, caller function name:func_main_0@builtinArrayFindFindIndex
  print(x.find(x => true)) //: 1
  //aot: [trace] aot inline builtin: Array.prototype.findIndex, caller function name:func_main_0@builtinArrayFindFindIndex
  print(x.findIndex(x => true)) //: 0
} catch(e) {
}

// Check without args
try {
  //aot: [trace] aot call builtin: Array.prototype.find, caller function name:func_main_0@builtinArrayFindFindIndex
  print(x.find())
} catch(e) {
  print(e) //: TypeError: the predicate is not callable.
}
try {
  //aot: [trace] aot call builtin: Array.prototype.findIndex, caller function name:func_main_0@builtinArrayFindFindIndex
  print(x.findIndex())
} catch(e) {
  print(e) //: TypeError: the predicate is not callable.
}

//aot: [trace] aot inline builtin: Array.prototype.find, caller function name:func_main_0@builtinArrayFindFindIndex
print(x.find(() => {})) //: undefined
//aot: [trace] aot inline builtin: Array.prototype.find, caller function name:func_main_0@builtinArrayFindFindIndex
print(x.find(x => true)) //: 1
//aot: [trace] aot inline builtin: Array.prototype.find, caller function name:func_main_0@builtinArrayFindFindIndex
print(x.find(x => false)) //: undefined
//aot: [trace] aot inline builtin: Array.prototype.find, caller function name:func_main_0@builtinArrayFindFindIndex
print(x.find((x,y) => {})) //: undefined
//aot: [trace] aot inline builtin: Array.prototype.find, caller function name:func_main_0@builtinArrayFindFindIndex
print(x.find((x,y) => x == true)) //: 1
//aot: [trace] aot inline builtin: Array.prototype.find, caller function name:func_main_0@builtinArrayFindFindIndex
print(x.find(x => x == true, 0)) //: 1

//aot: [trace] aot inline builtin: Array.prototype.findIndex, caller function name:func_main_0@builtinArrayFindFindIndex
print(x.findIndex(() => {})) //: -1
//aot: [trace] aot inline builtin: Array.prototype.findIndex, caller function name:func_main_0@builtinArrayFindFindIndex
print(x.findIndex(x => true)) //: 0
//aot: [trace] aot inline builtin: Array.prototype.findIndex, caller function name:func_main_0@builtinArrayFindFindIndex
print(x.findIndex(x => false)) //: -1
//aot: [trace] aot inline builtin: Array.prototype.findIndex, caller function name:func_main_0@builtinArrayFindFindIndex
print(x.findIndex((x,y) => {})) //: -1
//aot: [trace] aot inline builtin: Array.prototype.findIndex, caller function name:func_main_0@builtinArrayFindFindIndex
print(x.findIndex((x,y) => x == true)) //: 0
//aot: [trace] aot inline builtin: Array.prototype.findIndex, caller function name:func_main_0@builtinArrayFindFindIndex
print(x.findIndex(x => x == true, 0)) //: 0


// Replace standard builtin
function replace(a : any) {
  return a;
}

let newArr = [1, 2]
let true_find = newArr.find
let true_findIndex = newArr.findIndex
newArr.find = replace
newArr.findIndex = replace

print(newArr.find(x => {})); //: Cannot get source code of funtion
newArr.find = true_find
print(newArr.findIndex(x => { return x == 11 })); //: Cannot get source code of funtion
newArr.findIndex = true_findIndex

//aot: [trace] Check Type: BuiltinInstanceHClassMismatch
print(newArr.find(x => true)); //: 1
print(newArr.findIndex(x => true)); //: 0

function findCase1() {
  print('case 1 find') //: case 1 find
  let arr1 = [1, 2]
  let arr2 = [1, 2]
  arr2.garbage = function(x: any): any {
    return undefined;
  }
  //aot: [trace] aot inline builtin: Array.prototype.find, caller function name:#*#findCase1@builtinArrayFindFindIndex
  print(arr1.find(x => x == 1)); //: 1
  print(arr2.find(x => x == 1)); //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
                                 //: 1
}
function findIndexCase1() {
  print('case 1 findIndex') //: case 1 findIndex
  let arr1 = [1, 2]
  let arr2 = [1, 2]
  arr2.garbage = function(x: any): any {
    return undefined;
  }
  //aot: [trace] aot inline builtin: Array.prototype.findIndex, caller function name:#*#findIndexCase1@builtinArrayFindFindIndex
  print(arr1.findIndex(x => x == 1)); //: 0
  print(arr2.findIndex(x => x == 1)); //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
                                      //: 0
}
findCase1()
findIndexCase1()


function findCase2() {
  print('case 2 find') //: case 2 find
  let arr1 = [1, 2]
  let arr2 = [1, 2]
  arr2.find = function(x: any) {
    return x
  }

  //aot: [trace] aot inline builtin: Object.getPrototypeOf, caller function name:#*#findCase2@builtinArrayFindFindIndex
  print(Object.getPrototypeOf(arr2) === Array.prototype) //: true

  //aot: [trace] aot inline builtin: Array.prototype.find, caller function name:#*#findCase2@builtinArrayFindFindIndex
  print(arr1.find(x => x == 1)); //: 1
  print(arr2.find(x => x == 1)); //: Cannot get source code of funtion
}
function findIndexCase2() {
  print('case 2 findIndex') //: case 2 findIndex
  let arr1 = [1, 2]
  let arr2 = [1, 2]

  //aot: [trace] aot inline builtin: Object.getPrototypeOf, caller function name:#*#findIndexCase2@builtinArrayFindFindIndex
  print(Object.getPrototypeOf(arr2) === Array.prototype) //: true
  arr2.findIndex = function(x: any) {
    return x
  }

  //aot: [trace] aot inline builtin: Array.prototype.findIndex, caller function name:#*#findIndexCase2@builtinArrayFindFindIndex
  print(arr1.findIndex(x => x == 1)); //: 0
  print(arr2.findIndex(x => x == 1)); //: Cannot get source code of funtion
}
findCase2()
findIndexCase2()


function findCase3() {
  print('case 3 find') //: case 3 find
  let marr = [1, 2]
  let true_find = marr.find
  let mimicArray = {
    find: true_find,
  }

  //aot: [trace] aot inline builtin: Array.prototype.find, caller function name:#*#findCase3@builtinArrayFindFindIndex
  print(marr.find(x => x == 1)); //: 1
  //aot: [trace] aot call builtin: Object.SetPrototypeOf, caller function name:#*#findCase3@builtinArrayFindFindIndex
  Object.setPrototypeOf(marr, mimicArray)

  print(marr.find(x => x == 1)); //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
                                 //: 1
}
function findIndexCase3() {
  print('case 3 findIndex') //: case 3 findIndex

  let marr = [1, 2]
  let true_findIndex = marr.findIndex

  let mimicArray = {
    findIndex: true_findIndex
  }

  print(marr.findIndex(x => x == 1)); //aot: [trace] aot inline builtin: Array.prototype.findIndex, caller function name:#*#findIndexCase3@builtinArrayFindFindIndex
                                      //: 0
  //aot: [trace] aot call builtin: Object.SetPrototypeOf, caller function name:#*#findIndexCase3@builtinArrayFindFindIndex
  Object.setPrototypeOf(marr, mimicArray)

  //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
  print(marr.findIndex(x => x == 1)); //: 0
}
findCase3()
findIndexCase3()


function findCase4() {
  print('case 4 find') //: case 4 find
  let arr1 = [1, 2]
  let arr2 = [1, 2]
  let notArray = {
    find(x: any) {
        return x(0)
    }
  }
  //aot: [trace] aot call builtin: Object.SetPrototypeOf, caller function name:#*#findCase4@builtinArrayFindFindIndex
  Object.setPrototypeOf(arr2, notArray)
  //aot: [trace] aot inline builtin: Array.prototype.find, caller function name:#*#findCase4@builtinArrayFindFindIndex
  print(arr1.find(x => x == 1)); //: 1
  //aot: [trace] aot inline function name: #*@6*#find@builtinArrayFindFindIndex caller function name: #*#findCase4@builtinArrayFindFindIndex
  //aot: [trace] aot inline function name: #*@6*#^1@builtinArrayFindFindIndex caller function name: #*@6*#find@builtinArrayFindFindIndex
  print(arr2.find(x => x == 1)); //: false
}
function findIndexCase4() {
  print('case 4 findIndex') //: case 4 findIndex
  let arr1 = [1, 2]
  let arr2 = [1, 2]
  let notArray = {
    findIndex(x: any) {
      return x(0)
    }
  }
  //aot: [trace] aot call builtin: Object.SetPrototypeOf, caller function name:#*#findIndexCase4@builtinArrayFindFindIndex
  Object.setPrototypeOf(arr2, notArray)

  //aot: [trace] aot inline builtin: Array.prototype.findIndex, caller function name:#*#findIndexCase4@builtinArrayFindFindIndex
  print(arr1.findIndex(x => x == 1)); //: 0
  //aot: [trace] aot inline function name: #*@7*#findIndex@builtinArrayFindFindIndex caller function name: #*#findIndexCase4@builtinArrayFindFindIndex
  //aot: [trace] aot inline function name: #*@7*#^1@builtinArrayFindFindIndex caller function name: #*@7*#findIndex@builtinArrayFindFindIndex
  print(arr2.findIndex(x => x == 1)); //: false
}
findCase4()
findIndexCase4()


function findCase5() {
  print('case 5 find') //: case 5 find
  let arr1 = [1, 2]
  Array.prototype.find = function(x: any) {
    return x(1)
  }

  //aot: [trace] aot inline function name: #*@8*#@builtinArrayFindFindIndex caller function name: #*#findCase5@builtinArrayFindFindIndex
  //aot: [trace] aot inline function name: #*@8*#^1@builtinArrayFindFindIndex caller function name: #*@8*#@builtinArrayFindFindIndex
  print(arr1.find(x => x == 1)); //: true
}
function findIndexCase5() {
  print('case 5 findIndex') //: case 5 findIndex
  let arr1 = [1, 2]
  Array.prototype.findIndex = function(x: any) {
    return x(1)
  }

  //aot: [trace] aot inline function name: #*@9*#@builtinArrayFindFindIndex caller function name: #*#findIndexCase5@builtinArrayFindFindIndex
  //aot: [trace] aot inline function name: #*@9*#^1@builtinArrayFindFindIndex caller function name: #*@9*#@builtinArrayFindFindIndex
  print(arr1.findIndex(x => x == 1));  //: true
}
findCase5()
findIndexCase5()