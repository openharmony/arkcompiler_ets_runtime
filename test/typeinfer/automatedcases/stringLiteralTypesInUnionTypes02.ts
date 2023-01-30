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

// === tests/cases/conformance/types/stringLiteral/stringLiteralTypesInUnionTypes02.ts ===
declare function AssertType(value:any, type:string):void;
type T = string | "foo" | "bar" | "baz";

let x: "foo" | "bar" | "baz" | string = undefined;
AssertType(x, "string");
AssertType(undefined, "undefined");

let y: T = undefined;
AssertType(y, "string");
AssertType(undefined, "undefined");

if (x === "foo") {
    let a = x;
AssertType(a, "string");
AssertType(x, "string");
}
else if (x !== "bar") {
    let b = x || y;
AssertType(b, "string");
AssertType(x || y, "string");
AssertType(x, "string");
AssertType(y, "string");
}
else {
    let c = x;
AssertType(c, "string");
AssertType(x, "string");

    let d = y;
AssertType(d, "string");
AssertType(y, "string");

    let e: (typeof x) | (typeof y) = c || d;
AssertType(e, "string");
AssertType(x, "string");
AssertType(y, "string");
AssertType(c || d, "string");
AssertType(c, "string");
AssertType(d, "string");
}

x = y;
AssertType(x = y, "string");
AssertType(x, "string");
AssertType(y, "string");

y = x;
AssertType(y = x, "string");
AssertType(y, "string");
AssertType(x, "string");


