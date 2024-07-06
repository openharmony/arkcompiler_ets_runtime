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


function normalForEach(){
  var x = [1,2.5,NaN,undefined,null,false,true]
  x.forEach(ele=>{
    if (ele != 1 || ele != true){
      print(ele)
    }
    })
}
function lenthChangeLess(){
  var y = [1,2.5,NaN,undefined,null,false,true]
  y.forEach(ele=>{
    if (ele == 2.5) {
      y.length = 5
    }
    if (ele != null) {
      print(ele)
    }
  })
}
function lengthChangeBigToDic(){
    var y = [1,2.5,NaN,undefined,null,false,true]
    y.forEach(ele=>{
    if (ele == 2.5) {
        y.length = 10000
        y[9999] = 9999
    }
    if (ele != null) {
        print(ele)
    }
    })
}
function prototypeChange(){
  var z = [1,,NaN,false,undefined,,,true]
  z.forEach(ele=>{
    if (ele == false){
      Array.prototype[1] = "position2"
      Array.prototype[5] = "position5"
      Object.prototype[6] = "position6"
    }
    if (ele == "position2" || ele == "position5" || ele == "position6"){
      print(ele)
    }
  })
}
//aot: [trace] aot inline function name: #*#normalForEach@builtinArrayForEachProtoChange caller function name: func_main_0@builtinArrayForEachProtoChange
//aot: [trace] aot inline builtin: Array.prototype.foreach, caller function name:#*#normalForEach@builtinArrayForEachProtoChange
//: 2.5
//: NaN
//: undefined
//: null
//: false
normalForEach()
//aot: [trace] aot inline function name: #*#lenthChangeLess@builtinArrayForEachProtoChange caller function name: func_main_0@builtinArrayForEachProtoChange
//: 1
//: 2.5
//: NaN
lenthChangeLess()
//aot: [trace] aot inline function name: #*#lengthChangeBigToDic@builtinArrayForEachProtoChange caller function name: func_main_0@builtinArrayForEachProtoChange
//: 1
//: 2.5
//: NaN
//: false
//: true
lengthChangeBigToDic()
//aot: [trace] aot inline function name: #*#prototypeChange@builtinArrayForEachProtoChange caller function name: func_main_0@builtinArrayForEachProtoChange
//: position5
//: position6
prototypeChange()
