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

// === tests/cases/conformance/types/typeRelationships/subtypesAndSuperTypes/subtypingWithObjectMembersOptionality.ts ===
declare function AssertType(value:any, type:string):void;
// Derived member is not optional but base member is, should be ok

interface Base { foo: string; }
interface Derived extends Base { bar: string; }
interface Derived2 extends Derived { baz: string; }

// S is a subtype of a type T, and T is a supertype of S, if one of the following is true, where S' denotes the apparent type (section 3.8.1) of S:
//   - S' and T are object types and, for each member M in T, one of the following is true:
//      - M is a property and S' contains a property N where
//          M and N have the same name,
//          the type of N is a subtype of that of M,
//          M and N are both public or both private, and
//          if M is a required property, N is also a required property.
//      - M is an optional property and S' contains no property of the same name as M.
interface T {
    Foo?: Base;
}

interface S extends T {
    Foo: Derived
}

interface T2 {
    1?: Base; 
}

interface S2 extends T2 {
    1: Derived;
}

interface T3 {
    '1'?: Base;
}

interface S3 extends T3 {
    '1.': Derived;
}

// object literal case
let a: { Foo?: Base; };
AssertType(a, "{ Foo?: Base; }");
AssertType(Foo, "Base");

let b = { Foo: <Derived>null };
AssertType(b, "{ Foo: Derived; }");
AssertType({ Foo: <Derived>null }, "{ Foo: Derived; }");
AssertType(Foo, "Derived");
AssertType(<Derived>null, "Derived");
AssertType(null, "null");

let r = true ? a : b;
AssertType(r, "{ Foo?: Base; }");
AssertType(true ? a : b, "{ Foo?: Base; }");
AssertType(true, "boolean");
AssertType(a, "{ Foo?: Base; }");
AssertType(b, "{ Foo: Derived; }");

module TwoLevels {
    interface T {
        Foo?: Base;
    }

    interface S extends T {
        Foo: Derived2 
    }

    interface T2 {
        1?: Base;
    }

    interface S2 extends T2 {
        1: Derived2;
    }

    interface T3 {
        '1'?: Base;
    }

    interface S3 extends T3 {
        '1.': Derived2;
    }

    // object literal case
    let a: { Foo?: Base; };
    let b = { Foo: <Derived2>null };
    let r = true ? a : b;
}

