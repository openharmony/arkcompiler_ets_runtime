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

// === tests/cases/conformance/expressions/binaryOperators/comparisonOperator/comparisonOperatorWithNoRelationshipObjectsOnInstantiatedCallSignature.ts ===
declare function AssertType(value:any, type:string):void;
class Base {
    public a: string;
}

class Derived extends Base {
    public b: string;
}

class C {
    public c: string;
}

let a1: { fn<T>(x: T): T };
AssertType(a1, "{ fn<T>(T): T; }");
AssertType(fn, "<T>(T) => T");
AssertType(x, "T");

let b1: { fn(): string };
AssertType(b1, "{ fn(): string; }");
AssertType(fn, "() => string");

let a2: { fn<T>(x: T): T };
AssertType(a2, "{ fn<T>(T): T; }");
AssertType(fn, "<T>(T) => T");
AssertType(x, "T");

let b2: { fn(x: string): number };
AssertType(b2, "{ fn(string): number; }");
AssertType(fn, "(string) => number");
AssertType(x, "string");

let a3: { fn<T>(x?: T): T };
AssertType(a3, "{ fn<T>(?T): T; }");
AssertType(fn, "<T>(?T) => T");
AssertType(x, "T");

let b3: { fn(x?: string): number };
AssertType(b3, "{ fn(?string): number; }");
AssertType(fn, "(?string) => number");
AssertType(x, "string");

let a4: { fn<T>(...x: T[]): T };
AssertType(a4, "{ fn<T>(...T[]): T; }");
AssertType(fn, "<T>(...T[]) => T");
AssertType(x, "T[]");

let b4: { fn(...x: string[]): number };
AssertType(b4, "{ fn(...string[]): number; }");
AssertType(fn, "(...string[]) => number");
AssertType(x, "string[]");

let a5: { fn<T>(x: T, y: T): T };
AssertType(a5, "{ fn<T>(T, T): T; }");
AssertType(fn, "<T>(T, T) => T");
AssertType(x, "T");
AssertType(y, "T");

let b5: { fn(x: string, y: number): string };
AssertType(b5, "{ fn(string, number): string; }");
AssertType(fn, "(string, number) => string");
AssertType(x, "string");
AssertType(y, "number");

let a6: { fn<T, U extends T>(x: T, y: U): T };
AssertType(a6, "{ fn<T, U extends T>(T, U): T; }");
AssertType(fn, "<T, U extends T>(T, U) => T");
AssertType(x, "T");
AssertType(y, "U");

let b6: { fn(x: Base, y: C): Base };
AssertType(b6, "{ fn(Base, C): Base; }");
AssertType(fn, "(Base, C) => Base");
AssertType(x, "Base");
AssertType(y, "C");

// operator <
let r1a1 = a1 < b1;
AssertType(r1a1, "boolean");
AssertType(a1 < b1, "boolean");
AssertType(a1, "{ fn<T>(T): T; }");
AssertType(b1, "{ fn(): string; }");

let r1a2 = a2 < b2;
AssertType(r1a2, "boolean");
AssertType(a2 < b2, "boolean");
AssertType(a2, "{ fn<T>(T): T; }");
AssertType(b2, "{ fn(string): number; }");

let r1a3 = a3 < b3;
AssertType(r1a3, "boolean");
AssertType(a3 < b3, "boolean");
AssertType(a3, "{ fn<T>(?T): T; }");
AssertType(b3, "{ fn(?string): number; }");

let r1a4 = a4 < b4;
AssertType(r1a4, "boolean");
AssertType(a4 < b4, "boolean");
AssertType(a4, "{ fn<T>(...T[]): T; }");
AssertType(b4, "{ fn(...string[]): number; }");

let r1a5 = a5 < b5;
AssertType(r1a5, "boolean");
AssertType(a5 < b5, "boolean");
AssertType(a5, "{ fn<T>(T, T): T; }");
AssertType(b5, "{ fn(string, number): string; }");

let r1a6 = a6 < b6;
AssertType(r1a6, "boolean");
AssertType(a6 < b6, "boolean");
AssertType(a6, "{ fn<T, U extends T>(T, U): T; }");
AssertType(b6, "{ fn(Base, C): Base; }");

let r1b1 = b1 < a1;
AssertType(r1b1, "boolean");
AssertType(b1 < a1, "boolean");
AssertType(b1, "{ fn(): string; }");
AssertType(a1, "{ fn<T>(T): T; }");

