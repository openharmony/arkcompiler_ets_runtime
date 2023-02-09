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

// === tests/cases/compiler/commentsOverloads.ts ===
declare function AssertType(value:any, type:string):void;
/** this is signature 1*/
function f1(/**param a*/a: number): number;
function f1(b: string): number;
function f1(aOrb: any) {
AssertType(10, "int");
    return 10;
}
f1("hello");
AssertType(f1("hello"), "number");
AssertType(f1, "{ (number): number; (string): number; }");
AssertType("hello", "string");

f1(10);
AssertType(f1(10), "number");
AssertType(f1, "{ (number): number; (string): number; }");
AssertType(10, "int");

function f2(a: number): number;
/** this is signature 2*/
function f2(b: string): number;
/** this is f2 let comment*/
function f2(aOrb: any) {
AssertType(10, "int");
    return 10;
}
f2("hello");
AssertType(f2("hello"), "number");
AssertType(f2, "{ (number): number; (string): number; }");
AssertType("hello", "string");

f2(10);
AssertType(f2(10), "number");
AssertType(f2, "{ (number): number; (string): number; }");
AssertType(10, "int");

function f3(a: number): number;
function f3(b: string): number;
function f3(aOrb: any) {
AssertType(10, "int");
    return 10;
}
f3("hello");
AssertType(f3("hello"), "number");
AssertType(f3, "{ (number): number; (string): number; }");
AssertType("hello", "string");

f3(10);
AssertType(f3(10), "number");
AssertType(f3, "{ (number): number; (string): number; }");
AssertType(10, "int");

/** this is signature 4 - with number parameter*/
function f4(/**param a*/a: number): number;
/** this is signature 4 - with string parameter*/
function f4(b: string): number;
function f4(aOrb: any) {
AssertType(10, "int");
    return 10;
}
f4("hello");
AssertType(f4("hello"), "number");
AssertType(f4, "{ (number): number; (string): number; }");
AssertType("hello", "string");

f4(10);
AssertType(f4(10), "number");
AssertType(f4, "{ (number): number; (string): number; }");
AssertType(10, "int");

interface i1 {
    /**this signature 1*/
    (/**param a*/ a: number): number;
    /**this is signature 2*/
    (b: string): number;
    /** foo 1*/
    foo(a: number): number;
    /** foo 2*/
    foo(b: string): number;
    // foo 3
    foo(arr: number[]): number;
    /** foo 4 */
    foo(arr: string[]): number;

    foo2(a: number): number;
    /** foo2 2*/
    foo2(b: string): number;
    foo3(a: number): number;
    foo3(b: string): number;
    /** foo4 1*/
    foo4(a: number): number;
    foo4(b: string): number;
    /** foo4 any */
    foo4(c: any): any;
    /// new 1
    new (a: string);
    /** new 1*/
    new (b: number);
}
let i1_i: i1;
AssertType(i1_i, "i1");

interface i2 {
    new (a: string);
    /** new 2*/
    new (b: number);
    (a: number): number;
    /**this is signature 2*/
    (b: string): number;
}
let i2_i: i2;
AssertType(i2_i, "i2");

interface i3 {
    /** new 1*/
    new (a: string);
    /** new 2*/
    new (b: number);
    /**this is signature 1*/
    (a: number): number;
    (b: string): number;
}
let i3_i: i3;
AssertType(i3_i, "i3");

interface i4 {
    new (a: string);
    new (b: number);
    (a: number): number;
    (b: string): number;
}
class c {
    public prop1(a: number): number;
    public prop1(b: string): number;
    public prop1(aorb: any) {
AssertType(10, "int");
        return 10;
    }
    /** prop2 1*/
    public prop2(a: number): number;
    public prop2(b: string): number;
    public prop2(aorb: any) {
AssertType(10, "int");
        return 10;
    }
    public prop3(a: number): number;
    /** prop3 2*/
    public prop3(b: string): number;
    public prop3(aorb: any) {
AssertType(10, "int");
        return 10;
    }
    /** prop4 1*/
    public prop4(a: number): number;
    /** prop4 2*/
    public prop4(b: string): number;
    public prop4(aorb: any) {
AssertType(10, "int");
        return 10;
    }
    /** prop5 1*/
    public prop5(a: number): number;
    /** prop5 2*/
    public prop5(b: string): number;
    /** Prop5 implementaion*/
    public prop5(aorb: any) {
AssertType(10, "int");
        return 10;
    }
}
class c1 {
    constructor(a: number);
    constructor(b: string);
    constructor(aorb: any) {
    }
}
class c2 {
    /** c2 1*/
    constructor(a: number);
    // c2 2
    constructor(b: string);
    constructor(aorb: any) {
    }
}
class c3 {
    constructor(a: number);
    /** c3 2*/
    constructor(b: string);
    constructor(aorb: any) {
    }
}
class c4 {
    /** c4 1*/
    constructor(a: number);
    /** c4 2*/
    constructor(b: string);
    /** c4 3 */
    constructor(aorb: any) {
    }
}
class c5 {
    /** c5 1*/
    constructor(a: number);
    /** c5 2*/
    constructor(b: string);
    /** c5 implementation*/
    constructor(aorb: any) {
    }
}
let c_i = new c();
AssertType(c_i, "c");
AssertType(new c(), "c");
AssertType(c, "typeof c");

let c1_i_1 = new c1(10);
AssertType(c1_i_1, "c1");
AssertType(new c1(10), "c1");
AssertType(c1, "typeof c1");
AssertType(10, "int");

let c1_i_2 = new c1("hello");
AssertType(c1_i_2, "c1");
AssertType(new c1("hello"), "c1");
AssertType(c1, "typeof c1");
AssertType("hello", "string");

let c2_i_1 = new c2(10);
AssertType(c2_i_1, "c2");
AssertType(new c2(10), "c2");
AssertType(c2, "typeof c2");
AssertType(10, "int");

let c2_i_2 = new c2("hello");
AssertType(c2_i_2, "c2");
AssertType(new c2("hello"), "c2");
AssertType(c2, "typeof c2");
AssertType("hello", "string");

let c3_i_1 = new c3(10);
AssertType(c3_i_1, "c3");
AssertType(new c3(10), "c3");
AssertType(c3, "typeof c3");
AssertType(10, "int");

let c3_i_2 = new c3("hello");
AssertType(c3_i_2, "c3");
AssertType(new c3("hello"), "c3");
AssertType(c3, "typeof c3");
AssertType("hello", "string");

let c4_i_1 = new c4(10);
AssertType(c4_i_1, "c4");
AssertType(new c4(10), "c4");
AssertType(c4, "typeof c4");
AssertType(10, "int");

let c4_i_2 = new c4("hello");
AssertType(c4_i_2, "c4");
AssertType(new c4("hello"), "c4");
AssertType(c4, "typeof c4");
AssertType("hello", "string");

let c5_i_1 = new c5(10);
AssertType(c5_i_1, "c5");
AssertType(new c5(10), "c5");
AssertType(c5, "typeof c5");
AssertType(10, "int");

let c5_i_2 = new c5("hello");
AssertType(c5_i_2, "c5");
AssertType(new c5("hello"), "c5");
AssertType(c5, "typeof c5");
AssertType("hello", "string");


