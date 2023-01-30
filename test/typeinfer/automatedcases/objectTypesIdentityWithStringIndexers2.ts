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

// === tests/cases/conformance/types/typeRelationships/typeAndMemberIdentity/objectTypesIdentityWithStringIndexers2.ts ===
declare function AssertType(value:any, type:string):void;
// object types are identical structurally

class Base { foo: string; }
class Derived extends Base { bar: string; }

class A {
    [x: string]: Base;
}

class B {
    [x: string]: Derived;
}

class C<T> {
    [x: string]: T;
}

interface I {
    [x: string]: Derived;
}

class PA extends A {
}

class PB extends B {
}

let a: {
AssertType(a, "{ [string]: Base; }");

    [x: string]: Base;
AssertType(x, "string");
}
let b: { [x: string]: Derived; } = { foo: <Derived>null };
AssertType(b, "{ [string]: Derived; }");
AssertType(x, "string");
AssertType({ foo: <Derived>null }, "{ foo: Derived; }");
AssertType(foo, "Derived");
AssertType(<Derived>null, "Derived");
AssertType(null, "null");

function foo1(x: A);
function foo1(x: A); // error
function foo1(x: any) { }

function foo1b(x: B);
function foo1b(x: B); // error
function foo1b(x: any) { }

function foo1c(x: C<string>);
function foo1c(x: C<string>); // error
function foo1c(x: any) { }

function foo2(x: I);
function foo2(x: I); // error
function foo2(x: any) { }

function foo3(x: typeof a);
function foo3(x: typeof a); // error
function foo3(x: any) { }

function foo4(x: typeof b);
function foo4(x: typeof b); // error
function foo4(x: any) { }

function foo5(x: A);
function foo5(x: B); // ok
function foo5(x: any) { }

function foo5b(x: A);
function foo5b(x: C<Derived>); // ok
function foo5b(x: any) { }

function foo5c(x: A);
function foo5c(x: PA); // error
function foo5c(x: any) { }

function foo5d(x: A);
function foo5d(x: PB); // ok
function foo5d(x: any) { }

function foo6(x: A);
function foo6(x: I); // ok
function foo6(x: any) { }

function foo7(x: A);
function foo7(x: typeof a); // error
function foo7(x: any) { }

function foo8(x: B);
function foo8(x: I); // error
function foo8(x: any) { }

function foo9(x: B);
function foo9(x: C<Base>); // ok
function foo9(x: any) { }

function foo10(x: B);
function foo10(x: typeof a); // ok
function foo10(x: any) { }

function foo11(x: B);
function foo11(x: typeof b); // error
function foo11(x: any) { }

function foo11b(x: B);
function foo11b(x: PA); // ok
function foo11b(x: any) { }

function foo11c(x: B);
function foo11c(x: PB); // error
function foo11c(x: any) { }

function foo12(x: I);
function foo12(x: C<Derived>); // error
function foo12(x: any) { }

function foo13(x: I);
function foo13(x: typeof a); // ok
function foo13(x: any) { }

function foo14(x: I);
function foo14(x: typeof b); // error
function foo14(x: any) { }

function foo15(x: I);
function foo15(x: PA); // ok
function foo15(x: any) { }

function foo16(x: I);
function foo16(x: PB); // error
function foo16(x: any) { }



