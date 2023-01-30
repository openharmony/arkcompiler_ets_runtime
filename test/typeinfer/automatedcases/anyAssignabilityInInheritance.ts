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

// === tests/cases/conformance/types/typeRelationships/assignmentCompatibility/anyAssignabilityInInheritance.ts ===
declare function AssertType(value:any, type:string):void;
// any is not a subtype of any other types, errors expected on all the below derived classes unless otherwise noted

interface I {
    [x: string]: any;
    foo: any; // ok, any identical to itself
}

let a: any;
AssertType(a, "any");

declare function foo2(x: number): number;
declare function foo2(x: any): any;
let r3 = foo2(a); // any, not a subtype of number so it skips that overload, is a subtype of itself so it picks second (if truly ambiguous it would pick first overload)
AssertType(r3, "any");
AssertType(foo2(a), "any");
AssertType(foo2, "{ (number): number; (any): any; }");
AssertType(a, "any");

declare function foo3(x: string): string;
declare function foo3(x: any): any;
let r3 = foo3(a); // any
AssertType(r3, "any");
AssertType(foo3(a), "any");
AssertType(foo3, "{ (string): string; (any): any; }");
AssertType(a, "any");

declare function foo4(x: boolean): boolean;
declare function foo4(x: any): any;
let r3 = foo3(a); // any
AssertType(r3, "any");
AssertType(foo3(a), "any");
AssertType(foo3, "{ (string): string; (any): any; }");
AssertType(a, "any");

declare function foo5(x: Date): Date;
declare function foo5(x: any): any;
let r3 = foo3(a); // any
AssertType(r3, "any");
AssertType(foo3(a), "any");
AssertType(foo3, "{ (string): string; (any): any; }");
AssertType(a, "any");

declare function foo6(x: RegExp): RegExp;
declare function foo6(x: any): any;
let r3 = foo3(a); // any
AssertType(r3, "any");
AssertType(foo3(a), "any");
AssertType(foo3, "{ (string): string; (any): any; }");
AssertType(a, "any");

declare function foo7(x: { bar: number }): { bar: number };
declare function foo7(x: any): any;
let r3 = foo3(a); // any
AssertType(r3, "any");
AssertType(foo3(a), "any");
AssertType(foo3, "{ (string): string; (any): any; }");
AssertType(a, "any");

declare function foo8(x: number[]): number[];
declare function foo8(x: any): any;
let r3 = foo3(a); // any
AssertType(r3, "any");
AssertType(foo3(a), "any");
AssertType(foo3, "{ (string): string; (any): any; }");
AssertType(a, "any");

interface I8 { foo: string }
declare function foo9(x: I8): I8;
declare function foo9(x: any): any;
let r3 = foo3(a); // any
AssertType(r3, "any");
AssertType(foo3(a), "any");
AssertType(foo3, "{ (string): string; (any): any; }");
AssertType(a, "any");

class A { foo: number; }
declare function foo10(x: A): A;
declare function foo10(x: any): any;
let r3 = foo3(a); // any
AssertType(r3, "any");
AssertType(foo3(a), "any");
AssertType(foo3, "{ (string): string; (any): any; }");
AssertType(a, "any");

class A2<T> { foo: T; }
declare function foo11(x: A2<string>): A2<string>;
declare function foo11(x: any): any;
let r3 = foo3(a); // any
AssertType(r3, "any");
AssertType(foo3(a), "any");
AssertType(foo3, "{ (string): string; (any): any; }");
AssertType(a, "any");

declare function foo12(x: (x) => number): (x) => number;
declare function foo12(x: any): any;
let r3 = foo3(a); // any
AssertType(r3, "any");
AssertType(foo3(a), "any");
AssertType(foo3, "{ (string): string; (any): any; }");
AssertType(a, "any");

declare function foo13(x: <T>(x: T) => T): <T>(x: T) => T;
declare function foo13(x: any): any;
let r3 = foo3(a); // any
AssertType(r3, "any");
AssertType(foo3(a), "any");
AssertType(foo3, "{ (string): string; (any): any; }");
AssertType(a, "any");

enum E { A }
declare function foo14(x: E): E;
declare function foo14(x: any): any;
let r3 = foo3(a); // any
AssertType(r3, "any");
AssertType(foo3(a), "any");
AssertType(foo3, "{ (string): string; (any): any; }");
AssertType(a, "any");

function f() { }
module f {
    export let bar = 1;
}
declare function foo15(x: typeof f): typeof f;
declare function foo15(x: any): any;
let r3 = foo3(a); // any
AssertType(r3, "any");
AssertType(foo3(a), "any");
AssertType(foo3, "{ (string): string; (any): any; }");
AssertType(a, "any");

class CC { baz: string }
module CC {
    export let bar = 1;
}
declare function foo16(x: CC): CC;
declare function foo16(x: any): any;
let r3 = foo3(a); // any
AssertType(r3, "any");
AssertType(foo3(a), "any");
AssertType(foo3, "{ (string): string; (any): any; }");
AssertType(a, "any");

declare function foo17(x: Object): Object;
declare function foo17(x: any): any;
let r3 = foo3(a); // any
AssertType(r3, "any");
AssertType(foo3(a), "any");
AssertType(foo3, "{ (string): string; (any): any; }");
AssertType(a, "any");

declare function foo18(x: {}): {};
declare function foo18(x: any): any;
let r3 = foo3(a); // any
AssertType(r3, "any");
AssertType(foo3(a), "any");
AssertType(foo3, "{ (string): string; (any): any; }");
AssertType(a, "any");


