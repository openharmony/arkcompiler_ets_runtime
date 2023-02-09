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

// === tests/cases/conformance/types/typeAliases/typeAliases.ts ===
declare function AssertType(value:any, type:string):void;
// Writing a reference to a type alias has exactly the same effect as writing the aliased type itself.

type T1 = number;
let x1: number;
AssertType(x1, "number");

let x1: T1;
AssertType(x1, "number");

type T2 = string;
let x2: string;
AssertType(x2, "string");

let x2: T2;
AssertType(x2, "string");

type T3 = boolean;
let x3: boolean;
AssertType(x3, "boolean");

let x3: T3;
AssertType(x3, "boolean");

type T4 = void;
let x4: void;
AssertType(x4, "void");

let x4: T4;
AssertType(x4, "void");

type T5 = any;
let x5: any;
AssertType(x5, "any");

let x5: T5;
AssertType(x5, "any");

interface I6 { x : string }
type T6 = I6;
let x6: I6;
AssertType(x6, "I6");

let x6: T6;
AssertType(x6, "I6");

class C7 { x: boolean }
type T7 = C7;
let x7: C7;
AssertType(x7, "C7");

let x7: T7;
AssertType(x7, "C7");

type T8 = string | boolean;
let x8: string | boolean;
AssertType(x8, "union");

let x8: T8;
AssertType(x8, "union");

type T9 = () => string;
let x9: () => string;
AssertType(x9, "() => string");

let x9: T9;
AssertType(x9, "() => string");

type T10 = { x: number };
let x10: { x: number };
AssertType(x10, "{ x: number; }");
AssertType(x, "number");

let x10: T10;
AssertType(x10, "{ x: number; }");

type T11 = { new(): boolean };
let x11: { new(): boolean };
AssertType(x11, "new () => boolean");

let x11: T11;
AssertType(x11, "new () => boolean");

interface I13 { x: string };
type T13 = I13;
let x13_1: I13;
AssertType(x13_1, "I13");

let x13_2: T13
AssertType(x13_2, "I13");

declare function foo13<T1 extends I13, T2 extends T13>(t1: T1, t2: T13): void;
foo13(x13_1, x13_2);
AssertType(foo13(x13_1, x13_2), "void");
AssertType(foo13, "<T1 extends I13, T2 extends I13>(T1, I13) => void");
AssertType(x13_1, "I13");
AssertType(x13_2, "I13");

foo13(x13_2, x13_1);
AssertType(foo13(x13_2, x13_1), "void");
AssertType(foo13, "<T1 extends I13, T2 extends I13>(T1, I13) => void");
AssertType(x13_2, "I13");
AssertType(x13_1, "I13");

type T14 = string;
let x14: T14;
AssertType(x14, "string");

declare function foo14_1(x: T14): void;

declare function foo14_2(x: "click"): void;
declare function foo14_2(x: T14): void;

type Meters = number

enum E { x = 10 }

declare function f15(a: string): boolean;
declare function f15(a: Meters): string;
f15(E.x).toLowerCase();
AssertType(f15(E.x).toLowerCase(), "string");
AssertType(f15(E.x).toLowerCase, "() => string");

type StringAndBoolean = [string, boolean]
declare function f16(s: StringAndBoolean): string;
let x: [string, boolean];
AssertType(x, "[string, boolean]");

f16(x);
AssertType(f16(x), "string");
AssertType(f16, "(StringAndBoolean) => string");
AssertType(x, "[string, boolean]");

let y: StringAndBoolean = ["1", false];
AssertType(y, "StringAndBoolean");
AssertType(["1", false], "[string, false]");
AssertType("1", "string");
AssertType(false, "boolean");

y[0].toLowerCase();
AssertType(y[0].toLowerCase(), "string");
AssertType(y[0].toLowerCase, "() => string");


