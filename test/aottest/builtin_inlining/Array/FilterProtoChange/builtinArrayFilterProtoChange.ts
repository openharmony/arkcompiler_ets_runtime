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


function normalFilter(){
  var x = [1,2.5,NaN,undefined,null,false,true]
  var xFilter = x.filter(ele=>{
    if (ele != 1 || ele != true){
        return true
    }
    })
    print(xFilter)
}
function lenthChangeLess(){
  var y = [1,2.5,NaN,undefined,null,false,true]
  var yFilter = y.filter(ele=>{
    if (ele != null) {
      return true
    } else if (ele == null) {
      y.length = 5
    }
  })
  print(yFilter)
}
function lengthChangeBigToDic(){
    var y = [1,2.5,NaN,undefined,null,false,true]
    var yFilter = y.filter(ele=>{
    if (ele == 2.5) {
        y.length = 10000
        y[9999] = 9999
    }
    if (ele != null) {
        return true
    }
    })
    print(yFilter)
}
function prototypeChange(){
  var z = [1,,NaN,false,undefined,,,true]
  var zFilter = z.filter(ele=>{
    if (ele == false){
      Array.prototype[1] = "position2"
      Array.prototype[5] = "position5"
      Object.prototype[6] = "position6"
    }
    if (ele == "position2" || ele == "position5" || ele == "position6"){
      return true
    }
  })
  print(zFilter)
}
//aot: [trace] aot inline function name: #*#normalFilter@builtinArrayFilterProtoChange caller function name: func_main_0@builtinArrayFilterProtoChange
//aot: [trace] aot inline builtin: Array.prototype.filter, caller function name:#*#normalFilter@builtinArrayFilterProtoChange
//: 2.5,NaN,,,false
normalFilter()
//aot: [trace] aot inline function name: #*#lenthChangeLess@builtinArrayFilterProtoChange caller function name: func_main_0@builtinArrayFilterProtoChange
//aot: [trace] aot call builtin: Array.prototype.filter, caller function name:#*#lenthChangeLess@builtinArrayFilterProtoChange
//: 1,2.5,NaN
lenthChangeLess()
//aot: [trace] aot inline function name: #*#lengthChangeBigToDic@builtinArrayFilterProtoChange caller function name: func_main_0@builtinArrayFilterProtoChange
//aot: [trace] aot call builtin: Array.prototype.filter, caller function name:#*#lengthChangeBigToDic@builtinArrayFilterProtoChange
//: 1,2.5,NaN,false,true
lengthChangeBigToDic()
//aot: [trace] aot inline function name: #*#prototypeChange@builtinArrayFilterProtoChange caller function name: func_main_0@builtinArrayFilterProtoChange
//aot: [trace] aot call builtin: Array.prototype.filter, caller function name:#*#prototypeChange@builtinArrayFilterProtoChange
//: position5,position6
prototypeChange()