let r1b2 = b2 < a2;
AssertType(r1b2, "boolean");
AssertType(b2 < a2, "boolean");
AssertType(b2, "{ fn(string): number; }");
AssertType(a2, "{ fn<T>(T): T; }");

let r1b3 = b3 < a3;
AssertType(r1b3, "boolean");
AssertType(b3 < a3, "boolean");
AssertType(b3, "{ fn(?string): number; }");
AssertType(a3, "{ fn<T>(?T): T; }");

let r1b4 = b4 < a4;
AssertType(r1b4, "boolean");
AssertType(b4 < a4, "boolean");
AssertType(b4, "{ fn(...string[]): number; }");
AssertType(a4, "{ fn<T>(...T[]): T; }");

let r1b5 = b5 < a5;
AssertType(r1b5, "boolean");
AssertType(b5 < a5, "boolean");
AssertType(b5, "{ fn(string, number): string; }");
AssertType(a5, "{ fn<T>(T, T): T; }");

let r1b6 = b6 < a6;
AssertType(r1b6, "boolean");
AssertType(b6 < a6, "boolean");
AssertType(b6, "{ fn(Base, C): Base; }");
AssertType(a6, "{ fn<T, U extends T>(T, U): T; }");

// operator >
let r2a1 = a1 > b1;
AssertType(r2a1, "boolean");
AssertType(a1 > b1, "boolean");
AssertType(a1, "{ fn<T>(T): T; }");
AssertType(b1, "{ fn(): string; }");

let r2a2 = a2 > b2;
AssertType(r2a2, "boolean");
AssertType(a2 > b2, "boolean");
AssertType(a2, "{ fn<T>(T): T; }");
AssertType(b2, "{ fn(string): number; }");

let r2a3 = a3 > b3;
AssertType(r2a3, "boolean");
AssertType(a3 > b3, "boolean");
AssertType(a3, "{ fn<T>(?T): T; }");
AssertType(b3, "{ fn(?string): number; }");

let r2a4 = a4 > b4;
AssertType(r2a4, "boolean");
AssertType(a4 > b4, "boolean");
AssertType(a4, "{ fn<T>(...T[]): T; }");
AssertType(b4, "{ fn(...string[]): number; }");

let r2a5 = a5 > b5;
AssertType(r2a5, "boolean");
AssertType(a5 > b5, "boolean");
AssertType(a5, "{ fn<T>(T, T): T; }");
AssertType(b5, "{ fn(string, number): string; }");

let r2a6 = a6 > b6;
AssertType(r2a6, "boolean");
AssertType(a6 > b6, "boolean");
AssertType(a6, "{ fn<T, U extends T>(T, U): T; }");
AssertType(b6, "{ fn(Base, C): Base; }");

let r2b1 = b1 > a1;
AssertType(r2b1, "boolean");
AssertType(b1 > a1, "boolean");
AssertType(b1, "{ fn(): string; }");
AssertType(a1, "{ fn<T>(T): T; }");

let r2b2 = b2 > a2;
AssertType(r2b2, "boolean");
AssertType(b2 > a2, "boolean");
AssertType(b2, "{ fn(string): number; }");
AssertType(a2, "{ fn<T>(T): T; }");

let r2b3 = b3 > a3;
AssertType(r2b3, "boolean");
AssertType(b3 > a3, "boolean");
AssertType(b3, "{ fn(?string): number; }");
AssertType(a3, "{ fn<T>(?T): T; }");

let r2b4 = b4 > a4;
AssertType(r2b4, "boolean");
AssertType(b4 > a4, "boolean");
AssertType(b4, "{ fn(...string[]): number; }");
AssertType(a4, "{ fn<T>(...T[]): T; }");

let r2b5 = b5 > a5;
AssertType(r2b5, "boolean");
AssertType(b5 > a5, "boolean");
AssertType(b5, "{ fn(string, number): string; }");
AssertType(a5, "{ fn<T>(T, T): T; }");

let r2b6 = b6 > a6;
AssertType(r2b6, "boolean");
AssertType(b6 > a6, "boolean");
AssertType(b6, "{ fn(Base, C): Base; }");
AssertType(a6, "{ fn<T, U extends T>(T, U): T; }");

// operator <=
let r3a1 = a1 <= b1;
AssertType(r3a1, "boolean");
AssertType(a1 <= b1, "boolean");
AssertType(a1, "{ fn<T>(T): T; }");
AssertType(b1, "{ fn(): string; }");

