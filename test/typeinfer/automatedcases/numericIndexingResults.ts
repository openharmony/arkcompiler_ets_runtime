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

// === tests/cases/conformance/types/objectTypeLiteral/indexSignatures/numericIndexingResults.ts ===
declare function AssertType(value:any, type:string):void;
class C {
    [x: number]: string;
    1 = '';
    "2" = ''
}

let c: C;
AssertType(c, "C");

let r1 = c['1'];
AssertType(r1, "string");
AssertType(c['1'], "string");
AssertType(c, "C");
AssertType('1', "string");

let r2 = c['2'];
AssertType(r2, "string");
AssertType(c['2'], "string");
AssertType(c, "C");
AssertType('2', "string");

let r3 = c['3'];
AssertType(r3, "string");
AssertType(c['3'], "string");
AssertType(c, "C");
AssertType('3', "string");

let r4 = c[1];
AssertType(r4, "string");
AssertType(c[1], "string");
AssertType(c, "C");
AssertType(1, "int");

let r5 = c[2];
AssertType(r5, "string");
AssertType(c[2], "string");
AssertType(c, "C");
AssertType(2, "int");

let r6 = c[3];
AssertType(r6, "string");
AssertType(c[3], "string");
AssertType(c, "C");
AssertType(3, "int");

interface I {
    [x: number]: string;
    1: string;
    "2": string;
}

let i: I
AssertType(i, "I");

let r1 = i['1'];
AssertType(r1, "string");
AssertType(i['1'], "string");
AssertType(i, "I");
AssertType('1', "string");

let r2 = i['2'];
AssertType(r2, "string");
AssertType(i['2'], "string");
AssertType(i, "I");
AssertType('2', "string");

let r3 = i['3'];
AssertType(r3, "string");
AssertType(i['3'], "string");
AssertType(i, "I");
AssertType('3', "string");

let r4 = i[1];
AssertType(r4, "string");
AssertType(i[1], "string");
AssertType(i, "I");
AssertType(1, "int");

let r5 = i[2];
AssertType(r5, "string");
AssertType(i[2], "string");
AssertType(i, "I");
AssertType(2, "int");

let r6 = i[3];
AssertType(r6, "string");
AssertType(i[3], "string");
AssertType(i, "I");
AssertType(3, "int");

let a: {
AssertType(a, "{ [number]: string; 1: string; "2": string; }");

    [x: number]: string;
AssertType(x, "number");

    1: string;
AssertType(1, "string");

    "2": string;
AssertType("2", "string");
}

let r1 = a['1'];
AssertType(r1, "string");
AssertType(a['1'], "string");
AssertType(a, "{ [number]: string; 1: string; "2": string; }");
AssertType('1', "string");

let r2 = a['2'];
AssertType(r2, "string");
AssertType(a['2'], "string");
AssertType(a, "{ [number]: string; 1: string; "2": string; }");
AssertType('2', "string");

let r3 = a['3'];
AssertType(r3, "string");
AssertType(a['3'], "string");
AssertType(a, "{ [number]: string; 1: string; "2": string; }");
AssertType('3', "string");

let r4 = a[1];
AssertType(r4, "string");
AssertType(a[1], "string");
AssertType(a, "{ [number]: string; 1: string; "2": string; }");
AssertType(1, "int");

let r5 = a[2];
AssertType(r5, "string");
AssertType(a[2], "string");
AssertType(a, "{ [number]: string; 1: string; "2": string; }");
AssertType(2, "int");

let r6 = a[3];
AssertType(r6, "string");
AssertType(a[3], "string");
AssertType(a, "{ [number]: string; 1: string; "2": string; }");
AssertType(3, "int");

let b: { [x: number]: string } = { 1: '', "2": '' 
AssertType(b, "{ [number]: string; }");

AssertType(x, "number");

AssertType({ 1: '', "2": '' }, "{ 1: string; "2": string; }");

AssertType(1, "string");

AssertType('', "string");

AssertType("2", "string");

AssertType('', "string");
}

let r1a = b['1'];
AssertType(r1a, "string");
AssertType(b['1'], "string");
AssertType(b, "{ [number]: string; }");
AssertType('1', "string");

let r2a = b['2'];
AssertType(r2a, "string");
AssertType(b['2'], "string");
AssertType(b, "{ [number]: string; }");
AssertType('2', "string");

let r3 = b['3'];
AssertType(r3, "string");
AssertType(b['3'], "string");
AssertType(b, "{ [number]: string; }");
AssertType('3', "string");

let r4 = b[1];
AssertType(r4, "string");
AssertType(b[1], "string");
AssertType(b, "{ [number]: string; }");
AssertType(1, "int");

let r5 = b[2];
AssertType(r5, "string");
AssertType(b[2], "string");
AssertType(b, "{ [number]: string; }");
AssertType(2, "int");

let r6 = b[3];
AssertType(r6, "string");
AssertType(b[3], "string");
AssertType(b, "{ [number]: string; }");
AssertType(3, "int");

let b2: { [x: number]: string; 1: string; "2": string; } = { 1: '', "2": '' 
AssertType(b2, "{ [number]: string; 1: string; "2": string; }");

AssertType(x, "number");

AssertType(1, "string");

AssertType("2", "string");

AssertType({ 1: '', "2": '' }, "{ 1: string; "2": string; }");

AssertType(1, "string");

AssertType('', "string");

AssertType("2", "string");

AssertType('', "string");
}

let r1b = b2['1'];
AssertType(r1b, "string");
AssertType(b2['1'], "string");
AssertType(b2, "{ [number]: string; 1: string; "2": string; }");
AssertType('1', "string");

let r2b = b2['2'];
AssertType(r2b, "string");
AssertType(b2['2'], "string");
AssertType(b2, "{ [number]: string; 1: string; "2": string; }");
AssertType('2', "string");

let r3 = b2['3'];
AssertType(r3, "string");
AssertType(b2['3'], "string");
AssertType(b2, "{ [number]: string; 1: string; "2": string; }");
AssertType('3', "string");

let r4 = b2[1];
AssertType(r4, "string");
AssertType(b2[1], "string");
AssertType(b2, "{ [number]: string; 1: string; "2": string; }");
AssertType(1, "int");

let r5 = b2[2];
AssertType(r5, "string");
AssertType(b2[2], "string");
AssertType(b2, "{ [number]: string; 1: string; "2": string; }");
AssertType(2, "int");

let r6 = b2[3];
AssertType(r6, "string");
AssertType(b2[3], "string");
AssertType(b2, "{ [number]: string; 1: string; "2": string; }");
AssertType(3, "int");


