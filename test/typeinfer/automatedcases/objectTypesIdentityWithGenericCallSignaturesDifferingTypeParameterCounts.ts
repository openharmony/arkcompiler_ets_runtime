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

// === tests/cases/conformance/types/typeRelationships/typeAndMemberIdentity/objectTypesIdentityWithGenericCallSignaturesDifferingTypeParameterCounts.ts ===
declare function AssertType(value:any, type:string):void;
// object types are identical structurally

class A {
    foo<T>(x: T): T { 
AssertType(null, "null");
return null; 
}
}

class B<U, V> {
    foo(x: U): U { 
AssertType(null, "null");
return null; 
}
}

class C<V, W, X> {
    foo(x: V): V { 
AssertType(null, "null");
return null; 
}
}

interface I<X, Y, Z, A> {
    foo(x: X): X;
}

interface I2 {
    foo<Y, Z, A, B>(x: Y): Y;
}

let a: { foo<Z, A, B, C, D>(x: Z): Z 
AssertType(a, "{ foo<Z, A, B, C, D>(Z): Z; }");

AssertType(foo, "<Z, A, B, C, D>(Z) => Z");

AssertType(x, "Z");
}

let b = { foo<A, B, C, D, E, F>(x: A) { 
AssertType(b, "{ foo<A, B, C, D, E, F>(A): A; }");
AssertType({ foo<A, B, C, D, E, F>(x: A) { return x; } }, "{ foo<A, B, C, D, E, F>(A): A; }");
AssertType(foo, "<A, B, C, D, E, F>(A) => A");
AssertType(x, "A");
AssertType(x, "A");
return x; } };

function foo1(x: A);
function foo1(x: A); // error
function foo1(x: any) { }

function foo1b(x: B<string, string>);
function foo1b(x: B<string, string>); // error
function foo1b(x: any) { }

function foo1c(x: C<string, number, boolean>);
function foo1c(x: C<string, number, boolean>); // error
function foo1c(x: any) { }

function foo2(x: I<string, boolean, number, string>);
function foo2(x: I<string, boolean, number, string>); // error
function foo2(x: any) { }

function foo3(x: typeof a);
function foo3(x: typeof a); // error
function foo3(x: any) { }

function foo4(x: typeof b);
function foo4(x: typeof b); // error
function foo4(x: any) { }

function foo5(x: A);
function foo5(x: B<string, string>); // ok
function foo5(x: any) { }

function foo5b(x: A);
function foo5b(x: C<string, number, boolean>); // ok
function foo5b(x: any) { }

function foo6(x: A);
function foo6(x: I<string, number, boolean, Date>); // ok
function foo6(x: any) { }

function foo7(x: A);
function foo7(x: typeof a); // no error, bug?
function foo7(x: any) { }

function foo8(x: B<string, string>);
function foo8(x: I<string, string, boolean, Date>); // error
function foo8(x: any) { }

function foo9(x: B<string, number>);
function foo9(x: C<string, number, B<string, string>>); // error
function foo9(x: any) { }

function foo10(x: B<string, boolean>);
function foo10(x: typeof a); // ok
function foo10(x: any) { }

function foo11(x: B<string, boolean>);
function foo11(x: typeof b); // ok
function foo11(x: any) { }

function foo12(x: I<B<string, number>, number, Date, string>);
function foo12(x: C<B<string, number>, number, Date>); // error
function foo12(x: any) { }

function foo12b(x: I2);
function foo12b(x: C<string, string, boolean>); // ok
function foo12b(x: any) { }

function foo13(x: I<string, Date, RegExp, Date>);
function foo13(x: typeof a); // ok
function foo13(x: any) { }

function foo14(x: I<string, Date, RegExp, boolean>);
function foo14(x: typeof b); // ok
function foo14(x: any) { }

function foo15(x: I2);
function foo15(x: C<number, B<string, string>, B<number, string>>); // ok
function foo15(x: any) { }

