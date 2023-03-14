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

// === tests/cases/conformance/expressions/binaryOperators/comparisonOperator/comparisonOperatorWithSubtypeObjectOnConstructorSignature.ts ===
declare function AssertType(value:any, type:string):void;
class Base {
    public a: string;
}

class Derived extends Base {
    public b: string;
}

let a1: { new (): Base };
AssertType(a1, "new () => Base");

let b1: { new (): Base };
AssertType(b1, "new () => Base");

let a2: { new (a: number, b: string): Base };
AssertType(a2, "new (number, string) => Base");
AssertType(a, "number");
AssertType(b, "string");

let b2: { new (a: number, b: string): Base };
AssertType(b2, "new (number, string) => Base");
AssertType(a, "number");
AssertType(b, "string");

let a3: { new (a: number, b: string): Base };
AssertType(a3, "new (number, string) => Base");
AssertType(a, "number");
AssertType(b, "string");

let b3: { new (a: number): Base };
AssertType(b3, "new (number) => Base");
AssertType(a, "number");

let a4: { new (a: number, b: string): Base };
AssertType(a4, "new (number, string) => Base");
AssertType(a, "number");
AssertType(b, "string");

let b4: { new (): Base };
AssertType(b4, "new () => Base");

let a5: { new (a: Base): Base };
AssertType(a5, "new (Base) => Base");
AssertType(a, "Base");

let b5: { new (a: Derived): Base };
AssertType(b5, "new (Derived) => Base");
AssertType(a, "Derived");

let a6: { new (a: Derived, b: Base): Base };
AssertType(a6, "new (Derived, Base) => Base");
AssertType(a, "Derived");
AssertType(b, "Base");

let b6: { new (a: Base, b: Derived): Base };
AssertType(b6, "new (Base, Derived) => Base");
AssertType(a, "Base");
AssertType(b, "Derived");

let a7: { new (): Base };
AssertType(a7, "new () => Base");

let b7: { new (): Derived };
AssertType(b7, "new () => Derived");

let a8: { new (a?: Base): Base };
AssertType(a8, "new (?Base) => Base");
AssertType(a, "Base");

let b8: { new (a?: Derived): Base };
AssertType(b8, "new (?Derived) => Base");
AssertType(a, "Derived");

let a9: { new (...a: Base[]): Base };
AssertType(a9, "new (...Base[]) => Base");
AssertType(a, "Base[]");

let b9: { new (...a: Derived[]): Base };
AssertType(b9, "new (...Derived[]) => Base");
AssertType(a, "Derived[]");

// operator <
let r1a1 = a1 < b1;
AssertType(r1a1, "boolean");
AssertType(a1 < b1, "boolean");
AssertType(a1, "new () => Base");
AssertType(b1, "new () => Base");

let r1a2 = a2 < b2;
AssertType(r1a2, "boolean");
AssertType(a2 < b2, "boolean");
AssertType(a2, "new (number, string) => Base");
AssertType(b2, "new (number, string) => Base");

let r1a3 = a3 < b3;
AssertType(r1a3, "boolean");
AssertType(a3 < b3, "boolean");
AssertType(a3, "new (number, string) => Base");
AssertType(b3, "new (number) => Base");

let r1a4 = a4 < b4;
AssertType(r1a4, "boolean");
AssertType(a4 < b4, "boolean");
AssertType(a4, "new (number, string) => Base");
AssertType(b4, "new () => Base");

let r1a5 = a5 < b5;
AssertType(r1a5, "boolean");
AssertType(a5 < b5, "boolean");
AssertType(a5, "new (Base) => Base");
AssertType(b5, "new (Derived) => Base");

let r1a6 = a6 < b6;
AssertType(r1a6, "boolean");
AssertType(a6 < b6, "boolean");
AssertType(a6, "new (Derived, Base) => Base");
AssertType(b6, "new (Base, Derived) => Base");

let r1a7 = a7 < b7;
AssertType(r1a7, "boolean");
AssertType(a7 < b7, "boolean");
AssertType(a7, "new () => Base");
AssertType(b7, "new () => Derived");

let r1a8 = a8 < b8;
AssertType(r1a8, "boolean");
AssertType(a8 < b8, "boolean");
AssertType(a8, "new (?Base) => Base");
AssertType(b8, "new (?Derived) => Base");

