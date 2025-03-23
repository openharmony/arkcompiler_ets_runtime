/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
  isAOTCompiled(args: any): boolean
}

//int array
let literalIntArrayWithHole = [0, , 2, , 4, ,]
let nIntArray = new Array(6)
nIntArray[0] = 0
nIntArray[2] = 2
nIntArray[4] = 4
function returnDoubleTypeIntNotConstant (x) {
  if (x > 0) {
    return 3.5 + 0.5
  } else {
    return 1.5 + 0.5
  }
}
//double array
let literalDoubleArrayWithHole = [0.5, , 2.5, , 4.5, ,]
function returnNotConstantDouble (x) {
  if (x > 0) {
    return 4 + 0.5
  } else {
    return 2 + 0.5
  }
}
let nDoubleArray = new Array(5)
nDoubleArray[1] = 1.5
nDoubleArray[4] = 4.5
//string array
let literalStringArrayWithHole = ['string1', , 'string2', , 'string4', ,]
let nStringArray = new Array(5)
nStringArray[1] = '1'
nStringArray[4] = '4'
function returnNotLitaralString (x) {
  if (x > 0) {
    return 'string' + '4'
  } else {
    return 'string4'
  }
}
//object array
let find1 = { 1: 1 }
class findClass {
  x
  constructor (x) {
    this.x = x
  }
}
let find3 = new findClass(3)
let find5 = new Date()
let objArrayWithHoleNeverFind = [{ 0: 0 }, , { 2: 2 }, , { 4: 4 }, ,]
let objnewArraywithHoleNeverFind = new Array(7)
objnewArraywithHoleNeverFind[0] = { 0: 0 }
objnewArraywithHoleNeverFind[2] = { 2: 2 }
objnewArraywithHoleNeverFind[4] = { 4: 4 }

let objArrayWithHoleCanFind = [, find1, , find3, , find5]
let objnewArraywithHoleCanFind = new Array(7)
objnewArraywithHoleCanFind[1] = find1
objnewArraywithHoleCanFind[3] = find3
objnewArraywithHoleCanFind[5] = find5

//====================start nomarl kind test=================//
//lastIndexOf int
print('---- Case group #1: int[] ----') //: ---- Case group #1: int[] ----
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 1.1: ', literalIntArrayWithHole.lastIndexOf(4)) //: case 1.1:  4
//aot: [trace] aot inline function name: #*#returnDoubleTypeIntNotConstant@builtinArrayLastIndexOf caller function name: func_main_0@builtinArrayLastIndexOf
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print(
  'case 1.2: ',
  literalIntArrayWithHole.lastIndexOf(returnDoubleTypeIntNotConstant(1))
) //: case 1.2:  4
//aot: [trace] aot inline function name: #*#returnDoubleTypeIntNotConstant@builtinArrayLastIndexOf caller function name: func_main_0@builtinArrayLastIndexOf
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print(
  'case 1.3: ',
  literalIntArrayWithHole.lastIndexOf(returnDoubleTypeIntNotConstant(0))
) //: case 1.3:  2
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 1.4: ', nIntArray.lastIndexOf(4)) //: case 1.4:  4
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 1.5: ', literalIntArrayWithHole.lastIndexOf(undefined)) //: case 1.5:  -1
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 1.6: ', nIntArray.lastIndexOf(nIntArray.length)) //: case 1.6:  -1

//lastIndexOf double
print('---- Case group #2: double[] ----') //: ---- Case group #2: double[] ----
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 2.1: ', literalDoubleArrayWithHole.lastIndexOf(4.5)) //: case 2.1:  4
//aot: [trace] aot inline function name: #*#returnNotConstantDouble@builtinArrayLastIndexOf caller function name: func_main_0@builtinArrayLastIndexOf
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print(
  'case 2.2: ',
  literalDoubleArrayWithHole.lastIndexOf(returnNotConstantDouble(1))
) //: case 2.2:  4
//aot: [trace] aot inline function name: #*#returnNotConstantDouble@builtinArrayLastIndexOf caller function name: func_main_0@builtinArrayLastIndexOf
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print(
  'case 2.3: ',
  literalDoubleArrayWithHole.lastIndexOf(returnNotConstantDouble(0))
) //: case 2.3:  2
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 2.4: ', nDoubleArray.lastIndexOf(4.5)) //: case 2.4:  4
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 2.5: ', literalDoubleArrayWithHole.lastIndexOf(undefined)) //: case 2.5:  -1

