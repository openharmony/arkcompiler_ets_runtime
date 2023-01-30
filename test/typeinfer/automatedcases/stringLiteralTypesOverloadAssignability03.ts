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

// === tests/cases/conformance/types/stringLiteral/stringLiteralTypesOverloadAssignability03.ts ===
declare function AssertType(value:any, type:string):void;
function f(x: "foo"): number;
function f(x: string): number {
AssertType(0, "int");
    return 0;
}

function g(x: "foo"): number;
function g(x: string): number {
AssertType(0, "int");
    return 0;
}

let a = f;
AssertType(a, "("foo") => number");
AssertType(f, "("foo") => number");

let b = g;
AssertType(b, "("foo") => number");
AssertType(g, "("foo") => number");

a = b;
AssertType(a = b, "("foo") => number");
AssertType(a, "("foo") => number");
AssertType(b, "("foo") => number");

b = a;
AssertType(b = a, "("foo") => number");
AssertType(b, "("foo") => number");
AssertType(a, "("foo") => number");


