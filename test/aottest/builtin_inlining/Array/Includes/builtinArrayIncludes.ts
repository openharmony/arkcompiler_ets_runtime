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
//int array
let literalIntArrayWithHole = [0,,2,,4,,]
let literalIntArrayNoHole = [0,1,2,3,4,5,6]
let nIntArray = new Array(6)
nIntArray[0] = 0
nIntArray[2] = 2
nIntArray[4] = 4
function returnDoubleTypeIntNotConstant(x){
  if (x>0){
    return 3.5+0.5
  } else {
    return 1.5+0.5
  }
}
//double array
let literalDoubleArrayWithHole = [0.5,,2.5,,4.5,,NaN,,]
function returnNotConstantDouble(x){
  if (x>0){
    return 4+0.5
  } else {
    return 2+0.5
  }
}
let nDoubleArray = new Array(7)
nDoubleArray[1] = 1.5
nDoubleArray[4] = 4.5
nDoubleArray[6] = NaN
//string array
let literalStringArrayWithHole = ["string1",,"string2",,"string4",,]
let nStringArray = new Array(5)
nStringArray[1] = "1"
nStringArray[4] = "4"
function returnNotLitaralString(x){
  if (x>0){
    return "string" + "4"
  } else {
    return "string4"
  }
}
//object array
let find1 = {1:1}
class findClass{
  x;
  constructor(x){
    this.x = x
  }
}
let find3 = new findClass(3)
let find5 = new Date()
let objArrayWithHoleNeverFind = [{0:0},,{2:2},,{4:4},,]
let objnewArraywithHoleNeverFind = new Array(7)
objnewArraywithHoleNeverFind[0] = {0:0}
objnewArraywithHoleNeverFind[2] = {2:2}
objnewArraywithHoleNeverFind[4] = {4:4}

let objArrayWithHoleCanFind = [,find1,,find3,,find5,]
let objnewArraywithHoleCanFind = new Array(7)
objnewArraywithHoleCanFind[1] = find1
objnewArraywithHoleCanFind[3] = find3
objnewArraywithHoleCanFind[5] = find5
//====================start nomarl kind test=================//
//includes int
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(literalIntArrayWithHole.includes(4)) //: true
//aot: [trace] aot inline function name: #*#returnDoubleTypeIntNotConstant@builtinArrayIncludes caller function name: func_main_0@builtinArrayIncludes
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(literalIntArrayWithHole.includes(returnDoubleTypeIntNotConstant(1))) //: true
//aot: [trace] aot inline function name: #*#returnDoubleTypeIntNotConstant@builtinArrayIncludes caller function name: func_main_0@builtinArrayIncludes
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(literalIntArrayWithHole.includes(returnDoubleTypeIntNotConstant(0))) //: true
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(nIntArray.includes(4)) //: true
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(nIntArray.includes(undefined)) //: true
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(literalIntArrayWithHole.includes(undefined)) //: true
//nohole hole
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(literalIntArrayNoHole.includes(4)) //: true
//aot: [trace] aot inline function name: #*#returnDoubleTypeIntNotConstant@builtinArrayIncludes caller function name: func_main_0@builtinArrayIncludes
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(literalIntArrayNoHole.includes(returnDoubleTypeIntNotConstant(1))) //: true
//aot: [trace] aot inline function name: #*#returnDoubleTypeIntNotConstant@builtinArrayIncludes caller function name: func_main_0@builtinArrayIncludes
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(literalIntArrayNoHole.includes(returnDoubleTypeIntNotConstant(0))) //: true
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(literalIntArrayNoHole.includes(undefined)) //: false
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(literalIntArrayNoHole.includes(NaN)) //: false
//includes double
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(literalDoubleArrayWithHole.includes(4.5)) //: true
//aot: [trace] aot inline function name: #*#returnNotConstantDouble@builtinArrayIncludes caller function name: func_main_0@builtinArrayIncludes
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(literalDoubleArrayWithHole.includes(returnNotConstantDouble(1))) //: true
//aot: [trace] aot inline function name: #*#returnNotConstantDouble@builtinArrayIncludes caller function name: func_main_0@builtinArrayIncludes
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(literalDoubleArrayWithHole.includes(returnNotConstantDouble(0))) //: true
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(nDoubleArray.includes(4.5)) //: true
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(nDoubleArray.includes(NaN)) //: true
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(literalDoubleArrayWithHole.includes(undefined)) //: true
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(literalIntArrayNoHole.includes(literalIntArrayNoHole.length)) //: false

//includes string
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(literalStringArrayWithHole.includes("string4")) //: true
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(literalStringArrayWithHole.includes(returnNotLitaralString(1))) //: true
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(literalStringArrayWithHole.includes(returnNotLitaralString(0))) //: true
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(nStringArray.includes("4")) //: true
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(nStringArray.includes(undefined)) //: true

//neverequal
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(objArrayWithHoleNeverFind.includes({4:4})) //: false
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(objnewArraywithHoleNeverFind.includes({4:4})) //: false
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(objnewArraywithHoleNeverFind.includes(undefined)) //: true
//can find
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(objArrayWithHoleCanFind.includes(find1)) //: true
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(objArrayWithHoleCanFind.includes(find3)) //: true
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(objArrayWithHoleCanFind.includes(find5)) //: true
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(objnewArraywithHoleCanFind.includes(find1)) //: true
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(objnewArraywithHoleCanFind.includes(find3)) //: true
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(objnewArraywithHoleCanFind.includes(find5)) //: true

