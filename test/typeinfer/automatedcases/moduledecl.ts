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

// === tests/cases/compiler/moduledecl.ts ===
declare function AssertType(value:any, type:string):void;
module a {
}

module b.a {
}

module c.a.b {
    import ma = a;
}

module mImport {
    import d = a;
    import e = b.a;
    import d1 = a;
    import e1 = b.a;
}

module m0 {
    function f1() {
    }

    function f2(s: string);
    function f2(n: number);
    function f2(ns: any) {
    }

    class c1 {
        public a : ()=>string;
        private b: ()=>number;
        private static s1;
        public static s2;
    }

    interface i1 {
        () : Object;
        [n: number]: c1;
    }

    import m2 = a;
    import m3 = b;
    import m4 = b.a;
    import m5 = c;
    import m6 = c.a;
    import m7 = c.a.b;
}

module m1 {
    export function f1() {
    }

    export function f2(s: string);
    export function f2(n: number);
    export function f2(ns: any) {
    }

    export class c1 {
        public a: () =>string;
        private b: () =>number;
        private static s1;
        public static s2;

        public d() {
AssertType("Hello", "string");
            return "Hello";
        }

        public e: { x: number; y: string; };
        constructor (public n, public n2: number, private n3, private n4: string) {
        }
    }

    export interface i1 {
        () : Object;
        [n: number]: c1;
    }

    import m2 = a;
    import m3 = b;
    import m4 = b.a;
    import m5 = c;
    import m6 = c.a;
    import m7 = c.a.b;
}

module m {
    export module m2 {
        let a = 10;
        export let b: number;
    }

    export module m3 {
        export let c: number;
    }
}

module m {

    export module m25 {
        export module m5 {
            export let c: number;
        }
    }
}

module m13 {
    export module m4 {
        export module m2 {
            export module m3 {
                export let c: number;
            }
        }

        export function f() {
AssertType(20, "int");
            return 20;
        }
    }
}

declare module m4 {
    export let b;
}

declare module m5 {
    export let c;
}

declare module m43 {
    export let b;
}

declare module m55 {
    export let c;
}

declare module "m3" {
    export let b: number;
}

module exportTests {
    export class C1_public {
        private f2() {
AssertType(30, "int");
            return 30;
        }

        public f3() {
AssertType("string", "string");
            return "string";
        }
    }
    class C2_private {
        private f2() {
AssertType(30, "int");
            return 30;
        }

        public f3() {
AssertType("string", "string");
            return "string";
        }
    }

    export class C3_public {
        private getC2_private() {
AssertType(new C2_private(), "C2_private");
AssertType(C2_private, "typeof C2_private");
            return new C2_private();
        }
        private setC2_private(arg: C2_private) {
        }
        private get c2() {
AssertType(new C2_private(), "C2_private");
AssertType(C2_private, "typeof C2_private");
            return new C2_private();
        }
        public getC1_public() {
AssertType(new C1_public(), "C1_public");
AssertType(C1_public, "typeof C1_public");
            return new C1_public();
        }
        public setC1_public(arg: C1_public) {
        }
        public get c1() {
AssertType(new C1_public(), "C1_public");
AssertType(C1_public, "typeof C1_public");
            return new C1_public();
        }
    }
}

declare module mAmbient {
    class C {
        public myProp: number;
    }

    function foo() : C;
    let aVar: C;
    interface B {
        x: number;
        y: C;
    }
    enum e {
        x,
        y,
        z
    }

    module m3 {
        class C {
            public myProp: number;
        }

        function foo(): C;
        let aVar: C;
        interface B {
            x: number;
            y: C;
        }
        enum e {
            x,
            y,
            z
        }
    }
}

function foo() {
AssertType(mAmbient.foo(), "mAmbient.C");
AssertType(mAmbient.foo, "() => mAmbient.C");
    return mAmbient.foo();
}

let cVar = new mAmbient.C();
AssertType(cVar, "mAmbient.C");
AssertType(new mAmbient.C(), "mAmbient.C");
AssertType(mAmbient.C, "typeof mAmbient.C");

let aVar = mAmbient.aVar;
AssertType(aVar, "mAmbient.C");
AssertType(mAmbient.aVar, "mAmbient.C");

let bB: mAmbient.B;
AssertType(bB, "mAmbient.B");
AssertType(mAmbient, "any");

let eVar: mAmbient.e;
AssertType(eVar, "mAmbient.e");
AssertType(mAmbient, "any");

function m3foo() {
AssertType(mAmbient.m3.foo(), "mAmbient.m3.C");
AssertType(mAmbient.m3.foo, "() => mAmbient.m3.C");
AssertType(mAmbient.m3, "typeof mAmbient.m3");
    return mAmbient.m3.foo();
}

let m3cVar = new mAmbient.m3.C();
AssertType(m3cVar, "mAmbient.m3.C");
AssertType(new mAmbient.m3.C(), "mAmbient.m3.C");
AssertType(mAmbient.m3.C, "typeof mAmbient.m3.C");

let m3aVar = mAmbient.m3.aVar;
AssertType(m3aVar, "mAmbient.m3.C");
AssertType(mAmbient.m3.aVar, "mAmbient.m3.C");

let m3bB: mAmbient.m3.B;
AssertType(m3bB, "mAmbient.m3.B");
AssertType(mAmbient, "any");
AssertType(m3, "any");

let m3eVar: mAmbient.m3.e;
AssertType(m3eVar, "mAmbient.m3.e");
AssertType(mAmbient, "any");
AssertType(m3, "any");



