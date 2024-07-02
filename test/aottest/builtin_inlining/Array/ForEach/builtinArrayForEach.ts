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
//aot: [trace] aot inline builtin: Array.prototype.foreach, caller function name:func_main_0@builtinArrayForEach
x.forEach(ele=>{
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
//aot: [trace] aot inline builtin: Array.prototype.foreach, caller function name:func_main_0@builtinArrayForEach
x.forEach(foo)
//: 1
//: 2.5
//: NaN
//: undefined
//: null
//: false
//: true
function testForeachEndHoleArray() {
    let y = [1,2,3,,,]
    y.forEach(x=>{
        print(x)
    })
}
//aot: [trace] aot inline function name: #*#testForeachEndHoleArray@builtinArrayForEach caller function name: func_main_0@builtinArrayForEach
//aot: [trace] Check Type: NotStableArray2
//: 1
//: 2
//: 3
testForeachEndHoleArray()

