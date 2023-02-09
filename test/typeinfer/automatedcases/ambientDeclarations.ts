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

// === tests/cases/conformance/ambient/ambientDeclarations.ts ===
declare function AssertType(value:any, type:string):void;
// Ambient letiable without type annotation
declare let n;
AssertType(n, "any");

// Ambient letiable with type annotation
declare let m: string;
AssertType(m, "string");

// Ambient function with no type annotations
declare function fn1();

// Ambient function with type annotations
declare function fn2(n: string): number;

// Ambient function with valid overloads
declare function fn3(n: string): number;
declare function fn4(n: number, y: number): string;

// Ambient function with optional parameters
declare function fn5(x, y?);
declare function fn6(e?);
declare function fn7(x, y?, ...z);
declare function fn8(y?, ...z: number[]);
declare function fn9(...q: {}[]);
declare function fn10<T>(...q: T[]);

// Ambient class
declare class cls {
    constructor();
    method(): cls;
    static static(p): number;
    static q;
    private fn();
    private static fns();
}

// Ambient enum
declare enum E1 {
    x,
    y,
    z
}

// Ambient enum with integer literal initializer
declare enum E2 {
    q,
    a = 1,
    b,
    c = 2,
    d
}

// Ambient enum members are always exported with or without export keyword
declare enum E3 {
    A
}
declare module E3 {
    let B;
}
let x = E3.B;
AssertType(x, "any");
AssertType(E3.B, "any");

// Ambient module
declare module M1 {
    let x;
    function fn(): number;
}

// Ambient module members are always exported with or without export keyword
let p = M1.x;
AssertType(p, "any");
AssertType(M1.x, "any");

let q = M1.fn();
AssertType(q, "number");
AssertType(M1.fn(), "number");
AssertType(M1.fn, "() => number");

// Ambient external module in the global module
// Ambient external module with a string literal name that is a top level external module name
declare module 'external1' {
    let q;
}



