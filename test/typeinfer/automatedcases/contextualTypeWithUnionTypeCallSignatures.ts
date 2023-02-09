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

// === tests/cases/conformance/types/union/contextualTypeWithUnionTypeCallSignatures.ts ===
declare function AssertType(value:any, type:string):void;
//When used as a contextual type, a union type U has those members that are present in any of 
// its constituent types, with types that are unions of the respective members in the constituent types. 

// Let S be the set of types in U that have call signatures.
// If S is not empty and the sets of call signatures of the types in S are identical ignoring return types,
// U has the same set of call signatures, but with return types that are unions of the return types of the respective call signatures from each type in S.

interface IWithNoCallSignatures {
    foo: string;
}
interface IWithCallSignatures {
    (a: number): string;
}
interface IWithCallSignatures2 {
    (a: number): number;
}
interface IWithCallSignatures3 {
    (b: string): number;
}
interface IWithCallSignatures4 {
    (a: number): string;
    (a: string, b: number): number;
}

// With no call signature | callSignatures
let x: IWithNoCallSignatures | IWithCallSignatures = a => a.toString();
AssertType(x, "union");
AssertType(a => a.toString(), "(number) => string");
AssertType(a, "number");
AssertType(a.toString(), "string");
AssertType(a.toString, "(?number) => string");

// With call signatures with different return type
let x2: IWithCallSignatures | IWithCallSignatures2 = a => a.toString(); // Like iWithCallSignatures
AssertType(x2, "union");
AssertType(a => a.toString(), "(number) => string");
AssertType(a, "number");
AssertType(a.toString(), "string");
AssertType(a.toString, "(?number) => string");

let x2: IWithCallSignatures | IWithCallSignatures2 = a => a; // Like iWithCallSignatures2
AssertType(x2, "union");
AssertType(a => a, "(number) => number");
AssertType(a, "number");
AssertType(a, "number");

// With call signatures of mismatching parameter type
let x3: IWithCallSignatures | IWithCallSignatures3 = a => /*here a should be any*/ a.toString();
AssertType(x3, "union");
AssertType(a => /*here a should be any*/ a.toString(), "(any) => any");
AssertType(a, "any");
AssertType(a.toString(), "any");
AssertType(a.toString, "any");

// With call signature count mismatch
let x4: IWithCallSignatures | IWithCallSignatures4 = a => /*here a should be any*/ a.toString();
AssertType(x4, "union");
AssertType(a => /*here a should be any*/ a.toString(), "(number) => string");
AssertType(a, "number");
AssertType(a.toString(), "string");
AssertType(a.toString, "(?number) => string");


