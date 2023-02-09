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

// === tests/cases/conformance/interfaces/declarationMerging/twoMergedInterfacesWithDifferingOverloads.ts ===
declare function AssertType(value:any, type:string):void;
// interfaces that merge must not have members that conflict

interface A {
    foo(x: number): number;
    foo(x: string): string;
}

interface A {
    foo(x: Date): Date;
}

interface B<T> {
    foo(x: T): number;
    foo(x: string): string;
}

interface B<T> {
    foo(x: T): Date;
    foo(x: Date): string;
}

let b: B<boolean>;
AssertType(b, "B<boolean>");

let r = b.foo(true); // 
AssertType(r, "Date");
AssertType(b.foo(true), "Date");
AssertType(b.foo, "{ (boolean): number; (string): string; (boolean): Date; (Date): string; }");
AssertType(true, "boolean");
returns Date

// add generic overload
interface C<T, U> {
    foo(x: T, y: U): string;
    foo(x: string, y: string): number;
}

interface C<T, U> {
    foo<W>(x: W, y: W): W;
}

let c: C<boolean, Date>;
AssertType(c, "C<boolean, Date>");

let r2 = c.foo(1, 2); // number
AssertType(r2, "number");
AssertType(c.foo(1, 2), "union");
AssertType(c.foo, "{ (boolean, Date): string; (string, string): number; <W>(W, W): W; }");
AssertType(1, "int");
AssertType(2, "int");

// add generic overload that would be ambiguous
interface D<T, U> {
    a: T;
    b: U;
    foo<A>(x: A, y: A): U;
}

interface D<T, U> {
    foo<W>(x: W, y: W): T;
}

let d: D<boolean, Date>;
AssertType(d, "D<boolean, Date>");

let r3 = d.foo(1, 1); // boolean, last definition wins
AssertType(r3, "boolean");
AssertType(d.foo(1, 1), "boolean");
AssertType(d.foo, "{ <A>(A, A): Date; <W>(W, W): boolean; }");
AssertType(1, "int");
AssertType(1, "int");