let r3a2 = a2 <= b2;
AssertType(r3a2, "boolean");
AssertType(a2 <= b2, "boolean");
AssertType(a2, "{ fn<T>(T): T; }");
AssertType(b2, "{ fn(string): number; }");

let r3a3 = a3 <= b3;
AssertType(r3a3, "boolean");
AssertType(a3 <= b3, "boolean");
AssertType(a3, "{ fn<T>(?T): T; }");
AssertType(b3, "{ fn(?string): number; }");

let r3a4 = a4 <= b4;
AssertType(r3a4, "boolean");
AssertType(a4 <= b4, "boolean");
AssertType(a4, "{ fn<T>(...T[]): T; }");
AssertType(b4, "{ fn(...string[]): number; }");

let r3a5 = a5 <= b5;
AssertType(r3a5, "boolean");
AssertType(a5 <= b5, "boolean");
AssertType(a5, "{ fn<T>(T, T): T; }");
AssertType(b5, "{ fn(string, number): string; }");

let r3a6 = a6 <= b6;
AssertType(r3a6, "boolean");
AssertType(a6 <= b6, "boolean");
AssertType(a6, "{ fn<T, U extends T>(T, U): T; }");
AssertType(b6, "{ fn(Base, C): Base; }");

let r3b1 = b1 <= a1;
AssertType(r3b1, "boolean");
AssertType(b1 <= a1, "boolean");
AssertType(b1, "{ fn(): string; }");
AssertType(a1, "{ fn<T>(T): T; }");

let r3b2 = b2 <= a2;
AssertType(r3b2, "boolean");
AssertType(b2 <= a2, "boolean");
AssertType(b2, "{ fn(string): number; }");
AssertType(a2, "{ fn<T>(T): T; }");

let r3b3 = b3 <= a3;
AssertType(r3b3, "boolean");
AssertType(b3 <= a3, "boolean");
AssertType(b3, "{ fn(?string): number; }");
AssertType(a3, "{ fn<T>(?T): T; }");

let r3b4 = b4 <= a4;
AssertType(r3b4, "boolean");
AssertType(b4 <= a4, "boolean");
AssertType(b4, "{ fn(...string[]): number; }");
AssertType(a4, "{ fn<T>(...T[]): T; }");

let r3b5 = b5 <= a5;
AssertType(r3b5, "boolean");
AssertType(b5 <= a5, "boolean");
AssertType(b5, "{ fn(string, number): string; }");
AssertType(a5, "{ fn<T>(T, T): T; }");

let r3b6 = b6 <= a6;
AssertType(r3b6, "boolean");
AssertType(b6 <= a6, "boolean");
AssertType(b6, "{ fn(Base, C): Base; }");
AssertType(a6, "{ fn<T, U extends T>(T, U): T; }");

// operator >=
let r4a1 = a1 >= b1;
AssertType(r4a1, "boolean");
AssertType(a1 >= b1, "boolean");
AssertType(a1, "{ fn<T>(T): T; }");
AssertType(b1, "{ fn(): string; }");

let r4a2 = a2 >= b2;
AssertType(r4a2, "boolean");
AssertType(a2 >= b2, "boolean");
AssertType(a2, "{ fn<T>(T): T; }");
AssertType(b2, "{ fn(string): number; }");

let r4a3 = a3 >= b3;
AssertType(r4a3, "boolean");
AssertType(a3 >= b3, "boolean");
AssertType(a3, "{ fn<T>(?T): T; }");
AssertType(b3, "{ fn(?string): number; }");

let r4a4 = a4 >= b4;
AssertType(r4a4, "boolean");
AssertType(a4 >= b4, "boolean");
AssertType(a4, "{ fn<T>(...T[]): T; }");
AssertType(b4, "{ fn(...string[]): number; }");

let r4a5 = a5 >= b5;
AssertType(r4a5, "boolean");
AssertType(a5 >= b5, "boolean");
AssertType(a5, "{ fn<T>(T, T): T; }");
AssertType(b5, "{ fn(string, number): string; }");

let r4a6 = a6 >= b6;
AssertType(r4a6, "boolean");
AssertType(a6 >= b6, "boolean");
AssertType(a6, "{ fn<T, U extends T>(T, U): T; }");
AssertType(b6, "{ fn(Base, C): Base; }");

