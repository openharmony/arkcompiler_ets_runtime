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

// === tests/cases/conformance/types/members/typesWithSpecializedConstructSignatures.ts ===
declare function AssertType(value:any, type:string):void;
// basic uses of specialized signatures without errors

class Base { foo: string }
class Derived1 extends Base { bar: string }
class Derived2 extends Base { baz: string }

class C {
    constructor(x: 'hi');
    constructor(x: 'bye');
    constructor(x: string);
    constructor(x) {
AssertType(x, "any");
        return x;
    }
}
let c = new C('a');
AssertType(c, "C");
AssertType(new C('a'), "C");
AssertType(C, "typeof C");
AssertType('a', "string");

interface I {
    new(x: 'hi'): Derived1;
    new(x: 'bye'): Derived2;
    new(x: string): Base;
}
let i: I;
AssertType(i, "I");

let a: {
AssertType(a, "{ new ('hi'): Derived1; new ('bye'): Derived2; new (string): Base; }");

    new(x: 'hi'): Derived1;
AssertType(x, "string");

    new(x: 'bye'): Derived2;
AssertType(x, "string");

    new(x: string): Base;
AssertType(x, "string");

};

c = i;
AssertType(c = i, "I");
AssertType(c, "C");
AssertType(i, "I");

c = a;
AssertType(c = a, "{ new ("hi"): Derived1; new ("bye"): Derived2; new (string): Base; }");
AssertType(c, "C");
AssertType(a, "{ new ("hi"): Derived1; new ("bye"): Derived2; new (string): Base; }");

i = a;
AssertType(i = a, "{ new ("hi"): Derived1; new ("bye"): Derived2; new (string): Base; }");
AssertType(i, "I");
AssertType(a, "{ new ("hi"): Derived1; new ("bye"): Derived2; new (string): Base; }");

a = i;
AssertType(a = i, "I");
AssertType(a, "{ new ("hi"): Derived1; new ("bye"): Derived2; new (string): Base; }");
AssertType(i, "I");

let r1 = new C('hi');
AssertType(r1, "C");
AssertType(new C('hi'), "C");
AssertType(C, "typeof C");
AssertType('hi', "string");

let r2: Derived2 = new i('bye');
AssertType(r2, "Derived2");
AssertType(new i('bye'), "Derived2");
AssertType(i, "I");
AssertType('bye', "string");

let r3: Base = new a('hm');
AssertType(r3, "Base");
AssertType(new a('hm'), "Base");
AssertType(a, "{ new ("hi"): Derived1; new ("bye"): Derived2; new (string): Base; }");
AssertType('hm', "string");


