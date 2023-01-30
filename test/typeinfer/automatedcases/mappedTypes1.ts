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

// === tests/cases/conformance/types/mapped/mappedTypes1.ts ===
declare function AssertType(value:any, type:string):void;
type Item = { a: string, b: number, c: boolean };

type T00 = { [P in "x" | "y"]: number };
type T01 = { [P in "x" | "y"]: P };
type T02 = { [P in "a" | "b"]: Item[P]; }
type T03 = { [P in keyof Item]: Date };

type T10 = { [P in keyof Item]: Item[P] };
type T11 = { [P in keyof Item]?: Item[P] };
type T12 = { readonly [P in keyof Item]: Item[P] };
type T13 = { readonly [P in keyof Item]?: Item[P] };

type T20 = { [P in keyof Item]: Item[P] | null };
type T21 = { [P in keyof Item]: Array<Item[P]> };

type T30 = { [P in keyof any]: void };
type T31 = { [P in keyof string]: void };
type T32 = { [P in keyof number]: void };
type T33 = { [P in keyof boolean]: void };
type T34 = { [P in keyof undefined]: void };
type T35 = { [P in keyof null]: void };
type T36 = { [P in keyof void]: void };
type T37 = { [P in keyof symbol]: void };
type T38 = { [P in keyof never]: void };

type T40 = { [P in string]: void };
type T43 = { [P in "a" | "b"]: void };
type T44 = { [P in "a" | "b" | "0" | "1"]: void };
type T47 = { [P in string | "a" | "b" | "0" | "1"]: void };

declare function f1<T1>(): { [P in keyof T1]: void };
declare function f2<T1 extends string>(): { [P in keyof T1]: void };
declare function f3<T1 extends number>(): { [P in keyof T1]: void };
declare function f4<T1 extends Number>(): { [P in keyof T1]: void };

let x1 = f1();
AssertType(x1, "{}");
AssertType(f1(), "{}");
AssertType(f1, "<T1>() => { [P in keyof T1]: void; }");

let x2 = f2();
AssertType(x2, "string");
AssertType(f2(), "string");
AssertType(f2, "<T1 extends string>() => { [P in keyof T1]: void; }");

let x3 = f3();
AssertType(x3, "number");
AssertType(f3(), "number");
AssertType(f3, "<T1 extends number>() => { [P in keyof T1]: void; }");

let x4 = f4();
AssertType(x4, "{ toString: void; toFixed: void; toExponential: void; toPrecision: void; valueOf: void; toLocaleString: void; }");
AssertType(f4(), "{ toString: void; toFixed: void; toExponential: void; toPrecision: void; valueOf: void; toLocaleString: void; }");
AssertType(f4, "<T1 extends Number>() => { [P in keyof T1]: void; }");


