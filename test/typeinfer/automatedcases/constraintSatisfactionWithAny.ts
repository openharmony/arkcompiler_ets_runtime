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

// === tests/cases/conformance/types/typeParameters/typeArgumentLists/constraintSatisfactionWithAny.ts ===
declare function AssertType(value:any, type:string):void;
// any is not a valid type argument unless there is no constraint, or the constraint is any

function foo<T extends String>(x: T): T { 
AssertType(null, "null");
return null; 
}

function foo2<T extends { x: number }>(x: T): T { 
AssertType(null, "null");
return null; 
}

//function foo3<T extends T[]>(x: T): T { return null; }
function foo4<T extends <T>(x: T) => void>(x: T): T { 
AssertType(null, "null");
return null; 
}

let a;
AssertType(a, "any");

foo(a);
AssertType(foo(a), "any");
AssertType(foo, "<T extends String>(T) => T");
AssertType(a, "any");

foo2(a);
AssertType(foo2(a), "any");
AssertType(foo2, "<T extends { x: number; }>(T) => T");
AssertType(a, "any");

foo4(a);
AssertType(foo4(a), "any");
AssertType(foo4, "<T extends <T>(T) => void>(T) => T");
AssertType(a, "any");

let b: number;
AssertType(b, "number");

foo<any>(b);
AssertType(foo<any>(b), "any");
AssertType(foo, "<T extends String>(T) => T");
AssertType(b, "number");

foo2<any>(b);
AssertType(foo2<any>(b), "any");
AssertType(foo2, "<T extends { x: number; }>(T) => T");
AssertType(b, "number");

//foo3<any>(b);
foo4<any>(b);
AssertType(foo4<any>(b), "any");
AssertType(foo4, "<T extends <T>(T) => void>(T) => T");
AssertType(b, "number");

class C<T extends String> {
    constructor(public x: T) { }
}

let c1 = new C(a);
AssertType(c1, "C<any>");
AssertType(new C(a), "C<any>");
AssertType(C, "typeof C");
AssertType(a, "any");

let c2 = new C<any>(b);
AssertType(c2, "C<any>");
AssertType(new C<any>(b), "C<any>");
AssertType(C, "typeof C");
AssertType(b, "number");

class C2<T extends { x: number }> {
    constructor(public x: T) { }
}

let c3 = new C2(a);
AssertType(c3, "C2<any>");
AssertType(new C2(a), "C2<any>");
AssertType(C2, "typeof C2");
AssertType(a, "any");

let c4 = new C2<any>(b);
AssertType(c4, "C2<any>");
AssertType(new C2<any>(b), "C2<any>");
AssertType(C2, "typeof C2");
AssertType(b, "number");

class C4<T extends <T>(x:T) => T> {
    constructor(public x: T) { }
}

let c7 = new C4(a);
AssertType(c7, "C4<any>");
AssertType(new C4(a), "C4<any>");
AssertType(C4, "typeof C4");
AssertType(a, "any");

let c8 = new C4<any>(b);
AssertType(c8, "C4<any>");
AssertType(new C4<any>(b), "C4<any>");
AssertType(C4, "typeof C4");
AssertType(b, "number");




