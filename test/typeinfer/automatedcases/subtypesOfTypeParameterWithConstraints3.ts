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

// === tests/cases/conformance/types/typeRelationships/subtypesAndSuperTypes/subtypesOfTypeParameterWithConstraints3.ts ===
declare function AssertType(value:any, type:string):void;
// checking whether other types are subtypes of type parameters with constraints

function f<T extends U, U, V>(t: T, u: U, v: V) {
    // ok
    let r = true ? t : u;
AssertType(r, "U");
AssertType(true ? t : u, "U");
AssertType(true, "boolean");
AssertType(t, "T");
AssertType(u, "U");

    let r = true ? u : t;
AssertType(r, "U");
AssertType(true ? u : t, "U");
AssertType(true, "boolean");
AssertType(u, "U");
AssertType(t, "T");

    // ok
    let r2 = true ? t : v;
AssertType(r2, "union");
AssertType(true ? t : v, "union");
AssertType(true, "boolean");
AssertType(t, "T");
AssertType(v, "V");

    let r2 = true ? v : t;
AssertType(r2, "union");
AssertType(true ? v : t, "union");
AssertType(true, "boolean");
AssertType(v, "V");
AssertType(t, "T");

    // ok
    let r3 = true ? v : u;
AssertType(r3, "union");
AssertType(true ? v : u, "union");
AssertType(true, "boolean");
AssertType(v, "V");
AssertType(u, "U");

    let r3 = true ? u : v;
AssertType(r3, "union");
AssertType(true ? u : v, "union");
AssertType(true, "boolean");
AssertType(u, "U");
AssertType(v, "V");
}