let r1a9 = a9 < b9;
AssertType(r1a9, "boolean");
AssertType(a9 < b9, "boolean");
AssertType(a9, "new (...Base[]) => Base");
AssertType(b9, "new (...Derived[]) => Base");

let r1b1 = b1 < a1;
AssertType(r1b1, "boolean");
AssertType(b1 < a1, "boolean");
AssertType(b1, "new () => Base");
AssertType(a1, "new () => Base");

let r1b2 = b2 < a2;
AssertType(r1b2, "boolean");
AssertType(b2 < a2, "boolean");
AssertType(b2, "new (number, string) => Base");
AssertType(a2, "new (number, string) => Base");

let r1b3 = b3 < a3;
AssertType(r1b3, "boolean");
AssertType(b3 < a3, "boolean");
AssertType(b3, "new (number) => Base");
AssertType(a3, "new (number, string) => Base");

let r1b4 = b4 < a4;
AssertType(r1b4, "boolean");
AssertType(b4 < a4, "boolean");
AssertType(b4, "new () => Base");
AssertType(a4, "new (number, string) => Base");

let r1b5 = b5 < a5;
AssertType(r1b5, "boolean");
AssertType(b5 < a5, "boolean");
AssertType(b5, "new (Derived) => Base");
AssertType(a5, "new (Base) => Base");

let r1b6 = b6 < a6;
AssertType(r1b6, "boolean");
AssertType(b6 < a6, "boolean");
AssertType(b6, "new (Base, Derived) => Base");
AssertType(a6, "new (Derived, Base) => Base");

let r1b7 = b7 < a7;
AssertType(r1b7, "boolean");
AssertType(b7 < a7, "boolean");
AssertType(b7, "new () => Derived");
AssertType(a7, "new () => Base");

let r1b8 = b8 < a8;
AssertType(r1b8, "boolean");
AssertType(b8 < a8, "boolean");
AssertType(b8, "new (?Derived) => Base");
AssertType(a8, "new (?Base) => Base");

let r1b9 = b9 < a9;
AssertType(r1b9, "boolean");
AssertType(b9 < a9, "boolean");
AssertType(b9, "new (...Derived[]) => Base");
AssertType(a9, "new (...Base[]) => Base");

// operator >
let r2a1 = a1 > b1;
AssertType(r2a1, "boolean");
AssertType(a1 > b1, "boolean");
AssertType(a1, "new () => Base");
AssertType(b1, "new () => Base");

let r2a2 = a2 > b2;
AssertType(r2a2, "boolean");
AssertType(a2 > b2, "boolean");
AssertType(a2, "new (number, string) => Base");
AssertType(b2, "new (number, string) => Base");

let r2a3 = a3 > b3;
AssertType(r2a3, "boolean");
AssertType(a3 > b3, "boolean");
AssertType(a3, "new (number, string) => Base");
AssertType(b3, "new (number) => Base");

let r2a4 = a4 > b4;
AssertType(r2a4, "boolean");
AssertType(a4 > b4, "boolean");
AssertType(a4, "new (number, string) => Base");
AssertType(b4, "new () => Base");

let r2a5 = a5 > b5;
AssertType(r2a5, "boolean");
AssertType(a5 > b5, "boolean");
AssertType(a5, "new (Base) => Base");
AssertType(b5, "new (Derived) => Base");

let r2a6 = a6 > b6;
AssertType(r2a6, "boolean");
AssertType(a6 > b6, "boolean");
AssertType(a6, "new (Derived, Base) => Base");
AssertType(b6, "new (Base, Derived) => Base");

let r2a7 = a7 > b7;
AssertType(r2a7, "boolean");
AssertType(a7 > b7, "boolean");
AssertType(a7, "new () => Base");
AssertType(b7, "new () => Derived");

let r2a8 = a8 > b8;
AssertType(r2a8, "boolean");
AssertType(a8 > b8, "boolean");
AssertType(a8, "new (?Base) => Base");
AssertType(b8, "new (?Derived) => Base");

let r2a9 = a9 > b9;
AssertType(r2a9, "boolean");
AssertType(a9 > b9, "boolean");
AssertType(a9, "new (...Base[]) => Base");
AssertType(b9, "new (...Derived[]) => Base");

let r2b1 = b1 > a1;
AssertType(r2b1, "boolean");
AssertType(b1 > a1, "boolean");
AssertType(b1, "new () => Base");
AssertType(a1, "new () => Base");

