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

// === tests/cases/conformance/enums/enumBasics.ts ===
declare function AssertType(value:any, type:string):void;
// Enum without initializers have first member = 0 and successive members = N + 1
enum E1 {
    A,
    B,
    C
}

// Enum type is a subtype of Number
let x: number = E1.A;
AssertType(x, "number");
AssertType(E1.A, "E1.A");

// Enum object type is anonymous with properties of the enum type and numeric indexer
let e = E1;
AssertType(e, "typeof E1");
AssertType(E1, "typeof E1");

let e: {
AssertType(e, "typeof E1");

    readonly A: E1.A;
AssertType(A, "E1.A");
AssertType(E1, "any");

    readonly B: E1.B;
AssertType(B, "E1.B");
AssertType(E1, "any");

    readonly C: E1.C;
AssertType(C, "E1.C");
AssertType(E1, "any");

    readonly [n: number]: string;
AssertType(n, "number");

};
let e: typeof E1;
AssertType(e, "typeof E1");
AssertType(E1, "typeof E1");

// Reverse mapping of enum returns string name of property 
let s = E1[e.A];
AssertType(s, "string");
AssertType(E1[e.A], "string");
AssertType(E1, "typeof E1");
AssertType(e.A, "E1.A");

let s: string;
AssertType(s, "string");


// Enum with only constant members
enum E2 {
    A = 1, B = 2, C = 3
}

// Enum with only computed members
enum E3 {
    X = 'foo'.length, Y = 4 + 3, Z = +'foo'
}

// Enum with constant members followed by computed members
enum E4 {
    X = 0, Y, Z = 'foo'.length
}

// Enum with > 2 constant members with no initializer for first member, non zero initializer for second element
enum E5 {
    A,
    B = 3,
    C // 4
}

enum E6 {
    A,
    B = 0,
    C // 1
}

// Enum with computed member initializer of type 'any'
enum E7 {
    A = 'foo'['foo']
}

// Enum with computed member initializer of type number
enum E8 {
    B = 'foo'['foo']
}

//Enum with computed member intializer of same enum type
enum E9 {
    A,
    B = A
}

// (refer to .js to validate)
// Enum constant members are propagated
let doNotPropagate = [
AssertType(doNotPropagate, "(union)[]");
AssertType([    E8.B, E7.A, E4.Z, E3.X, E3.Y, E3.Z], "(union)[]");

    E8.B, E7.A, E4.Z, E3.X, E3.Y, E3.Z
AssertType(E8.B, "E8");
AssertType(E7.A, "E7");
AssertType(E4.Z, "E4.Z");
AssertType(E3.X, "E3.X");
AssertType(E3.Y, "E3.Y");
AssertType(E3.Z, "E3.Z");

];
// Enum computed members are not propagated
let doPropagate = [
AssertType(doPropagate, "(union)[]");
AssertType([    E9.A, E9.B, E6.B, E6.C, E6.A, E5.A, E5.B, E5.C], "(union)[]");

    E9.A, E9.B, E6.B, E6.C, E6.A, E5.A, E5.B, E5.C
AssertType(E9.A, "E9");
AssertType(E9.B, "E9");
AssertType(E6.B, "E6.A");
AssertType(E6.C, "E6.C");
AssertType(E6.A, "E6.A");
AssertType(E5.A, "E5.A");
AssertType(E5.B, "E5.B");
AssertType(E5.C, "E5.C");

];



