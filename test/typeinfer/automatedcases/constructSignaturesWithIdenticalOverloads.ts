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

// === tests/cases/conformance/types/objectTypeLiteral/constructSignatures/constructSignaturesWithIdenticalOverloads.ts ===
declare function AssertType(value:any, type:string):void;
// Duplicate overloads of construct signatures should generate errors

class C {
    constructor(x: number, y: string);
    constructor(x: number, y: string); // error
    constructor(x: number) { }
}

let r1 = new C(1, '');
AssertType(r1, "C");
AssertType(new C(1, ''), "C");
AssertType(C, "typeof C");
AssertType(1, "int");
AssertType('', "string");

class C2<T> {
    constructor(x: T, y: string);
    constructor(x: T, y: string); // error
    constructor(x: T) { }
}

let r2 = new C2(1, '');
AssertType(r2, "C2<number>");
AssertType(new C2(1, ''), "C2<number>");
AssertType(C2, "typeof C2");
AssertType(1, "int");
AssertType('', "string");

interface I {
    new (x: number, y: string): C;
    new (x: number, y: string): C; // error
}

let i: I;
AssertType(i, "I");

let r3 = new i(1, '');
AssertType(r3, "C");
AssertType(new i(1, ''), "C");
AssertType(i, "I");
AssertType(1, "int");
AssertType('', "string");

interface I2<T> {
    new (x: T, y: string): C2<T>;
    new (x: T, y: string): C2<T>; // error
    new <T>(x: T, y: string): C2<T>;
    new <T>(x: T, y: string): C2<T>; // error
}

let i2: I2<number>;
AssertType(i2, "I2<number>");

let r4 = new i2(1, '');
AssertType(r4, "C2<number>");
AssertType(new i2(1, ''), "C2<number>");
AssertType(i2, "I2<number>");
AssertType(1, "int");
AssertType('', "string");

let a: {
AssertType(a, "{ new (number, string): C; new (number, string): C; }");

    new (x: number, y: string): C;
AssertType(x, "number");
AssertType(y, "string");

    new (x: number, y: string): C; // error
AssertType(x, "number");
AssertType(y, "string");
}

let r5 = new a(1, '');
AssertType(r5, "C");
AssertType(new a(1, ''), "C");
AssertType(a, "{ new (number, string): C; new (number, string): C; }");
AssertType(1, "int");
AssertType('', "string");

let b: {
AssertType(b, "{ new <T>(T, string): C2<T>; new <T>(T, string): C2<T>; }");

    new <T>(x: T, y: string): C2<T>;
AssertType(x, "T");
AssertType(y, "string");

    new <T>(x: T, y: string): C2<T>; // error
AssertType(x, "T");
AssertType(y, "string");
}

let r6 = new b(1, '');
AssertType(r6, "C2<number>");
AssertType(new b(1, ''), "C2<number>");
AssertType(b, "{ new <T>(T, string): C2<T>; new <T>(T, string): C2<T>; }");
AssertType(1, "int");
AssertType('', "string");


