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

// === tests/cases/conformance/types/members/objectTypeWithNumericProperty.ts ===
declare function AssertType(value:any, type:string):void;
// no errors here

class C {
    1: number;
    1.1: string;
}

let c: C;
AssertType(c, "C");

let r1 = c[1];
AssertType(r1, "number");
AssertType(c[1], "number");
AssertType(c, "C");
AssertType(1, "int");

let r2 = c[1.1];
AssertType(r2, "string");
AssertType(c[1.1], "string");
AssertType(c, "C");
AssertType(1.1, "double");

let r3 = c['1'];
AssertType(r3, "number");
AssertType(c['1'], "number");
AssertType(c, "C");
AssertType('1', "string");

let r4 = c['1.1'];
AssertType(r4, "string");
AssertType(c['1.1'], "string");
AssertType(c, "C");
AssertType('1.1', "string");

interface I {
    1: number;
    1.1: string;
}

let i: I;
AssertType(i, "I");

let r1 = i[1];
AssertType(r1, "number");
AssertType(i[1], "number");
AssertType(i, "I");
AssertType(1, "int");

let r2 = i[1.1];
AssertType(r2, "string");
AssertType(i[1.1], "string");
AssertType(i, "I");
AssertType(1.1, "double");

let r3 = i['1'];
AssertType(r3, "number");
AssertType(i['1'], "number");
AssertType(i, "I");
AssertType('1', "string");

let r4 = i['1.1'];
AssertType(r4, "string");
AssertType(i['1.1'], "string");
AssertType(i, "I");
AssertType('1.1', "string");

let a: {
AssertType(a, "{ 1: number; 1.1: string; }");

    1: number;
AssertType(1, "number");

    1.1: string;
AssertType(1.1, "string");
}

let r1 = a[1];
AssertType(r1, "number");
AssertType(a[1], "number");
AssertType(a, "{ 1: number; 1.1: string; }");
AssertType(1, "int");

let r2 = a[1.1];
AssertType(r2, "string");
AssertType(a[1.1], "string");
AssertType(a, "{ 1: number; 1.1: string; }");
AssertType(1.1, "double");

let r3 = a['1'];
AssertType(r3, "number");
AssertType(a['1'], "number");
AssertType(a, "{ 1: number; 1.1: string; }");
AssertType('1', "string");

let r4 = a['1.1'];
AssertType(r4, "string");
AssertType(a['1.1'], "string");
AssertType(a, "{ 1: number; 1.1: string; }");
AssertType('1.1', "string");

let b = {
AssertType(b, "{ 1: number; 1.1: string; }");
AssertType({    1: 1,    1.1: ""}, "{ 1: number; 1.1: string; }");

    1: 1,
AssertType(1, "number");
AssertType(1, "int");

    1.1: ""
AssertType(1.1, "string");
AssertType("", "string");
}

let r1 = b[1];
AssertType(r1, "number");
AssertType(b[1], "number");
AssertType(b, "{ 1: number; 1.1: string; }");
AssertType(1, "int");

let r2 = b[1.1];
AssertType(r2, "string");
AssertType(b[1.1], "string");
AssertType(b, "{ 1: number; 1.1: string; }");
AssertType(1.1, "double");

let r3 = b['1'];
AssertType(r3, "number");
AssertType(b['1'], "number");
AssertType(b, "{ 1: number; 1.1: string; }");
AssertType('1', "string");

let r4 = b['1.1'];
AssertType(r4, "string");
AssertType(b['1.1'], "string");
AssertType(b, "{ 1: number; 1.1: string; }");
AssertType('1.1', "string");


