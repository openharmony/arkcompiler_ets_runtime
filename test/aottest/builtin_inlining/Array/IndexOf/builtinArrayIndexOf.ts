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
let literalDoubleArrayWithHole = [0.5,,2.5,,4.5,,]
function returnNotConstantDouble(x){
  if (x>0){
    return 4+0.5
  } else {
    return 2+0.5
  }
}
let nDoubleArray = new Array(5)
nDoubleArray[1] = 1.5
nDoubleArray[4] = 4.5
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
//indexOf int
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(literalIntArrayWithHole.indexOf(4)) //: 4
//aot: [trace] aot inline function name: #*#returnDoubleTypeIntNotConstant@builtinArrayIndexOf caller function name: func_main_0@builtinArrayIndexOf
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(literalIntArrayWithHole.indexOf(returnDoubleTypeIntNotConstant(1))) //: 4
//aot: [trace] aot inline function name: #*#returnDoubleTypeIntNotConstant@builtinArrayIndexOf caller function name: func_main_0@builtinArrayIndexOf
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(literalIntArrayWithHole.indexOf(returnDoubleTypeIntNotConstant(0))) //: 2
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(nIntArray.indexOf(4)) //: 4
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(literalIntArrayWithHole.indexOf(undefined)) //: -1
//indexOf double
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(literalDoubleArrayWithHole.indexOf(4.5)) //: 4
//aot: [trace] aot inline function name: #*#returnNotConstantDouble@builtinArrayIndexOf caller function name: func_main_0@builtinArrayIndexOf
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(literalDoubleArrayWithHole.indexOf(returnNotConstantDouble(1))) //: 4
//aot: [trace] aot inline function name: #*#returnNotConstantDouble@builtinArrayIndexOf caller function name: func_main_0@builtinArrayIndexOf
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(literalDoubleArrayWithHole.indexOf(returnNotConstantDouble(0))) //: 2
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(nDoubleArray.indexOf(4.5)) //: 4
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(literalDoubleArrayWithHole.indexOf(undefined)) //: -1
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(nIntArray.indexOf(nIntArray.length)) //: -1

//indexOf string
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(literalStringArrayWithHole.indexOf("string4")) //: 4
//aot: [trace] aot inline function name: #*#returnNotLitaralString@builtinArrayIndexOf caller function name: func_main_0@builtinArrayIndexOf
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(literalStringArrayWithHole.indexOf(returnNotLitaralString(1))) //: 4
//aot: [trace] aot inline function name: #*#returnNotLitaralString@builtinArrayIndexOf caller function name: func_main_0@builtinArrayIndexOf
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(literalStringArrayWithHole.indexOf(returnNotLitaralString(0))) //: 4
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(nStringArray.indexOf("4")) //: 4
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(nStringArray.indexOf(undefined)) //: -1

//indexof obj
//neverequal
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(objArrayWithHoleNeverFind.indexOf({4:4})) //: -1
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(objnewArraywithHoleNeverFind.indexOf({4:4})) //: -1
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(objnewArraywithHoleNeverFind.indexOf(undefined)) //: -1
//can find
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(objArrayWithHoleCanFind.indexOf(find1)) //: 1
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(objArrayWithHoleCanFind.indexOf(find3)) //: 3
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(objArrayWithHoleCanFind.indexOf(find5)) //: 5
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(objnewArraywithHoleCanFind.indexOf(find1)) //: 1
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(objnewArraywithHoleCanFind.indexOf(find3)) //: 3
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(objnewArraywithHoleCanFind.indexOf(find5)) //: 5

//============special test
//aot: [trace] aot inline builtin: BigInt, caller function name:func_main_0@builtinArrayIndexOf
let specialArray = [null, , false, true, undefined, +0, -0, BigInt(123456), NaN, 5, 5.5]
//indexOf use strict equal
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(specialArray.indexOf(NaN)) //: -1
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(specialArray.indexOf(undefined, 3)) //: 4
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(specialArray.indexOf(undefined)) //: 4
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(specialArray.indexOf(NaN)) //: -1
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(specialArray.indexOf(+0)) //: 5
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(specialArray.indexOf(-0)) //: 5
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(specialArray.indexOf(false)) //: 2
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(specialArray.indexOf(true)) //: 3
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(specialArray.indexOf(null)) //: 0
//aot: [trace] aot inline builtin: BigInt, caller function name:func_main_0@builtinArrayIndexOf
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(specialArray.indexOf(BigInt(123456))) //: 7
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(specialArray.indexOf(5)) //: 9
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(specialArray.indexOf(5.5)) //: 10

