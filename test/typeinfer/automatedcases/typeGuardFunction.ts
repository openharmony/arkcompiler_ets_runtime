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

// === tests/cases/conformance/expressions/typeGuards/typeGuardFunction.ts ===
declare function AssertType(value:any, type:string):void;
class A {
    propA: number;
}

class B {
    propB: number;
}

class C extends A {
    propC: number;
}

declare function isA(p1: any): p1 is A;
declare function isB(p1: any): p1 is B;
declare function isC(p1: any): p1 is C;

declare function retC(): C;

let a: A;
AssertType(a, "A");

let b: B;
AssertType(b, "B");

// Basic
if (isC(a)) {
    a.propC;
AssertType(a.propC, "number");
}

// Sub type
let subType: C;
AssertType(subType, "C");

if(isA(subType)) {
    subType.propC;
AssertType(subType.propC, "number");
}

// Union type
let union: A | B;
AssertType(union, "union");

if(isA(union)) {
    union.propA;
AssertType(union.propA, "number");
}

// Call signature
interface I1 {
    (p1: A): p1 is C;
}

// The parameter index and argument index for the type guard target is matching.
// The type predicate type is assignable to the parameter type.
declare function isC_multipleParams(p1, p2): p1 is C;
if (isC_multipleParams(a, 0)) {
    a.propC;
AssertType(a.propC, "number");
}

// Methods
let obj: {
AssertType(obj, "{ func1(A): p1 is C; }");

    func1(p1: A): p1 is C;
AssertType(func1, "(A) => p1 is C");
AssertType(p1, "A");
}
class D {
    method1(p1: A): p1 is C {
AssertType(true, "boolean");
        return true;
    }
}

// Arrow function
let f1 = (p1: A): p1 is C => false;
AssertType(f1, "(A) => p1 is C");
AssertType((p1: A): p1 is C => false, "(A) => p1 is C");
AssertType(p1, "A");
AssertType(false, "boolean");

// Function type
declare function f2(p1: (p1: A) => p1 is C);

// Function expressions
f2(function(p1: A): p1 is C {
AssertType(f2, "((A) => p1 is C) => any");
AssertType(function(p1: A): p1 is C {    return true;}, "(A) => p1 is C");
AssertType(p1, "A");
AssertType(f2(function(p1: A): p1 is C {    return true;}), "any");

AssertType(true, "boolean");
    return true;

});

// Evaluations are asssignable to boolean.
declare function acceptingBoolean(a: boolean);
acceptingBoolean(isA(a));
AssertType(acceptingBoolean(isA(a)), "any");
AssertType(acceptingBoolean, "(boolean) => any");
AssertType(isA(a), "boolean");
AssertType(isA, "(any) => p1 is A");
AssertType(a, "A");

// Type predicates with different parameter name.
declare function acceptingTypeGuardFunction(p1: (item) => item is A);
acceptingTypeGuardFunction(isA);
AssertType(acceptingTypeGuardFunction(isA), "any");
AssertType(acceptingTypeGuardFunction, "((any) => item is A) => any");
AssertType(isA, "(any) => p1 is A");

// Binary expressions
let union2: C | B;
AssertType(union2, "union");

let union3: boolean | B = isA(union2) || union2;
AssertType(union3, "union");
AssertType(isA(union2) || union2, "union");
AssertType(isA(union2), "boolean");
AssertType(isA, "(any) => p1 is A");
AssertType(union2, "union");
AssertType(union2, "B");


