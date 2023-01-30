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

// === tests/cases/conformance/classes/members/inheritanceAndOverriding/derivedClassOverridesProtectedMembers2.ts ===
declare function AssertType(value:any, type:string):void;
let x: { foo: string; 
AssertType(x, "{ foo: string; }");

AssertType(foo, "string");
}

let y: { foo: string; bar: string; 
AssertType(y, "{ foo: string; bar: string; }");

AssertType(foo, "string");

AssertType(bar, "string");
}

class Base {
    protected a: typeof x;
    protected b(a: typeof x) { }
    protected get c() { 
AssertType(x, "{ foo: string; }");
return x; 
}

    protected set c(v: typeof x) { }
    protected d: (a: typeof x) => void ;

    protected static r: typeof x;
    protected static s(a: typeof x) { }
    protected static get t() { 
AssertType(x, "{ foo: string; }");
return x; 
}

    protected static set t(v: typeof x) { }
    protected static u: (a: typeof x) => void ;

constructor(a: typeof x) { }
}

// Increase visibility of all protected members to public
class Derived extends Base {
    a: typeof y;
    b(a: typeof y) { }
    get c() { 
AssertType(y, "{ foo: string; bar: string; }");
return y; 
}

    set c(v: typeof y) { }
    d: (a: typeof y) => void;

    static r: typeof y;
    static s(a: typeof y) { }
    static get t() { 
AssertType(y, "{ foo: string; bar: string; }");
return y; 
}

    static set t(a: typeof y) { }
    static u: (a: typeof y) => void;

    constructor(a: typeof y) { super(a); 
AssertType(super(a), "void");

AssertType(super, "typeof Base");

AssertType(a, "{ foo: string; bar: string; }");
}
}

let d: Derived = new Derived(y);
AssertType(d, "Derived");
AssertType(new Derived(y), "Derived");
AssertType(Derived, "typeof Derived");
AssertType(y, "{ foo: string; bar: string; }");

let r1 = d.a;
AssertType(r1, "{ foo: string; bar: string; }");
AssertType(d.a, "{ foo: string; bar: string; }");

let r2 = d.b(y);
AssertType(r2, "void");
AssertType(d.b(y), "void");
AssertType(d.b, "({ foo: string; bar: string; }) => void");
AssertType(y, "{ foo: string; bar: string; }");

let r3 = d.c;
AssertType(r3, "{ foo: string; bar: string; }");
AssertType(d.c, "{ foo: string; bar: string; }");

let r3a = d.d;
AssertType(r3a, "({ foo: string; bar: string; }) => void");
AssertType(d.d, "({ foo: string; bar: string; }) => void");

d.c = y;
AssertType(d.c = y, "{ foo: string; bar: string; }");
AssertType(d.c, "{ foo: string; bar: string; }");
AssertType(y, "{ foo: string; bar: string; }");

let r4 = Derived.r;
AssertType(r4, "{ foo: string; bar: string; }");
AssertType(Derived.r, "{ foo: string; bar: string; }");

let r5 = Derived.s(y);
AssertType(r5, "void");
AssertType(Derived.s(y), "void");
AssertType(Derived.s, "({ foo: string; bar: string; }) => void");
AssertType(y, "{ foo: string; bar: string; }");

let r6 = Derived.t;
AssertType(r6, "{ foo: string; bar: string; }");
AssertType(Derived.t, "{ foo: string; bar: string; }");

let r6a = Derived.u;
AssertType(r6a, "({ foo: string; bar: string; }) => void");
AssertType(Derived.u, "({ foo: string; bar: string; }) => void");

Derived.t = y;
AssertType(Derived.t = y, "{ foo: string; bar: string; }");
AssertType(Derived.t, "{ foo: string; bar: string; }");
AssertType(y, "{ foo: string; bar: string; }");

class Base2 {
    [i: string]: Object;
    [i: number]: typeof x;
}

class Derived2 extends Base2 {
    [i: string]: typeof x;
    [i: number]: typeof y;
}

let d2: Derived2;
AssertType(d2, "Derived2");

let r7 = d2[''];
AssertType(r7, "{ foo: string; }");
AssertType(d2[''], "{ foo: string; }");
AssertType(d2, "Derived2");
AssertType('', "string");

let r8 = d2[1];
AssertType(r8, "{ foo: string; bar: string; }");
AssertType(d2[1], "{ foo: string; bar: string; }");
AssertType(d2, "Derived2");
AssertType(1, "int");



