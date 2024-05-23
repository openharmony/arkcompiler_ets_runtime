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
//aot: [trace] aot inline builtin: Reflect.getPrototypeOf, caller function name:func_main_0@builtinReflectReflect
print(Reflect.getPrototypeOf({})) //: [object Object]
try {
    //aot: [trace] aot inline builtin: Reflect.getPrototypeOf, caller function name:func_main_0@builtinReflectReflect
    Reflect.getPrototypeOf(1);
} catch(e) {
    print(e instanceof TypeError); //: true
}
let o = {a:123, b: "abc"};
//aot: [trace] aot inline builtin: Reflect.get, caller function name:func_main_0@builtinReflectReflect
print(Reflect.get(o, "a")); //: 123
//aot: [trace] aot inline builtin: Reflect.get, caller function name:func_main_0@builtinReflectReflect
print(Reflect.get(o, "b")); //: abc
//aot: [trace] aot inline builtin: Reflect.get, caller function name:func_main_0@builtinReflectReflect
print(Reflect.get(o, "c")); //: undefined

//aot: [trace] aot inline builtin: Reflect.has, caller function name:func_main_0@builtinReflectReflect
print(Reflect.has(o, "a")); //: true
//aot: [trace] aot inline builtin: Reflect.has, caller function name:func_main_0@builtinReflectReflect
print(Reflect.has(o, "c")); //: false
//aot: [trace] aot inline builtin: Reflect.has, caller function name:func_main_0@builtinReflectReflect
print(Reflect.has(o, "toString")); //: true
function foo(a: any, b: any) {
    return this + a + b;
}
//aot: [trace] aot inline builtin: Reflect.apply, caller function name:func_main_0@builtinReflectReflect
print(Reflect.apply(foo, 1, [2, 3])); //: 6
class C {
    x:number;
    constructor(x: number = 123) {
        this.x = x;
    }
}
//aot: [trace] aot inline builtin: Reflect.construct, caller function name:func_main_0@builtinReflectReflect
print(Reflect.construct(C, []).x); //: 123
print(Reflect.construct(C, [456]).x); //: 456