//============special test
//aot: [trace] aot inline builtin: BigInt, caller function name:func_main_0@builtinArrayIncludes
let specialArray = [null, , false, true, undefined, +0, -0, BigInt(123456), NaN, 5, 5.5]
//includes use samevaluezero
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(specialArray.includes(NaN)) //: true
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(specialArray.includes(undefined, 3)) //: true
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(specialArray.includes(undefined)) //: true
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(specialArray.includes(NaN)) //: true
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(specialArray.includes(+0)) //: true
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(specialArray.includes(-0)) //: true
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(specialArray.includes(false)) //: true
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(specialArray.includes(true)) //: true
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(specialArray.includes(null)) //: true
//aot: [trace] aot inline builtin: BigInt, caller function name:func_main_0@builtinArrayIncludes
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(specialArray.includes(BigInt(123456))) //: true
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(specialArray.includes(5)) //: true
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(specialArray.includes(5.5)) //: true

print('unusual cases') //: unusual cases
//aot: [trace] aot call builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(specialArray.includes()) //: true
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(specialArray.includes(() => {})) //: false
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(specialArray.includes(NaN, 0)) //: true
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(specialArray.includes(NaN, 2000000)) //: false
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(specialArray.includes(NaN, -2000000)) //: true
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(specialArray.includes(NaN, -1)) //: false
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(specialArray.includes(5, 5, 78)) //: true

function notIntIndex() {
  let specialArray = [1, 2, NaN]
  //aot: [trace] Check Type: IndexNotInt
  print(specialArray.includes(false, "str")) //: false
}
notIntIndex()

// Check inside try-block
try {
  //aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
  print(specialArray.includes(null)) //: true
} catch(e) { 
}

let obj = {};
obj.valueOf = (() => { return 5; })
//aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
print(specialArray.includes(obj)); //: false

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
    //aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
    print(specialArray.includes(throwingObj)); //: false
} catch(e) {
    print(e);
} finally {
    //aot: [trace] aot inline builtin: Array.prototype.includes, caller function name:func_main_0@builtinArrayIncludes
    print(specialArray.includes(obj)); //: false
}

//===========deopt type
function prototypeChange(){
  let tArray = [1,,3]
  Array.prototype[1] = 2
  print(tArray.includes(2))
}
//aot: [trace] Check Type: NotStableArray1
prototypeChange() //: true
function lengthChange(){
    let tArray = [1,,3]
    tArray.length = 2
    print(tArray.includes(3))
}
//aot: [trace] Check Type: NotStableArray1
lengthChange() //: false


// Replace standard builtin
function replace(a : any) {
  return a;
}

let newArr = [1, 2, NaN]
let true_includes = newArr.includes
newArr.includes = replace

print(newArr.includes(undefined)); //: undefined
newArr.includes = true_includes
print(newArr.includes(undefined)); //: false


function doIncludes(x: any): any {
  return newArr.includes(x);
}

function printIncludes(x: any) {
  try {
      print(doIncludes(x));
  } finally {
  }
}

if (ArkTools.isAOTCompiled(printIncludes)) {
  // Replace standard builtin after call to standard builtin was profiled
  newArr.includes = replace
}
printIncludes(2.5); //pgo: false
//aot: [trace] Check Type: NotCallTarget1
//aot: 2.5

printIncludes("abc"); //pgo: false
//aot: [trace] Check Type: NotCallTarget1
//aot: abc

newArr.includes = true_includes

function includesCase1() {
  print('case 1 includes') //: case 1 includes
  let arr1 = [1, 2]
  let arr2 = [1, 2]
  arr2.garbage = function(x: any): any {
      return undefined;
  }
  //aot: [trace] Check Type: NotStableArray1
  print(arr1.includes(1)); //: true
  print(arr2.includes(2)); //: true
}
includesCase1()


function includesCase2() {
  print('case 2 includes') //: case 2 includes
  let arr1 = [1, 2]
  let arr2 = [1, 2]
  arr2.includes = function(x: any) {
      return x
  }

  //aot: [trace] aot inline builtin: Object.getPrototypeOf, caller function name:#*#includesCase2@builtinArrayIncludes
  print(Object.getPrototypeOf(arr2) === Array.prototype) //: true

  //aot: [trace] Check Type: NotStableArray1
  print(arr1.includes(1)); //: true
  print(arr2.includes(1)); //: 1
}
includesCase2()


function includesCase3() {
  print('case 3 includes') //: case 3 includes
  let marr = [1, 2]
  let true_includes = marr.includes
  let mimicArray = {
      includes: true_includes,
  }

  //aot: [trace] Check Type: NotStableArray1
  print(marr.includes(500)); //: false
  Object.setPrototypeOf(marr, mimicArray)

  print(marr.includes(500)); //: false
}
includesCase3()


function includesCase4() {
  print('case 4 includes') //: case 4 includes
  let arr1 = [1, 2]
  let arr2 = [1, 2]
  let notArray = {
      includes(x: any) {
          return -100
      }
  }
  //aot: [trace] aot call builtin: Object.SetPrototypeOf, caller function name:#*#includesCase4@builtinArrayIncludes
  Object.setPrototypeOf(arr2, notArray)

  //aot: [trace] Check Type: NotStableArray1
  print(arr1.includes(1)); //: true
  print(arr2.includes(1)); //: -100
}
includesCase4()


function includesCase5() {
  print('case 5 includes') //: case 5 includes
  let arr1 = [1, 2]
  Array.prototype.includes = function(x: any) {
      return x
  }

  //aot: [trace] Check Type: NotStableArray1
  print(arr1.includes(1)); //: 1
}
includesCase5()
