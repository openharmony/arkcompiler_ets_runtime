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

// === tests/cases/conformance/types/stringLiteral/stringLiteralCheckedInIf02.ts ===
declare function AssertType(value:any, type:string):void;
type S = "a" | "b";
type T = S[] | S;

function isS(t: T): t is S {
AssertType(t === "a" || t === "b", "boolean");
AssertType(t === "a", "boolean");
AssertType(t, "T");
AssertType("a", "string");
AssertType(t === "b", "boolean");
AssertType(t, "union");
AssertType("b", "string");
    return t === "a" || t === "b";
}

function f(foo: T) {
    if (isS(foo)) {
AssertType(isS(foo), "boolean");
AssertType(isS, "(T) => t is S");
AssertType(foo, "T");

AssertType(foo, "S");
        return foo;
    }
    else { 
AssertType(foo[0], "S");
AssertType(foo, "S[]");
AssertType(0, "int");
        return foo[0];
    }
}

