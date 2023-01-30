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

// === tests/cases/conformance/types/union/unionTypeCallSignatures2.ts ===
declare function AssertType(value:any, type:string):void;
interface A {
    (x: number): number;
    (x: string, y?: string): boolean;
    (x: Date): void;
    <T>(x: T[]): T[];
}

interface B {
    (x: number): number;
    (x: string): string;
    (x: Date): void;
    <T>(x: T[]): T[];
}

interface C {
    (x: string, ...y: string[]): number;
    (x: number, s?: string): number;
    <T>(x: T[]): T[];
}

let f1: A | B | C;
AssertType(f1, "union");

let n1 = f1(42);             // number
AssertType(n1, "number");
AssertType(f1(42), "number");
AssertType(f1, "union");
AssertType(42, "int");

let s1 = f1("abc");          // boolean | string | number
AssertType(s1, "union");
AssertType(f1("abc"), "union");
AssertType(f1, "union");
AssertType("abc", "string");

let a1 = f1([true, false]);  // boolean[]
AssertType(a1, "boolean[]");
AssertType(f1([true, false]), "boolean[]");
AssertType(f1, "union");
AssertType([true, false], "boolean[]");
AssertType(true, "boolean");
AssertType(false, "boolean");

let f2: C | B | A;
AssertType(f2, "union");

let n2 = f2(42);             // number
AssertType(n2, "number");
AssertType(f2(42), "number");
AssertType(f2, "union");
AssertType(42, "int");

let s2 = f2("abc");          // number | string | boolean
AssertType(s2, "union");
AssertType(f2("abc"), "union");
AssertType(f2, "union");
AssertType("abc", "string");

let a2 = f2([true, false]);  // boolean[]
AssertType(a2, "boolean[]");
AssertType(f2([true, false]), "boolean[]");
AssertType(f2, "union");
AssertType([true, false], "boolean[]");
AssertType(true, "boolean");
AssertType(false, "boolean");

let f3: B | A | C;
AssertType(f3, "union");

let n3 = f3(42);             // number
AssertType(n3, "number");
AssertType(f3(42), "number");
AssertType(f3, "union");
AssertType(42, "int");

let s3 = f3("abc");          // string | boolean | number
AssertType(s3, "union");
AssertType(f3("abc"), "union");
AssertType(f3, "union");
AssertType("abc", "string");

let a3 = f3([true, false]);  // boolean[]
AssertType(a3, "boolean[]");
AssertType(f3([true, false]), "boolean[]");
AssertType(f3, "union");
AssertType([true, false], "boolean[]");
AssertType(true, "boolean");
AssertType(false, "boolean");