let r2b2 = b2 > a2;
AssertType(r2b2, "boolean");
AssertType(b2 > a2, "boolean");
AssertType(b2, "new (number, string) => Base");
AssertType(a2, "new (number, string) => Base");

let r2b3 = b3 > a3;
AssertType(r2b3, "boolean");
AssertType(b3 > a3, "boolean");
AssertType(b3, "new (number) => Base");
AssertType(a3, "new (number, string) => Base");

let r2b4 = b4 > a4;
AssertType(r2b4, "boolean");
AssertType(b4 > a4, "boolean");
AssertType(b4, "new () => Base");
AssertType(a4, "new (number, string) => Base");

let r2b5 = b5 > a5;
AssertType(r2b5, "boolean");
AssertType(b5 > a5, "boolean");
AssertType(b5, "new (Derived) => Base");
AssertType(a5, "new (Base) => Base");

let r2b6 = b6 > a6;
AssertType(r2b6, "boolean");
AssertType(b6 > a6, "boolean");
AssertType(b6, "new (Base, Derived) => Base");
AssertType(a6, "new (Derived, Base) => Base");

let r2b7 = b7 > a7;
AssertType(r2b7, "boolean");
AssertType(b7 > a7, "boolean");
AssertType(b7, "new () => Derived");
AssertType(a7, "new () => Base");

let r2b8 = b8 > a8;
AssertType(r2b8, "boolean");
AssertType(b8 > a8, "boolean");
AssertType(b8, "new (?Derived) => Base");
AssertType(a8, "new (?Base) => Base");

let r2b9 = b9 > a9;
AssertType(r2b9, "boolean");
AssertType(b9 > a9, "boolean");
AssertType(b9, "new (...Derived[]) => Base");
AssertType(a9, "new (...Base[]) => Base");

// operator <=
let r3a1 = a1 <= b1;
AssertType(r3a1, "boolean");
AssertType(a1 <= b1, "boolean");
AssertType(a1, "new () => Base");
AssertType(b1, "new () => Base");

let r3a2 = a2 <= b2;
AssertType(r3a2, "boolean");
AssertType(a2 <= b2, "boolean");
AssertType(a2, "new (number, string) => Base");
AssertType(b2, "new (number, string) => Base");

let r3a3 = a3 <= b3;
AssertType(r3a3, "boolean");
AssertType(a3 <= b3, "boolean");
AssertType(a3, "new (number, string) => Base");
AssertType(b3, "new (number) => Base");

let r3a4 = a4 <= b4;
AssertType(r3a4, "boolean");
AssertType(a4 <= b4, "boolean");
AssertType(a4, "new (number, string) => Base");
AssertType(b4, "new () => Base");

let r3a5 = a5 <= b5;
AssertType(r3a5, "boolean");
AssertType(a5 <= b5, "boolean");
AssertType(a5, "new (Base) => Base");
AssertType(b5, "new (Derived) => Base");

let r3a6 = a6 <= b6;
AssertType(r3a6, "boolean");
AssertType(a6 <= b6, "boolean");
AssertType(a6, "new (Derived, Base) => Base");
AssertType(b6, "new (Base, Derived) => Base");

let r3a7 = a7 <= b7;
AssertType(r3a7, "boolean");
AssertType(a7 <= b7, "boolean");
AssertType(a7, "new () => Base");
AssertType(b7, "new () => Derived");

let r3a8 = a8 <= b8;
AssertType(r3a8, "boolean");
AssertType(a8 <= b8, "boolean");
AssertType(a8, "new (?Base) => Base");
AssertType(b8, "new (?Derived) => Base");

let r3a9 = a9 <= b9;
AssertType(r3a9, "boolean");
AssertType(a9 <= b9, "boolean");
AssertType(a9, "new (...Base[]) => Base");
AssertType(b9, "new (...Derived[]) => Base");

//let r3a10 = a10 <= b10;

let r3b1 = b1 <= a1;
AssertType(r3b1, "boolean");
AssertType(b1 <= a1, "boolean");
AssertType(b1, "new () => Base");
AssertType(a1, "new () => Base");

let r3b2 = b2 <= a2;
AssertType(r3b2, "boolean");
AssertType(b2 <= a2, "boolean");
AssertType(b2, "new (number, string) => Base");
AssertType(a2, "new (number, string) => Base");

let r3b3 = b3 <= a3;
AssertType(r3b3, "boolean");
AssertType(b3 <= a3, "boolean");
AssertType(b3, "new (number) => Base");
AssertType(a3, "new (number, string) => Base");