let r4b1 = b1 >= a1;
AssertType(r4b1, "boolean");
AssertType(b1 >= a1, "boolean");
AssertType(b1, "{ fn(): string; }");
AssertType(a1, "{ fn<T>(T): T; }");

let r4b2 = b2 >= a2;
AssertType(r4b2, "boolean");
AssertType(b2 >= a2, "boolean");
AssertType(b2, "{ fn(string): number; }");
AssertType(a2, "{ fn<T>(T): T; }");

let r4b3 = b3 >= a3;
AssertType(r4b3, "boolean");
AssertType(b3 >= a3, "boolean");
AssertType(b3, "{ fn(?string): number; }");
AssertType(a3, "{ fn<T>(?T): T; }");

let r4b4 = b4 >= a4;
AssertType(r4b4, "boolean");
AssertType(b4 >= a4, "boolean");
AssertType(b4, "{ fn(...string[]): number; }");
AssertType(a4, "{ fn<T>(...T[]): T; }");

let r4b5 = b5 >= a5;
AssertType(r4b5, "boolean");
AssertType(b5 >= a5, "boolean");
AssertType(b5, "{ fn(string, number): string; }");
AssertType(a5, "{ fn<T>(T, T): T; }");

let r4b6 = b6 >= a6;
AssertType(r4b6, "boolean");
AssertType(b6 >= a6, "boolean");
AssertType(b6, "{ fn(Base, C): Base; }");
AssertType(a6, "{ fn<T, U extends T>(T, U): T; }");

// operator ==
let r5a1 = a1 == b1;
AssertType(r5a1, "boolean");
AssertType(a1 == b1, "boolean");
AssertType(a1, "{ fn<T>(T): T; }");
AssertType(b1, "{ fn(): string; }");

let r5a2 = a2 == b2;
AssertType(r5a2, "boolean");
AssertType(a2 == b2, "boolean");
AssertType(a2, "{ fn<T>(T): T; }");
AssertType(b2, "{ fn(string): number; }");

let r5a3 = a3 == b3;
AssertType(r5a3, "boolean");
AssertType(a3 == b3, "boolean");
AssertType(a3, "{ fn<T>(?T): T; }");
AssertType(b3, "{ fn(?string): number; }");

let r5a4 = a4 == b4;
AssertType(r5a4, "boolean");
AssertType(a4 == b4, "boolean");
AssertType(a4, "{ fn<T>(...T[]): T; }");
AssertType(b4, "{ fn(...string[]): number; }");

let r5a5 = a5 == b5;
AssertType(r5a5, "boolean");
AssertType(a5 == b5, "boolean");
AssertType(a5, "{ fn<T>(T, T): T; }");
AssertType(b5, "{ fn(string, number): string; }");

let r5a6 = a6 == b6;
AssertType(r5a6, "boolean");
AssertType(a6 == b6, "boolean");
AssertType(a6, "{ fn<T, U extends T>(T, U): T; }");
AssertType(b6, "{ fn(Base, C): Base; }");

let r5b1 = b1 == a1;
AssertType(r5b1, "boolean");
AssertType(b1 == a1, "boolean");
AssertType(b1, "{ fn(): string; }");
AssertType(a1, "{ fn<T>(T): T; }");

let r5b2 = b2 == a2;
AssertType(r5b2, "boolean");
AssertType(b2 == a2, "boolean");
AssertType(b2, "{ fn(string): number; }");
AssertType(a2, "{ fn<T>(T): T; }");

let r5b3 = b3 == a3;
AssertType(r5b3, "boolean");
AssertType(b3 == a3, "boolean");
AssertType(b3, "{ fn(?string): number; }");
AssertType(a3, "{ fn<T>(?T): T; }");

let r5b4 = b4 == a4;
AssertType(r5b4, "boolean");
AssertType(b4 == a4, "boolean");
AssertType(b4, "{ fn(...string[]): number; }");
AssertType(a4, "{ fn<T>(...T[]): T; }");

let r5b5 = b5 == a5;
AssertType(r5b5, "boolean");
AssertType(b5 == a5, "boolean");
AssertType(b5, "{ fn(string, number): string; }");
AssertType(a5, "{ fn<T>(T, T): T; }");

let r5b6 = b6 == a6;
AssertType(r5b6, "boolean");
AssertType(b6 == a6, "boolean");
AssertType(b6, "{ fn(Base, C): Base; }");
AssertType(a6, "{ fn<T, U extends T>(T, U): T; }");

