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

// === tests/cases/conformance/types/typeRelationships/typeAndMemberIdentity/objectTypesIdentityWithGenericConstructSignaturesDifferingByConstraints.ts ===
declare function AssertType(value:any, type:string):void;
// Two call or construct signatures are considered identical when they have the same number of type parameters and, considering those 
// parameters pairwise identical, have identical type parameter constraints, identical number of parameters with identical kind(required, 
// optional or rest) and types, and identical return types.

class B<T extends Array<number>> {
    constructor(x: T) { 
AssertType(null, "null");
return null; 
}
}

class C<T extends String> {
    constructor(x: T) { 
AssertType(null, "null");
return null; 
}
}

interface I<T extends Number> {
    new(x: T): string;
}

interface I2 {
    new<T extends Boolean>(x: T): string;
}

let a: { new<T extends Array<string>>(x: T): string 
AssertType(a, "new <T extends string[]>(T) => string");

AssertType(x, "T");
}

let b = { new<T extends RegExp>(x: T) { 
AssertType(b, "{ new<T extends RegExp>(T): string; }");
AssertType({ new<T extends RegExp>(x: T) { return ''; } }, "{ new<T extends RegExp>(T): string; }");
AssertType(x, "T");
AssertType('', "string");
return ''; } }; // not a construct signature, function called new

function foo1b(x: B<Array<number>>);
function foo1b(x: B<Array<number>>); // error
function foo1b(x: any) { }

function foo1c(x: C<String>);
function foo1c(x: C<String>); // error
function foo1c(x: any) { }

function foo2(x: I<Number>);
function foo2(x: I<Number>); // error
function foo2(x: any) { }

function foo3(x: typeof a);
function foo3(x: typeof a); // error
function foo3(x: any) { }

function foo4(x: typeof b);
function foo4(x: typeof b); // error
function foo4(x: any) { }

function foo8(x: B<Array<number>>);
function foo8(x: I<Number>); // ok
function foo8(x: any) { }

function foo9(x: B<Array<number>>);
function foo9(x: C<String>); // error, types are structurally equal
function foo9(x: any) { }

function foo10(x: B<Array<number>>);
function foo10(x: typeof a); // ok
function foo10(x: any) { }

function foo11(x: B<Array<number>>);
function foo11(x: typeof b); // ok
function foo11(x: any) { }

function foo12(x: I<Number>);
function foo12(x: C<String>); // ok
function foo12(x: any) { }

function foo12b(x: I2);
function foo12b(x: C<String>); // ok
function foo12b(x: any) { }

function foo13(x: I<Number>);
function foo13(x: typeof a); // ok
function foo13(x: any) { }

function foo14(x: I<Number>);
function foo14(x: typeof b); // ok
function foo14(x: any) { }


