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