// operator !=
let r6a1 = a1 != b1;
AssertType(r6a1, "boolean");
AssertType(a1 != b1, "boolean");
AssertType(a1, "{ fn<T>(T): T; }");
AssertType(b1, "{ fn(): string; }");

let r6a2 = a2 != b2;
AssertType(r6a2, "boolean");
AssertType(a2 != b2, "boolean");
AssertType(a2, "{ fn<T>(T): T; }");
AssertType(b2, "{ fn(string): number; }");

let r6a3 = a3 != b3;
AssertType(r6a3, "boolean");
AssertType(a3 != b3, "boolean");
AssertType(a3, "{ fn<T>(?T): T; }");
AssertType(b3, "{ fn(?string): number; }");

let r6a4 = a4 != b4;
AssertType(r6a4, "boolean");
AssertType(a4 != b4, "boolean");
AssertType(a4, "{ fn<T>(...T[]): T; }");
AssertType(b4, "{ fn(...string[]): number; }");

let r6a5 = a5 != b5;
AssertType(r6a5, "boolean");
AssertType(a5 != b5, "boolean");
AssertType(a5, "{ fn<T>(T, T): T; }");
AssertType(b5, "{ fn(string, number): string; }");

let r6a6 = a6 != b6;
AssertType(r6a6, "boolean");
AssertType(a6 != b6, "boolean");
AssertType(a6, "{ fn<T, U extends T>(T, U): T; }");
AssertType(b6, "{ fn(Base, C): Base; }");

let r6b1 = b1 != a1;
AssertType(r6b1, "boolean");
AssertType(b1 != a1, "boolean");
AssertType(b1, "{ fn(): string; }");
AssertType(a1, "{ fn<T>(T): T; }");

let r6b2 = b2 != a2;
AssertType(r6b2, "boolean");
AssertType(b2 != a2, "boolean");
AssertType(b2, "{ fn(string): number; }");
AssertType(a2, "{ fn<T>(T): T; }");

let r6b3 = b3 != a3;
AssertType(r6b3, "boolean");
AssertType(b3 != a3, "boolean");
AssertType(b3, "{ fn(?string): number; }");
AssertType(a3, "{ fn<T>(?T): T; }");

let r6b4 = b4 != a4;
AssertType(r6b4, "boolean");
AssertType(b4 != a4, "boolean");
AssertType(b4, "{ fn(...string[]): number; }");
AssertType(a4, "{ fn<T>(...T[]): T; }");

let r6b5 = b5 != a5;
AssertType(r6b5, "boolean");
AssertType(b5 != a5, "boolean");
AssertType(b5, "{ fn(string, number): string; }");
AssertType(a5, "{ fn<T>(T, T): T; }");

let r6b6 = b6 != a6;
AssertType(r6b6, "boolean");
AssertType(b6 != a6, "boolean");
AssertType(b6, "{ fn(Base, C): Base; }");
AssertType(a6, "{ fn<T, U extends T>(T, U): T; }");

// operator ===
let r7a1 = a1 === b1;
AssertType(r7a1, "boolean");
AssertType(a1 === b1, "boolean");
AssertType(a1, "{ fn<T>(T): T; }");
AssertType(b1, "{ fn(): string; }");

let r7a2 = a2 === b2;
AssertType(r7a2, "boolean");
AssertType(a2 === b2, "boolean");
AssertType(a2, "{ fn<T>(T): T; }");
AssertType(b2, "{ fn(string): number; }");

let r7a3 = a3 === b3;
AssertType(r7a3, "boolean");
AssertType(a3 === b3, "boolean");
AssertType(a3, "{ fn<T>(?T): T; }");
AssertType(b3, "{ fn(?string): number; }");

let r7a4 = a4 === b4;
AssertType(r7a4, "boolean");
AssertType(a4 === b4, "boolean");
AssertType(a4, "{ fn<T>(...T[]): T; }");
AssertType(b4, "{ fn(...string[]): number; }");

let r7a5 = a5 === b5;
AssertType(r7a5, "boolean");
AssertType(a5 === b5, "boolean");
AssertType(a5, "{ fn<T>(T, T): T; }");
AssertType(b5, "{ fn(string, number): string; }");

let r7a6 = a6 === b6;
AssertType(r7a6, "boolean");
AssertType(a6 === b6, "boolean");
AssertType(a6, "{ fn<T, U extends T>(T, U): T; }");
AssertType(b6, "{ fn(Base, C): Base; }");