//lastIndexOf string
print('---- Case group #3: string[] ----') //: ---- Case group #3: string[] ----
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 3.1: ', literalStringArrayWithHole.lastIndexOf('string4')) //: case 3.1:  4
//aot: [trace] aot inline function name: #*#returnNotLitaralString@builtinArrayLastIndexOf caller function name: func_main_0@builtinArrayLastIndexOf
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print(
  'case 3.2: ',
  literalStringArrayWithHole.lastIndexOf(returnNotLitaralString(1))
) //: case 3.2:  4
//aot: [trace] aot inline function name: #*#returnNotLitaralString@builtinArrayLastIndexOf caller function name: func_main_0@builtinArrayLastIndexOf
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print(
  'case 3.3: ',
  literalStringArrayWithHole.lastIndexOf(returnNotLitaralString(0))
) //: case 3.3:  4
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 3.4: ', nStringArray.lastIndexOf('4')) //: case 3.4:  4
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 3.5: ', nStringArray.lastIndexOf(undefined)) //: case 3.5:  -1

//lastIndexOf obj
print('---- Case group #4: object[] ----') //: ---- Case group #4: object[] ----
//neverequal
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 4.1: ', objArrayWithHoleNeverFind.lastIndexOf({ 4: 4 })) //: case 4.1:  -1
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 4.2: ', objnewArraywithHoleNeverFind.lastIndexOf({ 4: 4 })) //: case 4.2:  -1
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 4.3: ', objnewArraywithHoleNeverFind.lastIndexOf(undefined)) //: case 4.3:  -1
//can find
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 4.4: ', objArrayWithHoleCanFind.lastIndexOf(find1)) //: case 4.4:  1
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 4.5: ', objArrayWithHoleCanFind.lastIndexOf(find3)) //: case 4.5:  3
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 4.6: ', objArrayWithHoleCanFind.lastIndexOf(find5)) //: case 4.6:  5
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 4.7: ', objnewArraywithHoleCanFind.lastIndexOf(find1)) //: case 4.7:  1
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 4.8: ', objnewArraywithHoleCanFind.lastIndexOf(find3)) //: case 4.8:  3
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 4.9: ', objnewArraywithHoleCanFind.lastIndexOf(find5)) //: case 4.9:  5

//============special test
print('---- Case group #5: special array ----') //: ---- Case group #5: special array ----
//aot: [trace] aot inline builtin: BigInt, caller function name:func_main_0@builtinArrayLastIndexOf
let specialArray = [
  null,
  ,
  false,
  true,
  undefined,
  +0,
  -0,
  BigInt(123456),
  NaN,
  5,
  5.5
]
//lastIndexOf use strict equal
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 5.1: ', specialArray.lastIndexOf(NaN)) //: case 5.1:  -1
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 5.2: ', specialArray.lastIndexOf(undefined, 3)) //: case 5.2:  -1
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 5.3: ', specialArray.lastIndexOf(undefined)) //: case 5.3:  4
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 5.4: ', specialArray.lastIndexOf(NaN)) //: case 5.4:  -1
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 5.5: ', specialArray.lastIndexOf(+0)) //: case 5.5:  6
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 5.6: ', specialArray.lastIndexOf(-0)) //: case 5.6:  6
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 5.7: ', specialArray.lastIndexOf(false)) //: case 5.7:  2
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 5.8: ', specialArray.lastIndexOf(true)) //: case 5.8:  3
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 5.9: ', specialArray.lastIndexOf(null)) //: case 5.9:  0
//aot: [trace] aot inline builtin: BigInt, caller function name:func_main_0@builtinArrayLastIndexOf
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 5.10: ', specialArray.lastIndexOf(BigInt(123456))) //: case 5.10:  7
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 5.11: ', specialArray.lastIndexOf(5)) //: case 5.11:  9
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 5.12: ', specialArray.lastIndexOf(5.5)) //: case 5.12:  10