let r3b4 = b4 <= a4;
AssertType(r3b4, "boolean");
AssertType(b4 <= a4, "boolean");
AssertType(b4, "new () => Base");
AssertType(a4, "new (number, string) => Base");

let r3b5 = b5 <= a5;
AssertType(r3b5, "boolean");
AssertType(b5 <= a5, "boolean");
AssertType(b5, "new (Derived) => Base");
AssertType(a5, "new (Base) => Base");

let r3b6 = b6 <= a6;
AssertType(r3b6, "boolean");
AssertType(b6 <= a6, "boolean");
AssertType(b6, "new (Base, Derived) => Base");
AssertType(a6, "new (Derived, Base) => Base");

let r3b7 = b7 <= a7;
AssertType(r3b7, "boolean");
AssertType(b7 <= a7, "boolean");
AssertType(b7, "new () => Derived");
AssertType(a7, "new () => Base");

let r3b8 = b8 <= a8;
AssertType(r3b8, "boolean");
AssertType(b8 <= a8, "boolean");
AssertType(b8, "new (?Derived) => Base");
AssertType(a8, "new (?Base) => Base");

let r3b9 = b9 <= a9;
AssertType(r3b9, "boolean");
AssertType(b9 <= a9, "boolean");
AssertType(b9, "new (...Derived[]) => Base");
AssertType(a9, "new (...Base[]) => Base");

//let r3b10 = b10 <= a10;

// operator >=
let r4a1 = a1 >= b1;
AssertType(r4a1, "boolean");
AssertType(a1 >= b1, "boolean");
AssertType(a1, "new () => Base");
AssertType(b1, "new () => Base");

let r4a2 = a2 >= b2;
AssertType(r4a2, "boolean");
AssertType(a2 >= b2, "boolean");
AssertType(a2, "new (number, string) => Base");
AssertType(b2, "new (number, string) => Base");

let r4a3 = a3 >= b3;
AssertType(r4a3, "boolean");
AssertType(a3 >= b3, "boolean");
AssertType(a3, "new (number, string) => Base");
AssertType(b3, "new (number) => Base");

let r4a4 = a4 >= b4;
AssertType(r4a4, "boolean");
AssertType(a4 >= b4, "boolean");
AssertType(a4, "new (number, string) => Base");
AssertType(b4, "new () => Base");

let r4a5 = a5 >= b5;
AssertType(r4a5, "boolean");
AssertType(a5 >= b5, "boolean");
AssertType(a5, "new (Base) => Base");
AssertType(b5, "new (Derived) => Base");

let r4a6 = a6 >= b6;
AssertType(r4a6, "boolean");
AssertType(a6 >= b6, "boolean");
AssertType(a6, "new (Derived, Base) => Base");
AssertType(b6, "new (Base, Derived) => Base");

let r4a7 = a7 >= b7;
AssertType(r4a7, "boolean");
AssertType(a7 >= b7, "boolean");
AssertType(a7, "new () => Base");
AssertType(b7, "new () => Derived");

let r4a8 = a8 >= b8;
AssertType(r4a8, "boolean");
AssertType(a8 >= b8, "boolean");
AssertType(a8, "new (?Base) => Base");
AssertType(b8, "new (?Derived) => Base");

let r4a9 = a9 >= b9;
AssertType(r4a9, "boolean");
AssertType(a9 >= b9, "boolean");
AssertType(a9, "new (...Base[]) => Base");
AssertType(b9, "new (...Derived[]) => Base");

//let r4a10 = a10 >= b10;

let r4b1 = b1 >= a1;
AssertType(r4b1, "boolean");
AssertType(b1 >= a1, "boolean");
AssertType(b1, "new () => Base");
AssertType(a1, "new () => Base");

let r4b2 = b2 >= a2;
AssertType(r4b2, "boolean");
AssertType(b2 >= a2, "boolean");
AssertType(b2, "new (number, string) => Base");
AssertType(a2, "new (number, string) => Base");

let r4b3 = b3 >= a3;
AssertType(r4b3, "boolean");
AssertType(b3 >= a3, "boolean");
AssertType(b3, "new (number) => Base");
AssertType(a3, "new (number, string) => Base");

let r4b4 = b4 >= a4;
AssertType(r4b4, "boolean");
AssertType(b4 >= a4, "boolean");
AssertType(b4, "new () => Base");
AssertType(a4, "new (number, string) => Base");

