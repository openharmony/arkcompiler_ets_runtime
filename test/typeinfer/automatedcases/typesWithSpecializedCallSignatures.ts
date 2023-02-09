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

// === tests/cases/conformance/types/members/typesWithSpecializedCallSignatures.ts ===
declare function AssertType(value:any, type:string):void;
// basic uses of specialized signatures without errors

class Base { foo: string }
class Derived1 extends Base { bar: string }
class Derived2 extends Base { baz: string }

class C {
    foo(x: 'hi'): Derived1;
    foo(x: 'bye'): Derived2;
    foo(x: string): Base;
    foo(x) {
AssertType(x, "any");
        return x;
    }
}
let c = new C();
AssertType(c, "C");
AssertType(new C(), "C");
AssertType(C, "typeof C");

interface I {
    foo(x: 'hi'): Derived1;
    foo(x: 'bye'): Derived2;
    foo(x: string): Base;
}
let i: I;
AssertType(i, "I");

let a: {
AssertType(a, "{ foo('hi'): Derived1; foo('bye'): Derived2; foo(string): Base; }");

    foo(x: 'hi'): Derived1;
AssertType(foo, "{ ('hi'): Derived1; ("bye"): Derived2; (string): Base; }");
AssertType(x, "string");

    foo(x: 'bye'): Derived2;
AssertType(foo, "{ ("hi"): Derived1; ('bye'): Derived2; (string): Base; }");
AssertType(x, "string");

    foo(x: string): Base;
AssertType(foo, "{ ("hi"): Derived1; ("bye"): Derived2; (string): Base; }");
AssertType(x, "string");

};

c = i;
AssertType(c = i, "I");
AssertType(c, "C");
AssertType(i, "I");

c = a;
AssertType(c = a, "{ foo("hi"): Derived1; foo("bye"): Derived2; foo(string): Base; }");
AssertType(c, "C");
AssertType(a, "{ foo("hi"): Derived1; foo("bye"): Derived2; foo(string): Base; }");

i = c;
AssertType(i = c, "C");
AssertType(i, "I");
AssertType(c, "C");

i = a;
AssertType(i = a, "{ foo("hi"): Derived1; foo("bye"): Derived2; foo(string): Base; }");
AssertType(i, "I");
AssertType(a, "{ foo("hi"): Derived1; foo("bye"): Derived2; foo(string): Base; }");

a = c;
AssertType(a = c, "C");
AssertType(a, "{ foo("hi"): Derived1; foo("bye"): Derived2; foo(string): Base; }");
AssertType(c, "C");

a = i;
AssertType(a = i, "I");
AssertType(a, "{ foo("hi"): Derived1; foo("bye"): Derived2; foo(string): Base; }");
AssertType(i, "I");

let r1: Derived1 = c.foo('hi');
AssertType(r1, "Derived1");
AssertType(c.foo('hi'), "Derived1");
AssertType(c.foo, "{ ("hi"): Derived1; ("bye"): Derived2; (string): Base; }");
AssertType('hi', "string");

let r2: Derived2 = c.foo('bye');
AssertType(r2, "Derived2");
AssertType(c.foo('bye'), "Derived2");
AssertType(c.foo, "{ ("hi"): Derived1; ("bye"): Derived2; (string): Base; }");
AssertType('bye', "string");

let r3: Base = c.foo('hm');
AssertType(r3, "Base");
AssertType(c.foo('hm'), "Base");
AssertType(c.foo, "{ ("hi"): Derived1; ("bye"): Derived2; (string): Base; }");
AssertType('hm', "string");


