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
var x = [1,2.1,-1,null,undefined,,false,NaN]
//aot: [trace] aot inline builtin: Array.prototype.pop, caller function name:func_main_0@builtinArrayPop
print(x.pop()) //: NaN
//aot: [trace] aot inline builtin: Array.prototype.pop, caller function name:func_main_0@builtinArrayPop
print(x.pop(undefined)) //: false
//aot: [trace] aot inline builtin: Array.prototype.pop, caller function name:func_main_0@builtinArrayPop
print(x.pop(1,2,3,4,5,6,7,8)) //: undefined
var y = new Array(10)
for (let i = 0; i < y.length; i++) {
    y[i] = i
}
//aot: [trace] aot inline builtin: Array.prototype.pop, caller function name:func_main_0@builtinArrayPop
print(y.pop()) //: 9
//aot: [trace] aot inline builtin: Array.prototype.pop, caller function name:func_main_0@builtinArrayPop
print(y.pop(undefined)) //: 8
//aot: [trace] aot inline builtin: Array.prototype.pop, caller function name:func_main_0@builtinArrayPop
print(y.pop(1,2,3,4,5,6,7,8)) //: 7