let r4b5 = b5 >= a5;
AssertType(r4b5, "boolean");
AssertType(b5 >= a5, "boolean");
AssertType(b5, "new (Derived) => Base");
AssertType(a5, "new (Base) => Base");

let r4b6 = b6 >= a6;
AssertType(r4b6, "boolean");
AssertType(b6 >= a6, "boolean");
AssertType(b6, "new (Base, Derived) => Base");
AssertType(a6, "new (Derived, Base) => Base");

let r4b7 = b7 >= a7;
AssertType(r4b7, "boolean");
AssertType(b7 >= a7, "boolean");
AssertType(b7, "new () => Derived");
AssertType(a7, "new () => Base");

let r4b8 = b8 >= a8;
AssertType(r4b8, "boolean");
AssertType(b8 >= a8, "boolean");
AssertType(b8, "new (?Derived) => Base");
AssertType(a8, "new (?Base) => Base");

let r4b9 = b9 >= a9;
AssertType(r4b9, "boolean");
AssertType(b9 >= a9, "boolean");
AssertType(b9, "new (...Derived[]) => Base");
AssertType(a9, "new (...Base[]) => Base");

//let r4b10 = b10 >= a10;

// operator ==
let r5a1 = a1 == b1;
AssertType(r5a1, "boolean");
AssertType(a1 == b1, "boolean");
AssertType(a1, "new () => Base");
AssertType(b1, "new () => Base");

let r5a2 = a2 == b2;
AssertType(r5a2, "boolean");
AssertType(a2 == b2, "boolean");
AssertType(a2, "new (number, string) => Base");
AssertType(b2, "new (number, string) => Base");

let r5a3 = a3 == b3;
AssertType(r5a3, "boolean");
AssertType(a3 == b3, "boolean");
AssertType(a3, "new (number, string) => Base");
AssertType(b3, "new (number) => Base");

let r5a4 = a4 == b4;
AssertType(r5a4, "boolean");
AssertType(a4 == b4, "boolean");
AssertType(a4, "new (number, string) => Base");
AssertType(b4, "new () => Base");

let r5a5 = a5 == b5;
AssertType(r5a5, "boolean");
AssertType(a5 == b5, "boolean");
AssertType(a5, "new (Base) => Base");
AssertType(b5, "new (Derived) => Base");

let r5a6 = a6 == b6;
AssertType(r5a6, "boolean");
AssertType(a6 == b6, "boolean");
AssertType(a6, "new (Derived, Base) => Base");
AssertType(b6, "new (Base, Derived) => Base");

let r5a7 = a7 == b7;
AssertType(r5a7, "boolean");
AssertType(a7 == b7, "boolean");
AssertType(a7, "new () => Base");
AssertType(b7, "new () => Derived");

let r5a8 = a8 == b8;
AssertType(r5a8, "boolean");
AssertType(a8 == b8, "boolean");
AssertType(a8, "new (?Base) => Base");
AssertType(b8, "new (?Derived) => Base");

let r5a9 = a9 == b9;
AssertType(r5a9, "boolean");
AssertType(a9 == b9, "boolean");
AssertType(a9, "new (...Base[]) => Base");
AssertType(b9, "new (...Derived[]) => Base");

//let r5a10 = a10 == b10;

let r5b1 = b1 == a1;
AssertType(r5b1, "boolean");
AssertType(b1 == a1, "boolean");
AssertType(b1, "new () => Base");
AssertType(a1, "new () => Base");

let r5b2 = b2 == a2;
AssertType(r5b2, "boolean");
AssertType(b2 == a2, "boolean");
AssertType(b2, "new (number, string) => Base");
AssertType(a2, "new (number, string) => Base");

let r5b3 = b3 == a3;
AssertType(r5b3, "boolean");
AssertType(b3 == a3, "boolean");
AssertType(b3, "new (number) => Base");
AssertType(a3, "new (number, string) => Base");

let r5b4 = b4 == a4;
AssertType(r5b4, "boolean");
AssertType(b4 == a4, "boolean");
AssertType(b4, "new () => Base");
AssertType(a4, "new (number, string) => Base");

let r5b5 = b5 == a5;
AssertType(r5b5, "boolean");
AssertType(b5 == a5, "boolean");
AssertType(b5, "new (Derived) => Base");
AssertType(a5, "new (Base) => Base");

