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

// === tests/cases/conformance/types/objectTypeLiteral/propertySignatures/stringNamedPropertyAccess.ts ===
declare function AssertType(value:any, type:string):void;
class C {
    "a b": number;
    static "c d": number;
}
let c: C;
AssertType(c, "C");

let r1 = c["a b"];
AssertType(r1, "number");
AssertType(c["a b"], "number");
AssertType(c, "C");
AssertType("a b", "string");

let r1b = C['c d'];
AssertType(r1b, "number");
AssertType(C['c d'], "number");
AssertType(C, "typeof C");
AssertType('c d', "string");

interface I {
    "a b": number;
}
let i: I;
AssertType(i, "I");

let r2 = i["a b"];
AssertType(r2, "number");
AssertType(i["a b"], "number");
AssertType(i, "I");
AssertType("a b", "string");

let a: {
AssertType(a, "{ "a b": number; }");

    "a b": number;
AssertType("a b", "number");
}
let r3 = a["a b"];
AssertType(r3, "number");
AssertType(a["a b"], "number");
AssertType(a, "{ "a b": number; }");
AssertType("a b", "string");

let b = {
AssertType(b, "{ "a b": number; }");
AssertType({    "a b": 1}, "{ "a b": number; }");

    "a b": 1
AssertType("a b", "number");
AssertType(1, "int");
}
let r4 = b["a b"];
AssertType(r4, "number");
AssertType(b["a b"], "number");
AssertType(b, "{ "a b": number; }");
AssertType("a b", "string");


