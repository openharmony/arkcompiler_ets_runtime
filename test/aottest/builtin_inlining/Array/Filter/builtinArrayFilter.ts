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
var x = [1,2.5,NaN,undefined,null,false,true]
function foo(ele){
    print(ele)
    if (ele != 1 || ele != true){
        return true
    }
    return false
}
//aot: [trace] aot inline builtin: Array.prototype.filter, caller function name:func_main_0@builtinArrayFilter
var xFilterArrow = x.filter(ele=>{
    print(ele)
    if (ele != 1 || ele != true){
        return true
    }
})
//: 1
//: 2.5
//: NaN
//: undefined
//: null
//: false
//: true
//: 2.5,NaN,,,false
print(xFilterArrow)
//aot: [trace] aot inline builtin: Array.prototype.filter, caller function name:func_main_0@builtinArrayFilter
var xFilterFunc = x.filter(foo)
//: 1
//: 2.5
//: NaN
//: undefined
//: null
//: false
//: true
//: 2.5,NaN,,,false
print(xFilterFunc)

