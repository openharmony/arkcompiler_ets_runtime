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

// === tests/cases/conformance/types/objectTypeLiteral/callSignatures/callSignaturesWithOptionalParameters.ts ===
declare function AssertType(value:any, type:string):void;
// Optional parameters should be valid in all the below casts

function foo(x?: number) { }
let f = function foo(x?: number) { 
AssertType(f, "(?number) => void");

AssertType(function foo(x?: number) { }, "(?number) => void");

AssertType(foo, "(?number) => void");

AssertType(x, "number");
}

let f2 = (x: number, y?: number) => { 
AssertType(f2, "(number, ?number) => void");

AssertType((x: number, y?: number) => { }, "(number, ?number) => void");

AssertType(x, "number");

AssertType(y, "number");
}

foo(1);
AssertType(foo(1), "void");
AssertType(foo, "(?number) => void");
AssertType(1, "int");

foo();
AssertType(foo(), "void");
AssertType(foo, "(?number) => void");

f(1);
AssertType(f(1), "void");
AssertType(f, "(?number) => void");
AssertType(1, "int");

f();
AssertType(f(), "void");
AssertType(f, "(?number) => void");

f2(1);
AssertType(f2(1), "void");
AssertType(f2, "(number, ?number) => void");
AssertType(1, "int");

f2(1, 2);
AssertType(f2(1, 2), "void");
AssertType(f2, "(number, ?number) => void");
AssertType(1, "int");
AssertType(2, "int");

class C {
    foo(x?: number) { }
}

let c: C;
AssertType(c, "C");

c.foo();
AssertType(c.foo(), "void");
AssertType(c.foo, "(?number) => void");

c.foo(1);
AssertType(c.foo(1), "void");
AssertType(c.foo, "(?number) => void");
AssertType(1, "int");

interface I {
    (x?: number);
    foo(x: number, y?: number);
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

i.foo(1);
AssertType(i.foo(1), "any");
AssertType(i.foo, "(number, ?number) => any");
AssertType(1, "int");

i.foo(1, 2);
AssertType(i.foo(1, 2), "any");
AssertType(i.foo, "(number, ?number) => any");
AssertType(1, "int");
AssertType(2, "int");

let a: {
AssertType(a, "{ (?number): any; foo(?number): any; }");

    (x?: number);
AssertType(x, "number");

    foo(x?: number);
AssertType(foo, "(?number) => any");
AssertType(x, "number");
}

a();
AssertType(a(), "any");
AssertType(a, "{ (?number): any; foo(?number): any; }");

a(1);
AssertType(a(1), "any");
AssertType(a, "{ (?number): any; foo(?number): any; }");
AssertType(1, "int");

a.foo();
AssertType(a.foo(), "any");
AssertType(a.foo, "(?number) => any");

a.foo(1);
AssertType(a.foo(1), "any");
AssertType(a.foo, "(?number) => any");
AssertType(1, "int");

let b = {
AssertType(b, "{ foo(?number): void; a: (number, ?number) => void; b: (?number) => void; }");
AssertType({    foo(x?: number) { },    a: function foo(x: number, y?: number) { },    b: (x?: number) => { }}, "{ foo(?number): void; a: (number, ?number) => void; b: (?number) => void; }");

    foo(x?: number) { },
AssertType(foo, "(?number) => void");
AssertType(x, "number");

    a: function foo(x: number, y?: number) { },
AssertType(a, "(number, ?number) => void");
AssertType(function foo(x: number, y?: number) { }, "(number, ?number) => void");
AssertType(foo, "(number, ?number) => void");
AssertType(x, "number");
AssertType(y, "number");

    b: (x?: number) => { 
AssertType(b, "(?number) => void");

AssertType((x?: number) => { }, "(?number) => void");

AssertType(x, "number");
}
}

b.foo();
AssertType(b.foo(), "void");
AssertType(b.foo, "(?number) => void");

b.foo(1);
AssertType(b.foo(1), "void");
AssertType(b.foo, "(?number) => void");
AssertType(1, "int");

b.a(1);
AssertType(b.a(1), "void");
AssertType(b.a, "(number, ?number) => void");
AssertType(1, "int");

b.a(1, 2);
AssertType(b.a(1, 2), "void");
AssertType(b.a, "(number, ?number) => void");
AssertType(1, "int");
AssertType(2, "int");

b.b();
AssertType(b.b(), "void");
AssertType(b.b, "(?number) => void");

b.b(1);
AssertType(b.b(1), "void");
AssertType(b.b, "(?number) => void");
AssertType(1, "int");


