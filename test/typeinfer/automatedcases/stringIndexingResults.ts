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

// === tests/cases/conformance/types/objectTypeLiteral/indexSignatures/stringIndexingResults.ts ===
declare function AssertType(value:any, type:string):void;
class C {
    [x: string]: string;
    y = '';
}

let c: C;
AssertType(c, "C");

let r1 = c['y']; 
AssertType(r1, "string");
AssertType(c['y'], "string");
AssertType(c, "C");
AssertType('y', "string");

let r2 = c['a'];
AssertType(r2, "string");
AssertType(c['a'], "string");
AssertType(c, "C");
AssertType('a', "string");

let r3 = c[1]; 
AssertType(r3, "string");
AssertType(c[1], "string");
AssertType(c, "C");
AssertType(1, "int");

interface I {
    [x: string]: string;
    y: string;
}

let i: I
AssertType(i, "I");

let r4 = i['y'];
AssertType(r4, "string");
AssertType(i['y'], "string");
AssertType(i, "I");
AssertType('y', "string");

let r5 = i['a'];
AssertType(r5, "string");
AssertType(i['a'], "string");
AssertType(i, "I");
AssertType('a', "string");

let r6 = i[1]; 
AssertType(r6, "string");
AssertType(i[1], "string");
AssertType(i, "I");
AssertType(1, "int");

let a: {
AssertType(a, "{ [string]: string; y: string; }");

    [x: string]: string;
AssertType(x, "string");

    y: string;
AssertType(y, "string");
}

let r7 = a['y'];
AssertType(r7, "string");
AssertType(a['y'], "string");
AssertType(a, "{ [string]: string; y: string; }");
AssertType('y', "string");

let r8 = a['a'];
AssertType(r8, "string");
AssertType(a['a'], "string");
AssertType(a, "{ [string]: string; y: string; }");
AssertType('a', "string");

let r9 = a[1];
AssertType(r9, "string");
AssertType(a[1], "string");
AssertType(a, "{ [string]: string; y: string; }");
AssertType(1, "int");

let b: { [x: string]: string } = { y: '' 
AssertType(b, "{ [string]: string; }");

AssertType(x, "string");

AssertType({ y: '' }, "{ y: string; }");

AssertType(y, "string");

AssertType('', "string");
}

let r10 = b['y'];
AssertType(r10, "string");
AssertType(b['y'], "string");
AssertType(b, "{ [string]: string; }");
AssertType('y', "string");

let r11 = b['a'];
AssertType(r11, "string");
AssertType(b['a'], "string");
AssertType(b, "{ [string]: string; }");
AssertType('a', "string");

let r12 = b[1];
AssertType(r12, "string");
AssertType(b[1], "string");
AssertType(b, "{ [string]: string; }");
AssertType(1, "int");


