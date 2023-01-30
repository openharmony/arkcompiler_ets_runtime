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

// === tests/cases/conformance/types/typeRelationships/typeInference/genericCallTypeArgumentInference.ts ===
declare function AssertType(value:any, type:string):void;
// Basic type inference with generic calls, no errors expected

function foo<T>(t: T) {
AssertType(t, "T");
    return t;
}

let r = foo(''); // string
AssertType(r, "string");
AssertType(foo(''), "string");
AssertType(foo, "<T>(T) => T");
AssertType('', "string");

function foo2<T, U>(t: T, u: U) {
AssertType(u, "U");
    return u;
}

function foo2b<T, U>(u: U) {
    let x: T;
AssertType(x, "T");

AssertType(x, "T");
    return x;
}

let r2 = foo2('', 1); // number
AssertType(r2, "number");
AssertType(foo2('', 1), "int");
AssertType(foo2, "<T, U>(T, U) => U");
AssertType('', "string");
AssertType(1, "int");

let r3 = foo2b(1); // {
AssertType(r3, "unknown");

AssertType(foo2b(1), "unknown");

AssertType(foo2b, "<T, U>(U) => T");

AssertType(1, "int");
}

class C<T, U> {
    constructor(public t: T, public u: U) {
    }

    foo(t: T, u: U) {
AssertType(t, "T");
        return t;
    }

    foo2(t: T, u: U) {
AssertType(u, "U");
        return u;
    }

    foo3<T>(t: T, u: U) {
AssertType(t, "T");
        return t;
    }

    foo4<U>(t: T, u: U) {
AssertType(t, "T");
        return t;
    }

    foo5<T,U>(t: T, u: U) {
AssertType(t, "T");
        return t;
    }

    foo6<T, U>() {
        let x: T;
AssertType(x, "T");

AssertType(x, "T");
        return x;
    }

    foo7<T, U>(u: U) {
        let x: T;
AssertType(x, "T");

AssertType(x, "T");
        return x;
    }

    foo8<T, U>() {
        let x: T;
AssertType(x, "T");

AssertType(x, "T");
        return x;
    }
}

let c = new C('', 1);
AssertType(c, "C<string, number>");
AssertType(new C('', 1), "C<string, number>");
AssertType(C, "typeof C");
AssertType('', "string");
AssertType(1, "int");

let r4 = c.foo('', 1); // string
AssertType(r4, "string");
AssertType(c.foo('', 1), "string");
AssertType(c.foo, "(string, number) => string");
AssertType('', "string");
AssertType(1, "int");

let r5 = c.foo2('', 1); // number
AssertType(r5, "number");
AssertType(c.foo2('', 1), "number");
AssertType(c.foo2, "(string, number) => number");
AssertType('', "string");
AssertType(1, "int");

let r6 = c.foo3(true, 1); // boolean
AssertType(r6, "boolean");
AssertType(c.foo3(true, 1), "boolean");
AssertType(c.foo3, "<T>(T, number) => T");
AssertType(true, "boolean");
AssertType(1, "int");

let r7 = c.foo4('', true); // string
AssertType(r7, "string");
AssertType(c.foo4('', true), "string");
AssertType(c.foo4, "<U>(string, U) => string");
AssertType('', "string");
AssertType(true, "boolean");

let r8 = c.foo5(true, 1); // boolean
AssertType(r8, "boolean");
AssertType(c.foo5(true, 1), "boolean");
AssertType(c.foo5, "<T, U>(T, U) => T");
AssertType(true, "boolean");
AssertType(1, "int");

let r9 = c.foo6(); // {
AssertType(r9, "unknown");

AssertType(c.foo6(), "unknown");

AssertType(c.foo6, "<T, U>() => T");
}

let r10 = c.foo7(''); // {
AssertType(r10, "unknown");

AssertType(c.foo7(''), "unknown");

AssertType(c.foo7, "<T, U>(U) => T");

AssertType('', "string");
}

let r11 = c.foo8(); // {
AssertType(r11, "unknown");

AssertType(c.foo8(), "unknown");

AssertType(c.foo8, "<T, U>() => T");
}

interface I<T, U> {
    new (t: T, u: U);
    foo(t: T, u: U): T;
    foo2(t: T, u: U): U;
    foo3<T>(t: T, u: U): T;
    foo4<U>(t: T, u: U): T;
    foo5<T, U>(t: T, u: U): T;
    foo6<T, U>(): T;
    foo7<T, U>(u: U): T;
    foo8<T, U>(): T;    
}

let i: I<string, number>;
AssertType(i, "I<string, number>");

let r4 = i.foo('', 1); // string
AssertType(r4, "string");
AssertType(i.foo('', 1), "string");
AssertType(i.foo, "(string, number) => string");
AssertType('', "string");
AssertType(1, "int");

let r5 = i.foo2('', 1); // number
AssertType(r5, "number");
AssertType(i.foo2('', 1), "number");
AssertType(i.foo2, "(string, number) => number");
AssertType('', "string");
AssertType(1, "int");

let r6 = i.foo3(true, 1); // boolean
AssertType(r6, "boolean");
AssertType(i.foo3(true, 1), "boolean");
AssertType(i.foo3, "<T>(T, number) => T");
AssertType(true, "boolean");
AssertType(1, "int");

let r7 = i.foo4('', true); // string
AssertType(r7, "string");
AssertType(i.foo4('', true), "string");
AssertType(i.foo4, "<U>(string, U) => string");
AssertType('', "string");
AssertType(true, "boolean");

let r8 = i.foo5(true, 1); // boolean
AssertType(r8, "boolean");
AssertType(i.foo5(true, 1), "boolean");
AssertType(i.foo5, "<T, U>(T, U) => T");
AssertType(true, "boolean");
AssertType(1, "int");

let r9 = i.foo6(); // {
AssertType(r9, "unknown");

AssertType(i.foo6(), "unknown");

AssertType(i.foo6, "<T, U>() => T");
}

let r10 = i.foo7(''); // {
AssertType(r10, "unknown");

AssertType(i.foo7(''), "unknown");

AssertType(i.foo7, "<T, U>(U) => T");

AssertType('', "string");
}

let r11 = i.foo8(); // {
AssertType(r11, "unknown");

AssertType(i.foo8(), "unknown");

AssertType(i.foo8, "<T, U>() => T");
}


