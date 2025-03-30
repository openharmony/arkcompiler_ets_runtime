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
var x = [1,2.1,-1,null,undefined,,false,NaN]
//aot: [trace] aot inline builtin: Array.prototype.pop, caller function name:func_main_0@builtinArrayPop
print(x.pop()) //: NaN
//aot: [trace] aot inline builtin: Array.prototype.pop, caller function name:func_main_0@builtinArrayPop
print(x.pop(undefined)) //: false
//aot: [trace] aot inline builtin: Array.prototype.pop, caller function name:func_main_0@builtinArrayPop
print(x.pop(1,2,3,4,5,6,7,8)) //: undefined
var y = new Array(10)
for (let i = 0; i < y.length; i++) {
    y[i] = i
}
//aot: [trace] aot inline builtin: Array.prototype.pop, caller function name:func_main_0@builtinArrayPop
print(y.pop()) //: 9
//aot: [trace] aot inline builtin: Array.prototype.pop, caller function name:func_main_0@builtinArrayPop
print(y.pop(undefined)) //: 8
//aot: [trace] aot inline builtin: Array.prototype.pop, caller function name:func_main_0@builtinArrayPop
print(y.pop(1,2,3,4,5,6,7,8)) //: 7

// Check inside try-block
try {
  //aot: [trace] aot inline builtin: Array.prototype.pop, caller function name:func_main_0@builtinArrayPop
  print(y.pop()) //: 6
} catch(e) { 
}

let obj = {};
obj.valueOf = (() => { return 5; })
//aot: [trace] aot inline builtin: Array.prototype.pop, caller function name:func_main_0@builtinArrayPop
print(y.pop(obj)); //: 5

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
    //aot: [trace] aot inline builtin: Array.prototype.pop, caller function name:func_main_0@builtinArrayPop
    print(y.pop(throwingObj)); //: 4
} catch(e) {
    print(e);
} finally {
    //aot: [trace] aot inline builtin: Array.prototype.pop, caller function name:func_main_0@builtinArrayPop
    print(y.pop(obj)); //: 3
}

// Replace standard builtin
function replace(a : any) {
  return a;
}

let newArr = [1, 2, NaN]
let true_pop = newArr.pop
newArr.pop = replace

print(newArr.pop(undefined)); //: undefined
newArr.pop = true_pop
//aot: [trace] Check Type: BuiltinInstanceHClassMismatch
print(newArr.pop(undefined)); //: NaN


function doPop(x: any): any {
  return newArr.pop(x);
}

function printPop(x: any) {
  try {
      print(doPop(x));
  } finally {
  }
}

if (ArkTools.isAOTCompiled(printPop)) {
  // Replace standard builtin after call to standard builtin was profiled
  newArr.pop = replace
}
printPop(-1); //pgo: 2
//aot: [trace] Check Type: NotCallTarget1
//aot: -1

printPop("abc"); //pgo: 1
//aot: [trace] Check Type: NotCallTarget1
//aot: abc

newArr.pop = true_pop

function popCase1() {
  print('case 1 pop') //: case 1 pop
  let arr1 = [1, 2]
  let arr2 = [1, 2]
  arr2.garbage = function(x: any): any {
      return undefined;
  }
  //aot: [trace] aot inline builtin: Array.prototype.pop, caller function name:#*#popCase1@builtinArrayPop
  print(arr1.pop()); //: 2
  print(arr2.pop()); //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
                     //: 2
}
popCase1()


function popCase2() {
  print('case 2 pop') //: case 2 pop
  let arr1 = [1, 2]
  let arr2 = [1, 2]
  arr2.pop = function(x: any) {
      return x
  }

  //aot: [trace] aot inline builtin: Object.getPrototypeOf, caller function name:#*#popCase2@builtinArrayPop
  print(Object.getPrototypeOf(arr2) === Array.prototype) //: true

  //aot: [trace] aot inline builtin: Array.prototype.pop, caller function name:#*#popCase2@builtinArrayPop
  print(arr1.pop(1)); //: 2
  print(arr2.pop(1)); //: 1
}
popCase2()


function popCase3() {
  print('case 3 pop') //: case 3 pop
  let marr = [1, 2]
  let true_pop = marr.pop
  let mimicArray = {
      pop: true_pop,
  }

  //aot: [trace] aot inline builtin: Array.prototype.pop, caller function name:#*#popCase3@builtinArrayPop
  print(marr.pop(500)); //: 2
  //aot: [trace] aot call builtin: Object.SetPrototypeOf, caller function name:#*#popCase3@builtinArrayPop
  Object.setPrototypeOf(marr, mimicArray)

  //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
  print(marr.pop()); //: 1
}
popCase3()


function popCase4() {
  print('case 4 pop') //: case 4 pop
  let arr1 = [1, 2]
  let arr2 = [1, 2]
  let notArray = {
      pop(x: any) {
          return -100
      }
  }
  //aot: [trace] aot call builtin: Object.SetPrototypeOf, caller function name:#*#popCase4@builtinArrayPop
  Object.setPrototypeOf(arr2, notArray)

  //aot: [trace] aot inline builtin: Array.prototype.pop, caller function name:#*#popCase4@builtinArrayPop
  print(arr1.pop(1)); //: 2
  //aot: [trace] aot inline function name: #*@3*#pop@builtinArrayPop caller function name: #*#popCase4@builtinArrayPop
  print(arr2.pop(1)); //: -100
}
popCase4()


function popCase5() {
  print('case 5 pop') //: case 5 pop
  let arr1 = [1, 2]
  Array.prototype.pop = function(x: any) {
      return x
  }

  print(arr1.pop(1)); //: 1
}
popCase5()