let r7b1 = b1 === a1;
AssertType(r7b1, "boolean");
AssertType(b1 === a1, "boolean");
AssertType(b1, "{ fn(): string; }");
AssertType(a1, "{ fn<T>(T): T; }");

let r7b2 = b2 === a2;
AssertType(r7b2, "boolean");
AssertType(b2 === a2, "boolean");
AssertType(b2, "{ fn(string): number; }");
AssertType(a2, "{ fn<T>(T): T; }");

let r7b3 = b3 === a3;
AssertType(r7b3, "boolean");
AssertType(b3 === a3, "boolean");
AssertType(b3, "{ fn(?string): number; }");
AssertType(a3, "{ fn<T>(?T): T; }");

let r7b4 = b4 === a4;
AssertType(r7b4, "boolean");
AssertType(b4 === a4, "boolean");
AssertType(b4, "{ fn(...string[]): number; }");
AssertType(a4, "{ fn<T>(...T[]): T; }");

let r7b5 = b5 === a5;
AssertType(r7b5, "boolean");
AssertType(b5 === a5, "boolean");
AssertType(b5, "{ fn(string, number): string; }");
AssertType(a5, "{ fn<T>(T, T): T; }");

let r7b6 = b6 === a6;
AssertType(r7b6, "boolean");
AssertType(b6 === a6, "boolean");
AssertType(b6, "{ fn(Base, C): Base; }");
AssertType(a6, "{ fn<T, U extends T>(T, U): T; }");

// operator !==
let r8a1 = a1 !== b1;
AssertType(r8a1, "boolean");
AssertType(a1 !== b1, "boolean");
AssertType(a1, "{ fn<T>(T): T; }");
AssertType(b1, "{ fn(): string; }");

let r8a2 = a2 !== b2;
AssertType(r8a2, "boolean");
AssertType(a2 !== b2, "boolean");
AssertType(a2, "{ fn<T>(T): T; }");
AssertType(b2, "{ fn(string): number; }");

let r8a3 = a3 !== b3;
AssertType(r8a3, "boolean");
AssertType(a3 !== b3, "boolean");
AssertType(a3, "{ fn<T>(?T): T; }");
AssertType(b3, "{ fn(?string): number; }");

let r8a4 = a4 !== b4;
AssertType(r8a4, "boolean");
AssertType(a4 !== b4, "boolean");
AssertType(a4, "{ fn<T>(...T[]): T; }");
AssertType(b4, "{ fn(...string[]): number; }");

let r8a5 = a5 !== b5;
AssertType(r8a5, "boolean");
AssertType(a5 !== b5, "boolean");
AssertType(a5, "{ fn<T>(T, T): T; }");
AssertType(b5, "{ fn(string, number): string; }");

let r8a6 = a6 !== b6;
AssertType(r8a6, "boolean");
AssertType(a6 !== b6, "boolean");
AssertType(a6, "{ fn<T, U extends T>(T, U): T; }");
AssertType(b6, "{ fn(Base, C): Base; }");

let r8b1 = b1 !== a1;
AssertType(r8b1, "boolean");
AssertType(b1 !== a1, "boolean");
AssertType(b1, "{ fn(): string; }");
AssertType(a1, "{ fn<T>(T): T; }");

let r8b2 = b2 !== a2;
AssertType(r8b2, "boolean");
AssertType(b2 !== a2, "boolean");
AssertType(b2, "{ fn(string): number; }");
AssertType(a2, "{ fn<T>(T): T; }");

let r8b3 = b3 !== a3;
AssertType(r8b3, "boolean");
AssertType(b3 !== a3, "boolean");
AssertType(b3, "{ fn(?string): number; }");
AssertType(a3, "{ fn<T>(?T): T; }");

let r8b4 = b4 !== a4;
AssertType(r8b4, "boolean");
AssertType(b4 !== a4, "boolean");
AssertType(b4, "{ fn(...string[]): number; }");
AssertType(a4, "{ fn<T>(...T[]): T; }");

let r8b5 = b5 !== a5;
AssertType(r8b5, "boolean");
AssertType(b5 !== a5, "boolean");
AssertType(b5, "{ fn(string, number): string; }");
AssertType(a5, "{ fn<T>(T, T): T; }");

let r8b6 = b6 !== a6;
AssertType(r8b6, "boolean");
AssertType(b6 !== a6, "boolean");
AssertType(b6, "{ fn(Base, C): Base; }");
AssertType(a6, "{ fn<T, U extends T>(T, U): T; }");


