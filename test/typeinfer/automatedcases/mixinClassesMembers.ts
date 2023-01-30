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

// === tests/cases/conformance/classes/mixinClassesMembers.ts ===
declare function AssertType(value:any, type:string):void;
declare class C1 {
    public a: number;
    protected b: number;
    private c: number;
    constructor(s: string);
    constructor(n: number);
}

declare class M1 {
    constructor(...args: any[]);
    p: number;
    static p: number;
}

declare class M2 {
    constructor(...args: any[]);
    f(): number;
    static f(): number;
}

declare const Mixed1: typeof M1 & typeof C1;
AssertType(Mixed1, "typeof M1 & typeof C1");
AssertType(M1, "typeof M1");
AssertType(C1, "typeof C1");

declare const Mixed2: typeof C1 & typeof M1;
AssertType(Mixed2, "typeof C1 & typeof M1");
AssertType(C1, "typeof C1");
AssertType(M1, "typeof M1");

declare const Mixed3: typeof M2 & typeof M1 & typeof C1;
AssertType(Mixed3, "typeof M2 & typeof M1 & typeof C1");
AssertType(M2, "typeof M2");
AssertType(M1, "typeof M1");
AssertType(C1, "typeof C1");

declare const Mixed4: typeof C1 & typeof M1 & typeof M2;
AssertType(Mixed4, "typeof C1 & typeof M1 & typeof M2");
AssertType(C1, "typeof C1");
AssertType(M1, "typeof M1");
AssertType(M2, "typeof M2");

declare const Mixed5: typeof M1 & typeof M2;
AssertType(Mixed5, "typeof M1 & typeof M2");
AssertType(M1, "typeof M1");
AssertType(M2, "typeof M2");

function f1() {
    let x1 = new Mixed1("hello");
AssertType(x1, "M1 & C1");
AssertType(new Mixed1("hello"), "M1 & C1");
AssertType(Mixed1, "typeof M1 & typeof C1");
AssertType("hello", "string");

    let x2 = new Mixed1(42);
AssertType(x2, "M1 & C1");
AssertType(new Mixed1(42), "M1 & C1");
AssertType(Mixed1, "typeof M1 & typeof C1");
AssertType(42, "int");

    let x3 = new Mixed2("hello");
AssertType(x3, "C1 & M1");
AssertType(new Mixed2("hello"), "C1 & M1");
AssertType(Mixed2, "typeof C1 & typeof M1");
AssertType("hello", "string");

    let x4 = new Mixed2(42);
AssertType(x4, "C1 & M1");
AssertType(new Mixed2(42), "C1 & M1");
AssertType(Mixed2, "typeof C1 & typeof M1");
AssertType(42, "int");

    let x5 = new Mixed3("hello");
AssertType(x5, "M2 & M1 & C1");
AssertType(new Mixed3("hello"), "M2 & M1 & C1");
AssertType(Mixed3, "typeof M2 & typeof M1 & typeof C1");
AssertType("hello", "string");

    let x6 = new Mixed3(42);
AssertType(x6, "M2 & M1 & C1");
AssertType(new Mixed3(42), "M2 & M1 & C1");
AssertType(Mixed3, "typeof M2 & typeof M1 & typeof C1");
AssertType(42, "int");

    let x7 = new Mixed4("hello");
AssertType(x7, "C1 & M1 & M2");
AssertType(new Mixed4("hello"), "C1 & M1 & M2");
AssertType(Mixed4, "typeof C1 & typeof M1 & typeof M2");
AssertType("hello", "string");

    let x8 = new Mixed4(42);
AssertType(x8, "C1 & M1 & M2");
AssertType(new Mixed4(42), "C1 & M1 & M2");
AssertType(Mixed4, "typeof C1 & typeof M1 & typeof M2");
AssertType(42, "int");

    let x9 = new Mixed5();
AssertType(x9, "M1 & M2");
AssertType(new Mixed5(), "M1 & M2");
AssertType(Mixed5, "typeof M1 & typeof M2");
}