let r5b6 = b6 == a6;
AssertType(r5b6, "boolean");
AssertType(b6 == a6, "boolean");
AssertType(b6, "new (Base, Derived) => Base");
AssertType(a6, "new (Derived, Base) => Base");

let r5b7 = b7 == a7;
AssertType(r5b7, "boolean");
AssertType(b7 == a7, "boolean");
AssertType(b7, "new () => Derived");
AssertType(a7, "new () => Base");

let r5b8 = b8 == a8;
AssertType(r5b8, "boolean");
AssertType(b8 == a8, "boolean");
AssertType(b8, "new (?Derived) => Base");
AssertType(a8, "new (?Base) => Base");

let r5b9 = b9 == a9;
AssertType(r5b9, "boolean");
AssertType(b9 == a9, "boolean");
AssertType(b9, "new (...Derived[]) => Base");
AssertType(a9, "new (...Base[]) => Base");

//let r5b10 = b10 == a10;

// operator !=
let r6a1 = a1 != b1;
AssertType(r6a1, "boolean");
AssertType(a1 != b1, "boolean");
AssertType(a1, "new () => Base");
AssertType(b1, "new () => Base");

let r6a2 = a2 != b2;
AssertType(r6a2, "boolean");
AssertType(a2 != b2, "boolean");
AssertType(a2, "new (number, string) => Base");
AssertType(b2, "new (number, string) => Base");

let r6a3 = a3 != b3;
AssertType(r6a3, "boolean");
AssertType(a3 != b3, "boolean");
AssertType(a3, "new (number, string) => Base");
AssertType(b3, "new (number) => Base");

let r6a4 = a4 != b4;
AssertType(r6a4, "boolean");
AssertType(a4 != b4, "boolean");
AssertType(a4, "new (number, string) => Base");
AssertType(b4, "new () => Base");

let r6a5 = a5 != b5;
AssertType(r6a5, "boolean");
AssertType(a5 != b5, "boolean");
AssertType(a5, "new (Base) => Base");
AssertType(b5, "new (Derived) => Base");

let r6a6 = a6 != b6;
AssertType(r6a6, "boolean");
AssertType(a6 != b6, "boolean");
AssertType(a6, "new (Derived, Base) => Base");
AssertType(b6, "new (Base, Derived) => Base");

let r6a7 = a7 != b7;
AssertType(r6a7, "boolean");
AssertType(a7 != b7, "boolean");
AssertType(a7, "new () => Base");
AssertType(b7, "new () => Derived");

let r6a8 = a8 != b8;
AssertType(r6a8, "boolean");
AssertType(a8 != b8, "boolean");
AssertType(a8, "new (?Base) => Base");
AssertType(b8, "new (?Derived) => Base");

let r6a9 = a9 != b9;
AssertType(r6a9, "boolean");
AssertType(a9 != b9, "boolean");
AssertType(a9, "new (...Base[]) => Base");
AssertType(b9, "new (...Derived[]) => Base");

//let r6a10 = a10 != b10;

let r6b1 = b1 != a1;
AssertType(r6b1, "boolean");
AssertType(b1 != a1, "boolean");
AssertType(b1, "new () => Base");
AssertType(a1, "new () => Base");

let r6b2 = b2 != a2;
AssertType(r6b2, "boolean");
AssertType(b2 != a2, "boolean");
AssertType(b2, "new (number, string) => Base");
AssertType(a2, "new (number, string) => Base");

let r6b3 = b3 != a3;
AssertType(r6b3, "boolean");
AssertType(b3 != a3, "boolean");
AssertType(b3, "new (number) => Base");
AssertType(a3, "new (number, string) => Base");

let r6b4 = b4 != a4;
AssertType(r6b4, "boolean");
AssertType(b4 != a4, "boolean");
AssertType(b4, "new () => Base");
AssertType(a4, "new (number, string) => Base");

let r6b5 = b5 != a5;
AssertType(r6b5, "boolean");
AssertType(b5 != a5, "boolean");
AssertType(b5, "new (Derived) => Base");
AssertType(a5, "new (Base) => Base");

let r6b6 = b6 != a6;
AssertType(r6b6, "boolean");
AssertType(b6 != a6, "boolean");
AssertType(b6, "new (Base, Derived) => Base");
AssertType(a6, "new (Derived, Base) => Base");

let r6b7 = b7 != a7;
AssertType(r6b7, "boolean");
AssertType(b7 != a7, "boolean");
AssertType(b7, "new () => Derived");
AssertType(a7, "new () => Base");

