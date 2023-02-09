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

// === tests/cases/compiler/restUnion.ts ===
declare function AssertType(value:any, type:string):void;
let union: { a: number, c: boolean } | { a: string, b: string };
AssertType(union, "union");
AssertType(a, "number");
AssertType(c, "boolean");
AssertType(a, "string");
AssertType(b, "string");

let rest1: { c: boolean } | { b: string };
AssertType(rest1, "union");
AssertType(c, "boolean");
AssertType(b, "string");

let {a, ...rest1 } = union;
AssertType(a, "union");
AssertType(rest1, "union");
AssertType(union, "union");


let undefinedUnion: { n: number } | undefined;
AssertType(undefinedUnion, "{ n: number; }");
AssertType(n, "number");

let rest2: {};
AssertType(rest2, "{}");

let {n, ...rest2 } = undefinedUnion;
AssertType(n, "number");
AssertType(rest2, "{}");
AssertType(undefinedUnion, "{ n: number; }");


let nullUnion: { n: number } | null;
AssertType(nullUnion, "{ n: number; }");
AssertType(n, "number");
AssertType(null, "null");

let rest3: {};
AssertType(rest3, "{}");

let {n, ...rest3 } = nullUnion;
AssertType(n, "number");
AssertType(rest3, "{}");
AssertType(nullUnion, "{ n: number; }");


