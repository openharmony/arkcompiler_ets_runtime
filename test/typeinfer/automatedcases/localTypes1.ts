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

// === tests/cases/conformance/types/localTypes/localTypes1.ts ===
declare function AssertType(value:any, type:string):void;
function f1() {
    enum E {
AssertType(E, "E");

        A, B, C
AssertType(A, "E.A");
AssertType(B, "E.B");
AssertType(C, "E.C");
    }
    class C {
        x: E;
    }
    interface I {
        x: E;
AssertType(x, "E");
    }
    type A = I[];
AssertType(A, "I[]");

    let a: A = [new C()];
AssertType(a, "I[]");
AssertType([new C()], "C[]");
AssertType(new C(), "C");
AssertType(C, "typeof C");

    a[0].x = E.B;
AssertType(a[0].x = E.B, "E.B");
AssertType(a[0].x, "E");
AssertType(a[0], "I");
AssertType(a, "I[]");
AssertType(0, "int");
AssertType(E.B, "E.B");

AssertType(a, "I[]");
    return a;
}

function f2() {
    function g() {
AssertType(g, "() => I[]");

        enum E {
AssertType(E, "E");

            A, B, C
AssertType(A, "E.A");
AssertType(B, "E.B");
AssertType(C, "E.C");
        }
        class C {
            x: E;
        }
        interface I {
            x: E;
AssertType(x, "E");
        }
        type A = I[];
AssertType(A, "I[]");

        let a: A = [new C()];
AssertType(a, "I[]");
AssertType([new C()], "C[]");
AssertType(new C(), "C");
AssertType(C, "typeof C");

        a[0].x = E.B;
AssertType(a[0].x = E.B, "E.B");
AssertType(a[0].x, "E");
AssertType(a[0], "I");
AssertType(a, "I[]");
AssertType(0, "int");
AssertType(E.B, "E.B");

AssertType(a, "I[]");
        return a;
    }
AssertType(g(), "I[]");
AssertType(g, "() => I[]");
    return g();
}

function f3(b: boolean) {
    if (true) {
AssertType(true, "boolean");

        enum E {
AssertType(E, "E");

            A, B, C
AssertType(A, "E.A");
AssertType(B, "E.B");
AssertType(C, "E.C");
        }
        if (b) {
AssertType(b, "boolean");

            class C {
                x: E;
            }
            interface I {
                x: E;
AssertType(x, "E");
            }
            type A = I[];
AssertType(A, "I[]");

            let a: A = [new C()];
AssertType(a, "I[]");
AssertType([new C()], "C[]");
AssertType(new C(), "C");
AssertType(C, "typeof C");

            a[0].x = E.B;
AssertType(a[0].x = E.B, "E.B");
AssertType(a[0].x, "E");
AssertType(a[0], "I");
AssertType(a, "I[]");
AssertType(0, "int");
AssertType(E.B, "E.B");

AssertType(a, "I[]");
            return a;
        }
        else {
            class A {
                x: E;
            }
            interface J {
                x: E;
AssertType(x, "E");
            }
            type C = J[];
AssertType(C, "J[]");

            let c: C = [new A()];
AssertType(c, "J[]");
AssertType([new A()], "A[]");
AssertType(new A(), "A");
AssertType(A, "typeof A");

            c[0].x = E.B;
AssertType(c[0].x = E.B, "E.B");
AssertType(c[0].x, "E");
AssertType(c[0], "J");
AssertType(c, "J[]");
AssertType(0, "int");
AssertType(E.B, "E.B");

AssertType(c, "J[]");
            return c;
        }
    }
}

function f5() {
    let z1 = function () {
AssertType(z1, "() => C");
AssertType(function () {        enum E {            A, B, C        }        class C {            x: E;        }        return new C();    }, "() => C");

        enum E {
AssertType(E, "E");

            A, B, C
AssertType(A, "E.A");
AssertType(B, "E.B");
AssertType(C, "E.C");
        }
        class C {
            x: E;
        }
AssertType(new C(), "C");
AssertType(C, "typeof C");
        return new C();
    }
    let z2 = () => {
AssertType(z2, "() => C");
AssertType(() => {        enum E {            A, B, C        }        class C {            x: E;        }        return new C();    }, "() => C");

        enum E {
AssertType(E, "E");

            A, B, C
AssertType(A, "E.A");
AssertType(B, "E.B");
AssertType(C, "E.C");
        }
        class C {
            x: E;
        }
AssertType(new C(), "C");
AssertType(C, "typeof C");
        return new C();
    }
}

class A {
    constructor() {
        enum E {
AssertType(E, "E");

            A, B, C
AssertType(A, "E.A");
AssertType(B, "E.B");
AssertType(C, "E.C");
        }
        class C {
            x: E;
        }
    }
    m() {
        enum E {
AssertType(E, "E");

            A, B, C
AssertType(A, "E.A");
AssertType(B, "E.B");
AssertType(C, "E.C");
        }
        class C {
            x: E;
        }
AssertType(new C(), "C");
AssertType(C, "typeof C");
        return new C();
    }
    get p() {
        enum E {
AssertType(E, "E");

            A, B, C
AssertType(A, "E.A");
AssertType(B, "E.B");
AssertType(C, "E.C");
        }
        class C {
            x: E;
        }
AssertType(new C(), "C");
AssertType(C, "typeof C");
        return new C();
    }
}

function f6() {
    class A {
        a: string;
    }
    function g() {
AssertType(g, "() => C");

        class B extends A {
            b: string;
        }
        function h() {
AssertType(h, "() => C");

            class C extends B {
                c: string;
            }
            let x = new C();
AssertType(x, "C");
AssertType(new C(), "C");
AssertType(C, "typeof C");

            x.a = "a";
AssertType(x.a = "a", "string");
AssertType(x.a, "string");
AssertType("a", "string");

            x.b = "b";
AssertType(x.b = "b", "string");
AssertType(x.b, "string");
AssertType("b", "string");

            x.c = "c";
AssertType(x.c = "c", "string");
AssertType(x.c, "string");
AssertType("c", "string");

AssertType(x, "C");
            return x;
        }
AssertType(h(), "C");
AssertType(h, "() => C");
        return h();
    }
AssertType(g(), "C");
AssertType(g, "() => C");
    return g();
}


