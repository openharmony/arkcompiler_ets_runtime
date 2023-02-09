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

// === tests/cases/conformance/types/objectTypeLiteral/callSignatures/callSignaturesWithOptionalParameters2.ts ===
declare function AssertType(value:any, type:string):void;
// Optional parameters should be valid in all the below casts

function foo(x?: number);
function foo(x?: number) { }

foo(1);
AssertType(foo(1), "any");
AssertType(foo, "(?number) => any");
AssertType(1, "int");

foo();
AssertType(foo(), "any");
AssertType(foo, "(?number) => any");

function foo2(x: number);
function foo2(x: number, y?: number);
function foo2(x: number, y?: number) { }

foo2(1);
AssertType(foo2(1), "any");
AssertType(foo2, "{ (number): any; (number, ?number): any; }");
AssertType(1, "int");

foo2(1, 2);
AssertType(foo2(1, 2), "any");
AssertType(foo2, "{ (number): any; (number, ?number): any; }");
AssertType(1, "int");
AssertType(2, "int");

class C {
    foo(x?: number);
    foo(x?: number) { }

    foo2(x: number);
    foo2(x: number, y?: number);
    foo2(x: number, y?: number) { }
}

let c: C;
AssertType(c, "C");

c.foo();
AssertType(c.foo(), "any");
AssertType(c.foo, "(?number) => any");

c.foo(1);
AssertType(c.foo(1), "any");
AssertType(c.foo, "(?number) => any");
AssertType(1, "int");

c.foo2(1);
AssertType(c.foo2(1), "any");
AssertType(c.foo2, "{ (number): any; (number, ?number): any; }");
AssertType(1, "int");

c.foo2(1, 2);
AssertType(c.foo2(1, 2), "any");
AssertType(c.foo2, "{ (number): any; (number, ?number): any; }");
AssertType(1, "int");
AssertType(2, "int");

interface I {
    (x?: number);
    (x?: number, y?: number);
    foo(x: number, y?: number);
    foo(x: number, y?: number, z?: number);
}

let i: I;
AssertType(i, "I");

i();
AssertType(i(), "any");
AssertType(i, "I");

i(1);
AssertType(i(1), "any");
AssertType(i, "I");
AssertType(1, "int");

i(1, 2);
AssertType(i(1, 2), "any");
AssertType(i, "I");
AssertType(1, "int");
AssertType(2, "int");

i.foo(1);
AssertType(i.foo(1), "any");
AssertType(i.foo, "{ (number, ?number): any; (number, ?number, ?number): any; }");
AssertType(1, "int");

i.foo(1, 2);
AssertType(i.foo(1, 2), "any");
AssertType(i.foo, "{ (number, ?number): any; (number, ?number, ?number): any; }");
AssertType(1, "int");
AssertType(2, "int");

i.foo(1, 2, 3);
AssertType(i.foo(1, 2, 3), "any");
AssertType(i.foo, "{ (number, ?number): any; (number, ?number, ?number): any; }");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

let a: {
AssertType(a, "{ (?number): any; (?number, ?number): any; foo(number, ?number): any; foo(number, ?number, ?number): any; }");

    (x?: number);
AssertType(x, "number");

    (x?: number, y?: number);
AssertType(x, "number");
AssertType(y, "number");

    foo(x: number, y?: number);
AssertType(foo, "{ (number, ?number): any; (number, ?number, ?number): any; }");
AssertType(x, "number");
AssertType(y, "number");

    foo(x: number, y?: number, z?: number);
AssertType(foo, "{ (number, ?number): any; (number, ?number, ?number): any; }");
AssertType(x, "number");
AssertType(y, "number");
AssertType(z, "number");
}

a();
AssertType(a(), "any");
AssertType(a, "{ (?number): any; (?number, ?number): any; foo(number, ?number): any; foo(number, ?number, ?number): any; }");

a(1);
AssertType(a(1), "any");
AssertType(a, "{ (?number): any; (?number, ?number): any; foo(number, ?number): any; foo(number, ?number, ?number): any; }");
AssertType(1, "int");

a(1, 2);
AssertType(a(1, 2), "any");
AssertType(a, "{ (?number): any; (?number, ?number): any; foo(number, ?number): any; foo(number, ?number, ?number): any; }");
AssertType(1, "int");
AssertType(2, "int");

a.foo(1);
AssertType(a.foo(1), "any");
AssertType(a.foo, "{ (number, ?number): any; (number, ?number, ?number): any; }");
AssertType(1, "int");

a.foo(1, 2);
AssertType(a.foo(1, 2), "any");
AssertType(a.foo, "{ (number, ?number): any; (number, ?number, ?number): any; }");
AssertType(1, "int");
AssertType(2, "int");

a.foo(1, 2, 3);
AssertType(a.foo(1, 2, 3), "any");
AssertType(a.foo, "{ (number, ?number): any; (number, ?number, ?number): any; }");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");