print('unusual cases') //: unusual cases
print(specialArray.indexOf()) //: 4
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(specialArray.indexOf(() => {})) //: -1
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(specialArray.indexOf(true, 2000000)) //: -1
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(specialArray.indexOf(true, -2000000)) //: 3
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(specialArray.indexOf(true, -1)) //: -1
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(specialArray.indexOf(5, 5, 78)) //: 9

function notIntIndex() {
  let specialArray = [1, 2, NaN]
  //aot: [trace] Check Type: IndexNotInt
  print(specialArray.indexOf(false, "str")) //: -1
}
notIntIndex()

// Check inside try-block
try {
  //aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
  print(specialArray.indexOf(null)) //: 0
} catch(e) { 
}

let obj = {};
obj.valueOf = (() => { return 5; })
//aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
print(specialArray.indexOf(obj)); //: -1

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
    //aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
    print(specialArray.indexOf(throwingObj)); //: -1
} catch(e) {
    print(e);
} finally {
    //aot: [trace] aot inline builtin: Array.prototype.indexOf, caller function name:func_main_0@builtinArrayIndexOf
    print(specialArray.indexOf(obj)); //: -1
}

//===========deopt type
function prototypeChange(){
  let tArray = [1,,3]
  Array.prototype[1] = 2
  print(tArray.indexOf(2))
}
//aot: [trace] Check Type: NotStableArray1
prototypeChange() //: 1
function lengthChange(){
    let tArray = [1,,3]
    tArray.length = 2
    print(tArray.indexOf(3))
}
//aot: [trace] Check Type: NotStableArray1
lengthChange() //: -1


// Replace standard builtin
function replace(a : any) {
  return a;
}

let newArr = [1, 2, NaN]
let true_indexOf = newArr.indexOf
newArr.indexOf = replace

print(newArr.indexOf(undefined)); //: undefined
newArr.indexOf = true_indexOf
//aot: [trace] Check Type: BuiltinInstanceHClassMismatch
print(newArr.indexOf(undefined)); //: -1


function doIndexOf(x: any): any {
  return newArr.indexOf(x);
}

function printIndexOf(x: any) {
  try {
      print(doIndexOf(x));
  } finally {
  }
}

if (ArkTools.isAOTCompiled(printIndexOf)) {
  // Replace standard builtin after call to standard builtin was profiled
  newArr.indexOf = replace
}
printIndexOf(2); //pgo: 1
//aot: [trace] Check Type: NotCallTarget1
//aot: 2

printIndexOf("abc"); //pgo: -1
//aot: [trace] Check Type: NotCallTarget1
//aot: abc

newArr.indexOf = true_indexOf

function indexOfCase1() {
  print('case 1 indexOf') //: case 1 indexOf
  let arr1 = [1, 2]
  let arr2 = [1, 2]
  arr2.garbage = function(x: any): any {
      return undefined;
  }
  //aot: [trace] Check Type: NotStableArray1
  print(arr1.indexOf(1)); //: 0
  print(arr2.indexOf(2)); //: 1
}
indexOfCase1()


function indexOfCase2() {
  print('case 2 indexOf') //: case 2 indexOf
  let arr1 = [1, 2]
  let arr2 = [1, 2]
  arr2.indexOf = function(x: any) {
      return x
  }

  //aot: [trace] aot inline builtin: Object.getPrototypeOf, caller function name:#*#indexOfCase2@builtinArrayIndexOf
  print(Object.getPrototypeOf(arr2) === Array.prototype) //: true

  //aot: [trace] Check Type: NotStableArray1
  print(arr1.indexOf(1)); //: 0
  print(arr2.indexOf(1)); //: 1
}
indexOfCase2()


function indexOfCase3() {
  print('case 3 indexOf') //: case 3 indexOf
  let marr = [1, 2]
  let true_indexOf = marr.indexOf
  let mimicArray = {
      indexOf: true_indexOf,
  }

  //aot: [trace] Check Type: NotStableArray1
  print(marr.indexOf(500)); //: -1
  Object.setPrototypeOf(marr, mimicArray)

  print(marr.indexOf(500)); //: -1
}
indexOfCase3()


function indexOfCase4() {
  print('case 4 indexOf') //: case 4 indexOf
  let arr1 = [1, 2]
  let arr2 = [1, 2]
  let notArray = {
      indexOf(x: any) {
          return -100
      }
  }
  Object.setPrototypeOf(arr2, notArray)

  //aot: [trace] Check Type: NotStableArray1
  print(arr1.indexOf(1)); //: 0
  print(arr2.indexOf(1)); //: -100
}
indexOfCase4()


function indexOfCase5() {
  print('case 5 indexOf') //: case 5 indexOf
  let arr1 = [1, 2]
  Array.prototype.indexOf = function(x: any) {
      return x
  }

  //aot: [trace] Check Type: NotStableArray1
  print(arr1.indexOf(1)); //: 1
}
indexOfCase5()
