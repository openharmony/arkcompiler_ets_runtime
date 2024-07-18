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
function foo(a: any, b: any) {
    return this + a + b;
}
print(foo.apply(1, [2, 3])); //: 6
print(foo.apply(1)); //: NaN
let bfoo = foo.bind(2);
print(bfoo(3, 4)); //: 9
let bfoo1 = foo.bind(5, 6);
print(bfoo1(7)); //: 18
print(foo.call(10, 20)); //: NaN
print(foo.call(10, 20, 30)); //: 60
class C {}
//aot: [trace] aot inline builtin: Function.prototype.hasInstance, caller function name:func_main_0@builtinFunctionFunction
print(C[Symbol.hasInstance](new C())); //: true
//aot: [trace] aot inline builtin: Function.prototype.hasInstance, caller function name:func_main_0@builtinFunctionFunction
print(C[Symbol.hasInstance]({})); //: false
