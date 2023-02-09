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

// === tests/cases/conformance/types/typeRelationships/bestCommonType/bestCommonTypeOfConditionalExpressions2.ts ===
declare function AssertType(value:any, type:string):void;
// conditional expressions return the best common type of the branches plus contextual type (using the first candidate if multiple BCTs exist)
// these are errors

class Base { foo: string; }
class Derived extends Base { bar: string; }
class Derived2 extends Base { baz: string; }
let base: Base;
AssertType(base, "Base");

let derived: Derived;
AssertType(derived, "Derived");

let derived2: Derived2;
AssertType(derived2, "Derived2");

let r2 = true ? 1 : '';
AssertType(r2, "union");
AssertType(true ? 1 : '', "union");
AssertType(true, "boolean");
AssertType(1, "int");
AssertType('', "string");

let r9 = true ? derived : derived2;
AssertType(r9, "union");
AssertType(true ? derived : derived2, "union");
AssertType(true, "boolean");
AssertType(derived, "Derived");
AssertType(derived2, "Derived2");

function foo<T, U>(t: T, u: U) {
AssertType(true ? t : u, "union");
AssertType(true, "boolean");
AssertType(t, "T");
AssertType(u, "U");
    return true ? t : u;
}

function foo2<T extends U, U>(t: T, u: U) { // Error for referencing own type parameter
AssertType(true ? t : u, "U");
AssertType(true, "boolean");
AssertType(t, "T");
AssertType(u, "U");
    return true ? t : u; // Ok because BCT(T, U) = U
}

function foo3<T extends U, U extends V, V>(t: T, u: U) {
AssertType(true ? t : u, "U");
AssertType(true, "boolean");
AssertType(t, "T");
AssertType(u, "U");
    return true ? t : u;
}

