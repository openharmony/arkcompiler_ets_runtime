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

// === tests/cases/conformance/types/typeRelationships/typeInference/genericCallWithConstraintsTypeArgumentInference.ts ===
declare function AssertType(value:any, type:string):void;
// Basic type inference with generic calls and constraints, no errors expected

class Base { foo: string; }
class Derived extends Base { bar: string; }
class Derived2 extends Derived { baz: string; }
let b: Base;
AssertType(b, "Base");

let d1: Derived;
AssertType(d1, "Derived");

let d2: Derived2;
AssertType(d2, "Derived2");

function foo<T extends Base>(t: T) {
AssertType(t, "T");
    return t;
}

let r = foo(b); // Base
AssertType(r, "Base");
AssertType(foo(b), "Base");
AssertType(foo, "<T extends Base>(T) => T");
AssertType(b, "Base");

let r2 = foo(d1); // Derived
AssertType(r2, "Derived");
AssertType(foo(d1), "Derived");
AssertType(foo, "<T extends Base>(T) => T");
AssertType(d1, "Derived");

function foo2<T extends Base, U extends Derived>(t: T, u: U) {
AssertType(u, "U");
    return u;
}

function foo2b<T extends Base, U extends Derived>(u: U) {
    let x: T;
AssertType(x, "T");

AssertType(x, "T");
    return x;
}

function foo2c<T extends Base, U extends Derived>() {
    let x: T;
AssertType(x, "T");

AssertType(x, "T");
    return x;
}

let r3 = foo2b(d1); // Base
AssertType(r3, "Base");
AssertType(foo2b(d1), "Base");
AssertType(foo2b, "<T extends Base, U extends Derived>(U) => T");
AssertType(d1, "Derived");

let r3b = foo2c(); // Base
AssertType(r3b, "Base");
AssertType(foo2c(), "Base");
AssertType(foo2c, "<T extends Base, U extends Derived>() => T");

class C<T extends Base, U extends Derived> {
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

    foo3<T extends Derived>(t: T, u: U) {
AssertType(t, "T");
        return t;
    }

    foo4<U extends Derived2>(t: T, u: U) {
AssertType(t, "T");
        return t;
    }

    foo5<T extends Derived, U extends Derived2>(t: T, u: U) {
AssertType(t, "T");
        return t;
    }

    foo6<T extends Derived, U extends Derived2>() {
        let x: T;
AssertType(x, "T");

AssertType(x, "T");
        return x;
    }

    foo7<T extends Base, U extends Derived>(u: U) {
        let x: T;
AssertType(x, "T");

AssertType(x, "T");
        return x;
    }

    foo8<T extends Base, U extends Derived>() {
        let x: T;
AssertType(x, "T");

AssertType(x, "T");
        return x;
    }
}

let c = new C(b, d1);
AssertType(c, "C<Base, Derived>");
AssertType(new C(b, d1), "C<Base, Derived>");
AssertType(C, "typeof C");
AssertType(b, "Base");
AssertType(d1, "Derived");

let r4 = c.foo(d1, d2); // Base
AssertType(r4, "Base");
AssertType(c.foo(d1, d2), "Base");
AssertType(c.foo, "(Base, Derived) => Base");
AssertType(d1, "Derived");
AssertType(d2, "Derived2");

let r5 = c.foo2(b, d2); // Derived
AssertType(r5, "Derived");
AssertType(c.foo2(b, d2), "Derived");
AssertType(c.foo2, "(Base, Derived) => Derived");
AssertType(b, "Base");
AssertType(d2, "Derived2");

let r6 = c.foo3(d1, d1); // Derived
AssertType(r6, "Derived");
AssertType(c.foo3(d1, d1), "Derived");
AssertType(c.foo3, "<T extends Derived>(T, Derived) => T");
AssertType(d1, "Derived");
AssertType(d1, "Derived");

let r7 = c.foo4(d1, d2); // Base
AssertType(r7, "Base");
AssertType(c.foo4(d1, d2), "Base");
AssertType(c.foo4, "<U extends Derived2>(Base, U) => Base");
AssertType(d1, "Derived");
AssertType(d2, "Derived2");

