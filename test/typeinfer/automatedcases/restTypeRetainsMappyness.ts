/*
* Copyright (c) Microsoft Corporation. All rights reserved.
* Copyright (c) 2023 Huawei Device Co., Ltd.
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
*
* This file has been modified by Huawei to verify type inference by adding verification statements.
*/

// === tests/cases/compiler/restTypeRetainsMappyness.ts ===
declare function AssertType(value:any, type:string):void;
type Foo<T extends any[]> = {
    [P in keyof T]: T[P]
}

function test<T extends any[]>(fn: (...args: Foo<T>) => void) {
    const arr: Foo<T> = {} as any
AssertType(arr, "Foo<T>");
AssertType({} as any, "any");
AssertType({}, "{}");

    fn(...arr) // Error: Argument of type 'any[]' is not assignable to parameter of type 'Foo<T>'
AssertType(fn(...arr), "void");
AssertType(fn, "(...Foo<T>) => void");
AssertType(...arr, "any");
AssertType(arr, "Foo<T>");
}


