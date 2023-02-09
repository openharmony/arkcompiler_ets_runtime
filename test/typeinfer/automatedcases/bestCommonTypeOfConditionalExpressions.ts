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

// === tests/cases/conformance/types/typeRelationships/bestCommonType/bestCommonTypeOfConditionalExpressions.ts ===
declare function AssertType(value:any, type:string):void;
// conditional expressions return the best common type of the branches plus contextual type (using the first candidate if multiple BCTs exist)
// no errors expected here

let a: { x: number; y?: number };
AssertType(a, "{ x: number; y?: number; }");
AssertType(x, "number");
AssertType(y, "number");

let b: { x: number; z?: number };
AssertType(b, "{ x: number; z?: number; }");
AssertType(x, "number");
AssertType(z, "number");

class Base { foo: string; }
class Derived extends Base { bar: string; }
class Derived2 extends Base { baz: string; }
let base: Base;
AssertType(base, "Base");

let derived: Derived;
AssertType(derived, "Derived");

let derived2: Derived2;
AssertType(derived2, "Derived2");

let r = true ? 1 : 2;
AssertType(r, "number");
AssertType(true ? 1 : 2, "union");
AssertType(true, "boolean");
AssertType(1, "int");
AssertType(2, "int");

let r3 = true ? 1 : {};
AssertType(r3, "{}");
AssertType(true ? 1 : {}, "union");
AssertType(true, "boolean");
AssertType(1, "int");
AssertType({}, "{}");

let r4 = true ? a : b; // typeof a
AssertType(r4, "union");
AssertType(true ? a : b, "union");
AssertType(true, "boolean");
AssertType(a, "{ x: number; y?: number; }");
AssertType(b, "{ x: number; z?: number; }");

let r5 = true ? b : a; // typeof b
AssertType(r5, "union");
AssertType(true ? b : a, "union");
AssertType(true, "boolean");
AssertType(b, "{ x: number; z?: number; }");
AssertType(a, "{ x: number; y?: number; }");

let r6 = true ? (x: number) => { } : (x: Object) => { }; // 
AssertType(r6, "union");
AssertType(true ? (x: number) => { } : (x: Object) => { }, "union");
AssertType(true, "boolean");
AssertType((x: number) => { }, "(number) => void");
AssertType(x, "number");
AssertType((x: Object) => { }, "(Object) => void");
AssertType(x, "Object");
returns number => void

let r7: (x: Object) => void = true ? (x: number) => { } : (x: Object) => { }; 
AssertType(r7, "(Object) => void");
AssertType(x, "Object");
AssertType(true ? (x: number) => { } : (x: Object) => { }, "union");
AssertType(true, "boolean");
AssertType((x: number) => { }, "(number) => void");
AssertType(x, "number");
AssertType((x: Object) => { }, "(Object) => void");
AssertType(x, "Object");

let r8 = true ? (x: Object) => { } : (x: number) => { }; // 
AssertType(r8, "union");
AssertType(true ? (x: Object) => { } : (x: number) => { }, "union");
AssertType(true, "boolean");
AssertType((x: Object) => { }, "(Object) => void");
AssertType(x, "Object");
AssertType((x: number) => { }, "(number) => void");
AssertType(x, "number");
returns Object => void

let r10: Base = true ? derived : derived2; // no error since we use the contextual type in BCT
AssertType(r10, "Base");
AssertType(true ? derived : derived2, "union");
AssertType(true, "boolean");
AssertType(derived, "Derived");
AssertType(derived2, "Derived2");

let r11 = true ? base : derived2;
AssertType(r11, "Base");
AssertType(true ? base : derived2, "Base");
AssertType(true, "boolean");
AssertType(base, "Base");
AssertType(derived2, "Derived2");

function foo5<T, U>(t: T, u: U): Object {
AssertType(true ? t : u, "union");
AssertType(true, "boolean");
AssertType(t, "T");
AssertType(u, "U");
    return true ? t : u; // BCT is Object
}

