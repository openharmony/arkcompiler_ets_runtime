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

// === tests/cases/conformance/types/typeRelationships/typeAndMemberIdentity/objectTypesIdentityWithGenericConstructSignaturesDifferingByReturnType2.ts ===
declare function AssertType(value:any, type:string):void;
// Two call or construct signatures are considered identical when they have the same number of type parameters and, considering those 
// parameters pairwise identical, have identical type parameter constraints, identical number of parameters with identical kind(required, 
// optional or rest) and types, and identical return types.

class B<T extends Date> {
    constructor(x: T) { 
AssertType(null, "null");
return null; 
}
}

class C<T extends Date> {
    constructor(x: T) { 
AssertType(null, "null");
return null; 
}
}

interface I<T extends Date> {
    new(x: T): Date;
}

interface I2 {
    new<T extends Date>(x: T): RegExp;
}

let a: { new<T extends Date>(x: T): T 
AssertType(a, "new <T extends Date>(T) => T");

AssertType(x, "T");
}

let b = { new<T extends Date>(x: T) { 
AssertType(b, "{ new<T extends Date>(T): any; }");
AssertType({ new<T extends Date>(x: T) { return null; } }, "{ new<T extends Date>(T): any; }");
AssertType(x, "T");
AssertType(null, "null");
return null; } }; // not a construct signature, function called new

function foo1b(x: B<Date>);
function foo1b(x: B<Date>); // error
function foo1b(x: any) { }

function foo1c(x: C<Date>);
function foo1c(x: C<Date>); // error
function foo1c(x: any) { }

function foo2(x: I<Date>);
function foo2(x: I<Date>); // error
function foo2(x: any) { }

function foo3(x: typeof a);
function foo3(x: typeof a); // error
function foo3(x: any) { }

function foo4(x: typeof b);
function foo4(x: typeof b); // error
function foo4(x: any) { }

function foo8(x: B<Date>);
function foo8(x: I<Date>); // ok
function foo8(x: any) { }

function foo9(x: B<Date>);
function foo9(x: C<Date>); // error since types are structurally equal
function foo9(x: any) { }

function foo10(x: B<Date>);
function foo10(x: typeof a); // ok
function foo10(x: any) { }

function foo11(x: B<Date>);
function foo11(x: typeof b); // ok
function foo11(x: any) { }

function foo12(x: I<Date>);
function foo12(x: C<Date>); // ok
function foo12(x: any) { }

function foo12b(x: I2);
function foo12b(x: C<Date>); // ok
function foo12b(x: any) { }

function foo13(x: I<Date>);
function foo13(x: typeof a); // ok
function foo13(x: any) { }

function foo14(x: I<Date>);
function foo14(x: typeof b); // ok
function foo14(x: any) { }

function foo15(x: I2);
function foo15(x: C<Date>); // ok
function foo15(x: any) { }