print('---- Case group #6: unusual cases ----') //: ---- Case group #6: unusual cases ----
//aot: [trace] aot call builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 6.1: ', specialArray.lastIndexOf()) //: case 6.1:  4
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print(
  'case 6.2: ',
  specialArray.lastIndexOf(() => {})
) //: case 6.2:  -1
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 6.3: ', specialArray.lastIndexOf(true, 2000000)) //: case 6.3:  3
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 6.4: ', specialArray.lastIndexOf(true, -2000000)) //: case 6.4:  -1
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 6.5: ', specialArray.lastIndexOf(true, -1)) //: case 6.5:  3
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 6.6: ', specialArray.lastIndexOf(5, 5, 78)) //: case 6.6:  -1

print('---- Case group #7: not int index ----') //: ---- Case group #7: not int index ----
function notIntIndex () {
  let specialArray = [1, 2, NaN]
  //aot: [trace] Check Type: IndexNotInt
  print('case 7.1: ', specialArray.lastIndexOf(false, 'str'))
}
notIntIndex() //: case 7.1:  -1

// Check inside try-block
print('---- Case group #8: inside try-block ----') //: ---- Case group #8: inside try-block ----
try {
  //aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
  print('case 8.1: ', specialArray.lastIndexOf(null)) //: case 8.1:  0
} catch (e) {}

print('---- Case group #9: target with valueOf ----') //: ---- Case group #9: target with valueOf ----
let obj = {}
obj.valueOf = () => {
  return 5
}
//aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
print('case 9.1: ', specialArray.lastIndexOf(obj)) //: case 9.1:  -1

print('---- Case group #10: throwing ----') //: ---- Case group #10: throwing ----
function Throwing () {
  this.value = 2
  Throwing.prototype.valueOf = function () {
    if (this.value > 0) {
      throw new Error('positive')
    }
    return this.value
  }
}
let throwingObj = new Throwing()
try {
  //aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
  print('case 10.1: ', specialArray.lastIndexOf(throwingObj)) //: case 10.1:  -1
} catch (e) {
  print(e)
} finally {
  //aot: [trace] aot inline builtin: Array.LastIndexOf, caller function name:func_main_0@builtinArrayLastIndexOf
  print('case 10.2: ', specialArray.lastIndexOf(obj)) //: case 10.2:  -1
}

//===========deopt type
print('---- Case group #11: de-opt ----') //: ---- Case group #11: de-opt ----
//aot: [trace] Check Type: NotStableArray1
function prototypeChange () {
  let tArray = [1, , 3]
  Array.prototype[1] = 2
  print('case ProtoChange: ', tArray.lastIndexOf(2))
}
prototypeChange() //: case ProtoChange:  1

//aot: [trace] Check Type: NotStableArray1
function lengthChange () {
  let tArray = [1, , 3]
  tArray.length = 2
  print('case LengthChange: ', tArray.lastIndexOf(3))
}
lengthChange() //: case LengthChange:  -1

// Replace standard builtin
function replace (a: any) {
  return a
}

let newArr = [1, 2, NaN]
let true_lastIndexOf = newArr.lastIndexOf
newArr.lastIndexOf = replace

print('case BuiltinFunctionChange: ', newArr.lastIndexOf(undefined)) //: case BuiltinFunctionChange:  undefined
newArr.lastIndexOf = true_lastIndexOf
//aot: [trace] Check Type: BuiltinInstanceHClassMismatch
print('case BuiltinHClassMismatch: ', newArr.lastIndexOf(undefined)) //: case BuiltinHClassMismatch:  -1

function dolastIndexOf (x: any): any {
  return newArr.lastIndexOf(x)
}