let r8 = c.foo5(d1, d2); // Derived
AssertType(r8, "Derived");
AssertType(c.foo5(d1, d2), "Derived");
AssertType(c.foo5, "<T extends Derived, U extends Derived2>(T, U) => T");
AssertType(d1, "Derived");
AssertType(d2, "Derived2");

let r8b = c.foo5(d2, d2); // Derived2
AssertType(r8b, "Derived2");
AssertType(c.foo5(d2, d2), "Derived2");
AssertType(c.foo5, "<T extends Derived, U extends Derived2>(T, U) => T");
AssertType(d2, "Derived2");
AssertType(d2, "Derived2");

let r9 = c.foo6(); // Derived
AssertType(r9, "Derived");
AssertType(c.foo6(), "Derived");
AssertType(c.foo6, "<T extends Derived, U extends Derived2>() => T");

let r10 = c.foo7(d1); // Base
AssertType(r10, "Base");
AssertType(c.foo7(d1), "Base");
AssertType(c.foo7, "<T extends Base, U extends Derived>(U) => T");
AssertType(d1, "Derived");

let r11 = c.foo8(); // Base
AssertType(r11, "Base");
AssertType(c.foo8(), "Base");
AssertType(c.foo8, "<T extends Base, U extends Derived>() => T");

interface I<T extends Base, U extends Derived> {
    new (t: T, u: U);
    foo(t: T, u: U): T;
    foo2(t: T, u: U): U;
    foo3<T extends Derived>(t: T, u: U): T;
    foo4<U extends Derived2>(t: T, u: U): T;
    foo5<T extends Derived, U extends Derived2>(t: T, u: U): T;
    foo6<T extends Derived, U extends Derived2>(): T;
    foo7<T extends Base, U extends Derived>(u: U): T;
    foo8<T extends Base, U extends Derived>(): T;
}

let i: I<Base, Derived>;
AssertType(i, "I<Base, Derived>");

let r4 = i.foo(d1, d2); // Base
AssertType(r4, "Base");
AssertType(i.foo(d1, d2), "Base");
AssertType(i.foo, "(Base, Derived) => Base");
AssertType(d1, "Derived");
AssertType(d2, "Derived2");

let r5 = i.foo2(b, d2); // Derived
AssertType(r5, "Derived");
AssertType(i.foo2(b, d2), "Derived");
AssertType(i.foo2, "(Base, Derived) => Derived");
AssertType(b, "Base");
AssertType(d2, "Derived2");

let r6 = i.foo3(d1, d1); // Derived
AssertType(r6, "Derived");
AssertType(i.foo3(d1, d1), "Derived");
AssertType(i.foo3, "<T extends Derived>(T, Derived) => T");
AssertType(d1, "Derived");
AssertType(d1, "Derived");

let r7 = i.foo4(d1, d2); // Base
AssertType(r7, "Base");
AssertType(i.foo4(d1, d2), "Base");
AssertType(i.foo4, "<U extends Derived2>(Base, U) => Base");
AssertType(d1, "Derived");
AssertType(d2, "Derived2");

let r8 = i.foo5(d1, d2); // Derived
AssertType(r8, "Derived");
AssertType(i.foo5(d1, d2), "Derived");
AssertType(i.foo5, "<T extends Derived, U extends Derived2>(T, U) => T");
AssertType(d1, "Derived");
AssertType(d2, "Derived2");

let r8b = i.foo5(d2, d2); // Derived2
AssertType(r8b, "Derived2");
AssertType(i.foo5(d2, d2), "Derived2");
AssertType(i.foo5, "<T extends Derived, U extends Derived2>(T, U) => T");
AssertType(d2, "Derived2");
AssertType(d2, "Derived2");

let r9 = i.foo6(); // Derived
AssertType(r9, "Derived");
AssertType(i.foo6(), "Derived");
AssertType(i.foo6, "<T extends Derived, U extends Derived2>() => T");

let r10 = i.foo7(d1); // Base
AssertType(r10, "Base");
AssertType(i.foo7(d1), "Base");
AssertType(i.foo7, "<T extends Base, U extends Derived>(U) => T");
AssertType(d1, "Derived");

let r11 = i.foo8(); // Base
AssertType(r11, "Base");
AssertType(i.foo8(), "Base");
AssertType(i.foo8, "<T extends Base, U extends Derived>() => T");


