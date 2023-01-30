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

// === tests/cases/conformance/types/typeRelationships/assignmentCompatibility/constructSignatureAssignabilityInInheritance4.ts ===
declare function AssertType(value:any, type:string):void;

// checking subtype relations for function types as it relates to contextual signature instantiation

class Base { foo: string; }
class Derived extends Base { bar: string; }
class Derived2 extends Derived { baz: string; }
class OtherDerived extends Base { bing: string; }

interface A { // T
    // M's
    a: new <T>(x: T) => T[];
    a2: new <T>(x: T) => string[];
    a3: new <T>(x: T) => void;
    a4: new <T, U>(x: T, y: U) => string;
    a5: new <T, U>(x: (arg: T) => U) => T;
    a6: new <T extends Base>(x: (arg: T) => Derived) => T;
    a11: new <T>(x: { foo: T }, y: { foo: T; bar: T }) => Base;
    a15: new <T>(x: { a: T; b: T }) => T[];
    a16: new <T extends Base>(x: { a: T; b: T }) => T[];
    a17: {
        new <T extends Base>(x: T): T[];
        new <U extends Derived>(x: U): U[];
    };
    a18: {
        new <T extends Derived>(x: T): number[];
        new <U extends Base>(x: U): number[];
    };
    a19: {
        new <T extends Derived>(x: new (a: T) => T): T[];
        new <U extends Base>(x: new (a: U) => U): U[];
    };
    a20: {
        new (x: {
            new <T extends Derived>(a: T): T;
            new <U extends Base>(a: U): U;
        }): any[];
        new (x: {
            new <T extends Base>(a: T): T;
            new <U extends Derived2>(a: U): U;
        }): any[];
    };
}

// S's
interface I extends A {
    // N's
    a: new <T>(x: T) => T[]; // ok, instantiation of N is a subtype of M, T is number
    a2: new <T>(x: T) => string[]; // ok
    a3: new <T>(x: T) => T; // ok since Base returns void
    a4: new <T, U>(x: T, y: U) => string; // ok, instantiation of N is a subtype of M, T is string, U is number
    a5: new <T, U>(x: (arg: T) => U) => T; // ok, U is in a parameter position so inferences can be made
    a6: new <T extends Base, U extends Derived>(x: (arg: T) => U) => T; // ok, same as a5 but with object type hierarchy
    a11: new <T, U>(x: { foo: T }, y: { foo: U; bar: U }) => Base; // ok
    a15: new <U, V>(x: { a: U; b: V; }) => U[]; // ok, T = U, T = V
    a16: new <T>(x: { a: T; b: T }) => T[]; // ok, more general parameter type
    a17: new <T extends Base>(x: T) => T[]; // ok, more general parameter type
    a18: new <T extends Base>(x: T) => number[]; // ok, more general parameter type
    a19: new <T extends Base>(x: new (a: T) => T) => T[]; // ok
    a20: new (x: new <T extends Base>(a: T) => T) => any[]; // ok
}

