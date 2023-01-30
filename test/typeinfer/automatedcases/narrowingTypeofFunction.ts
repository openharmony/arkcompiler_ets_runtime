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

// === tests/cases/compiler/narrowingTypeofFunction.ts ===
declare function AssertType(value:any, type:string):void;
type Meta = { foo: string }
interface F { (): string }

function f1(a: (F & Meta) | string) {
    if (typeof a === "function") {
AssertType(typeof a === "function", "boolean");
AssertType(typeof a, "union");
AssertType(a, "union");
AssertType("function", "string");

        a;
AssertType(a, "F & Meta");
    }
    else {
        a;
AssertType(a, "string");
    }
}

function f2<T>(x: (T & F) | T & string) {
    if (typeof x === "function") {
AssertType(typeof x === "function", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("function", "string");

        x;
AssertType(x, "T & F");
    }
    else {
        x;
AssertType(x, "T & string");
    }
}

function f3(x: { _foo: number } & number) {
    if (typeof x === "function") {
AssertType(typeof x === "function", "boolean");
AssertType(typeof x, "union");
AssertType(x, "{ _foo: number; } & number");
AssertType("function", "string");

        x;
AssertType(x, "never");
    }
}