function printlastIndexOf (promptStr: string, x: any) {
  try {
    print(promptStr, dolastIndexOf(x))
  } finally {
  }
}

let promptStr = 'pgo: '
if (ArkTools.isAOTCompiled(printlastIndexOf)) {
  // Replace standard builtin after call to standard builtin was profiled
  newArr.lastIndexOf = replace
  promptStr = 'aot: '
}
printlastIndexOf(promptStr, 2) //pgo: pgo:  1
//aot: [trace] Check Type: NotCallTarget1
//aot: aot:  2

printlastIndexOf(promptStr, 'abc') //pgo: pgo:  -1
//aot: [trace] Check Type: NotCallTarget1
//aot: aot:  abc

newArr.lastIndexOf = true_lastIndexOf

function lastIndexOfCase1 () {
  print('case 1 lastIndexOf') //: case 1 lastIndexOf
  let arr1 = [1, 2]
  let arr2 = [1, 2]
  arr2.garbage = function (x: any): any {
    return undefined
  }
  //aot: [trace] Check Type: NotStableArray1
  print('de-opt case 1.1: ', arr1.lastIndexOf(1)) //: de-opt case 1.1:  0
  print('de-opt case 1.2: ', arr2.lastIndexOf(2)) //: de-opt case 1.2:  1
}
lastIndexOfCase1()

function lastIndexOfCase2 () {
  print('case 2 lastIndexOf') //: case 2 lastIndexOf
  let arr1 = [1, 2]
  let arr2 = [1, 2]
  arr2.lastIndexOf = function (x: any) {
    return x
  }

  //aot: [trace] aot inline builtin: Object.getPrototypeOf, caller function name:#*#lastIndexOfCase2@builtinArrayLastIndexOf
  print(Object.getPrototypeOf(arr2) === Array.prototype) //: true

  //aot: [trace] Check Type: NotStableArray1
  print('de-opt case 2.1: ', arr1.lastIndexOf(1)) //: de-opt case 2.1:  0
  print('de-opt case 2.2: ', arr2.lastIndexOf(1)) //: de-opt case 2.2:  1
}
lastIndexOfCase2()

function lastIndexOfCase3 () {
  print('case 3 lastIndexOf') //: case 3 lastIndexOf
  let marr = [1, 2]
  let true_lastIndexOf = marr.lastIndexOf
  let mimicArray = {
    lastIndexOf: true_lastIndexOf
  }

  //aot: [trace] Check Type: NotStableArray1
  print('de-opt case 3.1: ', marr.lastIndexOf(500)) //: de-opt case 3.1:  -1
  Object.setPrototypeOf(marr, mimicArray)

  print('de-opt case 3.2: ', marr.lastIndexOf(500)) //: de-opt case 3.2:  -1
}
lastIndexOfCase3()

function lastIndexOfCase4 () {
  print('case 4 lastIndexOf') //: case 4 lastIndexOf
  let arr1 = [1, 2]
  let arr2 = [1, 2]
  let notArray = {
    lastIndexOf (x: any) {
      return -100
    }
  }
  //aot: [trace] aot call builtin: Object.SetPrototypeOf, caller function name:#*#lastIndexOfCase4@builtinArrayLastIndexOf
  Object.setPrototypeOf(arr2, notArray)

  //aot: [trace] Check Type: NotStableArray1
  print('de-opt case 4.1: ', arr1.lastIndexOf(1)) //: de-opt case 4.1:  0
  print('de-opt case 4.2: ', arr2.lastIndexOf(1)) //: de-opt case 4.2:  -100
}
lastIndexOfCase4()

function lastIndexOfCase5 () {
  print('case 5 lastIndexOf') //: case 5 lastIndexOf
  let arr1 = [1, 2]
  Array.prototype.lastIndexOf = function (x: any) {
    return x
  }

  //aot: [trace] Check Type: NotStableArray1
  print('de-opt case 5.1: ', arr1.lastIndexOf(300)) //: de-opt case 5.1:  300
}
lastIndexOfCase5()
