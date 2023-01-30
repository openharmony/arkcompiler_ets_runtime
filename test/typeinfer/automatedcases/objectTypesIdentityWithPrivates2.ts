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

// === tests/cases/conformance/types/typeRelationships/typeAndMemberIdentity/objectTypesIdentityWithPrivates2.ts ===
declare function AssertType(value:any, type:string):void;
// object types are identical structurally

class C<T> {
    private foo: T;
}

class D<T> extends C<T> {
}

function foo1(x: C<string>);
function foo1(x: C<number>); // ok
function foo1(x: any) { }

function foo2(x: D<string>);
function foo2(x: D<number>); // ok
function foo2(x: any) { }

function foo3(x: C<string>);
function foo3(x: D<number>); // ok
function foo3(x: any) { }

function foo4(x: C<number>): number; 
function foo4(x: D<number>): string; // BUG 831926
function foo4(x: any): any { }

let r = foo4(new C<number>());
AssertType(r, "number");
AssertType(foo4(new C<number>()), "number");
AssertType(foo4, "{ (C<number>): number; (D<number>): string; }");
AssertType(new C<number>(), "C<number>");
AssertType(C, "typeof C");

let r = foo4(new D<number>());
AssertType(r, "number");
AssertType(foo4(new D<number>()), "number");
AssertType(foo4, "{ (C<number>): number; (D<number>): string; }");
AssertType(new D<number>(), "D<number>");
AssertType(D, "typeof D");

function foo5(x: C<number>): number;
function foo5(x: C<number>): string; // error
function foo5(x: any): any { }

function foo6(x: D<number>): number;
function foo6(x: D<number>): string; // error
function foo6(x: any): any { }




