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

// === tests/cases/conformance/types/typeRelationships/typeAndMemberIdentity/objectTypesIdentityWithGenericCallSignaturesDifferingTypeParameterCounts2.ts ===
declare function AssertType(value:any, type:string):void;
// object types are identical structurally


interface I<X, Y, Z, A> {
    (x: X): X;
}

interface I2 {
    <Y, Z, A, B>(x: Y): Y;
}

let a: { <Z, A, B, C, D>(x: Z): Z 
AssertType(a, "<Z, A, B, C, D>(Z) => Z");

AssertType(x, "Z");
}

function foo1(x: I<string, boolean, number, string>);
function foo1(x: I<string, boolean, number, string>); // error
function foo1(x: any) { }

function foo2(x: I2);
function foo2(x: I2); // error
function foo2(x: any) { }

function foo3(x: typeof a);
function foo3(x: typeof a); // error
function foo3(x: any) { }

function foo13(x: I<boolean, string, number, Date>);
function foo13(x: typeof a); // ok
function foo13(x: any) { }

function foo14(x: I<boolean, string, number, Date>);
function foo14(x: I2); // error
function foo14(x: any) { }

function foo14b(x: typeof a);
function foo14b(x: I2); // ok
function foo14b(x: any) { }

function foo15(x: I<boolean, string, number, Date>);
function foo15(x: I2); // ok
function foo15(x: any) { }

