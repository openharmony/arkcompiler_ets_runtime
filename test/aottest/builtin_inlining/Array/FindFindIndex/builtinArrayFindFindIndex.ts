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
