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

// === tests/cases/conformance/types/union/contextualTypeWithUnionTypeIndexSignatures.ts ===
declare function AssertType(value:any, type:string):void;
//When used as a contextual type, a union type U has those members that are present in any of 
// its constituent types, with types that are unions of the respective members in the constituent types. 
interface SomeType {
    (a: number): number;
}
interface SomeType2 {
    (a: number): string;
}

interface IWithNoStringIndexSignature {
    foo: string;
}
interface IWithNoNumberIndexSignature {
    0: string;
}
interface IWithStringIndexSignature1 {
    [a: string]: SomeType;
}
interface IWithStringIndexSignature2 {
    [a: string]: SomeType2;
}
interface IWithNumberIndexSignature1 {
    [a: number]: SomeType;
}
interface IWithNumberIndexSignature2 {
    [a: number]: SomeType2;
}

// When an object literal is contextually typed by a type that includes a string index signature, 
// the resulting type of the object literal includes a string index signature with the union type of 
// the types of the properties declared in the object literal, or the Undefined type if the object literal 
// is empty.Likewise, when an object literal is contextually typed by a type that includes a numeric index 
// signature, the resulting type of the object literal includes a numeric index signature with the union type
// of the types of the numerically named properties(section 3.7.4) declared in the object literal, 
// or the Undefined type if the object literal declares no numerically named properties.

// Let S be the set of types in U that has a string index signature.
// If S is not empty, U has a string index signature of a union type of 
// the types of the string index signatures from each type in S.
let x: IWithNoStringIndexSignature | IWithStringIndexSignature1 = { z: a => a }; // a should be number
AssertType(x, "union");
AssertType({ z: a => a }, "{ z: (number) => number; }");
AssertType(z, "(number) => number");
AssertType(a => a, "(number) => number");
AssertType(a, "number");
AssertType(a, "number");

let x: IWithNoStringIndexSignature | IWithStringIndexSignature1 = { foo: a => a }; // a should be number (because of index signature of IWithStringIndexSignature1)
AssertType(x, "union");
AssertType({ foo: a => a }, "{ foo: (number) => number; }");
AssertType(foo, "(number) => number");
AssertType(a => a, "(number) => number");
AssertType(a, "number");
AssertType(a, "number");

let x: IWithNoStringIndexSignature | IWithStringIndexSignature1 = { foo: "hello" }; 
AssertType(x, "union");
AssertType({ foo: "hello" }, "{ foo: string; }");
AssertType(foo, "string");
AssertType("hello", "string");

let x2: IWithStringIndexSignature1 | IWithStringIndexSignature2 = { z: a => a.toString() }; // a should be number
AssertType(x2, "union");
AssertType({ z: a => a.toString() }, "{ z: (number) => string; }");
AssertType(z, "(number) => string");
AssertType(a => a.toString(), "(number) => string");
AssertType(a, "number");
AssertType(a.toString(), "string");
AssertType(a.toString, "(?number) => string");

let x2: IWithStringIndexSignature1 | IWithStringIndexSignature2 = { z: a => a }; // a should be number
AssertType(x2, "union");
AssertType({ z: a => a }, "{ z: (number) => number; }");
AssertType(z, "(number) => number");
AssertType(a => a, "(number) => number");
AssertType(a, "number");
AssertType(a, "number");


// Let S be the set of types in U that has a numeric index signature.
// If S is not empty, U has a numeric index signature of a union type of 
// the types of the numeric index signatures from each type in S.
let x3: IWithNoNumberIndexSignature | IWithNumberIndexSignature1 = { 1: a => a }; // a should be number
AssertType(x3, "union");
AssertType({ 1: a => a }, "{ 1: (number) => number; }");
AssertType(1, "(number) => number");
AssertType(a => a, "(number) => number");
AssertType(a, "number");
AssertType(a, "number");

let x3: IWithNoNumberIndexSignature | IWithNumberIndexSignature1 = { 0: a => a }; // a should be number (because of index signature of IWithNumberIndexSignature1)
AssertType(x3, "union");
AssertType({ 0: a => a }, "{ 0: (number) => number; }");
AssertType(0, "(number) => number");
AssertType(a => a, "(number) => number");
AssertType(a, "number");
AssertType(a, "number");

let x3: IWithNoNumberIndexSignature | IWithNumberIndexSignature1 = { 0: "hello" };
AssertType(x3, "union");
AssertType({ 0: "hello" }, "{ 0: string; }");
AssertType(0, "string");
AssertType("hello", "string");

let x4: IWithNumberIndexSignature1 | IWithNumberIndexSignature2 = { 1: a => a.toString() }; // a should be number
AssertType(x4, "union");
AssertType({ 1: a => a.toString() }, "{ 1: (number) => string; }");
AssertType(1, "(number) => string");
AssertType(a => a.toString(), "(number) => string");
AssertType(a, "number");
AssertType(a.toString(), "string");
AssertType(a.toString, "(?number) => string");

let x4: IWithNumberIndexSignature1 | IWithNumberIndexSignature2 = { 1: a => a }; // a should be number
AssertType(x4, "union");
AssertType({ 1: a => a }, "{ 1: (number) => number; }");
AssertType(1, "(number) => number");
AssertType(a => a, "(number) => number");
AssertType(a, "number");
AssertType(a, "number");


