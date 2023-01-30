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

// === tests/cases/conformance/types/stringLiteral/stringLiteralTypesInUnionTypes01.ts ===
declare function AssertType(value:any, type:string):void;
type T = "foo" | "bar" | "baz";

let x: "foo" | "bar" | "baz" = undefined;
AssertType(x, "union");
AssertType(undefined, "undefined");

let y: T = undefined;
AssertType(y, "T");
AssertType(undefined, "undefined");

if (x === "foo") {
    let a = x;
AssertType(a, "string");
AssertType(x, "string");
}
else if (x !== "bar") {
    let b = x || y;
AssertType(b, "T");
AssertType(x || y, "T");
AssertType(x, "string");
AssertType(y, "T");
}
else {
    let c = x;
AssertType(c, "string");
AssertType(x, "string");

    let d = y;
AssertType(d, "T");
AssertType(y, "T");

    let e: (typeof x) | (typeof y) = c || d;
AssertType(e, "T");
AssertType(x, "string");
AssertType(y, "T");
AssertType(c || d, "T");
AssertType(c, "string");
AssertType(d, "T");
}

x = y;
AssertType(x = y, "T");
AssertType(x, "union");
AssertType(y, "T");

y = x;
AssertType(y = x, "union");
AssertType(y, "T");
AssertType(x, "union");


