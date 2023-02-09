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

// === tests/cases/conformance/types/typeRelationships/subtypesAndSuperTypes/subtypingWithObjectMembersOptionality3.ts ===
declare function AssertType(value:any, type:string):void;
// Base property is optional and derived type has no property of that name

interface Base { foo: string; }
interface Derived extends Base { bar: string; }

interface T {
    Foo?: Base;
}

interface S extends T {
    Foo2: Derived // ok
}

interface T2 {
    1?: Base;
}

interface S2 extends T2 {
    2: Derived; // ok
}

interface T3 {
    '1'?: Base;
}

interface S3 extends T3 {
    '1.0': Derived; // ok
}

// object literal case
let a: { Foo?: Base; 
AssertType(a, "{ Foo?: Base; }");

AssertType(Foo, "Base");
}

let b: { Foo2: Derived; 
AssertType(b, "{ Foo2: Derived; }");

AssertType(Foo2, "Derived");
}

let r = true ? a : b; // ok
AssertType(r, "union");
AssertType(true ? a : b, "union");
AssertType(true, "boolean");
AssertType(a, "{ Foo?: Base; }");
AssertType(b, "{ Foo2: Derived; }");


