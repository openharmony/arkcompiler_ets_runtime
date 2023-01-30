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

// === tests/cases/conformance/types/stringLiteral/stringLiteralTypesAndParenthesizedExpressions01.ts ===
declare function AssertType(value:any, type:string):void;
declare function myRandBool(): boolean;

let a: "foo" = ("foo");
AssertType(a, "string");
AssertType(("foo"), "string");
AssertType("foo", "string");

let b: "foo" | "bar" = ("foo");
AssertType(b, "union");
AssertType(("foo"), "string");
AssertType("foo", "string");

let c: "foo" = (myRandBool ? "foo" : ("foo"));
AssertType(c, "string");
AssertType((myRandBool ? "foo" : ("foo")), "string");
AssertType(myRandBool ? "foo" : ("foo"), "string");
AssertType(myRandBool, "() => boolean");
AssertType("foo", "string");
AssertType(("foo"), "string");
AssertType("foo", "string");

let d: "foo" | "bar" = (myRandBool ? "foo" : ("bar"));
AssertType(d, "union");
AssertType((myRandBool ? "foo" : ("bar")), "union");
AssertType(myRandBool ? "foo" : ("bar"), "union");
AssertType(myRandBool, "() => boolean");
AssertType("foo", "string");
AssertType(("bar"), "string");
AssertType("bar", "string");