let r6b8 = b8 != a8;
AssertType(r6b8, "boolean");
AssertType(b8 != a8, "boolean");
AssertType(b8, "new (?Derived) => Base");
AssertType(a8, "new (?Base) => Base");

let r6b9 = b9 != a9;
AssertType(r6b9, "boolean");
AssertType(b9 != a9, "boolean");
AssertType(b9, "new (...Derived[]) => Base");
AssertType(a9, "new (...Base[]) => Base");

//let r6b10 = b10 != a10;

// operator ===
let r7a1 = a1 === b1;
AssertType(r7a1, "boolean");
AssertType(a1 === b1, "boolean");
AssertType(a1, "new () => Base");
AssertType(b1, "new () => Base");

let r7a2 = a2 === b2;
AssertType(r7a2, "boolean");
AssertType(a2 === b2, "boolean");
AssertType(a2, "new (number, string) => Base");
AssertType(b2, "new (number, string) => Base");

let r7a3 = a3 === b3;
AssertType(r7a3, "boolean");
AssertType(a3 === b3, "boolean");
AssertType(a3, "new (number, string) => Base");
AssertType(b3, "new (number) => Base");

let r7a4 = a4 === b4;
AssertType(r7a4, "boolean");
AssertType(a4 === b4, "boolean");
AssertType(a4, "new (number, string) => Base");
AssertType(b4, "new () => Base");

let r7a5 = a5 === b5;
AssertType(r7a5, "boolean");
AssertType(a5 === b5, "boolean");
AssertType(a5, "new (Base) => Base");
AssertType(b5, "new (Derived) => Base");

let r7a6 = a6 === b6;
AssertType(r7a6, "boolean");
AssertType(a6 === b6, "boolean");
AssertType(a6, "new (Derived, Base) => Base");
AssertType(b6, "new (Base, Derived) => Base");

let r7a7 = a7 === b7;
AssertType(r7a7, "boolean");
AssertType(a7 === b7, "boolean");
AssertType(a7, "new () => Base");
AssertType(b7, "new () => Derived");

let r7a8 = a8 === b8;
AssertType(r7a8, "boolean");
AssertType(a8 === b8, "boolean");
AssertType(a8, "new (?Base) => Base");
AssertType(b8, "new (?Derived) => Base");

let r7a9 = a9 === b9;
AssertType(r7a9, "boolean");
AssertType(a9 === b9, "boolean");
AssertType(a9, "new (...Base[]) => Base");
AssertType(b9, "new (...Derived[]) => Base");

//let r7a10 = a10 === b10;

let r7b1 = b1 === a1;
AssertType(r7b1, "boolean");
AssertType(b1 === a1, "boolean");
AssertType(b1, "new () => Base");
AssertType(a1, "new () => Base");

let r7b2 = b2 === a2;
AssertType(r7b2, "boolean");
AssertType(b2 === a2, "boolean");
AssertType(b2, "new (number, string) => Base");
AssertType(a2, "new (number, string) => Base");

let r7b3 = b3 === a3;
AssertType(r7b3, "boolean");
AssertType(b3 === a3, "boolean");
AssertType(b3, "new (number) => Base");
AssertType(a3, "new (number, string) => Base");

let r7b4 = b4 === a4;
AssertType(r7b4, "boolean");
AssertType(b4 === a4, "boolean");
AssertType(b4, "new () => Base");
AssertType(a4, "new (number, string) => Base");

let r7b5 = b5 === a5;
AssertType(r7b5, "boolean");
AssertType(b5 === a5, "boolean");
AssertType(b5, "new (Derived) => Base");
AssertType(a5, "new (Base) => Base");

let r7b6 = b6 === a6;
AssertType(r7b6, "boolean");
AssertType(b6 === a6, "boolean");
AssertType(b6, "new (Base, Derived) => Base");
AssertType(a6, "new (Derived, Base) => Base");

let r7b7 = b7 === a7;
AssertType(r7b7, "boolean");
AssertType(b7 === a7, "boolean");
AssertType(b7, "new () => Derived");
AssertType(a7, "new () => Base");

let r7b8 = b8 === a8;
AssertType(r7b8, "boolean");
AssertType(b8 === a8, "boolean");
AssertType(b8, "new (?Derived) => Base");
AssertType(a8, "new (?Base) => Base");

