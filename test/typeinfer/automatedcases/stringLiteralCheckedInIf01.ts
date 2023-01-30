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

// === tests/cases/conformance/types/stringLiteral/stringLiteralCheckedInIf01.ts ===
declare function AssertType(value:any, type:string):void;
type S = "a" | "b";
type T = S[] | S;

function f(foo: T) {
    if (foo === "a") {
AssertType(foo === "a", "boolean");
AssertType(foo, "T");
AssertType("a", "string");

AssertType(foo, "string");
        return foo;
    }
    else if (foo === "b") {
AssertType(foo === "b", "boolean");
AssertType(foo, "union");
AssertType("b", "string");

AssertType(foo, "string");
        return foo;
    }
    else { 
AssertType((foo as S[])[0], "S");
AssertType((foo as S[]), "S[]");
AssertType(foo as S[], "S[]");
AssertType(foo, "S[]");
AssertType(0, "int");
        return (foo as S[])[0];
    }
}

