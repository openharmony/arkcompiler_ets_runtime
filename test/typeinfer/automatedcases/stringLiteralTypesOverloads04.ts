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

// === tests/cases/conformance/types/stringLiteral/stringLiteralTypesOverloads04.ts ===
declare function AssertType(value:any, type:string):void;
declare function f(x: (p: "foo" | "bar") => "foo");

f(y => {
AssertType(f, "((union) => "foo") => any");
AssertType(y => {    const z = y = "foo";    return z;}, "(union) => "foo"");
AssertType(y, "union");
AssertType(f(y => {    const z = y = "foo";    return z;}), "any");

    const z = y = "foo";
AssertType(z, "string");
AssertType(y = "foo", "string");
AssertType(y, "union");
AssertType("foo", "string");

AssertType(z, "string");
    return z;

})