let r7b9 = b9 === a9;
AssertType(r7b9, "boolean");
AssertType(b9 === a9, "boolean");
AssertType(b9, "new (...Derived[]) => Base");
AssertType(a9, "new (...Base[]) => Base");

//let r7b10 = b10 === a10;

// operator !==
let r8a1 = a1 !== b1;
AssertType(r8a1, "boolean");
AssertType(a1 !== b1, "boolean");
AssertType(a1, "new () => Base");
AssertType(b1, "new () => Base");

let r8a2 = a2 !== b2;
AssertType(r8a2, "boolean");
AssertType(a2 !== b2, "boolean");
AssertType(a2, "new (number, string) => Base");
AssertType(b2, "new (number, string) => Base");

let r8a3 = a3 !== b3;
AssertType(r8a3, "boolean");
AssertType(a3 !== b3, "boolean");
AssertType(a3, "new (number, string) => Base");
AssertType(b3, "new (number) => Base");

let r8a4 = a4 !== b4;
AssertType(r8a4, "boolean");
AssertType(a4 !== b4, "boolean");
AssertType(a4, "new (number, string) => Base");
AssertType(b4, "new () => Base");

let r8a5 = a5 !== b5;
AssertType(r8a5, "boolean");
AssertType(a5 !== b5, "boolean");
AssertType(a5, "new (Base) => Base");
AssertType(b5, "new (Derived) => Base");

let r8a6 = a6 !== b6;
AssertType(r8a6, "boolean");
AssertType(a6 !== b6, "boolean");
AssertType(a6, "new (Derived, Base) => Base");
AssertType(b6, "new (Base, Derived) => Base");

let r8a7 = a7 !== b7;
AssertType(r8a7, "boolean");
AssertType(a7 !== b7, "boolean");
AssertType(a7, "new () => Base");
AssertType(b7, "new () => Derived");

let r8a8 = a8 !== b8;
AssertType(r8a8, "boolean");
AssertType(a8 !== b8, "boolean");
AssertType(a8, "new (?Base) => Base");
AssertType(b8, "new (?Derived) => Base");

let r8a9 = a9 !== b9;
AssertType(r8a9, "boolean");
AssertType(a9 !== b9, "boolean");
AssertType(a9, "new (...Base[]) => Base");
AssertType(b9, "new (...Derived[]) => Base");

//let r8a10 = a10 !== b10;

let r8b1 = b1 !== a1;
AssertType(r8b1, "boolean");
AssertType(b1 !== a1, "boolean");
AssertType(b1, "new () => Base");
AssertType(a1, "new () => Base");

let r8b2 = b2 !== a2;
AssertType(r8b2, "boolean");
AssertType(b2 !== a2, "boolean");
AssertType(b2, "new (number, string) => Base");
AssertType(a2, "new (number, string) => Base");

let r8b3 = b3 !== a3;
AssertType(r8b3, "boolean");
AssertType(b3 !== a3, "boolean");
AssertType(b3, "new (number) => Base");
AssertType(a3, "new (number, string) => Base");

let r8b4 = b4 !== a4;
AssertType(r8b4, "boolean");
AssertType(b4 !== a4, "boolean");
AssertType(b4, "new () => Base");
AssertType(a4, "new (number, string) => Base");

let r8b5 = b5 !== a5;
AssertType(r8b5, "boolean");
AssertType(b5 !== a5, "boolean");
AssertType(b5, "new (Derived) => Base");
AssertType(a5, "new (Base) => Base");

let r8b6 = b6 !== a6;
AssertType(r8b6, "boolean");
AssertType(b6 !== a6, "boolean");
AssertType(b6, "new (Base, Derived) => Base");
AssertType(a6, "new (Derived, Base) => Base");

let r8b7 = b7 !== a7;
AssertType(r8b7, "boolean");
AssertType(b7 !== a7, "boolean");
AssertType(b7, "new () => Derived");
AssertType(a7, "new () => Base");

let r8b8 = b8 !== a8;
AssertType(r8b8, "boolean");
AssertType(b8 !== a8, "boolean");
AssertType(b8, "new (?Derived) => Base");
AssertType(a8, "new (?Base) => Base");

let r8b9 = b9 !== a9;
AssertType(r8b9, "boolean");
AssertType(b9 !== a9, "boolean");
AssertType(b9, "new (...Derived[]) => Base");
AssertType(a9, "new (...Base[]) => Base");

//let r8b10 = b10 !== a10;