function f2() {
    let x = new Mixed1("hello");
AssertType(x, "M1 & C1");
AssertType(new Mixed1("hello"), "M1 & C1");
AssertType(Mixed1, "typeof M1 & typeof C1");
AssertType("hello", "string");

    x.a;
AssertType(x.a, "number");

    x.p;
AssertType(x.p, "number");

    Mixed1.p;
AssertType(Mixed1.p, "number");
}

function f3() {
    let x = new Mixed2("hello");
AssertType(x, "C1 & M1");
AssertType(new Mixed2("hello"), "C1 & M1");
AssertType(Mixed2, "typeof C1 & typeof M1");
AssertType("hello", "string");

    x.a;
AssertType(x.a, "number");

    x.p;
AssertType(x.p, "number");

    Mixed2.p;
AssertType(Mixed2.p, "number");
}

function f4() {
    let x = new Mixed3("hello");
AssertType(x, "M2 & M1 & C1");
AssertType(new Mixed3("hello"), "M2 & M1 & C1");
AssertType(Mixed3, "typeof M2 & typeof M1 & typeof C1");
AssertType("hello", "string");

    x.a;
AssertType(x.a, "number");

    x.p;
AssertType(x.p, "number");

    x.f();
AssertType(x.f(), "number");
AssertType(x.f, "() => number");

    Mixed3.p;
AssertType(Mixed3.p, "number");

    Mixed3.f();
AssertType(Mixed3.f(), "number");
AssertType(Mixed3.f, "() => number");
}

function f5() {
    let x = new Mixed4("hello");
AssertType(x, "C1 & M1 & M2");
AssertType(new Mixed4("hello"), "C1 & M1 & M2");
AssertType(Mixed4, "typeof C1 & typeof M1 & typeof M2");
AssertType("hello", "string");

    x.a;
AssertType(x.a, "number");

    x.p;
AssertType(x.p, "number");

    x.f();
AssertType(x.f(), "number");
AssertType(x.f, "() => number");

    Mixed4.p;
AssertType(Mixed4.p, "number");

    Mixed4.f();
AssertType(Mixed4.f(), "number");
AssertType(Mixed4.f, "() => number");
}

function f6() {
    let x = new Mixed5();
AssertType(x, "M1 & M2");
AssertType(new Mixed5(), "M1 & M2");
AssertType(Mixed5, "typeof M1 & typeof M2");

    x.p;
AssertType(x.p, "number");

    x.f();
AssertType(x.f(), "number");
AssertType(x.f, "() => number");

    Mixed5.p;
AssertType(Mixed5.p, "number");

    Mixed5.f();
AssertType(Mixed5.f(), "number");
AssertType(Mixed5.f, "() => number");
}

class C2 extends Mixed1 {
    constructor() {
        super("hello");
AssertType(super("hello"), "void");
AssertType(super, "typeof M1 & typeof C1");
AssertType("hello", "string");

        this.a;
AssertType(this.a, "number");
AssertType(this, "this");

        this.b;
AssertType(this.b, "number");
AssertType(this, "this");

        this.p;
AssertType(this.p, "number");
AssertType(this, "this");
    }
}

class C3 extends Mixed3 {
    constructor() {
        super(42);
AssertType(super(42), "void");
AssertType(super, "typeof M2 & typeof M1 & typeof C1");
AssertType(42, "int");

        this.a;
AssertType(this.a, "number");
AssertType(this, "this");

        this.b;
AssertType(this.b, "number");
AssertType(this, "this");

        this.p;
AssertType(this.p, "number");
AssertType(this, "this");

        this.f();
AssertType(this.f(), "number");
AssertType(this.f, "() => number");
AssertType(this, "this");
    }
    f() { 
AssertType(super.f(), "number");
return super.f(); 

AssertType(super.f, "() => number");

AssertType(super, "M2 & M1 & C1");
}
}


