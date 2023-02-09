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

// === tests/cases/compiler/declarationEmitDestructuringObjectLiteralPattern2.ts ===
declare function AssertType(value:any, type:string):void;
let { a: x11, b: { a: y11, b: { a: z11 }}} = { a: 1, b: { a: "hello", b: { a: true } } };
AssertType(a, "any");
AssertType(x11, "number");
AssertType(b, "any");
AssertType(a, "any");
AssertType(y11, "string");
AssertType(b, "any");
AssertType(a, "any");
AssertType(z11, "boolean");
AssertType({ a: 1, b: { a: "hello", b: { a: true } } }, "{ a: number; b: { a: string; b: { a: boolean; }; }; }");
AssertType(a, "number");
AssertType(1, "int");
AssertType(b, "{ a: string; b: { a: boolean; }; }");
AssertType({ a: "hello", b: { a: true } }, "{ a: string; b: { a: boolean; }; }");
AssertType(a, "string");
AssertType("hello", "string");
AssertType(b, "{ a: boolean; }");
AssertType({ a: true }, "{ a: boolean; }");
AssertType(a, "boolean");
AssertType(true, "boolean");

function f15() {
    let a4 = "hello";
AssertType(a4, "string");
AssertType("hello", "string");

    let b4 = 1;
AssertType(b4, "number");
AssertType(1, "int");

    let c4 = true;
AssertType(c4, "boolean");
AssertType(true, "boolean");

AssertType({ a4, b4, c4 }, "{ a4: string; b4: number; c4: boolean; }");
AssertType(a4, "string");
AssertType(b4, "number");
AssertType(c4, "boolean");
    return { a4, b4, c4 };
}
let { a4, b4, c4 } = f15();
AssertType(a4, "string");
AssertType(b4, "number");
AssertType(c4, "boolean");
AssertType(f15(), "{ a4: string; b4: number; c4: boolean; }");
AssertType(f15, "() => { a4: string; b4: number; c4: boolean; }");

module m {
    export let { a4, b4, c4 } = f15();
}

