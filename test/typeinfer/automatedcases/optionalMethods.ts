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

// === tests/cases/conformance/types/namedTypes/optionalMethods.ts ===
declare function AssertType(value:any, type:string):void;
interface Foo {
    a: number;
    b?: number;
    f(): number;
    g?(): number;
}

function test1(x: Foo) {
    x.a;
AssertType(x.a, "number");

    x.b;
AssertType(x.b, "union");

    x.f;
AssertType(x.f, "() => number");

    x.g;
AssertType(x.g, "union");

    let f1 = x.f();
AssertType(f1, "number");
AssertType(x.f(), "number");
AssertType(x.f, "() => number");

    let g1 = x.g && x.g();
AssertType(g1, "union");
AssertType(x.g && x.g(), "union");
AssertType(x.g, "union");
AssertType(x.g(), "number");
AssertType(x.g, "() => number");

    let g2 = x.g ? x.g() : 0;
AssertType(g2, "number");
AssertType(x.g ? x.g() : 0, "number");
AssertType(x.g, "union");
AssertType(x.g(), "number");
AssertType(x.g, "() => number");
AssertType(0, "int");
}

class Bar {
    a: number;
    b?: number;
    c? = 2;
    constructor(public d?: number, public e = 10) {}
    f() {
AssertType(1, "int");
        return 1;
    }
    g?(): number;  // Body of optional method can be omitted
    h?() {
AssertType(2, "int");
        return 2;
    }
}

function test2(x: Bar) {
    x.a;
AssertType(x.a, "number");

    x.b;
AssertType(x.b, "union");

    x.c;
AssertType(x.c, "union");

    x.d;
AssertType(x.d, "union");

    x.e;
AssertType(x.e, "number");

    x.f;
AssertType(x.f, "() => number");

    x.g;
AssertType(x.g, "union");

    let f1 = x.f();
AssertType(f1, "number");
AssertType(x.f(), "number");
AssertType(x.f, "() => number");

    let g1 = x.g && x.g();
AssertType(g1, "union");
AssertType(x.g && x.g(), "union");
AssertType(x.g, "union");
AssertType(x.g(), "number");
AssertType(x.g, "() => number");

    let g2 = x.g ? x.g() : 0;
AssertType(g2, "number");
AssertType(x.g ? x.g() : 0, "number");
AssertType(x.g, "union");
AssertType(x.g(), "number");
AssertType(x.g, "() => number");
AssertType(0, "int");

    let h1 = x.h && x.h();
AssertType(h1, "union");
AssertType(x.h && x.h(), "union");
AssertType(x.h, "union");
AssertType(x.h(), "number");
AssertType(x.h, "() => number");

    let h2 = x.h ? x.h() : 0;
AssertType(h2, "number");
AssertType(x.h ? x.h() : 0, "number");
AssertType(x.h, "union");
AssertType(x.h(), "number");
AssertType(x.h, "() => number");
AssertType(0, "int");
}

class Base {
    a?: number;
    f?(): number;
}

class Derived extends Base {
    a = 1;
    f(): number { 
AssertType(1, "int");
return 1; 
}
}


