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
//aot: [trace] aot inline builtin: Object.is, caller function name:func_main_0@builtinObjectObject
print(Object.is(1, 1)); //: true
//aot: [trace] aot inline builtin: Object.is, caller function name:func_main_0@builtinObjectObject
print(Object.is(1, 2)); //: false
//aot: [trace] aot inline builtin: Object.is, caller function name:func_main_0@builtinObjectObject
print(Object.is(1, "abc")); //: false
//aot: [trace] aot inline builtin: Object.is, caller function name:func_main_0@builtinObjectObject
print(Object.is(1, {})); //: false
//aot: [trace] aot inline builtin: Object.getPrototypeOf, caller function name:func_main_0@builtinObjectObject
print(Object.getPrototypeOf({})) //: [object Object]
//aot: [trace] aot inline builtin: Object.getPrototypeOf, caller function name:func_main_0@builtinObjectObject
print(Object.getPrototypeOf(1)) //: 0
//aot: [trace] aot inline builtin: Object.getPrototypeOf, caller function name:func_main_0@builtinObjectObject
print(Object.getPrototypeOf(true)) //: false
let proto = {name: "create"};
//aot: [trace] aot inline builtin: Object.create, caller function name:func_main_0@builtinObjectObject
print(Object.create(proto).name); //: create
//aot: [trace] aot call builtin: Object.create, caller function name:func_main_0@builtinObjectObject
print(Object.create(proto, {
    name: {
        value: "new create",
        enumerable: false,
        writable: true,
        configurable: true,
    },
}).name); //: new create
try {
    //aot: [trace] aot inline builtin: Object.create, caller function name:func_main_0@builtinObjectObject
    Object.create(undefined);
} catch(e) {
    print(e instanceof TypeError); //: true
}
let isProto = {};
//aot: [trace] aot inline builtin: Object.prototype.isPrototypeOf, caller function name:func_main_0@builtinObjectObject
print(isProto.isPrototypeOf(isProto)); //: false
//aot: [trace] aot inline builtin: Object.prototype.isPrototypeOf, caller function name:func_main_0@builtinObjectObject
print(Object.prototype.isPrototypeOf(isProto)); //: true
let hasOwnProp = {a: 123};
//aot: [trace] aot inline builtin: Object.prototype.hasOwnProperty, caller function name:func_main_0@builtinObjectObject
print(hasOwnProp.hasOwnProperty("a")); //: true
//aot: [trace] aot inline builtin: Object.prototype.hasOwnProperty, caller function name:func_main_0@builtinObjectObject
print(hasOwnProp.hasOwnProperty("b")); //: false
