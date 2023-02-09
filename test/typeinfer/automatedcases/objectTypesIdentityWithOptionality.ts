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

// === tests/cases/conformance/types/typeRelationships/typeAndMemberIdentity/objectTypesIdentityWithOptionality.ts ===
declare function AssertType(value:any, type:string):void;
// object types are identical structurally

class A {
    foo: string;
}

class B {
    foo: string;
}

class C<T> {
    foo: T;
}

interface I {
    foo?: string;
}

let a: { foo?: string; 
AssertType(a, "{ foo?: string; }");

AssertType(foo, "string");
}

let b = { foo: '' };
AssertType(b, "{ foo: string; }");
AssertType({ foo: '' }, "{ foo: string; }");
AssertType(foo, "string");
AssertType('', "string");

function foo2(x: I);
function foo2(x: I); // error
function foo2(x: any) { }

function foo3(x: typeof a);
function foo3(x: typeof a); // error
function foo3(x: any) { }

function foo6(x: A);
function foo6(x: I); // ok
function foo6(x: any) { }

function foo7(x: A);
function foo7(x: typeof a); // ok
function foo7(x: any) { }

function foo8(x: B);
function foo8(x: I); // ok
function foo8(x: any) { }

function foo10(x: B);
function foo10(x: typeof a); // ok
function foo10(x: any) { }

function foo12(x: I);
function foo12(x: C<string>); // ok
function foo12(x: any) { }

function foo13(x: I);
function foo13(x: typeof a); // error
function foo13(x: any) { }

function foo14(x: I);
function foo14(x: typeof b); // ok
function foo14(x: any) { }

