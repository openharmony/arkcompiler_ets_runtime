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
function changeToDic() {
    let z = [1,2,3]
    z.length = 10000
    z[9999] = 9999
    print(z.pop())
}
//aot: [trace] aot inline function name: #*#changeToDic@builtinArrayPopProtoChange caller function name: func_main_0@builtinArrayPopProtoChange
//aot: [trace] Check Type: BuiltinInstanceHClassMismatch
changeToDic() //: 9999
function prototypeChange(){
  Array.prototype[1] = 1
  var x = [0]
  x.length = 2
  var pop = x.pop()
  //aot: [trace] Check Type: NotStableArray2
  print(pop) //: 1
  Object.prototype.length =  2
}
prototypeChange()