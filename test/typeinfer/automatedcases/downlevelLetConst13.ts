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

// === tests/cases/compiler/downlevelLetConst13.ts ===
declare function AssertType(value:any, type:string):void;
'use strict'
AssertType('use strict', "string");

// exported let\const bindings should not be renamed

export let foo = 10;
AssertType(foo, "number");
AssertType(10, "int");

export const bar = "123"
AssertType(bar, "string");
AssertType("123", "string");

export let [bar1] = [1];
AssertType(bar1, "number");
AssertType([1], "[number]");
AssertType(1, "int");

export const [bar2] = [2];
AssertType(bar2, "number");
AssertType([2], "[number]");
AssertType(2, "int");

export let {a: bar3} = { a: 1 };
AssertType(a, "any");
AssertType(bar3, "number");
AssertType({ a: 1 }, "{ a: number; }");
AssertType(a, "number");
AssertType(1, "int");

export const {a: bar4} = { a: 1 };
AssertType(a, "any");
AssertType(bar4, "number");
AssertType({ a: 1 }, "{ a: number; }");
AssertType(a, "number");
AssertType(1, "int");

export module M {
    export let baz = 100;
    export const baz2 = true;
    export let [bar5] = [1];
    export const [bar6] = [2];
    export let {a: bar7} = { a: 1 };
    export const {a: bar8} = { a: 1 };
}

