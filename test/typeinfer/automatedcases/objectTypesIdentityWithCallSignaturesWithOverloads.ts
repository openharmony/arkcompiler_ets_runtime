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

// === tests/cases/conformance/types/typeRelationships/typeAndMemberIdentity/objectTypesIdentityWithCallSignaturesWithOverloads.ts ===
declare function AssertType(value:any, type:string):void;
// object types are identical structurally

class A {
    foo(x: number): number;
    foo(x: string): string;
    foo(x: any): any { 
AssertType(null, "null");
return null; 
}
}

class B {
    foo(x: number): number;
    foo(x: string): string;
    foo(x: any): any { 
AssertType(null, "null");
return null; 
}
}

class C<T> {
    foo(x: number): number;
    foo(x: string): string;
    foo(x: T): T;
    foo(x: any): any { 
AssertType(null, "null");
return null; 
}
}

interface I {
    foo(x: number): number;
    foo(x: string): string;    
}

interface I2<T> {
    foo(x: number): number;
    foo(x: string): string;
    foo(x: T): T;
}

let a: {
AssertType(a, "{ foo(number): number; foo(string): string; }");

    foo(x: number): number
AssertType(foo, "{ (number): number; (string): string; }");
AssertType(x, "number");

    foo(x: string): string
AssertType(foo, "{ (number): number; (string): string; }");
AssertType(x, "string");
}

let b = {
AssertType(b, "{ foo(any): any; }");
AssertType({    foo(x: any) { return <any>''; }}, "{ foo(any): any; }");

    foo(x: any) { 
AssertType(foo, "(any) => any");
return <any>''; 

AssertType(x, "any");

AssertType(<any>'', "any");

AssertType('', "string");
}

};

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
function foo5(x: B); // error
function foo5(x: any) { }

function foo5b(x: A);
function foo5b(x: C<string>); // ok
function foo5b(x: any) { }

function foo6(x: A);
function foo6(x: I); // BUG 831930
function foo6(x: any) { }

function foo7(x: A);
function foo7(x: typeof a); // BUG 831930
function foo7(x: any) { }

function foo8(x: B);
function foo8(x: I); // BUG 831930
function foo8(x: any) { }

function foo9(x: B);
function foo9(x: C<string>); // ok
function foo9(x: any) { }

function foo10(x: B);
function foo10(x: typeof a); // BUG 831930
function foo10(x: any) { }

function foo11(x: B);
function foo11(x: typeof b); // ok
function foo11(x: any) { }

function foo12(x: I);
function foo12(x: C<string>); // ok
function foo12(x: any) { }

function foo12b(x: I2<string>);
function foo12b(x: C<string>); // ok
function foo12b(x: any) { }

function foo13(x: I);
function foo13(x: typeof a);  // error
function foo13(x: any) { }

function foo14(x: I);
function foo14(x: typeof b); // ok
function foo14(x: any) { }

function foo15(x: I2<string>);
function foo15(x: C<number>); // ok
function foo15(x: any) { }

